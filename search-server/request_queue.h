#pragma once

#include "search_server.h"
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer &search_server) : search_server_(search_server) {
    }

    template<typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate) {

        auto request = search_server_.FindTopDocuments(raw_query, document_predicate);
        RequestsBufferUpdate();
        if (request.empty()) {
            requests_.push_back({raw_query});
        }
        return request;
    }

    std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string &raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        std::string raw_query;
        DocumentStatus doc_status;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    size_t requests_counter_ = 0;
    bool buffer_complete = false;
    const SearchServer &search_server_;

    void RequestsBufferUpdate();
};

