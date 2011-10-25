#ifdef USESYNOPSIS

#ifndef NOTE_H
#define NOTE_H

#include "synopsis/synopsis.h"

class SynopsisNote: public TSynopsisNote{

	public:
		SynopsisNote();
		int GetPage();
		int Compare(TSynopsisItem *pOther);
		virtual ~SynopsisNote();

		void GetNoteBookmarks(int y1, int y2, char *sbookmark, char *ebookmark);

};

inline SynopsisNote::SynopsisNote():TSynopsisNote(){};
inline SynopsisNote::~SynopsisNote(){};

#endif

#endif
