#pragma once

#include <vector>
#include <deque>
#include <string>

#include "document.h"

#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
   
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    
    int GetNoResultRequests() const;

private:
    struct QueryResult {
        bool is_no_result;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    int number_of_requests_ = 0;
    int number_of_no_result_requests_ = 0;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    /* Счётчик запросов */
    if (number_of_requests_ == min_in_day_) {
        /* Прошло больше суток. Удаляем лишний (1-й) запрос из очереди
           Значение счётчика запросов больше не меняется  */
        /* Уменьшаем счётчик пустых запросов, если удаляемый запрос был пустой */
        if (requests_.front().is_no_result) {
            --number_of_no_result_requests_;
        }
        requests_.pop_front();
    } else {
        ++number_of_requests_;
    }
    /* Добавляем новый запрос в очередь, проверив его на результативность */
    QueryResult result;
    auto search_results = search_server_.FindTopDocuments(raw_query, document_predicate);
    result.is_no_result = search_results.empty();
    requests_.push_back(result);
    
    if (result.is_no_result) ++number_of_no_result_requests_;   /* Обновляем счётчик пустых запросов */
    
    return search_results;
}