#pragma once

#include <algorithm>
#include <iostream>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange() = default;
    IteratorRange(Iterator it_begin, Iterator it_end): begin_(it_begin), end_(it_end)
    {
    }
    Iterator begin() const {
        return begin_;
    }
    Iterator end() const {
        return end_;
    }
    int size() {
        return distance(end_, begin_);
    }

private:
    Iterator begin_;
    Iterator end_;
};
template <typename MyIterator>
std::ostream &operator<< (std::ostream &stream, const IteratorRange<MyIterator> range) {
    for(MyIterator tmp = range.begin(); tmp != range.end(); ++tmp) {
        stream << *tmp; 
    }
    return stream;
}

template <typename Iterator>
class Paginator {
public:
    Paginator() = default;
    Paginator(Iterator it_begin, Iterator it_end, size_t page_size) {
       for(size_t docs_left = distance(it_begin, it_end); docs_left > 0; ) {
            const size_t current_page_size = std::min(page_size, docs_left);
            const auto current_it_page_end = next(it_begin, current_page_size);
            pages_.push_back({it_begin, current_it_page_end});

            docs_left -= current_page_size;
            it_begin = current_it_page_end;
       } 
    }
    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    size_t size() {
        return pages_.size();
    }


private:
     std::vector<IteratorRange<Iterator>> pages_;
}; 

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
