/*
 * Copyright (C) 2011 Most Publishing
 * Copyright (C) 2011 Denis Kuzko <kda6666@ukr.net>
 */

#include <ZLApplication.h>
#include "../../fbreader/FBReader.h"
#include "../../fbreader/BookTextView.h"
#include "../../fbreader/FootnoteView.h"

#include "../../area/ZLTextSelectionModel.h"

#include "pbnotes.h"
#include "main.h"
#include "pbmainframe.h"
#include <inkview.h>
#include <inkinternal.h>

extern ZLApplication *mainApplication;
extern BookTextView *bookview;
extern FootnoteView *footview;

extern int calc_in_progress;
extern int calc_position_changed;
extern int npages;
extern long long cpos;

extern unsigned char *screenbuf;
extern int imgposx, imgposy, imgwidth, imgheight, scanline;

extern SynWordList *acwordlist;
extern int acwlistcount;
extern int acwlistsize;
extern "C" const ibitmap ac_note, ac_bmk;

TSynopsisWordList List;
SynopsisPen *Pen;
SynopsisTOC SynTOC;
SynopsisToolBar ToolBar;

int selMarkery1, selMarkery2;
int updMarkerx, updMarkery, updMarkerw, updMarkerh;
int trailMarker = 0;
bool selectingMarker = false;
int maxstrheight;
bool hadMarkers = false;
int Tool = 0;
bool calc_paused = false;

extern ibitmap comment_icon, pencil_icon;

fb2_settings PenSettings;

// Content
int SynopsisContent::Compare(TSynopsisItem *pOther)
{
	return strcmp(this->GetPosition(), pOther->GetPosition());
}

int SynopsisContent::GetPage()
{
	return calc_in_progress ? -1 : position_to_page(this->GetLongPosition());
}

// Bookmark
int SynopsisBookmark::Compare(TSynopsisItem *pOther)
{
	return strcmp(this->GetPosition(), pOther->GetPosition());
}

int SynopsisBookmark::GetPage()
{
	return calc_in_progress ? -1 : position_to_page(this->GetLongPosition());
}

// Marker
int SynopsisMarker::GetPage()
{
	return calc_in_progress ? -1 : position_to_page(this->GetLongPosition());
}

int SynopsisMarker::Compare(TSynopsisItem *pOther)
{
	return strcmp(this->GetPosition(), pOther->GetPosition());
}

// Note
int SynopsisNote::GetPage()
{
	return calc_in_progress ? -1 : position_to_page(this->GetLongPosition());
}

int SynopsisNote::Compare(TSynopsisItem *pOther)
{
	return strcmp(this->GetPosition(), pOther->GetPosition());
}

// Pen
int SynopsisPen::GetPage()
{
	return calc_in_progress ? -1 : position_to_page(this->GetLongPosition());
}

int SynopsisPen::Compare(TSynopsisItem *pOther)
{
	return strcmp(this->GetPosition(), pOther->GetPosition());
}

bool SynopsisPen::Equal(const char *spos, const char *epos)
{
	return (strcmp(this->GetPosition(), spos) == 0 && strcmp(this->GetEndPosition(), epos) == 0);
}

// Snapshot
int SynopsisSnapshot::GetPage()
{
	return calc_in_progress ? -1 : position_to_page(this->GetLongPosition());
}

int SynopsisSnapshot::Compare(TSynopsisItem *pOther)
{
	return strcmp(this->GetPosition(), pOther->GetPosition());
}

// TOC
TSynopsisItem *SynopsisTOC::CreateObject(int type){

	if (type == SYNOPSIS_CONTENT)
		return new SynopsisContent();
	if (type == SYNOPSIS_NOTE)
		return new SynopsisNote();
	if (type == SYNOPSIS_BOOKMARK)
		return new SynopsisBookmark();
	if (type == SYNOPSIS_PEN)
	{
		SynopsisPen *pen = new SynopsisPen();
		pen->SetSettingsId(GetSettings()->GetFB2SettingsId(PenSettings));
		return pen;
	}
	if (type == SYNOPSIS_MARKER)
		return new SynopsisMarker();
	if (type == SYNOPSIS_COMMENT)
	{
		PBSynopsisItem *item = new SynopsisMarker();
		item->SetType(SYNOPSIS_COMMENT);
		return item;
	}
	if (type == SYNOPSIS_SNAPSHOT)
		return new SynopsisSnapshot();
	return NULL;
}

// ToolBar
void SynopsisToolBar::PageBack()
{
	one_page_back(-1);
	OpenSynopsisToolBar();
}

