#ifdef USESYNOPSIS

#include "synopsis/content.h"

int SynopsisContent::Compare(TSynopsisItem *pOther){

	return strcmp(this->GetPosition(), pOther->GetPosition());
	
}

#endif
