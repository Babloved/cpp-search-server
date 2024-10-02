#pragma once

#include <iostream>
#include <vector>

template<typename Iterator>
class IteratorRange {
    Iterator it_begin_;
    Iterator it_end_;
public:
    IteratorRange(Iterator it_begin, Iterator it_end) :
            it_begin_(it_begin),
            it_end_(it_end) {}

    Iterator begin() const {
        return it_begin_;
    }

    Iterator end() const {
        return it_end_;
    }

    Iterator size() {
        return std::distance(it_begin_, it_end_);
    }
};

template<typename Iterator>
std::ostream &operator<<(std::ostream &out, const IteratorRange<Iterator> &range) {
    for (Iterator it = range.begin(); it != range.end(); ++it) {
        out << *it;
    }
    return out;
}


template<typename Iterator>
class Paginator {
public:
    Paginator(Iterator it_begin, Iterator it_end, const int page_size) {
        unsigned int current_page = 1;
        while (next(it_begin, current_page * page_size) < it_end) {
            pages_.push_back({it_begin, next(it_begin, current_page * page_size)});
            current_page++;
        }
        pages_.push_back({next(it_begin, current_page), it_end});
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template<typename Container>
auto Paginate(const Container &c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

