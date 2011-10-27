/*
 * Copyright (C) 2008 Most Publishing
 * Copyright (C) 2008 Dmitry Zakharov <dmitry-z@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "pdfviewer.h"
#include "main.h"
#include "pbmainframe.h"

int cpage = 1, npages = 1, subpage = 0, nsubpages = 1;
int * cpage_main = &cpage;
int scale = 100, rscale = 150;
int reflow_mode = 0;
int orient = 0;
int clock_left = 0;
int offx, offy;
int scrx, scry;
int flowpage = -1, flowscale, flowwidth, flowheight, savereflow;
int thx, thy, thw, thh, thix, thiy, thiw, thih, panelh;
int search_mode = 0;
int zoom_mode = 0;
static int bmkrem;
struct sresult results[MAXRESULTS];
int nresults;
bool doRestart = false;
int argc_main;
char **argv_main;
char* book_title = "";
PDFDoc* doc;
MySplashOutputDev* splashOut;
TextOutputDev* textout;
SearchOutputDev* searchout;
tdocstate docstate;

ibitmap* bmk_flag;

static long long bmkpos[32];

static char kbdbuffer[128];

static int savescale;
static char* FileName;
static char* DataFile;
static int ready_sent = 0;
static int ipcfd;

static char* stext;
static int spage, sdir;

static tocentry* TOC = NULL;
static int tocsize = 0, toclen = 0;
static char** named_dest = NULL;
static int named_size = 0, named_count = 0;
static int no_save_state = 0;

static iconfig* gcfg;
static int ko;

static iv_wlist* diclist = NULL;
static int diclen = 0;

static int ScaleZoomType = 0;

char *OriginalName;

void page_selected(int page)
{
	if (page < 1) page = 1;
	if (page > npages) page = npages;
	cpage = page;
	subpage = 0;
	offx = offy = 0;
	out_page(1);
}

void page_selected_background(int page)
{
    if (page < 1) page = 1;
    if (page > npages) page = npages;
    cpage = page;
    offx = offy = 0;
    out_page(-1);
}
int isReflowMode()
{
	return reflow_mode;
}

int get_page_word_list(iv_wlist** word_list, int* wlist_len, int page)
{
	int result = 0, i;

	if (word_list && wlist_len)
	{
		if (diclist)
		{
			for (i = 0; i < diclen; i++) free(diclist[i].word);
			free(diclist);
			diclist = NULL;
		}

		if (reflow_mode)
		{
			splashOut->getWordList(&diclist, &diclen, page != -1 ? page : subpage);
		}
		else
		{
			FixedPoint fx1, fx2, fy1, fy2;
			int x1, x2, y1, y2;
			char* s;
			int i, sw, sh, pw, ph, marginx, marginy, len;
			double sres;

			sw = ScreenWidth();
			sh = ScreenHeight();

			textout = new TextOutputDev(NULL, gFalse, gFalse, gFalse);
			getpagesize(page != -1 ? page : cpage, &pw, &ph, &sres, &marginx, &marginy);
			doc->displayPage(textout, page != -1 ? page : cpage, sres, sres, 0, false, true, false);
			TextWordList* wlist = textout->makeWordList();
			if (wlist == NULL)
			{
				fprintf(stderr, "wlist=NULL\n");
				len = 0;
			}
			else
			{
				len = wlist->getLength();
			}
			diclist = (iv_wlist*) malloc((len + 1) * sizeof(iv_wlist));
			diclen = 0;
			fprintf(stderr, "len=%i\n", len);
			for (i = 0; i < len; i++)
			{
				TextWord* tw = wlist->get(i);
				s = tw->getText()->getCString();
				tw->getBBox(&fx1, &fy1, &fx2, &fy2);
				x1 = (int)fx1 + scrx - offx;
				x2 = (int)fx2 + scrx - offx;
				y1 = (int)fy1 + scry - offy;
				y2 = (int)fy2 + scry - offy;

				if (s != NULL && *s != '\0' && x1 < sw && x2 > 0 && y1 < sh && y2 > 0)
				{
					diclist[diclen].word = strdup(s);
					diclist[diclen].x1 = x1 - 1;
					diclist[diclen].y1 = y1 - 1;
					diclist[diclen].x2 = x2 + 1;
					diclist[diclen].y2 = y2 + 1;
					diclen++;
				}
			}
			diclist[diclen].word = NULL;
			delete wlist;
			delete textout;
		}

		*word_list = diclist;
		*wlist_len = diclen;
		result = 1;
	}

	return result;
}

static void search_timer()
{
	FixedPoint xMin = 0, yMin = 0, xMax = 9999999, yMax = 9999999;
	unsigned int ucs4[32];
	unsigned int ucs4_len;
	int i, pw, ph, marginx, marginy;
	double sres;

	if (stext == NULL || ! search_mode) return;
	//fprintf(stderr, "%i\n", spage);

	if (spage < 1 || spage > npages)
	{
		HideHourglass();
		Message(ICON_INFORMATION, GetLangText("@Search"), GetLangText("@No_more_matches"), 2000);
		nresults = 0;
		return;
	}

	getpagesize(spage, &pw, &ph, &sres, &marginx, &marginy);

	/*
	  void displayPage(OutputDev *out, int page,
		 double hDPI, double vDPI, int rotate,
		 GBool useMediaBox, GBool crop, GBool printing,
		 GBool (*abortCheckCbk)(void *data) = NULL,
		 void *abortCheckCbkData = NULL,
			   GBool (*annotDisplayDecideCbk)(Annot *annot, void *user_data) = NULL,
			   void *annotDisplayDecideCbkData = NULL);
	*/

	searchout->found = 0;
	doc->displayPage(searchout, spage, sres, sres, 0, false, false, false);
	if (! searchout->found)
	{
		spage += sdir;
		SetHardTimer("SEARCH", search_timer, 1);
		return;
	}

	doc->displayPage(textout, spage, sres, sres, 0, false, true, false);
	TextPage* textPage = textout->takeText();

	xMin = yMin = 0;
	ucs4_len = utf2ucs4(stext, (unsigned int*)ucs4, 32);

	nresults = 0;
	while (textPage->findText(ucs4, ucs4_len,
							  gFalse, gTrue, gTrue, gFalse, gFalse, gFalse, &xMin, &yMin, &xMax, &yMax))
	{
		i = nresults++;
		results[i].x = (int)xMin - 2;
		results[i].y = (int)yMin - 2;
		results[i].w = (int)(xMax - xMin) + 4;
		results[i].h = (int)(yMax - yMin) + 4;
		if (i == MAXRESULTS - 1) break;
	}

	if (nresults > 0)
	{
		cpage = spage;
		out_page(1);
	}
	else
	{
		spage += sdir;
		SetHardTimer("SEARCH", search_timer, 1);
	}

	delete textPage;
}