void SynopsisToolBar::PageForward()
{
	one_page_forward(-1);
	OpenSynopsisToolBar();
}

void SynopsisToolBar::OpenTOC()
{
	open_synopsis_contents();
}

void SynopsisToolBar::PauseCalcPage()
{
	if (calc_in_progress)
		calc_paused = true;
}

void SynopsisToolBar::ContinueCalcPage()
{
	if (calc_in_progress && calc_paused)
	{
		calc_paused = false;
		SetHardTimer("CalcPages", calc_timer, 0);
	}
}

void SynopsisToolBar::RenderPage()
{
	repaint_all(-2);
}

void SynopsisToolBar::CreateSnapshot(PBSynopsisSnapshot *snapshot)
{
	snapshot->Create((PBSynopsisTOC *)&SynTOC, cpos);
}

// Markers functions

void FlushMarker(PBSynopsisItem *item)
{
	// Flush marker or comment on page
	if (item != NULL && (item->GetType() == SYNOPSIS_MARKER || item->GetType() == SYNOPSIS_COMMENT))
	{
		int spi, sei, sci, epi, eei, eci;
		PBSynopsisMarker *marker = (PBSynopsisMarker *)item;
		if (marker != NULL)
		{
			unpack_position(marker->GetLongPosition(), &spi, &sei, &sci);
			unpack_position(marker->GetEndLongPosition(), &epi, &eei, &eci);
			int i;
			for (i = 0; i < 2; i++)
			{
				bookview->selectionModel().activate(0, 0);
				bookview->selectionModel().extendTo(ScreenWidth(), ScreenHeight());
				bookview->selectionModel().SetSelection(spi, sei, sci, epi, eei, eci);
				repaint_all(-2);
				PartialUpdateBW(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight());
				bookview->selectionModel().clear();
				repaint_all(-2);
				PartialUpdateBW(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight());
			}
		}
	}
}

void MarkMarkers()
{
	// highlight markers and comments on page
	PrepareSynTOC(0);
	hadMarkers = false;
	bookview->selectionModel().ClearMarkers();
	restore_current_position();
	mainApplication->refreshWindow();

	IfEmptyWordList();

	if (acwlistcount == 0)
		return;

	long long spos, epos;
	spos = pack_position(acwordlist[0].pnum, acwordlist[0].wnum, 0);
	epos = pack_position(acwordlist[acwlistcount - 1].pnum, acwordlist[acwlistcount - 1].wnum, 0);

	PBSynopsisItem *item = SynTOC.GetHeader();
	while (item != NULL && item->GetLongPosition() <= epos)
	{
		if (item->GetType() == SYNOPSIS_MARKER || item->GetType() == SYNOPSIS_COMMENT)
		{
			SynopsisMarker *marker = (SynopsisMarker *)item;
			long long startpos = marker->GetLongPosition();
			long long endpos = marker->GetEndLongPosition();
			if ((spos >= startpos && spos <= endpos) ||
				(spos <= startpos && epos >= endpos) ||
				(epos >= startpos && epos <= endpos))
			{
				int spi, sei, sci, epi, eei, eci;
				unpack_position(startpos, &spi, &sei, &sci);
				unpack_position(endpos, &epi, &eei, &eci);
				bookview->selectionModel().AddMarker(spi, sei, sci, epi, eei, eci);
				hadMarkers = true;
			}
		}
		item = item->GetBelowM(SYNOPSIS_MARKER | SYNOPSIS_COMMENT);
	}
}

