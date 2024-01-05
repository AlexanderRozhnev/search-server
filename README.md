#  Search Server

[Русский язык](README.ru.md)

Search engine for searching documents based on queries. Uses algorithms used in real search engines. Single and multi-threaded operating mode.

1. [Functionalty](#functionality)
2. [System requirements and build](#requirements)
3. [Using of SearchServer class](#class)
4. [Multithreading](#multithreading)
5. [Deduplicator](#deduplicator)
6. [Paginator](#paginator)

<a id="functionality"></a>
## Functionality:
- Documents search by key words with TF-IDF ranging
- Minus-words - words to exclude document from search results
- Stop-words - words not affected to search
- Single and multythreadig mode
- Deduplicate documents (function `void RemoveDuplicates(SearchServer& search_server);`)
- Class `Paginator` for paginate results during output.

### TF-IDF ranging
TF - “term frequency”. For a specific word and a specific document, this is the share that this word occupies among all.

IDF - “inverse document frequency” is a measure of the importance of a word in a document from a collection of documents. Acts as a criterion for the relevance of a document to a search query. This value is a property of the word, not the document. The more documents a word appears in, the lower the IDF.

TF-IDF calculated as suum of all products TF to it's IDF.

<a id="requirements"></a>
## System requirements and build
C++ compiler standard 17 or above.

<a id="class"></a>
## Using of SearchServer class

SearchServer constructor gets string with stop-words separated by space or container with stop-words.

Search server filled with documents with method SearchServer::AddDocument. Method gets: document id, document's text, status, rating.

Documents search runned with method SearchServer::FindTopDocuments. Result is vector of documents, ranged on TF-IDF. It is possible to filter results by document status or filter with predicate by document id, status, rating.
Method FindTopDocuments could be called in single or multithreding mode. Multithreading version provides at least 30% better speed.

```c++
// Stop-words list
std::vector stop_words = {"in"s, "the"s, "at"s, "on"s, "near"s};
	
// server C'tor
SearchServer server(stop_words);
	
// Adding document with id only
server.AddDocument(1, "brown cat with fluffy tail"s);

// Adding documents with id, status, rating
const std::vector<int> rating = {1, 2 ,3};
server.AddDocument(2, "brown parrot in the city"s, DocumentStatus::ACTUAL, rating);
server.AddDocument(3, "brown fluffy dog with brown fluffy tail in the city"s, DocumentStatus::BANNED, rating);

// Get quantity of documents loaded
std::cout << "Q-ty of documents in the server : "s << server.GetDocumentCount() << std::endl;

// Search documensts
const auto found_docs1 = server.FindTopDocuments("brown"s);

// Search documents, excluding minus-words (minus words starts with '-')
const auto found_docs2 = server.FindTopDocuments("brown -tail"s);

// Search documents with status DocumentStatus::BANNED
const auto found_docs3 = server.FindTopDocuments("brown fluffy cat"s, DocumentStatus::BANNED);

// Search with predicate. Example search document with document id = 1
const auto predicate = [](int document_id, DocumentStatus status, int rating){
    return document_id == 1;};
const auto found_docs4 = server.FindTopDocuments("brown fluffy cat"s, predicate);
```
<a id="multithreading"></a>
## Example using multithreading search

```c++
#include <execution>

// Try policy execution::seq or execution::par

template <typename ExecutionPolicy>
void Test(std::string_view mark, const SearchServer& search_server, 
				const std::vector<std::string>& queries, ExecutionPolicy&& policy) {
    double total_relevance = 0;
    for (const std::string_view query : queries) {
        for (const auto& document : search_server.FindTopDocuments(policy, query)) {
            total_relevance += document.relevance;
        }
    }
    std::cout << total_relevance << std::endl;
}
```

<a id="deduplicator"></a>
## Deduplicator
Method `void RemoveDuplicates(SearchServer& search_server);` deletes duplicates.  
Duplicates are considered documents whose sets of words are the same. Frequency matching is not necessary. Word order is not important and stop words are ignored.

<a id="paginator"></a>
## Paginator
Example:
```c++
// Get search results
const auto search_results = server.FindTopDocuments("search query"s);

// Use paginator to split on pages
size_t page_size = 2;
const auto pages = Paginate(search_results, page_size);
for (auto page : pages) {
    cout << page << endl;
    cout << "Page break"s << endl;
}
```