static void do_search(char* text, int frompage, int dir)
{
	search_mode = 1;
	stext = strdup(text);
	spage = frompage;
	sdir = dir;
	ShowHourglass();
	search_timer();
}

static void stop_search()
{
	ClearTimer(search_timer);
	delete textout;
	delete searchout;
	free(stext);
	stext = NULL;
	search_mode = 0;
	scale = savescale;
	reflow_mode = savereflow;
	SetEventHandler(PBMainFrame::main_handler/*main_handler*/);
}

static int search_handler(int type, int par1, int par2)
{
	static int x0, y0;

	switch (type)
	{
		case EVT_SHOW:
			//out_page(0);
			break;

		case EVT_POINTERDOWN:
			x0 = par1;
			y0 = par2;
			break;

		case EVT_POINTERUP:
			if (par1 - x0 > EPSX)
			{
				turn_page(-1);
				return 1;
			}
			if (x0 - par1 > EPSX)
			{
				turn_page(1);
				return 1;
			}
			if (par1 > ScreenWidth() - searchbm.width && par2 > ScreenHeight() - PanelHeight() - searchbm.height)
			{
				if (par1 < ScreenWidth() - searchbm.width / 2)
				{
					do_search(stext, spage - 1, -1);
				}
				else
				{
					do_search(stext, spage + 1, +1);
				}
				return 1;
			}
			else
			{
				stop_search();
			}
			break;

		case EVT_KEYPRESS:
			switch (par1)
			{
				case KEY_OK:
				case KEY_BACK:
					stop_search();
					break;
				case KEY_LEFT:
					do_search(stext, spage - 1, -1);
					break;
				case KEY_RIGHT:
					do_search(stext, spage + 1, +1);
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}

	return 0;
}

void search_enter(char* text)
{
	if (text == NULL || text[0] == 0) return;
	savescale = scale;
	savereflow = reflow_mode;
	scale = 105;
	reflow_mode = 0;
	textout = new TextOutputDev(NULL, gFalse, gFalse, gFalse);
	searchout = new SearchOutputDev(text);
	if (! textout->isOk())
	{
		fprintf(stderr, "cannot create text output device\n");
		return;
	}

	SetEventHandler(search_handler);
	do_search(text, cpage, +1);
}

static void bmk_added(int page)
{
	char buf[256];

	//sprintf(buf, "%s %i", GetLangText("@Add_Bmk_P"), page);
	//Message(ICON_INFORMATION,GetLangText("@Bookmarks"), buf, 1500);
	draw_bmk_flag(1);
}

static void bmk_handler(int action, int page, long long pos)
{
	switch (action)
	{
		case BMK_ADDED:
			bmk_added(page);
			break;
		case BMK_REMOVED:
			bmkrem = 1;
			break;
		case BMK_SELECTED:
			page_selected(page);
			break;
		case BMK_CLOSED:
			if (bmkrem) out_page(0);
			break;
	}
}

void open_bookmarks()
{
	int i;
	bmkrem = 0;
	for (i = 0; i < docstate.nbmk; i++) bmkpos[i] = docstate.bmk[i];
	OpenBookmarks(cpage, cpage, docstate.bmk, bmkpos, &docstate.nbmk, 30, bmk_handler);
}

void new_bookmark()
{
	int i;
	bmkrem = 0;
	for (i = 0; i < docstate.nbmk; i++) bmkpos[i] = docstate.bmk[i];
	SwitchBookmark(cpage, cpage, docstate.bmk, bmkpos, &docstate.nbmk, 30, bmk_handler);
	if (bmkrem) out_page(0);
}

static void toc_handler(long long position)
{
	if (position < 100000)
	{
		cpage = position;
	}
	else
	{
		LinkDest* dest = doc->findDest(new GooString(named_dest[position-100000]));
		if (dest && dest->isOk())
		{
			Ref page_ref = dest->getPageRef();
			cpage = doc->findPage(page_ref.num, page_ref.gen);
		}
	}
	if (cpage < 1) cpage = 1;
	if (cpage > npages) cpage = npages;
	subpage = 0;
	offx = offy = 0;
	out_page(1);
}

void rotate_handler(int n)
{
	draw_wait_thumbnail();
	if (n == -1 || ko == 0)
	{
		SetGlobalOrientation(n);
	}
	else
	{
		SetOrientation(n);
	}
	orient = GetOrientation();
	//offx = offy = 0;
	out_page(1);
}

void turn_page(int n)
{
	offx = 0;
	offy = 0;
	if (reflow_mode && n == -1 && subpage > 0)
	{
		subpage--;
		n = 0;
	}
	if (reflow_mode && n == 1 && subpage + 1 < nsubpages)
	{
		subpage++;
		n = 0;
	}
	if (n > 0 && cpage < npages)
	{
		cpage += n;
		subpage = 0;
	}
	if (n < 0 && cpage > 1)
	{
		cpage += n;
		subpage = (reflow_mode && n == -1) ? 9999999 : 0;
	}
	if (cpage < 1)
	{
		cpage = 1;
		subpage = 0;
	}
	if (cpage > npages) cpage = npages;
	out_page(1);
}

static void draw_jump_info(char* text)
{
	int x, y, w, h;

	SetFont(menu_n_font, BLACK);
	w = StringWidth("999%") + 20;
	if (StringWidth(text) > w - 10) w = StringWidth(text) + 20;
	h = (menu_n_font->height * 3) / 2;
	x = ScreenWidth() - w - 5;
	y = ScreenHeight() - h - 30;
	FillArea(x + 1, y + 1, w - 2, h - 2, WHITE);
	DrawRect(x + 1, y, w - 2, h, BLACK);
	DrawRect(x, y + 1, w, h - 2, BLACK);
	DrawTextRect(x, y, w, h, text, ALIGN_CENTER | VALIGN_MIDDLE);
	PartialUpdateBW(x, y, w, h);
}

void jump_pages(int n)
{
	char buf[16];

	cpage += n;
	subpage = 0;
	if (cpage < 1) cpage = 1;
	if (cpage > npages) cpage = npages;
	offx = offy = 0;
	sprintf(buf, "%i", cpage);
	draw_jump_info(buf);
}

int get_cpage()
{
	return cpage;
}

int get_npage()
{
	return npages;
}

void new_note()
{
	CreateNote(FileName, book_title, cpage);
}

void save_page_note()
{
	CreateNoteFromPage(FileName, book_title, cpage);
}

void open_notes()
{
	OpenNotepad(NULL);
}

void open_contents()
{
	if (toclen == 0)
	{
		Message(ICON_INFORMATION, "PDF Viewer", "@No_contents", 2000);
	}
	else
	{
		OpenContents(TOC, toclen, cpage, toc_handler);
	}
}

static void zoom_timer()
{
	out_page(1);
}

void zoom_in()
{
	char buf[16];
	if (reflow_mode)
	{
		rscale = (rscale < 200) ? rscale + 50 : rscale + 100;
		if (rscale > 500) rscale = 150;
		out_page(1);
		return;
	}
	int ssc = scale;
	update_value(&scale, +1, SC_DIRECT);
	if (ssc >= 200)
	{
		offx = (offx * scale) / ssc;
		offy = (offy * scale) / ssc;
	}
	sprintf(buf, "%i%%", scale);
	draw_jump_info(buf);
	SetHardTimer("ZOOM", zoom_timer, 1000);
}

void zoom_out()
{
	char buf[16];
	if (reflow_mode)
	{
		rscale = (rscale > 200) ? rscale - 100 : rscale - 50;
		if (rscale < 150) rscale = 500;
		out_page(1);
		return;
	}
	int ssc = scale;
	update_value(&scale, -1, SC_DIRECT);
	if (ssc >= 200)
	{
		offx = (offx * scale) / ssc;
		offy = (offy * scale) / ssc;
	}
	sprintf(buf, "%i%%", scale);
	draw_jump_info(buf);
	SetHardTimer("ZOOM", zoom_timer, 1000);
}

void apply_scale_factor(int zoom_type, int new_scale)
{
	ScaleZoomType = 0;
	switch(zoom_type)
	{
		case ZoomTypeFitWidth:
			reflow_mode = 0;
			scale = get_fit_scale();
			ScaleZoomType = ZoomTypeFitWidth;
			break;
		case ZoomTypeReflow:
			rscale = new_scale;
			reflow_mode = 1;
			break;
		default:
			scale = new_scale;
			reflow_mode = 0;
			break;
	}
	out_page(1);
	return;
}

void zoom_update_value(int* val, int d, const int* variants)
{

	int i, n, mind = 9999999, ci = 0, cd;

	for (n = 0; variants[n] != -1; n++) ;

	for (i = 0; i < n; i++)
	{
		cd = *val - variants[i];
		if (cd < 0) cd = -cd;
		if (cd < mind)
		{
			mind = cd;
			ci = i;
		}
	}

	ci += d;
	if (ci < 0) ci = n - 1;
	if (ci >= n) ci = 0;
	*val = variants[ci];

}

int get_zoom_param(int& zoom_type, int& zoom_param)
{
	const int SC_PREVIEW[] = { 33, 50, -1 };
	const int SC_NORMAL[] = { 75, 80, 85, 90, 95, 100, 105, 120, 130, 140, 150, 170, -1 };
	const int SC_COLUMNS[] = { 200, 300, 400, 500, -1 };
	const int SC_REFLOW[] = { 150, 200, 300, 400, 500, -1 };

	if (reflow_mode)
	{
		zoom_type = ZoomTypeReflow;
		zoom_param = rscale;
		zoom_update_value(&zoom_param, 0, SC_REFLOW);
	}
	else if (scale <= 50)
	{
		zoom_type = ZoomTypePreview;
		zoom_param = scale;
		zoom_update_value(&zoom_param, 0, SC_PREVIEW);
	}
	else if (scale >= 200)
	{
		zoom_type = ZoomTypeColumns;
		zoom_param = scale;
		zoom_update_value(&zoom_param, 0, SC_COLUMNS);
	}
	else
	{
		if (ScaleZoomType == ZoomTypeFitWidth)
		{
			zoom_type = ZoomTypeFitWidth;
			zoom_param = 100;
		}
		else
		{
			zoom_type = ZoomTypeNormal;
			zoom_param = scale;
			if (zoom_param >= 200) zoom_param = 100;
			zoom_update_value(&zoom_param, 0, SC_NORMAL);
		}
	}
	return zoom_param;
}

void show_hide_panel()
{
	SetPanelType((panelh == 0) ? 1 : 0);
	panelh = PanelHeight();
	out_page(1);
}

void pdf_mode()
{
	if (scale >= 200 && ! reflow_mode)
	{
		area_selector();
	}
	else if (reflow_mode)
	{
		reflow_mode = 0;
		out_page(1);
	}
	else
	{
		reflow_mode = 1;
		if (rscale < 150) rscale = 150;
		out_page(1);
	}
}

static void handle_navikey(int key)
{
	int pageinc = 1;
	switch (scale)
	{
		case 50:
			pageinc = is_portrait() ? 4 : 2;
			break;
		case 33:
			pageinc = is_portrait() ? 9 : 6;
			break;
	}
	if (reflow_mode) pageinc = 1;

	switch (key)
	{
		case KEY_LEFT:
			if (scale >= 200)
			{
				find_off_x(-1);
				out_page(1);
			}
			else
			{
				turn_page(-pageinc);
			}
			break;

		case KEY_RIGHT:
			if (scale >= 200)
			{
				find_off_x(+1);
				out_page(1);
			}
			else
			{
				turn_page(pageinc);
			}
			break;

		case KEY_UP:
			find_off(-1);
			out_page(1);
			break;

		case KEY_DOWN:
			find_off(+1);
			out_page(1);
			break;


		case KEY_PREV:
		case KEY_PREV2:
			if (scale >= 200 || orient == 1 || orient == 2)
			{
				find_off(-1);
				out_page(1);
			}
			else
			{
				turn_page(-pageinc);
			}
			break;

		case KEY_NEXT:
		case KEY_NEXT2:
			if (scale >= 200 || orient == 1 || orient == 2)
			{
				find_off(+1);
				out_page(1);
			}
			else
			{
				turn_page(pageinc);
			}
			break;
	}
}

void set_panel_height()
{
	panelh = PanelHeight();
}

void main_show()
{
	SetKeyboardRate(700, 500);
	out_page(1);
	if (!ready_sent) BookReady(FileName);
	ready_sent = 1;
}

void save_screenshot()
{
	DrawPanel((ibitmap*)PANELICON_LOAD, "@snapshot_info", NULL, -1);
	PageSnapshot();
}

void main_prevpage()
{
	if (reflow_mode || (scale > 50 && scale < 200))
	{
		turn_page(-1);
	}
	else if (scale <= 50)
	{
		handle_navikey(KEY_LEFT);
	}
	else
	{
		handle_navikey(KEY_UP);
	}
}

void main_nextpage()
{
	if (reflow_mode || (scale > 50 && scale < 200))
	{
		turn_page(1);
	}
	else if (scale <= 50)
	{
		handle_navikey(KEY_RIGHT);
	}
	else
	{
		handle_navikey(KEY_DOWN);
	}
}

static void add_toc_item(int level, char* label, int page, long long position)
{
	if (tocsize <= toclen + 1)
	{
		tocsize += 64;
		TOC = (tocentry*) realloc(TOC, tocsize * sizeof(tocentry));
	}
	TOC[toclen].level = level;
	TOC[toclen].page = page;
	TOC[toclen].position = position;
	TOC[toclen].text = strdup(label);
	toclen++;
}

static void update_toc(GooList* items, int level)
{
	unsigned short ucs[256];
	char label[256];
	int i, j;

	if (! items) return;
	if (items->getLength() < 1) return;

	for (i = 0; i < items->getLength(); i++)
	{
		OutlineItem* outlineItem = (OutlineItem*)items->get(i);
		Unicode* title = outlineItem->getTitle();
		int tlen = outlineItem->getTitleLength();
		if (tlen > sizeof(ucs) - 1) tlen = sizeof(ucs) - 1;
		for (j = 0; j < tlen; j++) ucs[j] = (unsigned short)title[j];
		ucs[j] = 0;
		ucs2utf(ucs, label, sizeof(label));

		LinkAction* a = outlineItem->getAction();
		if (a && (a->getKind() == actionGoTo))
		{
			// page number is contained/referenced in a LinkGoTo
			LinkGoTo* g = static_cast< LinkGoTo* >(a);
			LinkDest* destination = g->getDest();
			if (!destination && g->getNamedDest())
			{
				GooString* s = g->getNamedDest();
				if (named_size <= named_count + 1)
				{
					named_size += 64;
					named_dest = (char**) realloc(named_dest, named_size * sizeof(char*));
				}
				named_dest[named_count] = strdup(s->getCString());
				add_toc_item(level, label, -1, 100000 + named_count);
				named_count++;
			}
			else if (destination && destination->isOk() && destination->isPageRef())
			{
				Ref page_ref = destination->getPageRef();
				int num = doc->findPage(page_ref.num, page_ref.gen);
				add_toc_item(level, label, num, num);
			}
			else
			{
				add_toc_item(level, label, -1, -1);
			}
		}
		else
		{
			add_toc_item(level, label, -1, -1);
		}
		outlineItem->open();
		GooList* children = outlineItem->getKids();
		if (children) update_toc(children, level + 1);
		outlineItem->close();
	}
}

#include <bookstate.h>

void save_settings()
{
	if (no_save_state) return;
	if (docstate.magic != 0x9751) return;
	fprintf(stderr, "pdfviewer: saving settings...\n");

	docstate.page = cpage;
	docstate.subpage = subpage;
	docstate.offx = offx;
	docstate.offy = offy;
	if (ScaleZoomType == ZoomTypeFitWidth)
	{
		docstate.scale = 0;
	}
	else
	{
		docstate.scale = scale;
	}
	docstate.rscale = rscale;
	docstate.orient = orient;
	if (reflow_mode) docstate.orient |= 0x80;

	FILE* f = iv_fopen(DataFile, "wb");
	if (f != NULL)
	{
		iv_fwrite(&docstate, 1, sizeof(tdocstate), f);
		iv_fclose(f);
	}

	bsHandle newstate = bsLoad(FileName);
	if (newstate)
	{
		bsSetCPage(newstate, cpage);
		bsSetNPage(newstate, npages);
		bsSetOpenTime(newstate, time(NULL));
		if (bsSave(newstate))
			fprintf(stderr, "Save to db ok!\n");
		else
			fprintf(stderr, "Save to db failed!\n");
		bsClose(newstate);
	}

	sync();
	fprintf(stderr, "pdfviewer: saving settings done\n");
}

static void pwd_query_kbdhandler(char* text)
{
	if (text != NULL) write(ipcfd, text, strlen(text));
	CloseApp();
}

static int pwd_query_handler(int type, int par1, int par2)
{
	if (type == EVT_INIT) OpenKeyboard("@Password", kbdbuffer, 63, KBD_PASSWORD, pwd_query_kbdhandler);
	return 0;
}

static char* query_password()
{
	char buf[64];
	int fdset[2];
	int n;

	if (pipe(fdset) < 0) return "";

	switch (fork())
	{
		case -1: // error
			return "";
		case 0:  // child
			close(fdset[0]);
			ipcfd = fdset[1];
			InkViewMain(pwd_query_handler);
			close(ipcfd);
			exit(0);
		default:
			close(fdset[1]);
			n = read(fdset[0], buf, sizeof(buf) - 1);
			if (n < 0) n = 0;
			buf[n] = 0;
			close(fdset[0]);
			usleep(500000);
			return strdup(buf);
	}
}

// against compiler bugs
void x_ready_sent(int n)
{
	ready_sent = n;
}

static void sigsegv_handler(int signum)
{
	x_ready_sent(ready_sent);
	if (! ready_sent)
	{
		fprintf(stderr, "SIGSEGV caught - clearing state\n");
		iv_unlink(DataFile);
		no_save_state = 1;
	}
	else
	{
		fprintf(stderr, "SIGSEGV caught\n");
	}
	exit(127);
}

static void sigfpe_handler(int signum)
{
	fprintf(stderr, "SIGFPE\n");
}

#ifdef USESYNOPSIS

long long page_to_position(int page)
{
	return ((long long)page << 40);
}

int position_to_page(long long position)
{
	int page = 0;

	if (position < 100000)
	{
		page = position;
	}
	else
	{
		LinkDest* dest = doc->findDest(new GooString(named_dest[position-100000]));
		if (dest && dest->isOk())
		{
			Ref page_ref = dest->getPageRef();
			page = doc->findPage(page_ref.num, page_ref.gen);
		}
	}
	if (page < 1) page = 1;
	if (page > npages) page = npages;
	return page;
}

void PrepareActiveContent(int opentoc)
{
	if (m_TOC.GetHeader() == NULL)
	{
		bookinfo *bi;
		bi = GetBookInfo(FileName);

		if (bi != NULL)
		{
			m_TOC.SetHeaderName((char *)bi->title);
			if (strrchr(FileName, '/') != NULL)
			{
				m_TOC.SetFileName(strrchr(FileName, '/') + 1);
			}
			else
			{
				m_TOC.SetFileName(FileName);
			}
//			m_TOC.SetFileName((char *)bi->filename);
			m_TOC.SetFilePath((char *)FileName);
#ifdef SYNOPSISV2
			m_TOC.SetVersion(2);
#endif
			m_TOC.SetReaderName("P");
			m_TOC.LoadTOC();
		}
	}

	if (m_TOC.GetHeader() == NULL)
	{
		int i;

		if (toclen == 0)
		{
			if (opentoc == 1)
				Message(ICON_INFORMATION, "PDF Viewer", "@No_contents", 2000);
		}

		for (i=0; i<toclen; i++)
		{
			Content = new SynopsisContent(TOC[i].level, (long long)position_to_page(TOC[i].position) << 40, TOC[i].text);
			m_TOC.AddTOCItem(Content);
		}
	}
}

void synopsis_toc_handler(char *cposition)
{
	long long position;
	TSynopsisItem::PositionToLong(cposition, &position);

//	cpage = position_to_page(position);
	cpage = position >> 40;
	if (cpage < 1) cpage = 1;
	if (cpage > npages) cpage = npages;
	subpage = 0;
	offx = offy = 0;
	out_page(1);
	SetEventHandler(PBMainFrame::main_handler);
}

void new_synopsis_bookmark()
{
	PrepareActiveContent(0);
	List.Clear();
	Bookmark = new SynopsisBookmark();
	Bookmark->Create((TSynopsisTOC*)&m_TOC, &List, ((long long)cpage << 40));
	SendEvent(GetEventHandler(), EVT_SHOW, 0, 0);
}

void new_synopsis_note()
{
	PrepareActiveContent(0);

	SynWordList* synWordList = new SynWordList[2];
	memset(synWordList, 0, sizeof(SynWordList) * 2);

	synWordList[0].word = strdup("");
	synWordList[0].x1 = 0;
	synWordList[0].y1 = 0;
	synWordList[0].x2 = 0;
	synWordList[0].y2 = 0;
	synWordList[0].pnum = cpage;
	synWordList[0].wnum = 1;

	synWordList[1].word = strdup("");
	synWordList[1].x1 = ScreenWidth();
	synWordList[1].y1 = ScreenHeight() - PanelHeight();
	synWordList[1].x2 = ScreenWidth();
	synWordList[1].y2 = ScreenHeight() - PanelHeight();
	synWordList[1].pnum = cpage;
	synWordList[1].wnum = 2;

	List.Clear();
	List.Add(synWordList, 2, 1);

	delete[] synWordList;

	ibitmap *bm1=NULL, *bm2=NULL;
	bm1 = BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight());
//	bm2 = BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight());
	Note = new SynopsisNote();
	Note->Create((TSynopsisTOC*)&m_TOC, &List, bm1, bm2, false);
//	SendEvent(GetEventHandler(), EVT_SHOW, 0, 0);
}

