#include "main.h"
#include "pbnotes.h"

#ifdef USESYNOPSIS

extern ibitmap ac_bmk;
// TOC

SynopsisTOC::SynopsisTOC():TSynopsisTOC()
{
}

SynopsisTOC::~SynopsisTOC()
{
}

TSynopsisItem *SynopsisTOC::CreateObject(int type)
{
	if (type == SYNOPSIS_CONTENT)
		return new SynopsisContent();
	if (type == SYNOPSIS_NOTE)
		return new SynopsisNote();
	if (type == SYNOPSIS_BOOKMARK)
		return new SynopsisBookmark();

#ifdef SYNOPSISV2
	if (type == SYNOPSIS_SNAPSHOT)
		return new PBPDFSnapshot();
	if (type == SYNOPSIS_PEN)
		return new PBPDFPen();
#endif

	return NULL;
}

// Bookmark

SynopsisBookmark::SynopsisBookmark():TSynopsisBookmark()
{
}

SynopsisBookmark::~SynopsisBookmark()
{
}

int SynopsisBookmark::GetPage()
{
	return this->GetLongPosition() >> 40;
}

int SynopsisBookmark::Compare(TSynopsisItem *pOther)
{
	return strcmp(this->GetPosition(), pOther->GetPosition());
}

// Content

SynopsisContent::SynopsisContent()
	:TSynopsisContent()
{
}

SynopsisContent::SynopsisContent(int level0, long long position0, char *title0)
	:TSynopsisContent(level0, position0, title0)
{
}

SynopsisContent::SynopsisContent(int level0, const char *position0, char *title0)
	:TSynopsisContent(level0, position0, title0)
{
}

SynopsisContent::~SynopsisContent()
{
}

int SynopsisContent::GetPage()
{
	return this->GetLongPosition() >> 40;
}

int SynopsisContent::Compare(TSynopsisItem *pOther)
{
	return strcmp(this->GetPosition(), pOther->GetPosition());
}

// Note

SynopsisNote::SynopsisNote()
	:TSynopsisNote()
{
}

SynopsisNote::~SynopsisNote()
{
}

int SynopsisNote::GetPage()
{
	return this->GetLongPosition() >> 40;
}

int SynopsisNote::Compare(TSynopsisItem *pOther)
{
	return strcmp(this->GetPosition(), pOther->GetPosition());
}

#ifdef SYNOPSISV2

// Pen

PBPDFPen::PBPDFPen()
	:PBSynopsisPen()
{
}

PBPDFPen::~PBPDFPen()
{
}

int PBPDFPen::GetPage()
{
	return this->GetLongPosition() >> 40;
}

int PBPDFPen::Compare(TSynopsisItem *pOther)
{
	return strcmp(this->GetPosition(), pOther->GetPosition());
}

bool PBPDFPen::Equal(const char *spos, const char *epos)
{
	return (strcmp(this->GetPosition(), spos) == 0 && strcmp(this->GetEndPosition(), epos) == 0);
}

// Snapshot

PBPDFSnapshot::PBPDFSnapshot()
	:PBSynopsisSnapshot()
{
}

PBPDFSnapshot::~PBPDFSnapshot()
{
}

int PBPDFSnapshot::GetPage()
{
	return this->GetLongPosition() >> 40;
}

int PBPDFSnapshot::Compare(TSynopsisItem *pOther)
{
	return strcmp(this->GetPosition(), pOther->GetPosition());
}

#endif

void DrawSynopsisBookmarks(long long pos)
{
	PBSynopsisItem *item = m_TOC.GetHeaderM(SYNOPSIS_BOOKMARK);

	while (item != NULL)
	{
		PBSynopsisBookmark *bmk = (PBSynopsisBookmark *)item;
		if (bmk->GetLongPosition() == pos)
		{
			DrawBitmap(ScreenWidth() - ac_bmk.width, 0, &ac_bmk);
			break;
		}
		item = item->GetBelowM(SYNOPSIS_BOOKMARK);
	}
}

#endif
