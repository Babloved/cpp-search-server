#include "document.h"
#include <iostream>

using namespace std;

std::ostream &operator<<(std::ostream &os, const Document doc) {
    std::cout << "{ "
              << "document_id = " << doc.id << ", "
              << "relevance = " << doc.relevance << ", "
              << "rating = " << doc.rating << " }";
    return os;
}
