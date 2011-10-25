#ifdef USESYNOPSIS

#include "synopsis/TOC.h"

TSynopsisItem *SynopsisTOC::CreateObject(int type){

	if (type == SYNOPSIS_CONTENT)
		return new SynopsisContent();
	if (type == SYNOPSIS_NOTE)
		return new SynopsisNote();
	if (type == SYNOPSIS_BOOKMARK)
		return new SynopsisBookmark();
	return NULL;
}

#endif
