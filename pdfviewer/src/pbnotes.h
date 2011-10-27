#ifndef PBNOTES_H
#define PBNOTES_H

#ifdef USESYNOPSIS

#include <synopsis/synopsis.h>

// TOC

class SynopsisTOC: public TSynopsisTOC
{
	public:
		SynopsisTOC();
		TSynopsisItem *CreateObject(int type);
		virtual ~SynopsisTOC();
};

// Bookmark

class SynopsisBookmark: public TSynopsisBookmark
{
	public:
		SynopsisBookmark();
		int GetPage();
		int Compare(TSynopsisItem *pOther);
		virtual ~SynopsisBookmark();
};

// Content

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

// Note

class SynopsisNote: public TSynopsisNote
{
	public:
		SynopsisNote();
		int GetPage();
		int Compare(TSynopsisItem *pOther);
		virtual ~SynopsisNote();
};

#ifdef SYNOPSISV2

// Pen

class PBPDFPen : public PBSynopsisPen
{
public:
	PBPDFPen();
	virtual ~PBPDFPen();

	int GetPage();
	int Compare(TSynopsisItem *pOther);
	virtual bool Equal(const char *spos, const char *epos);

//protected:
//	dp::ref<dpdoc::Location> m_spStartLoc;
};

// Snapshot

class PBPDFSnapshot : public PBSynopsisSnapshot
{
public:
	PBPDFSnapshot();
	virtual ~PBPDFSnapshot();
	int GetPage();
	int Compare(TSynopsisItem *pOther);

//protected:
//	dp::ref<dpdoc::Location> m_spStartLoc;
};

#endif

void DrawSynopsisBookmarks(long long pos);
#endif // USESYNOPSIS

#endif // PBNOTES_H