void MergeMarkers(long long spos, long long epos)
{
	// merge markers or comments with new marker
	PBSynopsisItem *item = SynTOC.GetHeader();
	SynopsisMarker *marker;
	while (item != NULL && item->GetLongPosition() <= epos)
	{
		if (item->GetType() == SYNOPSIS_MARKER)
		{
			marker = (SynopsisMarker *)item;
			long long startpos = marker->GetLongPosition();
			long long endpos = marker->GetEndLongPosition();
			if ((spos < startpos && epos >= startpos && epos <= endpos) ||
				(spos <= startpos && epos >= endpos) ||
				(spos >= startpos && spos <= endpos && epos > endpos))
			{
				if (spos > startpos)
				{
					spos = startpos;
				}
				if (epos < endpos)
				{
					epos = endpos;
				}
				PBSynopsisItem *olditem = item;
				item = item->GetBelowM(SYNOPSIS_MARKER | SYNOPSIS_COMMENT);
				SynTOC.DeleteItem(olditem);
				continue;
			}
			else if (spos >= startpos && epos <= endpos)
			{
				return;
			}
		}

		if (item->GetType() == SYNOPSIS_COMMENT)
		{
			marker = (SynopsisMarker *)item;
			long long startpos = marker->GetLongPosition();
			long long endpos = marker->GetEndLongPosition();
			if (spos < startpos && epos >= startpos && epos <= endpos)
			{
				marker = new SynopsisMarker();
				marker->Create((TSynopsisTOC*)&SynTOC, spos, endpos, item->GetTitle(), item->GetText());
				SynTOC.DeleteItem(item);
				return;
			}
			else if (spos <= startpos && epos > endpos)
			{
				marker = new SynopsisMarker();
				marker->Create((TSynopsisTOC*)&SynTOC, spos, endpos, item->GetTitle(), item->GetText());
				SynTOC.DeleteItem(item);
				int pi, ei, ci;
				unpack_position(endpos, &pi, &ei, &ci);
				spos = pack_position(pi, ++ei, 0);
			}
			else if (spos >= startpos && spos <= endpos && epos > endpos)
			{
				int pi, ei, ci;
				unpack_position(endpos, &pi, &ei, &ci);
				spos = pack_position(pi, ++ei, 0);
			}
			else if (spos >= startpos && epos <= endpos)
			{
				return;
			}
		}
		item = item->GetBelowM(SYNOPSIS_MARKER | SYNOPSIS_COMMENT);
	}
	int spi, sei, sci, epi, eei, eci;
	unpack_position(spos, &spi, &sei, &sci);
	unpack_position(epos, &epi, &eei, &eci);
	bookview->selectionModel().SetSelection(spi, sei, sci, epi, eei, eci);
	const char *text = bookview->selectionModel().text().c_str();
	if (text != NULL && strlen(text) > 2)
	{
		marker = new SynopsisMarker();
		marker->Create((TSynopsisTOC*)&SynTOC, spos, epos, text);
	}
}

void GetSelectionInfo(long long *spos, long long *epos, bool *EmptyText, bool *ClickOnly)
{
	// Get start and end position, have text and make selection or not
	int spi, sei, sci, epi, eei, eci;
	bookview->selectionModel().GetSelection(&spi, &sei, &sci, &epi, &eei, &eci);
	*ClickOnly = (spi == epi && sei == eei);
	*spos = pack_position(spi, sei, sci);
	*epos = pack_position(epi, eei, eci);
	long long tmp;
	if (*spos > *epos)
	{
		tmp = *spos;
		*spos = *epos;
		*epos = tmp;
	}
	const char *text = bookview->selectionModel().text().c_str();
	*EmptyText = (text != NULL && strlen(text) < 2);
}

void OpenMarkerMenu(PBSynopsisItem *item)
{
	// Prepare page and open marker menu
	if (item != NULL)
	{
		int spi, sei, sci, epi, eei, eci;
		PBSynopsisMarker *marker = (PBSynopsisMarker *)item;
		selectingMarker = false;
		ClearTimer(SelectionTimer);
		unpack_position(marker->GetLongPosition(), &spi, &sei, &sci);
		unpack_position(marker->GetEndLongPosition(), &epi, &eei, &eci);
		bookview->selectionModel().SetSelection(spi, sei, sci, epi, eei, eci);
		repaint_all(-1);
		ToolBar.OutputPage(BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight()));
		ToolBar.OpenMarkerMenu(marker);
		bookview->selectionModel().clear();
	}
}

int usersleep, lastx, lasty;

void SelectionTimer()
{
	repaint_all(-1);
	ToolBar.OutputPage(BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight()),
				1, updMarkerx, updMarkery, updMarkerw, updMarkerh);
	if (++usersleep < 50)
		SetHardTimer("SelectionTimer", SelectionTimer, 25);
	else
		MarkerHandler(EVT_POINTERUP, lastx, lasty);
}

