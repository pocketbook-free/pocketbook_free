#ifdef USESYNOPSIS

#ifndef CONTENT_H
#define CONTENT_H

#include "synopsis/synopsis.h"

class SynopsisContent: public TSynopsisContent{

	public:
		SynopsisContent();
		SynopsisContent(int level0, long long position0, char *title0);
		SynopsisContent(int level0, const char *position0, char *title0);
		int GetPage();
		int Compare(TSynopsisItem *pOther);
		virtual ~SynopsisContent();

};

inline SynopsisContent::SynopsisContent():TSynopsisContent(){};
inline SynopsisContent::SynopsisContent(int level0, long long position0, char *title0):TSynopsisContent(level0, position0, title0){};
inline SynopsisContent::SynopsisContent(int level0, const char *position0, char *title0):TSynopsisContent(level0, position0, title0){};

inline SynopsisContent::~SynopsisContent(){};

#endif

#endif
