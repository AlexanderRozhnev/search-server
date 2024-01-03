#  Search Server

1. [Functionalty](#functionality)
2. [System requirements and build](#requirements)
3. [Using of SearchServer class](#class)
4. [Multithreading](#multithreading)

<a id="functionality"></a>
## Function:
- Поиск документов по ключевым словам с ранжированием документов по TF-IDF
- Минус-слова - слова, исключающие, содержащие их документы из результата поиска
- Стоп-слова - слова, не участвующие в поиске
- Поиск документов в режиме последовательных и парраллельных вычислений

Ранжирование TF-IDF
TF - term frequency, «частота термина». Для конкретного слова и конкретного документа это доля, которую данное слово занимает среди всех.

IDF - inverse document frequency, «обратная частота документа» — мера важности слова в документе из коллекции документов. Выступает критерием релевантности документа поисковому запросу. Эта величина — свойство слова, а не документа. Чем в большем количестве документов есть слово, тем ниже IDF.

Параметр TF-IDF документа вычислется как сумма произведений всех TF слова на его IDF.

<a id="requirements"></a>
## System requirements and build
C++ compiler standard 17 or above.

<a id="class"></a>
## Using of SearchServer class
В конструктор класса передаётся строка со списком стоп-слов, разделённых пробелами либо контейнер со стоп-словами.

Заполенение поискового сервера документами осуществлется с помощью метоода SearchServer::AddDocument. Метод принимает: id документа, его текст, статус, рейтинги.

Поиск документов, осуществляется методом SearchServer::FindTopDocuments. Результат возвращается в виде вектора документов, отсортированных по TF-IDF. Возможна фильтрация результата по статусу или по произвольному предикату, позволяющему состировать по id документа, статусу, рейтингу.
Метод FindTopDocuments имеет однопоточную и многопоточную версии. Многопоточная версия обеспечивает по меньшей мере на 30% более высокое быстродействие.

```
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
## Пример поиска в многопоточном режиме

```
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

## RequestQueue