int MarkerHandler(int type, int par1, int par2)
{
	if (type == EVT_POINTERDOWN)
	{
		selectingMarker = true;
		restore_current_position();
		mainApplication->refreshWindow();
		bookview->selectionModel().clear();
		bookview->selectionModel().activate(par1 - imgposx, par2 - imgposy);
		SetHardTimer("SelectionTimer", SelectionTimer, 250);
		selMarkery1 = selMarkery2 = lasty = par2;
		lastx = par1;
		usersleep = 0;
	}
	else if (type == EVT_POINTERMOVE)
	{
		if (++trailMarker > 8)
		{
			if (selectingMarker)
			{
				lastx = par1;
				lasty = par2;
				bookview->selectionModel().extendTo(par1 - imgposx, par2 - imgposy);
				selMarkery2 = par2;
				usersleep = 0;
			}
			trailMarker = 0;
		}
	}
	else if (type == EVT_POINTERUP)
	{
		if (!selectingMarker)
			return 0;
		selectingMarker = false;
		ClearTimer(SelectionTimer);
		restore_current_position();
		mainApplication->refreshWindow();
		bookview->selectionModel().extendTo(par1 - imgposx, par2 - imgposy);

		long long spos, epos;
		bool clickOnly, emptyText;
		GetSelectionInfo(&spos, &epos, &emptyText, &clickOnly);
		if ((clickOnly && SynTOC.GetMarkerByPos(spos) != NULL) || emptyText)
		{
			OpenMarkerMenu(SynTOC.GetMarkerByPos(spos));
		}
		else
		{
			MergeMarkers(spos, epos);
			bookview->selectionModel().clear();
			repaint_all(-2);
			ToolBar.OutputPage(BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight()),
						2, updMarkerx, updMarkery, updMarkerw, updMarkerh);
		}

	}
	else if (type == EVT_POINTERHOLD || type == EVT_POINTERLONG)
	{
		restore_current_position();
		mainApplication->refreshWindow();
		bookview->selectionModel().extendTo(par1 - imgposx, par2 - imgposy);

		long long spos, epos;
		bool clickOnly, emptyText;
		GetSelectionInfo(&spos, &epos, &emptyText, &clickOnly);
		if (clickOnly || emptyText)
		{
			OpenMarkerMenu(SynTOC.GetMarkerByPos(spos));
		}
	}
	return 0;
}

// other functions

static void synopsis_toc_selected(char *pos)
{
	long long position = 0;
	TSynopsisItem::PositionToLong(pos, &position);
	cpos = position;

	PBSynopsisItem *cur = SynTOC.GetCurrent();

	if (cur != NULL && cur->GetType() == SYNOPSIS_PEN)
	{
		PBSynopsisPen *pen = (PBSynopsisPen *)cur;
		ChangeSettings(SynTOC.GetSettings()->GetFB2Settings(pen->GetSettingsId()));
	}

	int p = position_to_page(cpos);
	if (is_footnote_mode())
	{
		((FBReader *)mainApplication)->showBookTextView();
	}
//	if (is_footnote_mode() || p >= npages) {
	if (p >= npages) {
		set_position(cpos);
//		mainApplication->doAction(ActionCode::PAGE_SCROLL_FORWARD);
		mainApplication->refreshWindow();
	} else {
		cpos = page_to_position(p);
		set_position(cpos);
		mainApplication->refreshWindow();
	}
	calc_position_changed = 1;
	SetEventHandler(PBMainFrame::main_handler);
	repaint_all(0);
	FlushMarker(cur);
}

void IfEmptyWordList()
{
	if (acwlistcount == 0)
	{
		ZLTextWordCursor cr = is_footnote_mode() ? footview->textArea().startCursor() : bookview->textArea().startCursor();
		int paragraph = !cr.isNull() ? cr.paragraphCursor().index() : 0;

		if (acwlistcount + 3 >= acwlistsize)
		{
			acwlistsize = acwlistsize + (acwlistsize >> 1) + 64;
			acwordlist = (SynWordList *)realloc(acwordlist, acwlistsize * sizeof(SynWordList));
		}

		// добавляем две пустышки с координатами верхнего левого и нижнего правого углов
		acwordlist[0].word = strdup("");
		acwordlist[0].x1 = 0;
		acwordlist[0].y1 = 0;
		acwordlist[0].x2 = 0;
		acwordlist[0].y2 = 0;
		acwordlist[0].pnum = paragraph;
		acwordlist[0].wnum = cr.elementIndex();

		cr = is_footnote_mode() ? footview->textArea().endCursor() : bookview->textArea().endCursor();
		paragraph = !cr.isNull() ? cr.paragraphCursor().index() : 0;

		acwordlist[1].word = strdup("");
		acwordlist[1].x1 = ScreenWidth();
		acwordlist[1].y1 = ScreenHeight() - PanelHeight();
		acwordlist[1].x2 = ScreenWidth();
		acwordlist[1].y2 = ScreenHeight() - PanelHeight();
		acwordlist[1].pnum = paragraph;
		acwordlist[1].wnum = cr.elementIndex();

		acwlistcount = 2;
	}
}

