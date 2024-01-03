#include "document.h"

using namespace std;

/*  Output formatting:

    { document_id = 2, relevance = 0.402359, rating = 2 }{ document_id = 4, relevance = 0.229073, rating = 2 }
    Page break
    { document_id = 5, relevance = 0.229073, rating = 1 }
    Page break
 
*/

ostream& operator<<(std::ostream& output, Document document) {
    output  << "{ document_id = " << document.id 
            << ", relevance = " << document.relevance 
            << ", rating = " << document.rating
            << " }";
    return output;
}