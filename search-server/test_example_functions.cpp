#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>
#include <iostream>

#include "search_server.h"

#include "test_example_functions.h"

using namespace std;

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s);
        assert(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        assert(doc0.id == doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,SearchServer server("with"s)
    // возвращает пустой результат
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        assert(server.FindTopDocuments("in"s).empty());
    }
}

/*
Разместите код остальных тестов здесь
*/

// Тест. Добавление документов. Добавленный документ должен находиться по поисковому запросу, который содержит слова из документа.
void TestAddDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city New York"s;
    const vector<int> ratings = {1, 2, 3};
    
    // Поиск документа по слову, не входящему в список стоп-слов
    // находит _нужный_ документ
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("city"s);
        assert(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        assert(doc0.id == doc_id);
    }
    // Убедимся, что документ НЕ будет найден по слову НЕ входящему в документ
    // возвращает пустой результат
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        assert(server.FindTopDocuments("citi"s).empty());
    }
}

// Поддержка стоп-слов. Стоп-слова исключаются из текста документов.
// Реализована выше в примере

// Поддержка минус-слов. Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.
void TestExcludeMinusWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Поиск без минус слов
    // находит _нужный_ документ
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        {
            const auto found_docs = server.FindTopDocuments("cat"s);
            assert(found_docs.size() == 1);
            const Document& doc0 = found_docs[0];
            assert(doc0.id == doc_id);
        }
        {
            const auto found_docs = server.FindTopDocuments("city"s);
            assert(found_docs.size() == 1);
            const Document& doc0 = found_docs[0];
            assert(doc0.id == doc_id);
        }
    }

    // Поиск с минус-словами
    // возвращает пустой результат
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        assert(server.FindTopDocuments("cat -city"s).empty());

        assert(server.FindTopDocuments("-cat city"s).empty());
    }
}

// Матчинг документов. 
// При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса,
// присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову,
// должен возвращаться пустой список слов.
void TestDocumentMatching() {
    const int doc_id = 42;
    const string content = "brown cat with fluffy tail"s;
    const vector<int> rating = {1, 2 ,3};
    
    // Поиск без минус слов
    // возвращает _все_слова_из_поисковго_запроса_ документа
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, rating);
        
        // const auto tuple<vector<string>, DocumentStatus> - {{слова_документа}, статус_документа}
        const auto [docs_words, status] = server.MatchDocument("cat brown tail"s, doc_id);
        assert(docs_words.size() == 3);
        // assert(docs_words[0] == "brown"sv);
        // assert(docs_words[1] == "cat"sv);
        // assert(docs_words[2] == "tail"sv);
    }
    // Поиск с минус словами
    // возвращает _пустой_ результат
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, rating);
        const auto& [docs_words, status] = server.MatchDocument("cat brown tail -fluffy"s, doc_id);
        assert(docs_words.size() == 0);
    }
}

// Сортировка найденных документов по релевантности. 
// Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void TestDocumentsSortedByRelevance() {
    const vector<int> rating = {1, 2 ,3};

    // Поиск документов
    // возвращает документы в порядке _убывания_релевантности_
    // релевантность выше у документов с большей частотность слова в документе (TF) и большей уникальностью слов (IDF)
    {
        SearchServer server("in the"s);
        server.AddDocument(111, "brown cat with fluffy tail"s, DocumentStatus::ACTUAL, rating);
        server.AddDocument(222, "brown parrot in the city"s, DocumentStatus::ACTUAL, rating);
        server.AddDocument(333, "brown fluffy dog with brown fluffy tail in the city"s, DocumentStatus::ACTUAL, rating);

        // brown, tail => 111, 333, 222
        {
            const auto found_docs = server.FindTopDocuments("brown fluffy tail"s);
            assert(found_docs.size() == 3);
            const Document& doc0 = found_docs[0];
            const Document& doc1 = found_docs[1];
            const Document& doc2 = found_docs[2];
            assert((doc0.relevance >= doc1.relevance) && (doc1.relevance >= doc2.relevance));
            assert(doc0.id == 111);
            assert(doc2.id == 222);
        }
        // parrot, fluffy, city => 222, 333, 111
        {
            const auto found_docs = server.FindTopDocuments("parrot fluffy city"s);
            assert(found_docs.size() == 3);
            const Document& doc0 = found_docs[0];
            const Document& doc1 = found_docs[1];
            const Document& doc2 = found_docs[2];
            assert((doc0.relevance >= doc1.relevance) && (doc1.relevance >= doc2.relevance));
            assert(doc0.id == 222);
            assert(doc2.id == 111);
        }
        // fluffy, dog, city, tail => 333, 111, 222
        {
            const auto found_docs = server.FindTopDocuments("dog fluffy tail city"s);
            assert(found_docs.size() == 3);
            const Document& doc0 = found_docs[0];
            const Document& doc1 = found_docs[1];
            const Document& doc2 = found_docs[2];
            assert((doc0.relevance >= doc1.relevance) && (doc1.relevance >= doc2.relevance));
            assert(doc0.id == 333);
            assert(doc2.id == 222);
        }
    }
}

//Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void TestDocumentsRating(){
    const vector<int> rating0 = {0, 0, 0};          const int avg0 = (0 + 0 + 0) / 3;
    const vector<int> rating1 = {1, 0, 0};          const int avg1 = (1 + 0 + 0) / 3;
    const vector<int> rating2 = {0, 1, 2, 99};      const int avg2 = (0 + 1 + 2 + 99) / 4;
    const vector<int> rating3 = {5000, 101, 1, 1};  const int avg3 = (5000 + 101 + 1 + 1) / 4;
    const vector<int> rating4 = {5, 5, 5};          const int avg4 = (5 + 5 + 5 + 5 + 5) / 5;

    // Поиск документов с возвратом рейтинга
    // возвращает _правильный_ рейтинг документов
    {
        SearchServer server("in the"s);
        server.AddDocument(111, "brown cat with fluffy tail"s, DocumentStatus::ACTUAL, rating0);
        const auto found_docs = server.FindTopDocuments("brown cat"s);
        const Document& doc = found_docs[0];
        assert(doc.rating == avg0);
    }
    {
        SearchServer server("in the"s);
        server.AddDocument(111, "brown cat with fluffy tail"s, DocumentStatus::ACTUAL, rating1);
        const auto found_docs = server.FindTopDocuments("brown cat"s);
        const Document& doc = found_docs[0];
        assert(doc.rating == avg1);
    }
    {
        SearchServer server("in the"s);
        server.AddDocument(111, "brown cat with fluffy tail"s, DocumentStatus::ACTUAL, rating2);
        const auto found_docs = server.FindTopDocuments("brown cat"s);
        const Document& doc = found_docs[0];
        assert(doc.rating == avg2);
    }
    {
        SearchServer server("in the"s);
        server.AddDocument(111, "brown cat with fluffy tail"s, DocumentStatus::ACTUAL, rating3);
        const auto found_docs = server.FindTopDocuments("brown cat"s);
        const Document& doc = found_docs[0];
        assert(doc.rating == avg3);
    }
    {
        SearchServer server("in the"s);
        server.AddDocument(111, "brown cat with fluffy tail"s, DocumentStatus::ACTUAL, rating4);
        const auto found_docs = server.FindTopDocuments("brown cat"s);
        const Document& doc = found_docs[0];
        assert(doc.rating == avg4);
    }
}

// Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestFindDocumentsByUserPredicate() {
    const int doc_id1 = 42;
    const int doc_id2 = 333;
    const string content1 = "brown cat with fluffy tail"s;
    const string content2 = "brown fluffy dog with brown fluffy tail in the city"s;
    const vector<int> rating1 = {1, 2 ,3}; const int avg1 = (1+2+3) / 3;
    const vector<int> rating2 = {3, 4 ,5}; const int avg2 = (3+4+5) / 3;
    {
        // Поиск документов с использованием предиката
        SearchServer server("in the"s);

        // возвращает _пустой_ список
        {
            const auto predicate = [](int document_id, DocumentStatus status, int rating){return status == DocumentStatus::ACTUAL;};
            const auto found_docs = server.FindTopDocuments("brown fluffy cat"s, predicate);
            assert(found_docs.size() == 0);
        }
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, rating1);
        server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, rating2);

        // возвращает один документ со статусом DocumentStatus::ACTUAL
        {
            const auto predicate = [](int document_id, DocumentStatus status, int rating){return status == DocumentStatus::ACTUAL;};
            const auto found_docs = server.FindTopDocuments("brown fluffy cat"s, predicate);
            assert(found_docs.size() == 1);
            const Document& doc = found_docs[0];
            assert(doc.id == doc_id1);
        }
        // Возвращает один документ со статусом DocumentStatus::BANNED
        {
            const auto predicate = [](int document_id, DocumentStatus status, int rating){return status == DocumentStatus::BANNED;};
            const auto found_docs = server.FindTopDocuments("brown fluffy cat"s, predicate);
            assert(found_docs.size() == 1);
            const Document& doc = found_docs[0];
            assert(doc.id == doc_id2);
        }
        // Возвращает один документ с doc_id1
        {
            const auto predicate = [](int document_id, DocumentStatus status, int rating){return document_id == doc_id1;};
            const auto found_docs = server.FindTopDocuments("brown fluffy cat"s, predicate);
            assert(found_docs.size() == 1);
            const Document& doc = found_docs[0];
            assert(doc.id == doc_id1);
        }
        // Возвращает один документ с doc_id2
        {
            const auto predicate = [](int document_id, DocumentStatus status, int rating){return document_id == doc_id2;};
            const auto found_docs = server.FindTopDocuments("brown fluffy cat"s, predicate);
            assert(found_docs.size() == 1);
            const Document& doc = found_docs[0];
            assert(doc.id == doc_id2);
        }
        // Возвращает один документ с рейтингом avg1
        {
            const auto predicate = [](int document_id, DocumentStatus status, int rating){return rating == avg1;};
            const auto found_docs = server.FindTopDocuments("brown fluffy cat"s, predicate);
            assert(found_docs.size() == 1);
            const Document& doc = found_docs[0];
            assert(doc.rating == avg1);
        }
        // Возвращает один документ с рейтингом avg2
        {
            const auto predicate = [](int document_id, DocumentStatus status, int rating){return rating == avg2;};
            const auto found_docs = server.FindTopDocuments("brown fluffy cat"s, predicate);
            assert(found_docs.size() == 1);
            const Document& doc = found_docs[0];
            assert(doc.rating == avg2);            
        }
    }
}