void new_synopsis_note()
{
	PrepareSynTOC(0);
	restore_current_position();
	mainApplication->refreshWindow();
	IfEmptyWordList();
	List.Clear();
	List.Add(acwordlist, acwlistcount, 1);

	ibitmap *bm1=NULL, *bm2=NULL;
	bm1 = BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight());
	if (one_page_forward(-1))
	{
		IfEmptyWordList();
		List.Add(acwordlist, acwlistcount, 2);
		bm2 = BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight());
		one_page_back(-1);
	}
	SynopsisNote *note = new SynopsisNote();
	note->Create((TSynopsisTOC*)&SynTOC, &List, bm1, bm2, true);
//	note->Create((TSynopsisTOC*)&SynTOC, &List, bm1, bm2, is_rtl_book() == false);
}


void new_synopsis_bookmark()
{
	PrepareSynTOC(0);
	restore_current_position();
	mainApplication->refreshWindow();
	IfEmptyWordList();
	List.Clear();
	List.Add(acwordlist, acwlistcount, 1);
	SynopsisBookmark *bookmark = new SynopsisBookmark();
	bookmark->Create((TSynopsisTOC*)&SynTOC, &List, (((long long)acwordlist[0].pnum << 40) | ((long long)acwordlist[0].wnum << 16)));
	SendEvent(GetEventHandler(), EVT_SHOW, 0, 0);
}

void open_synopsis_contents()
{
	char pos[25];
	PrepareSynTOC(1);
	restore_current_position();
	SynTOC.SetBackTOCHandler(synopsis_toc_selected);
	TSynopsisItem::PositionToChar(get_end_position(), pos);

	SynopsisContent Location(0, pos, NULL);
	SynTOC.Open(&Location);
}

