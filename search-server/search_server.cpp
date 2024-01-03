#include <cmath>
#include <numeric>

#include "search_server.h"

using namespace std;

SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}

SearchServer::SearchServer(string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}

void SearchServer::AddDocument(int document_id, const string_view document, 
                                    DocumentStatus status, const vector<int>& ratings) {
    
    if (document_id < 0) throw invalid_argument("Invalid document_id"s);
    if (documents_.count(document_id) > 0) throw invalid_argument("document_id already exist"s);

    const vector<string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / static_cast<double>(words.size());

    for (const string_view word : words) {
        word_to_document_freqs_[string{word}][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.push_back(document_id);    
}

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query,
                            [status](int document_id, DocumentStatus document_status, int rating) {
                                [document_id](){};
                                [rating](){};
                                return document_status == status; });
}

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

const vector<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

const vector<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

/* Get words frequencies by doc_id. Output: map{word, frequency} */
const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const map<string_view, double> empty_map_{};

    auto find_it = document_to_word_freqs_.find(document_id);
    if (find_it != document_to_word_freqs_.end()) {     /* check if doc_id exist at server */
        return find_it->second;
    }
    return empty_map_;
}

void SearchServer::RemoveDocument(int document_id) {
    {
        auto it_find = find(document_ids_.begin(), document_ids_.end(), document_id);
        /* check if doc_id exist at server */
        if (it_find == document_ids_.end()) return;

        /* Delete from document_ids_ */
        document_ids_.erase(it_find);
    }

    /* delete from word_frequencies_for_doc_id_ */
    for (const auto& [word, _] : document_to_word_freqs_.at(document_id)) {
        auto it = word_to_document_freqs_.find(word);
        auto& id_freq = it->second;
        id_freq.erase(document_id);

        /* delete word from word_to_document, if no more documents with this word */
        if (id_freq.empty()) {
            word_to_document_freqs_.erase(it);
        }
    }

    /* delete from document_to_word_freqs_ */
    document_to_word_freqs_.erase(document_id);

    /* delete from documents_ */
    documents_.erase(document_id);
}

