#ifdef USESYNOPSIS

#ifndef BOOKMARK_H
#define BOOKMARK_H

#include "synopsis/synopsis.h"

class SynopsisBookmark: public TSynopsisBookmark{

	public:
		SynopsisBookmark();
		int GetPage();
		int Compare(TSynopsisItem *pOther);
		virtual ~SynopsisBookmark();

};

inline SynopsisBookmark::SynopsisBookmark():TSynopsisBookmark(){};
inline SynopsisBookmark::~SynopsisBookmark(){};

#endif

#endif
