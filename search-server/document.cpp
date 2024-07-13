#include "document.h"

using namespace std;


ostream &operator<< (ostream &stream, const Document document) {
    stream << "{ document_id = "s << document.id << ", "s  <<  "relevance = "s 
    <<  document.relevance << ", "s <<  "rating = "s << document.rating << " }"; 
    return stream;
}