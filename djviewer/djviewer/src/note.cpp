#ifdef USESYNOPSIS

#include "synopsis/note.h"

int SynopsisNote::Compare(TSynopsisItem *pOther){

	return strcmp(this->GetPosition(), pOther->GetPosition());
	
}

#endif
