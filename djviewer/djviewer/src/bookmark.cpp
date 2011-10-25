#ifdef USESYNOPSIS

#include "synopsis/bookmark.h"

int SynopsisBookmark::Compare(TSynopsisItem *pOther){

	return strcmp(this->GetPosition(), pOther->GetPosition());
	
}
#endif
