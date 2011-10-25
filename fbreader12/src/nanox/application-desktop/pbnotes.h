/*
 * Copyright (C) 2011 Most Publishing
 * Copyright (C) 2011 Denis Kuzko <kda6666@ukr.net>
 */

#ifndef PBNOTES_H
#define PBNOTES_H

#include <synopsis/synopsis.h>

class SynopsisContent: public TSynopsisContent
{
	public:
		SynopsisContent();
		SynopsisContent(int level0, long long position0, char *title0);
		SynopsisContent(int level0, const char *position0, char *title0);
		int GetPage();
		int Compare(TSynopsisItem *pOther);
		virtual ~SynopsisContent();
};

class SynopsisBookmark: public TSynopsisBookmark
{
	public:
		SynopsisBookmark();
		int GetPage();
		int Compare(TSynopsisItem *pOther);
		virtual ~SynopsisBookmark();
};

class SynopsisMarker: public PBSynopsisMarker
{
	public:
		SynopsisMarker();
		~SynopsisMarker();
		int GetPage();
		int Compare(TSynopsisItem *pOther);
};

class SynopsisNote: public TSynopsisNote
{
	public:
		SynopsisNote();
		int GetPage();
		int Compare(TSynopsisItem *pOther);
		virtual ~SynopsisNote();
};

class SynopsisPen: public TSynopsisPen
{
	public:
		SynopsisPen();
		int GetPage();
		int Compare(TSynopsisItem *pOther);
		virtual bool Equal(const char *spos, const char *epos);
		virtual ~SynopsisPen();
};

class SynopsisSnapshot: public TSynopsisSnapshot
{
	public:
		SynopsisSnapshot();
		int GetPage();
		int Compare(TSynopsisItem *pOther);
		virtual ~SynopsisSnapshot();
};

class SynopsisTOC: public TSynopsisTOC
{
	public:
		SynopsisTOC();
		TSynopsisItem *CreateObject(int type);
		virtual ~SynopsisTOC();
};

class SynopsisToolBar: public PBSynopsisToolBar
{
	public:
		SynopsisToolBar();
		virtual ~SynopsisToolBar();

	private:
		virtual void PageBack();
		virtual void PageForward();
		virtual void OpenTOC();
		virtual void PauseCalcPage();
		virtual void ContinueCalcPage();
		virtual void RenderPage();
		virtual void CreateSnapshot(PBSynopsisSnapshot *snapshot);
};

inline SynopsisContent::SynopsisContent():TSynopsisContent(){};
inline SynopsisContent::SynopsisContent(int level0, long long position0, char *title0):TSynopsisContent(level0, position0, title0){};
inline SynopsisContent::SynopsisContent(int level0, const char *position0, char *title0):TSynopsisContent(level0, position0, title0){};
inline SynopsisContent::~SynopsisContent(){};

inline SynopsisBookmark::SynopsisBookmark():TSynopsisBookmark(){};
inline SynopsisBookmark::~SynopsisBookmark(){};

inline SynopsisMarker::SynopsisMarker():PBSynopsisMarker(){};
inline SynopsisMarker::~SynopsisMarker(){};

inline SynopsisNote::SynopsisNote():TSynopsisNote(){};
inline SynopsisNote::~SynopsisNote(){};

inline SynopsisPen::SynopsisPen():TSynopsisPen(){};
inline SynopsisPen::~SynopsisPen(){};

inline SynopsisSnapshot::SynopsisSnapshot():TSynopsisSnapshot(){};
inline SynopsisSnapshot::~SynopsisSnapshot(){};

inline SynopsisToolBar::SynopsisToolBar():PBSynopsisToolBar(){};
inline SynopsisToolBar::~SynopsisToolBar(){};

inline SynopsisTOC::SynopsisTOC():TSynopsisTOC(){};
inline SynopsisTOC::~SynopsisTOC(){};

void IfEmptyWordList();
int MarkerHandler(int type, int par1, int par2);
void SelectionTimer();

void MarkMarkers();
void DrawSynopsisLabels();
int Note_Handler(int type, int par1, int par2);
void CreateToolBar();

#endif // PBNOTES_H