void DrawSynopsisLabels()
{
	long long spos, epos, tmppos;
	int i;
	int sy, ey;

	if (SynTOC.GetHeader() == NULL)
	{
		SynTOC.SetHeaderName(book_title);
		SynTOC.SetFilePath(OriginalName);
		if (strrchr(OriginalName, '/') != NULL)
		{
			SynTOC.SetFileName(strrchr(OriginalName, '/') + 1);
		}
		else
		{
			SynTOC.SetFileName(OriginalName);
		}
		SynTOC.SetVersion(2);
		SynTOC.SetReaderName("F");
		SynTOC.LoadTOC();
	}

	restore_current_position();
	mainApplication->refreshWindow();
	IfEmptyWordList();

	if (acwlistcount == 0)
	{
		return;
	}

	spos = pack_position(acwordlist[0].pnum, acwordlist[0].wnum, 0);
	epos = pack_position(acwordlist[acwlistcount - 1].pnum, acwordlist[acwlistcount - 1].wnum, 0);
	char startpos[30], endpos[30];

	TSynopsisItem *item = SynTOC.GetHeader();
	PBSynopsisNote *noteItem;
	PBSynopsisPen *Pen, *CurPen;
	PBSynopsisMarker *marker;
	int pi, ei, ci;
	int x1, x2, y1, y2;
	while (item != NULL && item->GetLongPosition() <= epos)
	{
		if (spos <= item->GetLongPosition())
		{
			switch(item->GetType())
			{
				case SYNOPSIS_BOOKMARK:
					if (epos > item->GetLongPosition())
						DrawBitmap(ScreenWidth()-ac_bmk.width, 0, &ac_bmk);
					break;

				case SYNOPSIS_NOTE:
					noteItem = (TSynopsisNote *)item;
					i = 0;
					tmppos = noteItem->GetLongPosition();
					while (i < acwlistcount && tmppos != pack_position(acwordlist[i].pnum, acwordlist[i].wnum, 0)){
						i++;
					}
					sy = 0;
					if (i < acwlistcount){
						sy = acwordlist[i].y1 - noteItem->GetBPos();
					} else {
						i = 0;
					}
					tmppos = noteItem->GetEndLongPosition();
					while (i < acwlistcount && tmppos != pack_position(acwordlist[i].pnum, acwordlist[i].wnum, 0)){
						i++;
					}
					ey = ScreenHeight() - PanelHeight();
					if (i < acwlistcount)
					{
						ey = acwordlist[i].y1 + noteItem->GetAPos();
					}
					if (noteItem->GetAPos() == 0 && i < acwlistcount)
					{
						 ey += acwordlist[i].y2 - acwordlist[i].y1;
					}
					if (sy < 0 && ey < 0)
						break;
					if (sy < 0)
						sy = 0;
					if (ey > ScreenHeight() - PanelHeight())
						ey = ScreenHeight() - PanelHeight();
					FillArea(imgposx / 2 - 1, sy, 2, ey - sy, BLACK);
					DrawBitmap(imgposx / 2 - ac_note.width / 2, sy, &ac_note);
					FillArea(imgposx / 2 - 2, ey - 1, 4, 4, BLACK);
					break;

				case SYNOPSIS_PEN:
					Pen = (PBSynopsisPen *)item;
					PBSynopsisItem::PositionToChar((((long long)acwordlist[0].pnum << 40) | ((long long)acwordlist[0].wnum << 16)), startpos);
					PBSynopsisItem::PositionToChar((((long long)acwordlist[acwlistcount - 1].pnum << 40) | ((long long)acwordlist[acwlistcount - 1].wnum << 16)), endpos);
					CurPen = (SynopsisPen *)SynTOC.GetPen(startpos, endpos);
					if (Pen == CurPen)
					{
						if (Tool == 0)
						{
							stb_matrix matrix;
							matrix.a = 1;
							matrix.b = 0;
							matrix.c = 0;
							matrix.d = 1;
							matrix.e = 0;
							matrix.f = 0;
							Pen->OutputSVG(matrix);
						}
					}
					else
					{
						unpack_position(Pen->GetLongPosition(), &pi, &ei, &ci);
						bookview->selectionModel().GetCoordByPos(pi, ei, &x1, &y1, &x2, &y2);
						DrawBitmap(ScreenWidth() - pencil_icon.width - 5, y1 + imgposy, &pencil_icon);
					}
					break;
			}
		}
		if (item->GetType() == SYNOPSIS_COMMENT)
		{
			marker = (PBSynopsisMarker *)item;
			if (spos <= marker->GetEndLongPosition() && marker->GetEndLongPosition() <= epos)
			{
				unpack_position(marker->GetEndLongPosition(), &pi, &ei, &ci);

				bookview->selectionModel().GetCoordByPos(pi, ei, &x1, &y1, &x2, &y2);
				DrawBitmap(x2 + imgposx - comment_icon.width + 2, y1 + imgposy, &comment_icon);
			}
		}
		if (item->GetType() == SYNOPSIS_NOTE)
		{
			noteItem = (TSynopsisNote *)item;
			if (spos > noteItem->GetLongPosition() && spos <= noteItem->GetEndLongPosition())
			{
				i = 0;
				sy = 0;
				tmppos = noteItem->GetEndLongPosition();
				while (i < acwlistcount && tmppos != pack_position(acwordlist[i].pnum, acwordlist[i].wnum, 0))
					i++;
				ey = ScreenHeight() - PanelHeight();
				if (i < acwlistcount)
					ey = acwordlist[i].y2 + noteItem->GetAPos();
				if (ey > ScreenHeight() - PanelHeight())
					ey = ScreenHeight() - PanelHeight();
				FillArea(imgposx / 2 - 1, sy, 2, ey - sy, BLACK);
				FillArea(imgposx / 2 - 2, ey - 1, 4, 4, BLACK);
			}
		}
		item = item->GetFollow();
	}
}

// Erase markers and comments
static int savetoc = 0;
static int comments_to_delete = 0;
std::vector<PBSynopsisItem *> vDeleteComments;

int ReplayHandler(int /*type*/, int par1, int par2)
{
	bookview->selectionModel().clear();
	bookview->selectionModel().activate(par1 - imgposx, par2 - imgposy);
	bookview->selectionModel().extendTo(par1 - imgposx, par2 - imgposy);

	int spi, sei, sci, epi, eei, eci;
	bookview->selectionModel().GetSelection(&spi, &sei, &sci, &epi, &eei, &eci);
	long long epos = pack_position(epi, eei, eci);
	PBSynopsisItem *item = SynTOC.GetHeader();
	while (item != NULL && item->GetLongPosition() <= epos)
	{
		if (item->GetType() == SYNOPSIS_MARKER)
		{
			SynopsisMarker *marker = (SynopsisMarker *)item;
			if (epos >= marker->GetLongPosition() && epos <= marker->GetEndLongPosition())
			{
				TSynopsisItem *olditem = item;
				item = item->GetBelowM(SYNOPSIS_MARKER | SYNOPSIS_COMMENT);
				SynTOC.DeleteItem(olditem);
				savetoc = 1;
				continue;
			}
		}
		else if (item->GetType() == SYNOPSIS_COMMENT)
		{
			SynopsisMarker *marker = (SynopsisMarker *)item;
			if (epos >= marker->GetLongPosition() && epos <= marker->GetEndLongPosition())
			{
				int i;
				int size = vDeleteComments.size();
				for(i = 0; i < size; i++)
				{
					if (vDeleteComments[i] == item)
					{
						break;
					}
				}
				if (i == size)
				{
					vDeleteComments.push_back(item);
				}
			}
		}
		item = item->GetBelowM(SYNOPSIS_MARKER | SYNOPSIS_COMMENT);
	}
	return 1;
}

