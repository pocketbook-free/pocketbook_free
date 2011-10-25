#ifdef USESYNOPSIS

#ifndef TOC_H
#define TOC_H

#include "synopsis/synopsis.h"
#include "content.h"
#include "note.h"
#include "bookmark.h"

class SynopsisTOC: public TSynopsisTOC{

	public:
		SynopsisTOC();
		TSynopsisItem *CreateObject(int type);
		virtual ~SynopsisTOC();

};

inline SynopsisTOC::SynopsisTOC():TSynopsisTOC(){};
inline SynopsisTOC::~SynopsisTOC(){};

#endif

#endif
