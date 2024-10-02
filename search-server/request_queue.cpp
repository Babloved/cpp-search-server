#include "request_queue.h"

using namespace std;

vector<Document> RequestQueue::AddFindRequest(const string &raw_query, DocumentStatus status) {
    auto request = search_server_.FindTopDocuments(raw_query, status);
    RequestsBufferUpdate();
    if (request.empty()) {
        requests_.push_back({raw_query, status});
    }
    return request;
}

vector<Document> RequestQueue::AddFindRequest(const string &raw_query) {
    auto request = search_server_.FindTopDocuments(raw_query);
    RequestsBufferUpdate();
    if (request.empty()) {
        requests_.push_back({raw_query, DocumentStatus::ACTUAL});
    }
    return request;
}

int RequestQueue::GetNoResultRequests() const {
    return requests_.size();
}

void RequestQueue::RequestsBufferUpdate() {
    if (buffer_complete) {
        requests_.pop_front();
    } else if (requests_counter_ >= min_in_day_) {
        buffer_complete = true;
        requests_counter_ = 0;
        requests_.pop_front();
    } else {
        requests_counter_++;
    }
}