// Поиск документов, имеющих заданный статус.
void TestFindDocumentsWithCertainStatus() {
    const int doc_id1 = 42;
    const int doc_id2 = 333;
    const string content1 = "brown cat with fluffy tail"s;
    const string content2 = "brown fluffy dog with brown fluffy tail in the city"s;
    const vector<int> rating1 = {1, 2 ,3};
    const vector<int> rating2 = {3, 4 ,5};
    const DocumentStatus status1 = DocumentStatus::ACTUAL;
    const DocumentStatus status2 = DocumentStatus::BANNED;
    {
        // Поиск документов по статусу
        SearchServer server("in the"s);
            
        // возвращает _пустой_ список
        {
            const auto found_docs = server.FindTopDocuments("brown fluffy cat"s, status1);
            assert(found_docs.size() == 0);
        }
        server.AddDocument(doc_id1, content1, status1, rating1);
        server.AddDocument(doc_id2, content2, status2, rating2);
        
        // возвращает один документ со статусом DocumentStatus::ACTUAL
        {
            const auto found_docs = server.FindTopDocuments("brown fluffy cat"s, status1);
            assert(found_docs.size() == 1);
            const Document& doc = found_docs[0];
            assert(doc.id == doc_id1);
        }
        // Возвращает один документ со статусом DocumentStatus::BANNED
        {
            const auto found_docs = server.FindTopDocuments("brown fluffy cat"s, status2);
            assert(found_docs.size() == 1);
            const Document& doc = found_docs[0];
            assert(doc.id == doc_id2);
        }
    }
}

// Корректное вычисление релевантности найденных документов.
// частотность слова в документе (TF) = кол-во term / всего слов в документе
// уникальность слова среди документов (IDF) = log(Кол-во документов * 1.0 / в скольких документах слово запроса);
void TestRelevanceCalculate() {
    const int doc_id1 = 42;
    const int doc_id2 = 333;
    const string content1 = "brown cat with fluffy tail"s;
    const string content2 = "brown fluffy dog with brown fluffy tail in the city"s;
    const string query1 = "cat"s;
    const double relevance1 = (1.0 / 5) * log( 2.0 / 1 );
    const string query2 = "brown cat"s;
    const double relevance2 = (1.0 / 5) * log( 2.0 / 2 ) + ( 1.0 /5 ) * log( 2.0 / 1 );
   
    const vector<int> rating1 = {1, 2 ,3};
    const vector<int> rating2 = {3, 4 ,5};
    const DocumentStatus status1 = DocumentStatus::ACTUAL;
    const DocumentStatus status2 = DocumentStatus::BANNED;

    // Поиск документов    
    // возвращает все добавленные документы с _правильно_посчитанными_рейтингами_
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id1, content1, status1, rating1);
        server.AddDocument(doc_id2, content2, status2, rating2);

        const auto found_docs = server.FindTopDocuments(query1, status1);
        assert(found_docs.size() == 1);
        const Document& doc = found_docs[0];
        assert(doc.relevance == relevance1);
    }
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id1, content1, status1, rating1);
        server.AddDocument(doc_id2, content2, status2, rating2);

        const auto found_docs = server.FindTopDocuments(query1, status1);
        assert(found_docs.size() == 1);
        const Document& doc = found_docs[0];
        assert(doc.relevance == relevance2);
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    TestAddDocumentContent();
    TestExcludeStopWordsFromAddedDocumentContent();
    TestExcludeMinusWordsFromAddedDocumentContent();
    TestDocumentMatching();
    TestDocumentsSortedByRelevance();
    TestDocumentsRating();
    TestFindDocumentsByUserPredicate();
    TestFindDocumentsWithCertainStatus();
    TestRelevanceCalculate();
}

// --------- Окончание модульных тестов поисковой системы -----------

void MyTest() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}