#endif

int main(int argc, char** argv)
{
        argc_main = argc;
        argv_main = argv;
	SplashColor paperColor;
	GooString* filename, *password;
	char* spwd;
	FILE* f = NULL;
	bookinfo* bi;
	char buf[1024];
	int i;

	mkdir(CACHEDIR, 0777);
	chmod(CACHEDIR, 0777);

	spwd = GetDeviceKey();

	if (setgid(102) != 0) fprintf(stderr, "warning: cannot set gid\n");
	if (setuid(102) != 0) fprintf(stderr, "warning: cannot set uid\n");

	if (spwd)
	{
		//fprintf(stderr, "password: %s\n", spwd);
		password = new GooString(spwd);
	}
	else
	{
		fprintf(stderr, "warning: cannot read password\n");
		password = NULL;
	}

	OpenScreen();

	signal(SIGFPE, sigfpe_handler);
	signal(SIGSEGV, sigsegv_handler);

	clock_left = GetThemeInt("panel.clockleft", 0);
	bmk_flag = GetResource("bmk_flag", NULL);

	if (argc < 2)
	{
		Message(ICON_WARNING, "PDF Viewer", "@Cant_open_file", 2000);
		return 0;
	}

	OriginalName = FileName = argv[1];
	bi = GetBookInfo(FileName);
	if (bi->title) book_title = strdup(bi->title);

	// read config file
	globalParams = new GlobalParams();

	globalParams->setEnableFreeType("yes");
	globalParams->setAntialias((char*)(ivstate.antialiasing ? "yes" : "no"));
	globalParams->setVectorAntialias("no");

	filename = new GooString(FileName);
	doc = new PDFDoc(filename, NULL, NULL);
	if (!doc->isOk())
	{
		int err = doc->getErrorCode();
		delete doc;
		if (err == 4)   // encrypted file
		{
			filename = new GooString(FileName);
			doc = new PDFDoc(filename, NULL, password);
			if (!doc->isOk())
			{
				delete doc;
				spwd = query_password();
				password = new GooString(spwd);
				filename = new GooString(FileName);
				doc = new PDFDoc(filename, NULL, password);
				if (!doc->isOk())
				{
					Message(ICON_WARNING, "PDF Viewer", "@Cant_open_file", 2000);
					return 0;
				}
			}
		}
		else
		{
			Message(ICON_WARNING, "PDF Viewer", "@Cant_open_file", 2000);
			return 0;
		}
	}

	npages = doc->getNumPages();
	paperColor[0] = 255;
	paperColor[1] = 255;
	paperColor[2] = 255;
	splashOut = new MySplashOutputDev(USE4 ? splashModeMono4 : splashModeMono8, 4, gFalse, paperColor);
	splashOut->startDoc(doc->getXRef());

	Outline* outline = doc->getOutline();
	if (outline && outline->getItems())
	{

		GooList* items = outline->getItems();
		if (items->getLength() == 1)
		{
			OutlineItem* first = (OutlineItem*)items->get(0);
			first->open();
			items = first->getKids();
			update_toc(items, 0);
			first->close();
		}
		else if (items->getLength() > 1)
		{
			update_toc(items, 0);
		}

	}

	DataFile = GetAssociatedFile(FileName, 0);
	f = fopen(DataFile, "rb");
	if (f == NULL || fread(&docstate, 1, sizeof(tdocstate), f) != sizeof(tdocstate) || docstate.magic != 0x9751)
	{
		docstate.magic = 0x9751;
		docstate.page = 1;
		docstate.offx = 0;
		docstate.offy = 0;
		docstate.scale = 100;
		docstate.rscale = 150;
		docstate.orient = 0;
		docstate.nbmk = 0;
	}
	if (f != NULL) fclose(f);

	cpage = docstate.page;
	subpage = docstate.subpage;
	offx = docstate.offx;
	offy = docstate.offy;
	if (docstate.scale == 0)
	{
		scale = get_fit_scale();
		ScaleZoomType = ZoomTypeFitWidth;
	}
	else
	{
		scale = docstate.scale;
	}
	rscale = docstate.rscale;
	reflow_mode = (docstate.orient & 0x80) ? 1 : 0;

	gcfg = GetGlobalConfig();
//	ko = ReadInt(gcfg, "keeporient", 0);
	ko = -1;
	if (GetGlobalOrientation() == -1 || ko == 0)
	{
		orient = GetOrientation();
	}
	else
	{
		orient = docstate.orient & 0x7f;
		SetOrientation(orient);
	}

	if (argc >= 3)
	{
		if (argv[2][0] == '=')
		{
#ifdef USESYNOPSIS
			long long position;
			TSynopsisItem::PositionToLong(argv[2] + 1, &position);
			cpage = position_to_page(position >> 40);
#else
//			cpage = atoi(position_to_page(argv[2] + 1));
			cpage = atoi(argv[2] + 1);
#endif
		}
		else
		{
			LinkDest* dest = doc->findDest(new GooString(argv[2]));
			if (dest && dest->isOk())
			{
				Ref page_ref = dest->getPageRef();
				cpage = doc->findPage(page_ref.num, page_ref.gen);
			}
		}
	}

	if (cpage < 1) cpage = 1;
	if (cpage > npages) cpage = npages;

#ifdef USESYNOPSIS
	PrepareActiveContent(0);
#endif
	PBMainFrame main_frame(FileName);
	main_frame.Create(NULL, 0, 0, 1, 1, PBWS_VISIBLE, 0);
	main_frame.Run();

	return 0;
}