void DeleteComments(int button)
{
	if (button == 1)
	{
		int i;
		int size = vDeleteComments.size();
		for (i = 0; i < size; i++)
		{
			SynTOC.DeleteItem(vDeleteComments[i]);
		}
	}
	bookview->selectionModel().clear();
	repaint_all(-2);
	ToolBar.OutputPage(BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight()),
				2, 0, 0, ScreenWidth(), ScreenHeight()-PanelHeight());
	if (savetoc)
		SynTOC.SaveTOC();
}

bool OpenPen(int x, int y)
{
	// verify click on pen icon
	bool res = false;
	restore_current_position();
	mainApplication->refreshWindow();
	IfEmptyWordList();

	if (acwlistcount == 0)
		return res;

	long long spos, epos;
	char startpos[30], endpos[30];
	spos = pack_position(acwordlist[0].pnum, acwordlist[0].wnum, 0);
	epos = pack_position(acwordlist[acwlistcount - 1].pnum, acwordlist[acwlistcount - 1].wnum, 0);
	PBSynopsisItem::PositionToChar((((long long)acwordlist[0].pnum << 40) | ((long long)acwordlist[0].wnum << 16)), startpos);
	PBSynopsisItem::PositionToChar((((long long)acwordlist[acwlistcount - 1].pnum << 40) | ((long long)acwordlist[acwlistcount - 1].wnum << 16)), endpos);

	PBSynopsisPen *Pen;
	int pi, ei, ci;
	int x1, x2, y1, y2;
	PBSynopsisItem *item = SynTOC.GetHeader();
	while (item != NULL && item->GetLongPosition() <= epos)
	{
		if (spos <= item->GetLongPosition() && item->GetType() == SYNOPSIS_PEN)
		{
			Pen = (PBSynopsisPen *)item;
			PBSynopsisPen *CurPen = (SynopsisPen *)SynTOC.GetPen(startpos, endpos);
			if (Pen != CurPen)
			{
				unpack_position(Pen->GetLongPosition(), &pi, &ei, &ci);
				bookview->selectionModel().GetCoordByPos(pi, ei, &x1, &y1, &x2, &y2);
				if (ScreenWidth() - 5 - pencil_icon.width <= x && ScreenWidth() - 5 >= x &&
						y1 + imgposy <= y && y1 + imgposy + pencil_icon.height >= y)
				{
					long long position = 0;
					PBSynopsisItem::PositionToLong(item->GetPosition(), &position);
					cpos = position;

					ChangeSettings(SynTOC.GetSettings()->GetFB2Settings(Pen->GetSettingsId()));

					int p = position_to_page(cpos);
					if (p >= npages)
					{
						set_position(cpos);
				//		mainApplication->doAction(ActionCode::PAGE_SCROLL_FORWARD);
						mainApplication->refreshWindow();
					}
					else
					{
						cpos = page_to_position(p);
						set_position(cpos);
						mainApplication->refreshWindow();
					}
					calc_position_changed = 1;
					OpenSynopsisToolBar();
					res = true;
					break;
				}
			}
		}
		item = item->GetBelowM(SYNOPSIS_PEN);
	}
	return res;
}

void ToolHandler(int button)
{
	// back handler for ToolBar
	// button - type of select tool in Toolbar
	if (Tool == button)
		return;

	if (Tool == STBI_PEN || Tool == STBI_ERASER)
	{
		Pen->Close();
	}

	Tool = button;
	switch(button)
	{
		case STBI_MARKER:
		case STBI_NOTE:
		case STBI_IMAGE:
			break;

		case STBI_PEN:
		case STBI_ERASER:
			PrepareSynTOC(0);
			MarkMarkers();
			restore_current_position();
			mainApplication->refreshWindow();
			repaint_all(-2);
			Pen = (SynopsisPen *)ToolBar.GetPen();
			Pen->Create((TSynopsisTOC*)&SynTOC, BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight()), ToolBar.GetZoomOut());
			ToolBar.OutputPage(BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight()),
				2, 0, 0, ScreenWidth(), ScreenHeight());
			break;

		case STBI_EXIT:
			Tool = 0;
			SetEventHandler(PBMainFrame::main_handler);
			break;

		default:
			Tool = 0;
			break;
	}
}