void SearchServer::RemoveDocument(execution::sequenced_policy policy, int document_id) {
    [policy](){};
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(execution::parallel_policy policy, int document_id) {
    [policy](){};
    
    if (documents_.empty() || word_to_document_freqs_.empty()) return;

    {
        auto it = find(document_ids_.begin(), document_ids_.end(), document_id);
        if (it == document_ids_.end()) return;  /* check if doc_id exist at server */
        document_ids_.erase(it);                /* Delete from document_ids_ */
    }

    auto doc_ptr = document_to_word_freqs_.find(document_id);    /* pointer to <document_id, map<word, freq>> with specified document_id  */
    if (doc_ptr == document_to_word_freqs_.end()) return;
    auto& word_freqs = doc_ptr->second;                          /* = document_to_word_freqs_.at(document_id) */

    /* delete from word_to_document_freq_ */
    /* parallel version */
    vector<string_view> document_words{word_freqs.size()};
    transform(execution::par, 
              word_freqs.begin(),
              word_freqs.end(),
              document_words.begin(),
              [](const auto& word_freq){ return word_freq.first; });

    for_each(execution::par, 
             document_words.begin(),
             document_words.end(),
             [this, document_id](const string_view word){ 
                    const auto word_ptr = word_to_document_freqs_.find(word);
                    //assert(word_ptr != word_to_document_freqs_.end());
                    auto& id_freq = word_ptr->second;         /* = word_to_document_freqs_.at(word) */
                    id_freq.erase(document_id);

                    /* delete word from word_to_document, if no more documents with this word */
                    if (id_freq.empty()) {
                        word_to_document_freqs_.erase(word_ptr);
                    } 
                    });

    /* delete from document_to_word_freqs_ */
    document_to_word_freqs_.erase(doc_ptr);

    /* delete from documents_ */
    documents_.erase(document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const string_view raw_query, int document_id) const {
    
    auto doc_ptr = document_to_word_freqs_.find(document_id);
    if (doc_ptr == document_to_word_freqs_.end()) {
        throw std::out_of_range("Invalid document id: "s + to_string(document_id));
    }
    const auto& word_freqs = doc_ptr->second;
    
    Query query = ParseQuery(raw_query);
    
    /* Check for minus words */
    if (any_of( query.minus_words.begin(), query.minus_words.end(),
                [&word_freqs](const string_view minus_word){ 
                    return word_freqs.count(minus_word); }))
    {
        return {vector<string_view> {}, documents_.at(document_id).status};
    }

    /* Check for plus words */
    tuple<vector<string_view>, DocumentStatus> result{vector<string_view>{}, documents_.at(document_id).status};
    auto& matched_words = get<vector<string_view>>(result);
    
    std::copy_if(query.plus_words.begin(), query.plus_words.end(), back_inserter(matched_words),
            [&word_freqs](const string_view plus_word){
                return word_freqs.count(plus_word); });

    return result;
}

tuple<vector<string_view>, DocumentStatus>
SearchServer::MatchDocument(std::execution::sequenced_policy policy, const string_view raw_query, int document_id) const {
    [policy](){};
    return MatchDocument(raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus>
SearchServer::MatchDocument(std::execution::parallel_policy policy, const string_view raw_query, int document_id) const {
    [policy](){};

    auto doc_ptr = document_to_word_freqs_.find(document_id);
    if (doc_ptr == document_to_word_freqs_.end()) {
        throw std::out_of_range("Invalid document id: "s + to_string(document_id));
    }
    const auto& word_freqs = doc_ptr->second;

    Query query = ParParseQuery(raw_query);

    /* Check for minus words */
    if (any_of(execution::par, query.minus_words.cbegin(), query.minus_words.cend(),
                [&word_freqs](const string_view minus_word){
                    return word_freqs.count(minus_word); }))
    {
        return {vector<string_view> {}, documents_.at(document_id).status};
    }

    /* Check for plus words */
    tuple<vector<string_view>, DocumentStatus> result{vector<string_view>{query.plus_words.size()}, documents_.at(document_id).status};
    auto& matched_words = get<vector<string_view>>(result);
    
    auto last = std::copy_if(execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(),
                        [&word_freqs](const string_view plus_word){ 
                            return word_freqs.count(plus_word); });
    matched_words.erase(last, matched_words.end());     // oversize correction

    /* Delete duplicates from matched_words */
    std::sort(matched_words.begin(), matched_words.end());
    auto last_el = unique(matched_words.begin(), matched_words.end());
    matched_words.erase(last_el, matched_words.end());

    return result;
}

bool SearchServer::IsStopWord(const string_view word) const {
    return stop_words_.count(word) != 0;
}

bool SearchServer::IsValidWord(const string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) { return c >= '\0' && c < ' '; });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(const string_view text) const {
    vector<string_view> words;
    for (const string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + string{word} + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const string_view text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string_view word{text};
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word.remove_prefix(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + string{text} + " is invalid");
    }
    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParParseQuery(const string_view text) const {
    Query result;
    for (const string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }

    return result;
}

SearchServer::Query SearchServer::ParseQuery(const string_view text) const {
    Query result = ParParseQuery(text);

    /* Delete duplicates */
    {
        std::sort(result.minus_words.begin(), result.minus_words.end());
        auto last = unique(result.minus_words.begin(), result.minus_words.end());
        result.minus_words.erase(last, result.minus_words.end());
    }
    {
        std::sort(result.plus_words.begin(), result.plus_words.end());
        auto last = unique(result.plus_words.begin(), result.plus_words.end());
        result.plus_words.erase(last, result.plus_words.end());
    }
    
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string_view word) const {
    auto word_ptr = word_to_document_freqs_.find(word);
    // assert(word_ptr != word_to_document_freqs_.end());
    return log(GetDocumentCount() * 1.0 / static_cast<double>(word_ptr->second.size()));
}