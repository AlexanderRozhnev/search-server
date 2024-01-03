#pragma once

#include <vector>

#include "document.h"
#include "search_server.h"

/* Принимает N запросов и возвращает вектор длины N, 
   i-й элемент которого — результат вызова FindTopDocuments для i-го запроса. */
std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries); 

/* Возвращает набор документов в плоском виде */
std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server, 
    const std::vector<std::string>& queries);
