#pragma once

#include <ostream>
#include <vector>

#include "document.h"

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator page_begin, Iterator page_end);
    Iterator begin() const;    
    Iterator end() const;    
    size_t size() const;
private:
    Iterator page_begin_;
    Iterator page_end_;
    size_t size_;
};

template <typename Iterator>
IteratorRange<Iterator>::IteratorRange(Iterator page_begin, Iterator page_end)
    : page_begin_(page_begin)
    , page_end_(page_end)
    , size_(distance(page_begin_, page_end_)) {
}

template <typename Iterator>
Iterator IteratorRange<Iterator>::begin() const {
    return page_begin_;
}

template <typename Iterator>
Iterator IteratorRange<Iterator>::end() const {
    return page_end_;
}

template <typename Iterator>
size_t IteratorRange<Iterator>::size() const {
    return size_;
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& output, const IteratorRange<Iterator> iterator_range) {
     for (Iterator it = iterator_range.begin(); it != iterator_range.end(); ++it) {
        output << *it;
     }
     return output;
}

template <typename Iterator>
class Paginator {
public:
    explicit Paginator(Iterator begin, Iterator end, size_t page_size);
    auto begin() const;
    auto end() const;
    size_t size() const;   
private:
    std::vector<IteratorRange<Iterator>> pages_;  /* пары {page_begin, page_end} */
};

template <typename Iterator>
Paginator<Iterator>::Paginator(Iterator begin, Iterator end, size_t page_size) {
    for (auto it = begin; it != end;) {
        Iterator page_end = it;
        if (distance(it, end) < page_size) {
            page_end = end;
        } else {
            advance(page_end, page_size);
        }
        pages_.push_back(IteratorRange(it, page_end));
        it = page_end;
    }
}

template <typename Iterator>
auto Paginator<Iterator>::begin() const {
     return pages_.begin();
}

template <typename Iterator>
auto Paginator<Iterator>::end() const {
     return pages_.end();
}

template <typename Iterator>
size_t Paginator<Iterator>::size() const {
     return pages_.size();
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator<typename Container::const_iterator>(begin(c), end(c), page_size);
}