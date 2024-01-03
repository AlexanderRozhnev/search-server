#include <algorithm>
#include <execution>
#include <numeric>

#include "process_queries.h"

using namespace std;

vector<vector<Document>> ProcessQueries(const SearchServer &search_server, const vector<string> &queries) {
    
    vector<vector<Document>> documents_lists(queries.size());

    transform(execution::par, 
              queries.begin(), 
              queries.end(), 
              documents_lists.begin(), 
              [&search_server](const string& query){ return search_server.FindTopDocuments(query);});

    return documents_lists;
}

vector<Document> ProcessQueriesJoined(const SearchServer &search_server, const vector<string> &queries) {
    vector<Document> result;
    for (const auto& documents_list : ProcessQueries(search_server, queries)) {
        for (const auto& document : documents_list) {
            result.push_back(document);
        }
    }
    return result;
}