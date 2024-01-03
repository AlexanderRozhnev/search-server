#pragma once

#include <set>
#include <map>
#include <vector>
#include <deque>
#include <string>
#include <string_view>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <execution>

#include "string_processing.h"
#include "document.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string& stop_words_text);
    explicit SearchServer(std::string_view stop_words_text);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status,
                     const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;
    
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query, DocumentStatus status) const;

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query) const;

    int GetDocumentCount() const;

    // int GetDocumentId(int index) const;
    const std::vector<int>::const_iterator begin() const;
    const std::vector<int>::const_iterator end() const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    /* Removes document with specified id */
    void RemoveDocument(int document_id);
    void RemoveDocument(std::execution::sequenced_policy policy, int document_id);
    void RemoveDocument(std::execution::parallel_policy policy, int document_id);

    /* Returns matched words in specidied document and it's status by request of raw query, 
       that could contains plus and minus words. In case raw query provides minus word(s), 
       that the document contains, the return vector strings wold be empty */
    std::tuple<std::vector<std::string_view>, DocumentStatus>
    MatchDocument(const std::string_view raw_query, int document_id) const;
    
    std::tuple<std::vector<std::string_view>, DocumentStatus>
    MatchDocument(std::execution::sequenced_policy policy, const std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus>
    MatchDocument(std::execution::parallel_policy policy, const std::string_view raw_query, int document_id) const;

private:
    /* Set of stop-words */
    const std::set<std::string, std::less<>> stop_words_;

    /* All documents ids */
    std::vector<int> document_ids_;

    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    /* Map <all document ids, DocumentsData{rating, doc status}> */
    std::map<int, DocumentData> documents_;

    /* Map <all document words, Map <document id, word frequency at this document>>
       This is basic owner of all words in document. Other containers operate with string_view to this. */
    std::map<std::string, std::map<int, double>, std::less<>> word_to_document_freqs_;
    
    /* Map <all document ids, Map <document words, word frequency at this document>> */
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;

    static int ComputeAverageRating(const std::vector<int>& ratings);
    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    bool IsStopWord(const std::string_view word) const;
    static bool IsValidWord(const std::string_view word);
    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };
    QueryWord ParseQueryWord(const std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };
    Query ParParseQuery(const std::string_view text) const;
    Query ParseQuery(const std::string_view text) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
                                      DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(std::execution::sequenced_policy policy, 
                                const Query &query, DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(std::execution::parallel_policy policy, 
                                const Query &query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        {
            using namespace std::string_literals;
            throw std::invalid_argument("Some of stop words are invalid"s);
        }
    }
}

template <typename DocumentPredicate>
inline std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {

    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename ExecutionPolicy, typename DocumentPredicate>
inline std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy, 
                                const std::string_view raw_query, DocumentPredicate document_predicate) const {
    
    const auto query = ParseQuery(raw_query);
    if (query.plus_words.empty()) return {};

    std::vector<Document> matched_documents = FindAllDocuments(policy, std::move(query), document_predicate);

    sort(policy, matched_documents.begin(), matched_documents.end(),
         [](const Document& lhs, const Document& rhs) {
             if (std::abs(lhs.relevance - rhs.relevance) < std::numeric_limits<double>::epsilon()) {
                 return lhs.rating > rhs.rating;
             } else {
                 return lhs.relevance > rhs.relevance;
             }
         });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename ExecutionPolicy>
inline std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy, 
                                const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query,
                        [status](int document_id, DocumentStatus document_status, int rating) {
                            [document_id](){};
                            [rating](){};
                            return document_status == status; });
}

template <typename ExecutionPolicy>
inline std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy,
                                const std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
inline std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view word : query.plus_words) {
        const auto word_ptr = word_to_document_freqs_.find(word);
        if (word_ptr == word_to_document_freqs_.end()) {
            continue;
        }
        const auto& id_freq = word_ptr->second;
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : id_freq) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }
    for (const std::string_view word : query.minus_words) {
        const auto word_ptr = word_to_document_freqs_.find(word);
        if (word_ptr == word_to_document_freqs_.end()) {
            continue;
        }
        const auto& id_freq = word_ptr->second;
        for (const auto [document_id, _] : id_freq) {
            document_to_relevance.erase(document_id);
        }
    }
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

template <typename DocumentPredicate>
inline std::vector<Document> SearchServer::FindAllDocuments(std::execution::sequenced_policy policy, 
                                const Query &query, DocumentPredicate document_predicate) const {
    // Call sequenced version
    return FindAllDocuments(query, document_predicate);
}

template <typename DocumentPredicate>
inline std::vector<Document> SearchServer::FindAllDocuments(std::execution::parallel_policy policy, 
                                const Query &query, DocumentPredicate document_predicate) const {

    // Parallel version
    // std::map<int, double> document_to_relevance;
    const size_t BUCKETS = 150;
    ConcurrentMap<int, double> document_to_relevance(BUCKETS);

    // work with plus words
    std::for_each(policy, 
                  query.plus_words.begin(), query.plus_words.end(), 
                  [this, &document_to_relevance, document_predicate](const std::string_view word){
                    const auto word_ptr = word_to_document_freqs_.find(word);
                    if (word_ptr == word_to_document_freqs_.end()) return;
    
                    const auto& id_freq = word_ptr->second;     // got map with all <doc_id's, freqs> for iterated plus word
                    const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                    for (const auto [document_id, term_freq] : id_freq) {
                        const auto& document_data = documents_.at(document_id);
                        if (document_predicate(document_id, document_data.status, document_data.rating)) {
                            document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;    // fill document_to_relevance here
                        }
                    }
                  });

    // work with minus words
    std::for_each(policy,
                  query.minus_words.begin(), query.minus_words.end(),
                  [this, &document_to_relevance](const std::string_view word){
                    const auto word_ptr = word_to_document_freqs_.find(word);
                    if (word_ptr == word_to_document_freqs_.end()) return;

                    const auto& id_freq = word_ptr->second;
                    for (const auto [document_id, _] : id_freq) {
                        document_to_relevance.Erase(document_id);
                    }
                  });

    // fill matched_documents (parallel)
    std::map<int, double> docs = document_to_relevance.BuildOrdinaryMap();
    std::vector<Document> matched_documents(docs.size());
    transform(policy,
              docs.begin(), docs.end(),
              matched_documents.begin(),
              [this](const auto item) {
                    return Document{item.first, item.second, documents_.at(item.first).rating};
              });

    return matched_documents;
}