int Note_Handler(int type, int par1, int par2)
{
	int replay = 0;

	if (type == EVT_POINTERDOWN)
	{
		if (calc_in_progress)
			calc_paused = true;
		if (OpenPen(par1, par2))
		{
			return 1;
		}
	}
	else if (type == EVT_POINTERUP)
	{
		if (calc_in_progress && calc_paused)
		{
			calc_paused = false;
			SetHardTimer("CalcPages", calc_timer, 0);
		}
	}


	if (Tool == STBI_PEN || Tool == STBI_ERASER)
	{
		replay = (Pen->PointerHandler(type, par1, par2) == 0);
		if (Tool == STBI_ERASER && !replay && type == EVT_POINTERUP)
		{
			restore_current_position();
			mainApplication->refreshWindow();
			repaint_all(-2);
			ToolBar.OutputPage(BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight()),
						2, 0, 0, ScreenWidth(), ScreenHeight()-PanelHeight());
		}
	}
	if (Tool == STBI_MARKER)
	{
		MarkerHandler(type, par1, par2);
	}

	if (replay)
	{
		restore_current_position();
		mainApplication->refreshWindow();
		savetoc = 0;
		comments_to_delete = 0;
		vDeleteComments.clear();
		Pen->Replay(ReplayHandler);
		int size = vDeleteComments.size();
		if (size > 0)
		{
			char tmp[200];
			if (size == 1)
			{
				sprintf(tmp, "%s", GetLangText("@DeleteItem"));
			}
			else
			{
				sprintf(tmp, "%s: %d?", GetLangText("@DeleteComments"), size);
			}
			Dialog(ICON_QUESTION, GetLangText("@Delete"), tmp, "@Yes", "@No", DeleteComments);
		}
		else
		{
			bookview->selectionModel().clear();
			repaint_all(-2);
			ToolBar.OutputPage(BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight()),
						2, 0, 0, ScreenWidth(), ScreenHeight()-PanelHeight());
			if (savetoc)
				SynTOC.SaveTOC();
		}
	}

	return 1;
}

// ToolBars function

void OpenSynopsisToolBar()
{
	if (is_footnote_mode())
	{
		repaint_all(0);
		Message(ICON_INFORMATION, "@Info", "@NotSupInFootnote", 3000);
		return;
	}
	PrepareSynTOC(0);
	MarkMarkers();
	restore_current_position();
	mainApplication->refreshWindow();
	repaint_all(-2);
	IfEmptyWordList();
	char spos[30], epos[30];
	PBSynopsisItem::PositionToChar((((long long)acwordlist[0].pnum << 40) | ((long long)acwordlist[0].wnum << 16)), spos);
	PBSynopsisItem::PositionToChar((((long long)acwordlist[acwlistcount - 1].pnum << 40) | ((long long)acwordlist[acwlistcount - 1].wnum << 16)), epos);
	ToolBar.Open(&SynTOC, PBMainFrame::main_handler,
			BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight()),
			ToolHandler,
			(SynopsisPen *)SynTOC.GetPen(spos, epos));
}

void CreateToolBar()
{
	ToolBar.AddPanel();
	ToolBar.AddItem(STBI_MARKER, 0);
	ToolBar.AddItem(STBI_PEN, 0);
	ToolBar.AddItem(STBI_ERASER, 0);
//	ToolBar.AddItem(STBI_UNDO, 0);
//	ToolBar.AddItem(STBI_REDO, 0);
	ToolBar.AddItem(STBI_SEPARATOR, 0);
//	ToolBar.AddItem(STBI_NOTE, 0);
//	ToolBar.SetItemState(7, 0, 0, STBIS_ACTIVE);
	ToolBar.AddItem(STBI_IMAGE, 0);
//	ToolBar.SetItemState(4, 0, 0, STBIS_ACTIVE);
	ToolBar.AddItem(STBI_SEPARATOR, 0);
	ToolBar.AddItem(STBI_CONTENT, 0);

	ToolBar.AddPanel();
	ToolBar.AddItem(STBI_HELP, 1);
	ToolBar.AddItem(STBI_EXIT, 1);

	ToolBar.SetPanelPosition(0, 50);
	ToolBar.SetPanelPosition(1, ScreenWidth() - 50 - 50);
}
