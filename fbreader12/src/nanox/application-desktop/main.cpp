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

#include <ZLibrary.h>
#include <ZLApplication.h>
#include "../../fbreader/FBReader.h"
#include "../../fbreader/BookTextView.h"
#include "../../fbreader/FootnoteView.h"
#include "../../fbreader/FBReaderActions.h"
#include "../../formats/FormatPlugin.h"
#include "../view/ZLNXViewWidget.h"
#include "../view/ZLNXPaintContext.h"
#include <pbframework/pbframework.h>
#include "../../style/ZLTextStyleCollection.h"
#include "../../bookmodel/FBTextKind.h"
#include "../../options/FBTextStyle.h"
#include "../../library/Book.h"
#include "../../util/ZLLanguageUtil.h"
#include "../../blockTreeView/ZLBlockTreeView.h"
#include "../../fbreader/ScrollingAction.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <inkview.h>
#include <inkinternal.h>

#include "main.h"
#include "pbmainframe.h"
#include "pbnotes.h"

//static long long pack_position(int para, int word, int letter);
//static void unpack_position(long long pos, int *para, int *word, int *letter);
//static long long page_to_position(int page);
//static int position_to_page(long long position);
//static long long get_end_position();
//static void set_position(long long pos);
//static void restore_current_position();
void select_page(int page);
static void invert_current_link(int update);
static void apply_config(int recalc, int canrestart);
static void DrawWaitMessage();
void save_state();
static char *get_backlink();
static bool firstUpdate = false;
char detectedlang[4];
bool doRestart = false;
static char *spacing_variants[] = { "70", "80", "90", "100", "120", "150", "200", NULL };
static char *font_step_variants[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", NULL };
static char *border_variants[] = { "@border_small", "@border_medium", "@border_large", NULL };
static char *textfmt_variants[] = { "@ip_auto", "@fmt_newline", "@fmt_emptyline", "@fmt_indent", NULL };
static char *hyph_variants[] = { "@Off", "@On", NULL };
static char *loadimg_variants[] = { "@Off", "@On", NULL };
static char *fastturn_variants[] = { "@fastturn_off", "@fastturn_on", NULL };
static char *textdir_variants[] = { "@LeftToRight", "@RightToLeft", NULL };
static char ** argv_main;
static int argc_main;
static char *encoding_variants[] = {

	"@ip_auto",
	"@enc_russian",
		":WINDOWS-1251",
		":ISO-8859-5",
		":IBM866",
		":KOI8-R",
		":KOI8-U",
	"@enc_west_eur",
		":ISO-8859-1",
		":ISO-8859-3",
		":ISO-8859-7",
		":ISO-8859-10",
		":ISO-8859-14",
		":ISO-8859-15",
		":WINDOWS-1252",
		":WINDOWS-1253",
	"@enc_east_eur",
		":ISO-8859-2",
		":ISO-8859-4",
		":ISO-8859-13",
		":ISO-8859-16",
		":WINDOWS-1250",
		":WINDOWS-1257",
	"@enc_mid_east",
		":ISO-8859-8",
		":WINDOWS-1255",
	"@enc_east_asia",
		":ISO-8859-9",
		":WINDOWS-1254",
		":Big5",
		":GBK",
	"UTF-8",
	NULL

};

static iconfigedit fbreader_ce[] = {

  { CFG_INFO,               &ci_about,          "@About_book",      NULL, "about",          NULL,           NULL,               NULL },
  { CFG_FONT,               &ci_font,           "@Font",            NULL, "font",           DEFREADERFONT,  NULL,               NULL },
  { CFG_CHOICE,             &ci_enc,            "@Encoding",        NULL, "encoding",       "@ip_auto",     encoding_variants,  NULL },
  { CFG_CHOICE,             &ci_spacing,        "@Linespacing",     NULL, "linespacing",    "100",          spacing_variants,   NULL },
  { CFG_CHOICE,             &ci_font_size_step, "@FontChangeStep",  NULL, "fontchangestep", "3",            font_step_variants, NULL },
  { CFG_INDEX,              &ci_margins,        "@Pagemargins",     NULL, "border",         "2",            border_variants,    NULL },
  { CFG_INDEX,              &ci_hyphen,         "@Hyphenation",     NULL, "hyphenations",   "1",            hyph_variants,      NULL },
  { CFG_INDEX,              &ci_textfmt,        "@TextFormat",      NULL, "preformatted",   "0",            textfmt_variants,   NULL },
  { CFG_INDEX,              &ci_font,           "@FastUpdate",      NULL, "fastturn",       "0",            fastturn_variants,  NULL },
  { CFG_INDEX,              &ci_textdir,        "@TextDirection",   NULL, "textdir",        "0",            textdir_variants,   NULL },
  { CFG_INDEX | CFG_HIDDEN, &ci_image,          "@LoadImages",      NULL, "loadimages",     "1",            loadimg_variants,   NULL },
  { 0,                      NULL,               NULL,               NULL, NULL,             NULL,           NULL,               NULL }

};

static bookinfo *bi;

tdocstate docstate;
char *OriginalName;

char *book_title="";
char *book_author="";
int fontChangeStep = 3;

static char *FileName;
static char *DataFile;
static char *ZetFile;
static char *PagesFile;


static int ready_sent = 0;
static int making_snapshot = 0;

//static int regStatus;
static iconfig *gcfg, *fcfg;
static int ko;
static int invertupdate;
static int upd_count=0;
int fast_turn = 1;

//static long long *pagelist=NULL;
typedef std::vector<long long> TPageList;
typedef TPageList::iterator TPageListIt;
TPageList oldPageList;
TPageList pageList;

//static int pagelistsize=0;
//static int npages;
//static long long cpos;
int npages;
long long cpos;

static int soft_update = 0;

tocentry *TOC;
int toc_size;

static int bmkpages[32];

static int search_mode = 0;



static int links_mode = 0;
static int link_back = -1;
static hlink links[MAXLINKS];
static int nlinks=0, clink;

int calc_in_progress = 0;
int calc_position_changed = 0;
static int current_position_changed = 0;
static int calc_current_page;
static int calc_orientation = -1;
static long long calc_current_position;

extern ZLApplication *mainApplication;
BookTextView *bookview;
FootnoteView *footview;

unsigned char *screenbuf;
int use_antialiasing;
static int orient;
int imgposx, imgposy, imgwidth, imgheight, scanline;

int no_save_state=0;
int dl_request=0;
//void *ballast=NULL;

static ibitmap *m3x3;
static ibitmap *bmk_flag;
static ibitmap *bgnd_p, *bgnd_l;
static irect textarea;

std::vector<std::string> xxx_notes;
#define TIME_UPDATE_PANEL 300
//struct xxx_link {
//	int x1, y1, x2, y2;
//	int kind;
//	std::string id;
//	bool next;
//};

std::vector<struct xxx_link> xxx_page_links;

struct xxx_toc_entry {
	int paragraph;
	int level;
	std::string text;
};

std::vector<xxx_toc_entry> xxx_myTOC;

iv_wlist *xxx_wordlist=NULL;
iv_wlist *rtl_wordlist=NULL;
int xxx_wlistlen=0, xxx_wlistsize=0, xxx_hasimages=0, xxx_grayscale=0, xxx_use_update=0;

int glb_page;

#define EPSX 50
#define EPSY 50

static bool Link;

SynWordList *acwordlist=NULL;
int acwlistcount = 0;
int acwlistsize = 0;

extern int selMarkery1, selMarkery2;
extern int updMarkerx, updMarkery, updMarkerw, updMarkerh;
extern int maxstrheight;
extern bool hadMarkers;

extern int Tool;
extern bool calc_paused;

extern SynopsisTOC SynTOC;

extern fb2_settings PenSettings;

static int is_doc(char *name);
static int is_docx(char *name);

static void draw_searchpan() {
	DrawBitmap(ScreenWidth()-searchbm.width, ScreenHeight()-searchbm.height-PanelHeight(), &searchbm);
}

static const ibitmap *panel_icon = NULL;
static char *panel_title="";
static char panel_buf[256] = "\0";
static int panel_percent = 0;
bool long_calc_progress = false;

static void UpdatePanelData()
{
    int cpage;
    panel_icon = NULL;
    if (making_snapshot) {
            panel_icon = PANELICON_LOAD;
            strcpy(panel_buf, "@snapshot_info");
            panel_title = NULL;
            panel_percent = -1;
    } else if (calc_in_progress) {
            panel_icon = PANELICON_BOOK;
            sprintf(panel_buf, "...       ");
            panel_title = book_title;
            panel_percent = -1;
    } else if (! is_footnote_mode()) {
            cpage = get_cpage();
            sprintf(panel_buf, "  %i / %i", cpage, npages);
            panel_title = book_title;
            panel_percent = (cpage * 100) / npages;
    } else {
            panel_buf[0] = 0;
            panel_title = book_title;
            panel_percent = -1;
    }

}
static void ApplyAntialiasing()
{
    switch (ReadInt(fcfg, "fastturn", 0))
    {
    case 0:
        use_antialiasing = 1;
        //ivstate.antialiasing = 0;
        break;
    case 1:
        use_antialiasing = 0;
        //ivstate.antialiasing = 1;
        break;
    }

}
int is_footnote_mode() {
	FBReader& fbreader = FBReader::Instance();
	return fbreader.mode() == FBReader::FOOTNOTE_MODE;

	//return (((FBReader *)mainApplication)->getMode() == FBReader::FOOTNOTE_MODE);

}

long long get_position() {

	ZLTextWordCursor cr = is_footnote_mode() ? footview->textArea().startCursor() : bookview->textArea().startCursor();
	int paragraph = !cr.isNull() ? cr.paragraphCursor().index() : 0;
	return pack_position(paragraph, cr.elementIndex(), cr.charIndex());
}

long long get_end_position() {

	ZLTextWordCursor cr = is_footnote_mode() ? footview->textArea().endCursor() : bookview->textArea().endCursor();
	if (cr.isNull()) {
		mainApplication->refreshWindow();
		cr = is_footnote_mode() ? footview->textArea().endCursor() : bookview->textArea().endCursor();
	}
	if (cr.isNull()) return get_position();
	return pack_position(cr.paragraphCursor().index(), cr.elementIndex(), cr.charIndex());

}



static long long get_bookview_position() {

	ZLTextWordCursor cr = bookview->textArea().startCursor();
	int paragraph = !cr.isNull() ? cr.paragraphCursor().index() : 0;
	return pack_position(paragraph, cr.elementIndex(), cr.charIndex());
}

static void draw_bmk_flag(int update) {
	int i, x, y, bmkset=0;
	long long s, e;
	if (! bmk_flag) return;
	if (is_footnote_mode()) return;
	s = get_position();
	e = get_end_position();
	for (i=0; i<docstate.nbmk; i++) {
		if (docstate.bmk[i] >= s && docstate.bmk[i] < e) bmkset = 1;
	}
	if (! bmkset) return;
	x = ScreenWidth()-bmk_flag->width;
	y = 0;
	DrawBitmap(x, y, bmk_flag);
	if (update) PartialUpdate(x, y, bmk_flag->width, bmk_flag->height);
}

//static void printpos(char *s, long long n) {

//	int p, w, l, cpage;
//	unpack_position(n, &p, &w, &l);
//	cpage = position_to_page(n);
//	fprintf(stderr, "%s %i  %lld %i %i %i\n", s, cpage, n, p, w, l);

//}

int get_cpage()
{
    return position_to_page(get_position());
}

int get_npage()
{
    return npages;
}

int get_calc_progress()
{
    return calc_in_progress;
}

int get_page_word_list(SynWordList **word_list, int *wlist_len)
{
	int result = 0;
	
	if (word_list && wlist_len)
	{
		*word_list = acwordlist;
		*wlist_len = acwlistcount;
                result = 1;
	}

	return result;
}

int get_text_direction()
{
	if (docstate.text_direction == -1)
	{
		FBReader &fbreader = FBReader::Instance();
		shared_ptr<Book> book = fbreader.currentBook();
		std::string language = book->language();
		docstate.text_direction = ZLLanguageUtil::isRTLLanguage(language) ? 1 : 0;
	}

	return docstate.text_direction;
}

bool is_rtl_book()
{
	return get_text_direction() == 1;
}

int get_page_word_list(iv_wlist **word_list, int *wlist_len)
{
	int result = 0;

	if (is_rtl_book())
	{
		if (rtl_wordlist != NULL)
		{
			delete[] rtl_wordlist;
			rtl_wordlist = NULL;
		}

		rtl_wordlist = new iv_wlist[xxx_wlistlen + 1];

		for (int i=0; i<xxx_wlistlen; i++)
			rtl_wordlist[i] = xxx_wordlist[xxx_wlistlen - i - 1];

		rtl_wordlist[xxx_wlistlen].word = NULL;
	}

	if (word_list && wlist_len)
	{
                *word_list =  rtl_wordlist!= NULL ? rtl_wordlist : xxx_wordlist;
		*wlist_len = xxx_wlistlen;
		result = 1;
	}

	return result;
}



static void repaint_panel() {

	if (downloading) {
		draw_download_panel(panel_buf, 0);
	} else {
		#ifdef PBBER
			DrawPanel((ibitmap *)panel_icon, panel_buf, GetLangText("@BeratorBig"), panel_percent);
		#else
			//DrawPanel((ibitmap *)icon, buf, regStatus ? title : GetLangText("@Unregistered"),  regStatus ? percent : -1);
			DrawPanel((ibitmap *)panel_icon, panel_buf, panel_title, panel_percent);
		#endif
	}

}

static void panel_update_timer() {

	repaint_panel();
	PartialUpdate(0, ScreenHeight()-PanelHeight(), ScreenWidth(), PanelHeight());

}

void repaint_all(int update_mode) {

	/* update_mode: -1=none 0=soft 1=full 2,3=only_mainhandler */

	int cpage;
	const ibitmap *bgnd;

	if (update_mode >= 2 && GetEventHandler() != PBMainFrame::main_handler) return;

	switch (update_mode) {
		case 0:
		case 2:
			if (xxx_hasimages && xxx_grayscale) {
				xxx_use_update = 1; // soft
			} else {
				xxx_use_update = 2; // bw
			}
			break;
		case 1:
		case 3:
			if (invertupdate > 0 && ++upd_count >= invertupdate) {
				xxx_use_update = 0; // full
			} else if (xxx_hasimages && xxx_grayscale) {
				xxx_use_update = 1; // soft
			} else {
				xxx_use_update = 2; // bw
			}
			break;
		case 4:
			xxx_use_update = 0;
			break;
		default:
			xxx_use_update = -1;
			break;
		case -1:
			xxx_use_update = 3;
			break;
		case -2:
			xxx_use_update = 4;
			break;
	}
	if (fast_turn == 0 && xxx_use_update == 2) xxx_use_update = 1;

        ClearScreen();

	bgnd = (orient == 0 || orient == 3) ? bgnd_p : bgnd_l;
        if (bgnd) DrawBitmap(0, 0, bgnd);

	if (webpanel) draw_webpanel(0);

	MarkMarkers();
	if (hadMarkers && xxx_use_update == 2)
		xxx_use_update = 1;

	restore_current_position();
	if (xxx_use_update == 2 || hadMarkers)
		mainApplication->refreshWindow();

	Stretch(screenbuf, IMAGE_GRAY4, imgwidth, imgheight, scanline,
		imgposx, imgposy, imgwidth, imgheight, 0);


        UpdatePanelData();

	if (links_mode && ! making_snapshot) {
		panel_icon = NULL;
		if (link_back >= 0) DrawBitmap(ScreenWidth() > 800 ? 8 : 0, ScreenWidth() > 800 ? 5 : 0, &arrow_back);
		invert_current_link(0);
		sprintf(panel_buf, "%s", GetLangText("@choose_link"));
	}
	if (search_mode && ! making_snapshot) {
		draw_searchpan();
	}

        repaint_panel();

        DrawSynopsisLabels();

	// prepare links on this page
	for (int i=0; i<nlinks; i++) {
		free(links[i].href);
		links[i].href = 0;
	}
	nlinks = 0;


        char *s = get_backlink();
        if (s != NULL) {
		links[nlinks].x = 2 + ScreenWidth() > 800 ? 8 : 0;
		links[nlinks].y = 2 + ScreenWidth() > 800 ? 5 : 0;
                links[nlinks].w = 30;
                links[nlinks].h = 14;
                links[nlinks].kind = 37; // external link
                links[nlinks].href = strdup(s);
                nlinks++;
        }
        FBView * viewX = is_footnote_mode() ? (FBView *)footview : (FBView *)bookview;

        if (viewX->textArea().get_xxx_links(xxx_page_links))
	{
		for (int i=0; i<(int)xxx_page_links.size(); i++) {
			xxx_link cl = xxx_page_links.at(i);
			links[nlinks].x = imgposx+cl.x1;
			links[nlinks].y = imgposy+cl.y1;
			links[nlinks].w = cl.x2-cl.x1+1;
			links[nlinks].h = cl.y2-cl.y1+1;
			links[nlinks].kind = cl.kind;
			links[nlinks].type = cl.type;
			links[nlinks].href = strdup(cl.id.c_str());

			nlinks++;
			if (nlinks >= MAXLINKS)
				break;
		}

		for (std::vector<struct xxx_link>::iterator it = xxx_page_links.begin(); it != xxx_page_links.end(); it++)
                {
                    if ((*it).kind != FOOTNOTE)
                    {
			if (!is_rtl_book())
				DrawLine((*it).x1+imgposx, (*it).y2+imgposy, (*it).x2+imgposx, (*it).y2+imgposy, BLACK);
			else
			{
				int viewportWidth = ScreenWidth() - imgposx*2;
				DrawLine(viewportWidth - (*it).x2+imgposx, (*it).y2+imgposy, viewportWidth - (*it).x1+imgposx, (*it).y2+imgposy, BLACK);
			}
                    }
		}
	}

        if (!firstUpdate)
        {
            if (xxx_use_update == 1 || xxx_use_update == 2)
            {
                xxx_use_update = 0;
                firstUpdate = true;
            }

        }
	// update screen
	switch (xxx_use_update) {
		case 0:                    	
                        ClearTimer(panel_update_timer);
                        if (HQUpdateSupported())
                            FullUpdateHQ();
                        else
                            FullUpdate();
			upd_count = 0;
			break;
		case 1:
			ClearTimer(panel_update_timer);
			SoftUpdate();
			break;
		case 2:
			PartialUpdateBW(0, 0, ScreenWidth(), ScreenHeight() - PanelHeight() - 3);
			SetHardTimer("UpdatePanel", panel_update_timer, 1200);
			break;
		case 3:
			if (selMarkery1 < selMarkery2)
			{
				selMarkery1 -= maxstrheight;
				if (selMarkery1 < 0)
					selMarkery1 = 0;
				selMarkery2 += maxstrheight;
				if (selMarkery2 > ScreenHeight() - PanelHeight() - 3)
					selMarkery2 = ScreenHeight() - PanelHeight() - 3;
				updMarkerx = 0;
				updMarkery = selMarkery1;
				updMarkerw = ScreenWidth();
				updMarkerh = selMarkery2 - selMarkery1;
				selMarkery2 -= maxstrheight;
				selMarkery1 = selMarkery2;
			}
			else
			{
				selMarkery1 += maxstrheight;
				if (selMarkery1 > ScreenHeight() - PanelHeight() - 3)
					selMarkery1 = ScreenHeight() - PanelHeight() - 3;
				selMarkery2 -= maxstrheight;
				if (selMarkery2 < 0)
					selMarkery2 = 0;
				updMarkerx = 0;
				updMarkery = selMarkery2;
				updMarkerw = ScreenWidth();
				updMarkerh = selMarkery1 - selMarkery2;
				selMarkery2 += maxstrheight;
				selMarkery1 = selMarkery2;
			}
			ClearTimer(panel_update_timer);
			break;
		case 4:
			updMarkerx = 0;
			updMarkery = 0;
			updMarkerw = ScreenWidth();
			updMarkerh = ScreenHeight() - PanelHeight() - 3;
			ClearTimer(panel_update_timer);
			break;
	}
	xxx_use_update = 0;


}

void refresh_page(int update) {

	restore_current_position();
	bookview->clearCaches();
	footview->clearCaches();
	mainApplication->refreshWindow();
	repaint_all(update); 

}

int one_page_back(int update_mode) {
	restore_current_position();
//	mainApplication->refreshWindow();
	long long tmppos=get_position();
	int p = position_to_page(tmppos);
	if (is_footnote_mode() || p > npages || p == 0 || calc_in_progress) {
		mainApplication->doAction(ActionCode::PAGE_SCROLL_BACKWARD);
		mainApplication->refreshWindow();
	} else {
		set_position(page_to_position(p-1));
		mainApplication->refreshWindow();
	}
	cpos = get_position();
//	printpos("< ", cpos);
	calc_position_changed = 1;
	if (cpos == tmppos && is_footnote_mode()) {
		mainApplication->doAction(ActionCode::CANCEL);
		mainApplication->refreshWindow();
		cpos = get_position();
	}
	repaint_all(update_mode); 
	return (cpos != tmppos);
}

int one_page_forward(int update_mode) {

	restore_current_position();
	long long tmppos=get_position();
	int p = position_to_page(tmppos);
	if (is_footnote_mode() || p >= npages) {
		mainApplication->doAction(ActionCode::PAGE_SCROLL_FORWARD);
		mainApplication->refreshWindow();
	} else {
		set_position(page_to_position(p+1));
		mainApplication->refreshWindow();
	}
	cpos = get_position();
//	printpos("> ", cpos);
	calc_position_changed = 1;
	repaint_all(update_mode); 
	return (cpos != tmppos);
}

void prev_section() {
	restore_current_position();
	if (is_footnote_mode()) {
		mainApplication->doAction(ActionCode::CANCEL);
	}
	mainApplication->doAction(ActionCode::GOTO_PREVIOUS_TOC_SECTION);
	cpos = get_position();
	calc_position_changed = 1;
	repaint_all(1); 
}

void next_section() {
	restore_current_position();
	if (is_footnote_mode()) {
		mainApplication->doAction(ActionCode::CANCEL);
	}
	mainApplication->doAction(ActionCode::GOTO_NEXT_TOC_SECTION);
	cpos = get_position();
	calc_position_changed = 1;
	repaint_all(1); 
}

void jump_pages(int n) {

	char buf[16];
	int x, y, w, h, cpage;

	if (calc_in_progress) {
                (n < 0) ? one_page_back(1) : one_page_forward(1);
	} else {
		cpage = position_to_page(cpos);
		cpage += n;
		if (cpage < 1) cpage = 1;
		if (cpage > npages) cpage = npages;
		cpos = page_to_position(cpage);
		set_position(cpos);
		w = 50;
		h = (menu_n_font->height*3) / 2;
		x = ScreenWidth()-w-5;
		y = ScreenHeight()-h-30;
		FillArea(x+1, y+1, w-2, h-2, WHITE);
		DrawRect(x+1, y, w-2, h, BLACK);
		DrawRect(x, y+1, w, h-2, BLACK);
		sprintf(buf, "%i", cpage);
		SetFont(menu_n_font, BLACK);
		DrawTextRect(x, y, w, h, buf, ALIGN_CENTER | VALIGN_MIDDLE);
		PartialUpdateBW(x, y, w, h);

	}

}

static void start_recalc_timer()
{
    calc_pages();
}
static const int minimalFontSize = 19;
static const int maximalFontSize = 35;

void font_change(int d) {

	char buf[256], *p;
	int size;

	restore_current_position();

	char *xfont = ReadString(fcfg, "font", DEFREADERFONT);
	strcpy(buf, xfont);
	p = strchr(buf, ',');
	if (p) {
		size = atoi(p+1);
	} else {
		p = buf+strlen(buf);
		size = 12;
	}

	size += d;
        if (size > 39)
            size = 19;
        else if (size < 19)
            size = 39;

	sprintf(p, ",%i", size);
	WriteString(fcfg, "font", buf);
        apply_config(1, 1);
	repaint_all(1);

}

void show_hide_panel() {

	restore_current_position();
	SetPanelType((PanelHeight() == 0) ? 1 : 0);
        apply_config(1, 1);
	repaint_all(1);

}

void show_hide_webpanel(int n) {

	restore_current_position();
	webpanel = n;
        apply_config(1, 1);
	repaint_all(0);

}

void ChangeSettings(fb2_settings settings)
{
	if (settings.id > 0)
	{
		SetOrientation(settings.orientation);
//		char tmp[200];
//		snprintf(tmp, 190, "%s,%d", settings.font.c_str(), settings.fontsize);
//		WriteString(fcfg, "font", tmp);
//		snprintf(tmp, 190, "%s,%d", settings.fontb.c_str(), settings.fontsize);
//		WriteString(fcfg, "font.b", tmp);
//		snprintf(tmp, 190, "%s,%d", settings.fonti.c_str(), settings.fontsize);
//		WriteString(fcfg, "font.i", tmp);
//		snprintf(tmp, 190, "%s,%d", settings.fontbi.c_str(), settings.fontsize);
//		WriteString(fcfg, "font.bi", tmp);

//		snprintf(tmp, 190, "%s,%d", settings.font.c_str(), settings.fontsize);
		WriteString(fcfg, "font", settings.font.c_str());
//		snprintf(tmp, 190, "%s,%d", settings.fontb.c_str(), settings.fontsize);
		WriteString(fcfg, "font.b", settings.fontb.c_str());
//		snprintf(tmp, 190, "%s,%d", settings.fonti.c_str(), settings.fontsize);
		WriteString(fcfg, "font.i", settings.fonti.c_str());
//		snprintf(tmp, 190, "%s,%d", settings.fontbi.c_str(), settings.fontsize);
		WriteString(fcfg, "font.bi", settings.fontbi.c_str());

		WriteString(fcfg, "encoding", settings.encoding.c_str());

		WriteInt(fcfg, "linespacing", settings.linespacing);
		WriteInt(fcfg, "border", settings.border);
		WriteInt(fcfg, "hyphenations", settings.hyphenations);
		WriteInt(fcfg, "preformatted", settings.preformatted);
		WriteInt(fcfg, "textdir", settings.textdir);
                apply_config(1, 0);
	}
}

static void apply_config(int recalc, int canrestart) {

	char buf[256], *p;
	int size;

	char *xfont = ReadString(fcfg, "font", DEFREADERFONT);
	strcpy(buf, xfont);
	p = strchr(buf, ',');
	if (p) *(p++) = 0;
	size = p ? atoi(p) : 12;


	static bool first_run = true;
	if (first_run && is_rtl_book())
	{
		// при первом запуске заменяем шрифт на арабский
		sprintf(buf, "DroidSans.ttf,%d", size); /*Droid Sans MTI Arabic*/
		WriteString(fcfg, "font", buf);

		sprintf(buf, "DroidSans-Bold.ttf,%d", size);
		WriteString(fcfg, "font.b", buf);

		// для курсива нет отдельных файлов - ставим обычный шрифт
		sprintf(buf, "DroidSans.ttf,%d", size);
		WriteString(fcfg, "font.bi", buf);

		sprintf(buf, "DroidSans.ttf,%d", size);
		WriteString(fcfg, "font.i", buf);

		first_run = false;
	}


        ifont *f = OpenFont(buf, size, 1);

	int linespacing = ReadInt(fcfg, "linespacing", 100);
        fontChangeStep = ReadInt(fcfg, "fontchangestep", 3);
        ApplyAntialiasing();

	FBReader &fbreader = FBReader::Instance();
	FBTextStyle &baseStyle = FBTextStyle::Instance();

	baseStyle.FontFamilyOption.setValue(f->family);
	baseStyle.FontSizeOption.setValue(size);
	baseStyle.BoldOption.setValue(f->isbold ? true : false);
	baseStyle.ItalicOption.setValue(f->isitalic ? true : false);
	baseStyle.LineSpacePercentOption.setValue(linespacing);


	// непонятно для чего это было сделано в старой версии. По идее должно было влиять на отступ первой
	// строки в абзаце, но нигде не использовалось
	//	ZLTextStyleCollection::Instance().SetUserDelta(0); /////.....

	int hyph_override = ReadInt(fcfg, "hyphenations", 0);
	fbreader.setGlobalAllowHyphenations(hyph_override != 0);

	load_images = ReadInt(fcfg, "loadimages", 1);

        fast_turn = ReadInt(fcfg, "fastturn", 0);


	std::string newenc = ReadString(fcfg, "encoding", "auto");
	int newfmt = ReadInt(fcfg, "preformatted", 0);
	int newtextdir = ReadInt(fcfg, "textdir", 0);
	if (canrestart && (newenc != docstate.encoding || newfmt != docstate.preformatted || newtextdir != docstate.text_direction)) {
		docstate.text_direction = newtextdir;
		ShowHourglass();
                RestartApplication(true);
	}

	orient = GetOrientation();
        int border = ReadInt(fcfg, "border", 2);
	ZLNXViewWidget *widget = (ZLNXViewWidget *) (&mainApplication->myViewWidget);

	if (orient == 0 || orient == 3) {
		imgposx = textarea.x;
		imgposy = textarea.y;
		imgwidth = textarea.w;
		imgheight = textarea.h;
	} else {
		imgposx = textarea.y;
		imgposy = textarea.x;
		imgwidth = textarea.h;
		imgheight = textarea.w;
	}
	if (imgposy+imgheight > ScreenHeight()-PanelHeight()) {
		imgheight = ScreenHeight()-PanelHeight()-imgposy;
	}

	if (webpanel) {
		imgposy += webpanel_height();
		imgheight -= webpanel_height();
	}

	if (border == 0) {
		imgposx += 8; // old_value = 4
		imgposy += 6;
		imgwidth -= 8;
		imgheight -= 10;
	} else if (border == 1) {
		imgposx += 22; // old_value = 18
		imgposy += 24;
		imgwidth -= 36;
		imgheight -= 36;
	} else {
		imgposx += 40; // old_value = 36
		imgposy += 36;
		imgwidth -= 72;
		imgheight -= 62;
	}

	imgwidth = (imgwidth+3) & ~3;
	imgheight = (imgheight+3) & ~3;
	scanline = imgwidth/2;
	widget->setSize(imgwidth, imgheight, scanline);

	fbreader.clearTextCaches();
        bookview->clearCaches();
        footview->clearCaches();
	mainApplication->refreshWindow();

	CloseFont(f);

	switch (recalc) {
		case 0:
			fprintf(stderr, "clr_timer\n");
			ClearTimer(start_recalc_timer);
			break;
		case 1:
			fprintf(stderr, "calc_pages\n");
                        calc_pages();
			break;
		case 2:
			if (calc_in_progress || calc_orientation == -1) {
                                calc_pages();
			} else {
				if (calc_orientation != GetOrientation() && calc_orientation+GetOrientation() != 3) {
					fprintf(stderr, "set_timer\n");
					SetHardTimer("RECALC", start_recalc_timer, 5000);
				} else {
					fprintf(stderr, "clr_timer\n");
					ClearTimer(start_recalc_timer);
				}
			}
			break;
	}

	// PenSettings
	PenSettings.id = 0;
	PenSettings.orientation = GetOrientation();
	PenSettings.font = ReadString(fcfg, "font", "default,12");
	PenSettings.fontb = ReadString(fcfg, "font.b", "defaultb,12");
	PenSettings.fonti = ReadString(fcfg, "font.i", "defaulti,12");
	PenSettings.fontbi = ReadString(fcfg, "font.bi", "defaultbi,12");
	PenSettings.fontsize = size;
	PenSettings.encoding = newenc;
	PenSettings.linespacing = linespacing;
	PenSettings.border = border;
	PenSettings.hyphenations = hyph_override;
	PenSettings.preformatted = newfmt;
	PenSettings.textdir = newtextdir;
}


void rotate_handler(int n) {

	restore_current_position();
	int cn = GetOrientation();
	if (n == -1 || ko == 0 || ko == 2) {
		SetGlobalOrientation(n);
	} else {
		SetOrientation(n);
		WriteInt(fcfg, "orientation", n);
		SaveConfig(fcfg);
	}
	orient = GetOrientation();
        apply_config((cn+orient == 3) ? 0 : 1, 1);
	repaint_all(4);

}

int ornevt_handler(int n) {

	restore_current_position();
	GetOrientation();
	SetOrientation(n);
	webpanel_update_orientation();
        apply_config(2, 1);
	repaint_all(4);
	return 0;

}

#include <bookstate.h>

void save_state() {

  if (no_save_state) return;
  if (docstate.magic != 0x9751) return;
  restore_current_position();
  docstate.position = get_bookview_position();
  strncpy(docstate.encoding, ReadString(fcfg, "encoding", "auto"), 15);
  docstate.preformatted = ReadInt(fcfg, "preformatted", 0);

  fprintf(stderr, "fbreader - save settings...\n");

  DataFile = GetAssociatedFile(OriginalName, 0);
  FILE *f = iv_fopen(DataFile, "wb");
  if (f != NULL) {
	iv_fwrite(&docstate, 1, sizeof(tdocstate), f);
	iv_fclose(f);
  }

	bsHandle newstate = is_doc(OriginalName) | is_docx(OriginalName) ?
				bsLoad(OriginalName) :
				bsLoad(FileName);
	if (newstate)
	{
		bsSetCPage(newstate, get_cpage());
		bsSetNPage(newstate, get_npage());
		bsSetOpenTime(newstate, time(NULL));
		if (bsSave(newstate))
			fprintf(stderr, "Save to db ok!\n");
		else
			fprintf(stderr, "Save to db failed!\n");
		bsClose(newstate);
	}

  if (! calc_in_progress) {
	int cpage = position_to_page(cpos);
	if (npages-cpage < 3 && cpage >= 5) {
		ZetFile = GetAssociatedFile(OriginalName, 'z');
		f = iv_fopen(ZetFile, "w");
		fclose(f);
	}
  }

  CloseConfig(fcfg);

  fprintf(stderr, "fbreader - save settings done\n");

}

void RestartApplication(bool saveState)
{
    DrawWaitMessage();
    if (saveState) save_state();
    switch (argc_main)
    {
    case 1:
        execl(argv_main[0], argv_main[0], 0, 0, 0);
        break;
    case 2:
        execl(argv_main[0], argv_main[0], argv_main[1], 0, 0);
        break;
    case 3:
        execl(argv_main[0], argv_main[0], argv_main[1], argv_main[2], 0);
        break;
    case 4:
        execl(argv_main[0], argv_main[0], argv_main[1], argv_main[2], argv_main[3]);
        break;
    }
}

#define VERSION_PAGES 6
const unsigned char DEFAULT_PAGES_FILE_VER[3] = {VERSION_PAGES, sizeof(int), sizeof(long long)};


bool SavePagesCache(const char * fileName, const TPageList & pages)
{
    //std::cerr << "**"<< fileName << "**";
    const unsigned char * md5;
    std::cerr << "Save Pages Cache: ";
    FILE * file = iv_fopen(fileName, "wb");

    if (!file)
    {
        std::cerr << "file open error\n";
        return false;
    }


    //Save FileVersion
    int c = iv_fwrite(DEFAULT_PAGES_FILE_VER, 1, 3, file);

    if (c != 3)
    {
        goto file_open_error;
    }

    //Save md5 of the current file
    md5 = PBMD5::MainFileMD5().GetData();
    c = iv_fwrite(md5, 1, PBMD5::MainFileMD5().GetLength(), file);
    if (c != PBMD5::MainFileMD5().GetLength())
    {
        goto file_open_error;
    }

    //Save pages quantity
    c = iv_fwrite(&npages, sizeof(int), 1, file);
    if (c != 1)
    {
        goto file_open_error;
    }

    //Save pages data
    c = iv_fwrite(pageList.data(), sizeof(long long), npages, file);
    if (c != npages)
    {
        goto file_open_error;
    }

    std::cerr << "file succesfully saved\n";
    fclose(file);
    return true;

file_open_error:
    std::cerr << "file write error\n";
    fclose(file);
    remove(PagesFile);
    return false;

}
//#include <stdio.h>
//#include <stdlib.h>

bool CheckCache()
{
    std::cerr << "Load Pages Cache: ";
    assert(sizeof(int) == 4);
    PagesFile = GetAssociatedFile(OriginalName, 'p');

    //std::cerr << "**"<< PagesFile << "**";

    FILE * file = iv_fopen(PagesFile, "rb");

    unsigned char ver[3];
    if (file)
    {
        int c = iv_fread(&ver, 1, 3, file);

        if (c != 3)
        {
            fclose(file);
            std::cerr << "Bad read bytes count. must be -- 3 realy " << c << "\n";
            return false;
            //SavePagesCache(PagesFile);
        }
        if (memcmp(ver,DEFAULT_PAGES_FILE_VER, 3))
        {
            std::cerr << "Bad version of Pages Cache file. must be Version ";
            for (int i = 0; i < 3; i++)
            {
                std::cerr << (int)(DEFAULT_PAGES_FILE_VER[i]) << "X";
            }
            std::cerr << " is: ";
            for (int i = 0; i < 3; i++)
            {
                std::cerr << (int)(ver[i]) << "X";
            }
            std::cerr << "\n";
            fclose(file);
            return false;
        }

        //Read md5 of the current file
        const unsigned char * md5 = PBMD5::MainFileMD5().GetData();
        unsigned char currentMd5[17];
        c = iv_fread(currentMd5, 1, 16, file);
        currentMd5[16] = 0;
        if (c != 16)
        {
            fclose(file);
            std::cerr << "MD5 of the cached file " << c << "\n";
            return false;
        }

        if (memcmp(currentMd5, md5, 16))
        {
            fclose(file);
            std::cerr << "MD5 of the cached file is not eqaul to owers\n";
            std::cerr << "Local file md5 is " << PBMD5::MainFileMD5().ToString() << "\n";
            std::cerr << "Cached file md5 is " << currentMd5 << "\n";
            return false;
        }

        int size;
        c = iv_fread(&size, sizeof(int), 1, file);
        if (c != 1)
        {
            fclose(file);
            std::cerr << "Size in Pages Cache file not read; \n";
            return false;
            //SavePagesCache(PagesFile);
        }

        pageList.resize(size, 0);

        c = iv_fread(pageList.data(), sizeof(long long), size, file);
        if (c != size)
        {
            fclose(file);
            std::cerr << "The whole Pages Cache file is not read; \n";
            return false;
            //SavePagesCache(PagesFile);
        }

        std::cerr << "Success\n ";
        npages = size;
        fclose(file);
        return true;

    }
    std::cerr << "File open error\n";
    return false;

}
void calc_pages() {

    if (/*doCheckCache && */CheckCache())
    {
        std::cerr << "Pages cache loaded. \n";
        oldPageList.resize(pageList.size());        
        oldPageList = pageList;
    }


    //remove(PagesFile);
    fprintf(stderr, "calc_pages\n");

    calc_in_progress = 1;
    FBReader::calc_in_progress = true;
    calc_position_changed = 1;
    current_position_changed = 1;
    calc_current_page = npages = 1;
    calc_current_position = pack_position(0, 0, 0);
    calc_orientation = -1;

    pageList.clear();
    pageList.push_back(calc_current_position);
    SetHardTimer("CalcPages", calc_timer, 1);
    //calcTimes = DEFAULT_TO_START_TIMER;


}


void calc_timer() {
    int timesLeft = 20;



    ZLNXPaintContext::lock_drawing = 1;
    current_position_changed = 1;

    if (calc_position_changed) {
            set_position(calc_current_position);
            calc_position_changed = 0;
    }


        int p = 0;

        if (! calc_in_progress) {                
                    calc_orientation = GetOrientation();    


                    bookview->clearCaches();
                    footview->clearCaches();




                mainApplication->refreshWindow();
                ZLNXPaintContext::lock_drawing = 0;
                if (GetEventHandler() == PBMainFrame::main_handler && PBMainFrame::is_menu_visible() == false)
                {
                        repaint_all(0);
                }

                return;
        }

        if (calc_paused)
        {
                ZLNXPaintContext::lock_drawing = 0;
                return;
        }

        if (is_footnote_mode()) {

                SetWeakTimer("CalcPages", calc_timer, 200);

                ZLNXPaintContext::lock_drawing = 0;
                return;
        }


        ZLNXPaintContext::lock_drawing = true;




            //mainApplication->doAction(ActionCode::PAGE_SCROLL_FORWARD);
            set_position(calc_current_position);            
            bool areEqual = false;
            if (!ForwardScrollingAction::run(*bookview, calc_current_position, calc_current_page, npages,  pageList, oldPageList, TIME_UPDATE_PANEL, areEqual))
            {

                //end
                mainApplication->doAction(ActionCode::GOTO_NEXT_TOC_SECTION);
                calc_current_position = get_position();
                int s2, s1 = pageList.size();
                s2 = oldPageList.size();
                if (areEqual || calc_current_position == pageList[calc_current_page-1])
                {
                        calc_in_progress = 0;
                        FBReader::calc_in_progress = false;
                        current_position_changed = 0;
                        SetHardTimer("CalcPages", calc_timer, 0);
                        ZLNXPaintContext::lock_drawing = false;                        
                        if (areEqual)
                        {
                            pageList.resize(oldPageList.size());
                            pageList = oldPageList;
                            npages = pageList.size();
                            std::cerr << "Saved cache pages are equal. \n";
                        }
                        else if (SavePagesCache(PagesFile, pageList))
                        {
                            std::cerr << "Pages cache saved. \n";
                            //Successfully saved
                        }
                        UpdatePanelData();
                        PartialUpdate(0, ScreenHeight() - 28, ScreenWidth(), 28);
                }                
           }
        SetHardTimer("CalcPages", calc_timer, 0 /*200*/);        
        ZLNXPaintContext::lock_drawing = false;        
}

long long pack_position(int para, int word, int letter) {

	return ((long long)para << 40) | ((long long)word << 16) | ((long long)letter);

}

void unpack_position(long long pos, int *para, int *word, int *letter) {

	*para = (pos >> 40) & 0xffffff;
	*word = (pos >> 16) & 0xffffff;
	*letter = pos & 0xffff;

}

long long page_to_position(int page) {

	if (page < 1) page = 1;
	if (page > npages) page = npages;
	return pageList[page-1];

}
int position_to_page(long long position) {
	if (position == 0)
		return 1;

	int page = 0;
	for (TPageListIt it = pageList.begin(); it != pageList.end(); it++, page++)
	{            
		if (position < (*it))
			return page;
		else if (position == (*it))
			return page+1;
	}

//	int i;

//	for (i=1; i<=npages; i++) {
//		if (position < pageListMap[i]){
//			return i;
//		}
//		//		if (position <= pagelist[i-1]) return i;
//	}
	return npages;

}

void set_position(long long pos) {

	int para, word, letter;

	unpack_position(pos, &para, &word, &letter);
	bookview->gotoPosition(para, word, letter);
	calc_position_changed = 1;

}

void make_update()
{
    repaint_all(0);
}

int set_page(int page)
{
    long long tmppos=get_position();
    if (is_footnote_mode() || page > npages) {
			mainApplication->doAction(ActionCode::PAGE_SCROLL_FORWARD);
    } else {
            set_position(page_to_position(page));
            mainApplication->refreshWindow();
    }
    cpos = get_position();
	//printpos("> ", cpos);
    calc_position_changed = 1;
    return (cpos != tmppos);

    //set_position(page_to_position(page));
}

void restore_current_position() {

	if (current_position_changed) {
		set_position(cpos);
		current_position_changed = 0;
		calc_position_changed = 1;
	}

}

static void wait_to_calc(int page) {

	int hgshown=0;
	while (calc_in_progress && calc_current_page < page) {
		if (! hgshown) ShowHourglass();
		hgshown=1;
		calc_timer();
	}
//	ClearTimer(calc_timer);


}
#include "pbpagehistorynavigation.h"

void select_page(int page) {


	wait_to_calc(page);        
	cpos = page_to_position(page);
	set_position(cpos);
	calc_position_changed = 1;
	mainApplication->refreshWindow();
	repaint_all(4);



}

static void bmk_paint(int /*page*/, long long pos) {

	set_position(pos);
	mainApplication->refreshWindow();
	repaint_all(-1);
	set_position(cpos);

}	

static void bmk_selected(int /*page*/, long long pos) {


	cpos = pos;
	set_position(pos);
	calc_position_changed = 1;
	mainApplication->refreshWindow();
	repaint_all(1);

}

static void bmk_added(int /*page*/, long long /*pos*/) {

/*
	char buf[256];

	if (page > 0 && page < 99999) {
		sprintf(buf, "%s %i", GetLangText("@Add_Bmk_P"), page);
	} else {
		sprintf(buf, "%s", GetLangText("@Add_Bmk_P"));
	}
	Message(ICON_INFORMATION, GetLangText("@Bookmarks"), buf, 1500);
*/
	draw_bmk_flag(1);

}

static void bmk_handler(int action, int page, long long pos) {

	switch (action) {
		case BMK_PAINT: bmk_paint(page, pos); break;
		case BMK_ADDED: bmk_added(page, pos); break;
		case BMK_SELECTED: bmk_selected(page, pos); break;
		case BMK_CLOSED: repaint_all(0); break;
	}

}

static void open_bookmarks() {

	int i;

	restore_current_position();
	if (is_footnote_mode()) mainApplication->doAction(ActionCode::CANCEL);
	for (i=0; i<docstate.nbmk; i++) {
		bmkpages[i] = calc_in_progress ? i+1 : position_to_page(docstate.bmk[i]);
	}
	long long bvpos = get_bookview_position();
	int bvpage = calc_in_progress ? -1 : position_to_page(bvpos);
	OpenBookmarks(bvpage, bvpos, bmkpages, docstate.bmk, &docstate.nbmk, 30, bmk_handler);

}

static void new_bookmark() {

	int i;

	restore_current_position();
	for (i=0; i<docstate.nbmk; i++) {
		bmkpages[i] = calc_in_progress ? i+1 : position_to_page(docstate.bmk[i]);
	}
	long long bvpos = get_bookview_position();
	int bvpage = calc_in_progress ? -1 : position_to_page(bvpos);
	SwitchBookmark(bvpage, bvpos, bmkpages, docstate.bmk, &docstate.nbmk, 30, bmk_handler);
	repaint_all(0);

}

static void toc_selected(long long position) {

	cpos = position;
	set_position(cpos);
	calc_position_changed = 1;
	mainApplication->refreshWindow();
	repaint_all(1);

}

void PrepareSynTOC(int opentoc)
{
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

	if (SynTOC.GetHeader() == NULL)
	{
		long long pos;
		char *p;
		int i;

		if (TOC)
		{
			for (i=0; i<toc_size; i++)
				free(TOC[i].text);
			free(TOC);
			TOC = NULL;
		}

		toc_size = xxx_myTOC.size();
		if (toc_size == 0)
		{
			if (opentoc == 1)
			{
				Message(ICON_INFORMATION, "FBReader", "@No_contents", 2000);
			}
			return;
		}

		TOC = (tocentry *) malloc((toc_size+1) * sizeof(tocentry));
		for (i=0; i<toc_size; i++)
		{
			TOC[i].level = xxx_myTOC[i].level;
			if (xxx_myTOC[i].paragraph == -1)
			{
				TOC[i].position = -1;
				TOC[i].page = 0;
			}
			else
			{
				pos = pack_position(xxx_myTOC[i].paragraph, 0, 0);
				TOC[i].position = pos;
				TOC[i].page = calc_in_progress ? -1 : position_to_page(pos);
			}
			TOC[i].text = strdup((char *)(xxx_myTOC[i].text.c_str()));
			p = TOC[i].text;
			while (*p)
			{
				if (*p == '\r' || *p == '\n')
					*p = ' ';
				p++;
			}
			SynopsisContent *Content = new SynopsisContent(TOC[i].level, TOC[i].position, TOC[i].text);
			SynTOC.AddTOCItem(Content);
		}
		restore_current_position();
		pos = get_end_position();
	}
}

static void open_contents() {

	long long pos;
	char *p;
	int i;

	if (TOC) {
		for (i=0; i<toc_size; i++) free(TOC[i].text);
		free(TOC);
		TOC = NULL;
	}

	toc_size = xxx_myTOC.size();
	if (toc_size == 0) {
		Message(ICON_INFORMATION, "FBReader", "@No_contents", 2000);
		return;
	}

	TOC = (tocentry *) malloc((toc_size+1) * sizeof(tocentry));
	for (i=0; i<toc_size; i++) {
		TOC[i].level = xxx_myTOC[i].level;
		if (xxx_myTOC[i].paragraph == -1) {
			TOC[i].position = -1;
			TOC[i].page = 0;
		} else {
			pos = pack_position(xxx_myTOC[i].paragraph, 0, 0);
			TOC[i].position = pos;
			TOC[i].page = calc_in_progress ? -1 : position_to_page(pos);
		}
		TOC[i].text = strdup((char *)(xxx_myTOC[i].text.c_str()));
		p = TOC[i].text;
		while (*p) {
			if (*p == '\r' || *p == '\n') *p = ' ';
			p++;
		}

		//int p, w, l;
		//unpack_position(pos, &p, &w, &l);
		//fprintf(stderr, "%i %i %i\n", p, w, l);
		//fprintf(stderr, "%llx %i %s\n", pos, TOC[i].page, TOC[i].text);
	}

	restore_current_position();
	pos = get_end_position();
	OpenContents(TOC, toc_size, pos, toc_selected);

}

static void open_notes_menu() {

	OpenNotesMenu(OriginalName, book_title, cpos);

}

static void configuration_updated() {

        SaveConfig(fcfg);
	restore_current_position();
	SetEventHandler(PBMainFrame::main_handler);
        apply_config(1, 1);

}

int human_size(char *buf, unsigned long long bsize, unsigned long long nblocks) {

	unsigned long long value = bsize * nblocks;
	int n;

	if (value < 1000LL) {
		n = snprintf(buf, 32, "%i %s", (int)value, GetLangText("@bytes"));
	} else if (value < 100000LL) {
		value = (value * 10LL) / 1024LL;
		n = snprintf(buf, 32, "%i.%i %s", (int)(value/10LL), (int)(value%10LL), GetLangText("@kb"));
	} else if (value < 1000000LL) {
		n = snprintf(buf, 32, "%i %s", (int)(value/1024LL), GetLangText("@kb"));
	} else if (value < 100000000LL) {
		value = (value * 10LL) / (1024LL * 1024LL);
		n = snprintf(buf, 32, "%i.%i %s", (int)(value/10LL), (int)(value%10LL), GetLangText("@mb"));
	} else {
		n = snprintf(buf, 32, "%i %s", (int)(value/(1024LL * 1024LL)), GetLangText("@mb"));
	}
	return n;

}

int human_date(char *buf, time_t t) {

	time_t tt = t;
	struct tm *ctm = localtime(&tt);

	return sprintf(buf, "%s  %02i:%02i", DateStr(t), ctm->tm_hour, ctm->tm_min);

}

void show_about() {

	char buf[2048], sbuf[128], *p;
	int i, n=0;
	int size = sizeof(buf)-1;

	p = strrchr(OriginalName, '/');
	n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@Filename"), p ? p+1 : OriginalName);

	if (bi->typedesc && bi->typedesc[0]) {
	  n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@Type"), GetLangText(bi->typedesc));
	}

	if (bi->title && bi->title[0]) {
	  n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@Title"), bi->title);
	}

	if (bi->author && bi->author[0]) {
	  n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@Author"), bi->author);
	}

	if (bi->series && bi->series[0]) {
	  n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@Series"), bi->series);
	}

	char LangGenre[1024];
	for (i=0; i<10; i++) {
	  if (bi->genre[i] == NULL) break;
	  if (bi->genre[i][0]) {
		LangGenre[0] = '@';
		strcpy(&LangGenre[1], bi->genre[i]);
		char *pLangGenre = GetLangText(LangGenre);
		if (pLangGenre && *pLangGenre != '@')
		{
		    n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@Genre"), pLangGenre);
		}
		else
		{
		    n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@Genre"), bi->genre[i]);
		}
	  }
	}

	human_size(sbuf, 1, bi->size);
	n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@Size"), sbuf);

	human_date(sbuf, bi->ctime);
	n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@Written"), sbuf);

	Message(ICON_INFORMATION, "@About_book", buf, 30000);

}

static void cfg_item_changed(char *name) {

	if (strcmp(name, "about") == 0) show_about();

}

static void invert_current_link(int update) {

	hlink *cl = &(links[clink]);
	if (!is_rtl_book() || (clink == 0 && cl->x == 2 && cl->y == 2) )
	{
		InvertAreaBW(cl->x, cl->y, cl->w, cl->h);
		if (update)
			PartialUpdateBW(cl->x, cl->y, cl->w, cl->h);
	}
	else
	{
		int viewportWidth = ScreenWidth();
		InvertAreaBW(viewportWidth - (cl->x + cl->w), cl->y, cl->w, cl->h);
		if (update)
			PartialUpdateBW(viewportWidth - (cl->x + cl->w), cl->y, cl->w, cl->h);
	}
}

static void end_link_mode() {

	int i;
	links_mode = 0;
	for (i=0; i<nlinks; i++) {
		free(links[i].href);
		links[i].href = 0;
	}

}

static void jump_ref(char *ref) {

	if (is_footnote_mode()) {
		mainApplication->doAction(ActionCode::CANCEL);
	}
	if (ref[0] == '=') {
		fprintf(stderr, "ref before = %s\n", ref);
		int lenref = strlen(ref);
		while(lenref > 2 && *(ref + 1) == '0')
		{
			ref++;
			lenref--;
		}
		fprintf(stderr, "lenref = %d ref after = %s\n", lenref, ref + 1);
		cpos = strtoll(ref+1, NULL, 0);
	} else {
		((FBReader *)mainApplication)->tryShowFootnoteView(ref, false);
		cpos = get_position();
	}
	set_position(cpos);

}

static void open_int_ext_link(char *href) {

	if (!href)
		return;

	char buf[1024], *tmp, *p, *pp;
	int len;

	fprintf(stderr, "openlink: %s\n", href);

	while (*href == ' ' || *href == '\"' || *href == '\'' || *href == '\\') href++;
	trim_right(href);
	p = href + strlen(href);
	while (p > href && (*(p-1) == '\"' || *(p-1) == '\'' || *(p-1) == '\\')) *(--p) = 0;
	if (*href == 0) return;
	for (p=href; *p; p++) if (*p == '\"') *p = '&';

	// #anchor in the same file

	len = strlen(OriginalName);
	if (strncasecmp(href, OriginalName, len) == 0 && href[len]=='#') {
		jump_ref(href+len+1);
		mainApplication->refreshWindow();
		return;
	}

	// unsupported absolute web links

	if (strncasecmp(href, "mailto:", 7) == 0 ||
	    strncasecmp(href, "news:", 5) == 0 ||
	    strncasecmp(href, "nntp:", 5) == 0 ||
	    strncasecmp(href, "telnet:", 7) == 0 ||
	    strncasecmp(href, "wais:", 5) == 0 ||
	    strncasecmp(href, "gopher:", 7) == 0 ||
	    strncasecmp(href, "javascript:", 11) == 0
	) {
		Message(ICON_WARNING, "FBReader", "@Is_ext_link", 2000);
		return;
	}

	// absolute web links

	if (strncasecmp(href, "http:", 5) == 0 ||
	    strncasecmp(href, "ftp:", 4) == 0 ||
	    strncasecmp(href, "https:", 6) == 0
	) {
		if (QueryNetwork() == 0) {
			Message(ICON_WARNING, "FBReader", "@Is_ext_link", 2000);
		} else {
			if (PBPath::IsExist("/ebrmain/bin/browser.app")) { // is browser available (midory)
				OpenBook("/ebrmain/bin/browser.app", href, OB_WITHRETURN);
			} else if (PBPath::IsExist("/ebrmain/bin/bookland.app")) { // links
				OpenBook("/ebrmain/bin/bookland.app", href, OB_WITHRETURN);
			} else {
				set_current_url(href);
				start_download();
			}
		}
		return;
	}

	// relative web links

	if (is_cached_url(OriginalName)) {

		// #anchor in the same url

		p = strchr(current_url, '#');
		len = (p == NULL) ? strlen(current_url) : p-current_url;
		if (strncasecmp(href, current_url, len) == 0 && href[len]=='#') {
			jump_ref(href+len+1);
			mainApplication->refreshWindow();





			return;
		}

		// server root url

		if (href[0] == '/') {
			p = strchr(current_url, ':');
			if (p) p = strchr(p+4, '/');
			if (! p) p = current_url+strlen(current_url);
			len = p - current_url;
			tmp = (char *) malloc(len+strlen(href)+4);
			strncpy(tmp, current_url, len);
			strcpy(tmp+len, href);
			set_current_url(tmp);
			free(tmp);
			start_download();
			return;
		}

		// url relative to current

		if (isalnum(href[0]) || href[0]=='_' || href[0]=='.' || href[0]=='%') {
			p = strrchr(current_url, '/');
			if (p == NULL || strchr(current_url, '/')+1 >= p) p = current_url+strlen(current_url);
			len = p - current_url;
			tmp = (char *) malloc(len+strlen(href)+4);
			strncpy(tmp, current_url, len);
			tmp[len] = '/';
			strcpy(tmp+len+1, href);
			set_current_url(tmp);
			free(tmp);
			start_download();
			return;
		}

	}

	// filesystem links

	if (href[0] == '/') {
		pp = strrchr(href, '/') + 1;
	} else {
		pp = href;
	}
	strcpy(buf, OriginalName);
	p = strrchr(buf, '/');
	if (p == NULL) p = buf; else p++;
	strcpy(p, pp);
	pp = strchr(p, '#');
	if (pp != NULL) {
		*(pp++) = 0;
	}
	ShowHourglass();
	OpenBook(buf, pp, 1);

}

static void process_link(int num) {

	hlink *cl = &(links[num]);
	FILE *f;

	restore_current_position();
	if (link_back >= 0 && num == 0) {
//		iv_truncate(HISTORYFILE, link_back);
		char historypath[256];
		snprintf(historypath, 255, "%s/history%s.lh", TEMPDIR, OriginalName);
		iv_buildpath(historypath);
		iv_truncate(historypath, link_back);
		char *s = get_backlink();
		if (s != NULL) {
			cl->href = strdup(s);
		}
	} else {
//		f = iv_fopen(HISTORYFILE, "a");
		char historypath[256];
		snprintf(historypath, 255, "%s/history%s.lh", TEMPDIR, OriginalName);
		iv_buildpath(historypath);
		f = iv_fopen(historypath, "a");
		if (f != NULL) {
			if (is_cached_url(OriginalName)) {
				fprintf(f, "%s\n", current_url);
			} else {
				fprintf(f, "%s#=%lld\n", OriginalName, get_position());
			}
			iv_fclose(f);
		}
	}

	fprintf(stderr, "%i  %s\n", cl->kind, cl->href);

	FBReader &fbreader = FBReader::Instance();

        if (cl->kind == INTERNAL_HYPERLINK) { // int. hyperlink

		if (is_footnote_mode()) {
			mainApplication->doAction(ActionCode::CANCEL);
		}
		fbreader.tryShowFootnoteView(cl->href, cl->type);

        } else if (cl->kind == FOOTNOTE) { // footnote

		fbreader.tryShowFootnoteView(cl->href, cl->type);


        } else if (cl->kind == EXTERNAL_HYPERLINK) { // ext. hyperlink

		open_int_ext_link(cl->href);

	}

	end_link_mode();
	if (GetEventHandler() != PBMainFrame::main_handler) {
		SetEventHandler(PBMainFrame::main_handler);
	} else {
		repaint_all(1);
	}

}


static int links_handler(int type, int par1, int /*par2*/) {

	hlink *cl = &(links[clink]);
	int y0a, y0b, y1a, y1b;

	switch (type) {

		case EVT_SHOW:
			repaint_all(0);
			break;

		case EVT_KEYPRESS:
		case EVT_KEYREPEAT:
			if (par1 == KEY_UP || par1 == KEY_DOWN || par1 == KEY_LEFT || par1 == KEY_BACK || par1 == KEY_OK) {
				invert_current_link(0);
				y0a = cl->y;
				y1a = cl->y+cl->h+1;
				switch (par1) {

				case KEY_UP:
					clink--;
					if (type == EVT_KEYREPEAT) clink = 0;
					if (clink < 0) goto lk_exit;
					goto lk_update;
				case KEY_DOWN:
					clink++;
					if (type == EVT_KEYREPEAT) clink = nlinks-1;
					if (clink >= nlinks) clink = 0;
					lk_update:
					y0b = links[clink].y;
					y1b = links[clink].y+links[clink].h+1;
					if (y0b < y0a) y0a = y0b;
					if (y1b > y1a) y1a = y1b;
					invert_current_link(0);
					PartialUpdateBW(0, y0a, ScreenWidth(), y1a-y0a);
					break;

				case KEY_LEFT:
				case KEY_BACK:
					lk_exit:
					end_link_mode();
					soft_update = 1;
					SetEventHandler(PBMainFrame::main_handler);
					break;

				case KEY_OK:
					process_link(clink);
					break;
				}
			}

	}
	return 0;

}

static char *get_backlink() {

	static char buf[1024], *p;
	FILE *f;
	int fp=0, cfp=0;

	link_back = -1;
	buf[0] = 0;
//	f = iv_fopen(HISTORYFILE, "r");
	char historypath[256];
	snprintf(historypath, 255, "%s/history%s.lh", TEMPDIR, OriginalName);
	iv_buildpath(historypath);
	f = iv_fopen(historypath, "r");
	if (f != NULL) {
		while (1) {
			cfp = iv_ftell(f);
			if (iv_fgets(buf, sizeof(buf)-1, f) == NULL || (buf[0] & 0xe0) == 0) break;
			fp = cfp;
		}
		iv_fseek(f, fp, SEEK_SET);
		iv_fgets(buf, sizeof(buf)-1, f);
		iv_fclose(f);
		p = buf + strlen(buf);
		while (p > buf && (*(p-1) & 0xe0) == 0) *(--p) = 0;
	}

	if (buf[0] != 0) {
		link_back = fp;
		return buf;
	}
	return NULL;

}

void back_link() {

	char *s = get_backlink();
	if (s != NULL) {
		nlinks = 1;
		links[0].kind = 37; // external link
		links[0].href = strdup(s);
		process_link(0);
	}

}

void open_links() {

	if (nlinks > 0 || link_back >= 0){
		links_mode = 1;
		clink = 0;
		invert_current_link(0);
		SetEventHandler(links_handler);
	} else {
		Message(ICON_INFORMATION, "FBReader", "@No_links", 2000);
	}

}


static void search_next(int dir) {

	long long xpos;

	restore_current_position();
	mainApplication->refreshWindow();
	xpos = cpos;
	ShowHourglass();

	bool res = false;
	if (dir > 0) {
		if (bookview->canFindNext()){
			bookview->findNext();
			res = true;
		}
	} else {
		if (bookview->canFindPrevious()){
			bookview->findPrevious();
			res = true;
		}
	}

	if (res){
		cpos = get_position();
		int page = position_to_page(cpos);
		set_page(page);

		if (cpos == xpos) {
			if (dir > 0)
				set_page(page+1);
			else
				set_page(page-1);
		}

		mainApplication->refreshWindow();
		repaint_all(1);
	} else {
		Message(ICON_INFORMATION, GetLangText("@Search"), GetLangText("@No_more_matches"), 2000);
		HideHourglass();
	}

}

static void cancel_search() {
	search_mode = 0;
	shared_ptr<ZLTextModel> model = bookview->textArea().model();
	model->removeAllMarks();
	SetEventHandler(PBMainFrame::main_handler);
}

static int search_handler(int type, int par1, int par2) {

	static int x0, y0;

	if (type == EVT_SHOW) {
		//out_page(0);
	}

	if (IsBookRTL() && ISKEYEVENT(type)) {
		if (par1 == KEY_LEFT) {
			par1 = KEY_RIGHT;
		} else if (par1 == KEY_RIGHT) {
			par1 = KEY_LEFT;
		}
	}

	if (type == EVT_POINTERDOWN) {
		x0 = par1;
		y0 = par2;
	}

	if (type == EVT_POINTERUP) {
		if (par1-x0 > EPSX) {
                        one_page_back(1);
			return 1;
		}
		if (x0-par1 > EPSX) {
                        one_page_forward(1);
			return 1;
		}
		if (par1 > ScreenWidth()-searchbm.width && par2 > ScreenHeight()-PanelHeight()-searchbm.height) {
			if (par1 < ScreenWidth()-searchbm.width/2) {
				search_next(-1);
			} else {
				search_next(+1);
			}
			return 1;
		} else {
			cancel_search();
		}

	}
		
	if (type == EVT_KEYPRESS) {

		switch (par1) {
			case KEY_OK:
			case KEY_BACK:
				cancel_search();
				break;
			case KEY_LEFT:
			case KEY_PREV:
			case KEY_PREV2:
				search_next(-1);
				break;
			case KEY_RIGHT:
			case KEY_NEXT:
			case KEY_NEXT2:
				search_next(+1);
				break;
		}

	}
	return 0;

}

void search_enter(char *text) {

	if (text == NULL || text[0] == 0)
		return;
	std::string s(text);

	restore_current_position();
	mainApplication->refreshWindow();
	SetEventHandler(search_handler);
	search_mode = 1;
	ShowHourglass();

	bookview->search(s, true, false, false, false);

	shared_ptr<ZLTextModel> model = bookview->textArea().model();
	model->marks().empty();

	if (model->marks().empty())
	{
		Message(ICON_INFORMATION, GetLangText("@Search"), GetLangText("@No_more_matches"), 2000);
		HideHourglass();
		cancel_search();
		return;
	}

	repaint_all(1);

}

void stop_jump() {
	if (! calc_in_progress) {
		mainApplication->refreshWindow();
		repaint_all(1);
	}
}

static void new_note() {
	ibitmap *bm1=NULL, *bm2=NULL;
	bm1 = BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight());
	if (one_page_forward(-1)) {
		bm2 = BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight());
		one_page_back(-1);
	}
	CreateNoteFromImages(OriginalName, book_title, get_bookview_position(), bm1, bm2);
}
void save_page_note() { CreateNoteFromPage(OriginalName, book_title, get_bookview_position()); }

static iconfigedit * item_by_name(iconfigedit *ce, char *name) {

	static iconfigedit dummy_ce;
	int i;

	for (i=0; ce[i].type!=0; i++) {
		if (ce[i].name == NULL) continue;
		if (strcmp(ce[i].name, name) == 0) return &(ce[i]);
	}
	return &dummy_ce;

}

void open_config() {

	if (is_webpage) {
		item_by_name(fbreader_ce, "loadimages")->type &= ~CFG_HIDDEN;
	}
	OpenConfigEditor("@FB_config", fcfg, fbreader_ce, configuration_updated, cfg_item_changed);

}

// against compiler bugs
static void x_ready_sent(int n) { ready_sent = n; }

void main_show()
{
    upd_count = 9999;
	restore_current_position();
	mainApplication->refreshWindow();
	repaint_all(2/*soft_update ? 0 : 1*/);
	//soft_update = 0;
    x_ready_sent(ready_sent);
    if (! ready_sent) BookReady(OriginalName);
    ready_sent = 1;
    Link=false;
    if (dl_request) {
            start_download();
            dl_request = 0;
    }
    if (is_webpage) {
            start_download_images();
    }
}

void save_screenshot()
{
    fprintf(stderr, "EVT_SNAPSHOT\n");
    making_snapshot = 1;
    repaint_all(-1);
    PageSnapshot();
    making_snapshot = 0;
}

static void sigsegv_handler(int /*signum*/) {

	x_ready_sent(ready_sent);
	if (! ready_sent) {
		fprintf(stderr, "SIGSEGV caught - clearing state\n");
		iv_unlink(DataFile);
		no_save_state = 1;
	} else {

		fprintf(stderr, "SIGSEGV caught\n");
	}
	exit(127);

}

//	ZLIntegerRangeOption &option = ZLTextStyleCollection::instance().baseStyle().FontSizeOption;

static int is_html(char *name) {

	char *p = strrchr(name, '.');
	if (! p) return 0;
	if (strcasecmp(p, ".html") == 0) return 1;
	if (strcasecmp(p, ".htm") == 0) return 1;
	if (strcasecmp(p, ".shtml") == 0) return 1;
	if (strcasecmp(p, ".asp") == 0) return 1;
	if (strcasecmp(p, ".php") == 0) return 1;
	if (strcasecmp(p, ".cgi") == 0) return 1;
	if (strcasecmp(p, ".jsp") == 0) return 1;
	return 0;

}

static int is_doc(char *name) {

	char *p = strrchr(name, '.');
	if (! p) return 0;
	return (strcasecmp(p, ".doc") == 0);

}

static int is_docx(char *name) {

	char *p = strrchr(name, '.');
	if (! p) return 0;
	return (strcasecmp(p, ".docx") == 0);

}

static int is_wlnk(char *name) {

	char *p = strrchr(name, '.');
	if (! p) return 0;
	return (strcasecmp(p, ".wlnk") == 0);

}

static void cleanup_temps() {

	system("rm -rf /tmp/fbreader.temp");

}

char *stristr(const char * str1, const char * str2)
{
      char *pptr, *sptr, *start;

	  for (start = (char *)str1; *start != '\0'; start++)
      {
            /* find start of pattern in string */
            for ( ; ((*start != '\0') && (toupper(*start) != toupper(*str2))); start++);
            if (*start == '\0')
                  return NULL;

            pptr = (char *)str2;
            sptr = (char *)start;

            while (toupper(*sptr) == toupper(*pptr))
            {
                  sptr++;
                  pptr++;

                  /* if end of pattern then pattern was found */

                  if (*pptr == '\0')
                        return (start);
            }
      }
      return NULL;
}

int pointer_handler(int type, int par1, int par2)
{
	int i;

	if (type == EVT_POINTERDOWN)
	{
		for (i=0; i<nlinks; i++)
			if (par1>=links[i].x && par1<=links[i].x+links[i].w && par2>=links[i].y && par2<=links[i].y+links[i].h)
				break;

		if (i==nlinks)
			return 0;
		Link=true;
		links_mode = 1;
		clink=i;
		return 1;

	}
	else if (type == EVT_POINTERUP)
	{
		//fprintf(stderr, "point up\n");
		if (Link)
		{
			invert_current_link(1);
			if (par1>=links[clink].x && par1<=links[clink].x+links[clink].w && par2>=links[clink].y && par2<=links[clink].y+links[clink].h)
			{
				process_link(clink);
				links_mode = 0;
				Link=false;
				return 1;
			}
			else
			{
				links_mode = 0;
				Link=false;
			}
		}
		return 0;
	}

	return 0;
}
static void DrawWaitMessage()
{
    //static const int helpRectHeight = 50;
   //PBRect rect(0, ScreenHeight() / 2 - helpRectHeight / 2, ScreenWidth(), helpRectHeight);
    ShowHourglass();
    //SetFont(OpenFont(DEFAULTFONT, 20, 1), BLACK);
    //DrawTextRect(rect.GetLeft(), rect.GetTop(), rect.GetWidth(), rect.GetHeight(), text, ALIGN_CENTER | VALIGN_MIDDLE);
    FullUpdate();
}

int main(int argc, char **argv)
{
        char * word = GetAssociatedFile("/home/hello.c", 'q');
        argv_main = argv;
        argc_main = argc;
	fprintf(stderr, "Starting FB2 reader, date of compilation: %s %s\n", __DATE__, __TIME__);
        ZLNXPaintContext::lock_drawing;
	FILE *f;
	char buf[1024];
	char *tbuf, *p, *pp;
	char *utfopt = "";
	int i, n;
	detectedlang[0]=0;

        cleanup_temps();
        OpenScreen();

        for (i = 0; i < argc; i++)
        {
            fprintf(stderr, "arg[%d]=%s\n", i, argv[i]);
        }

        gcfg = GetGlobalConfig();
        fcfg = OpenConfig(USERDATA"/config/fbreader.cfg", fbreader_ce);
	ko = -1;
        if (GetGlobalOrientation() == -1 || ko == 0 || ko == 2) {
            orient = GetOrientation();
        } else {
            orient = ReadInt(fcfg, "orientation", GetOrientation());
            SetOrientation(orient);
        }
        invertupdate = ReadInt(gcfg, "invertupdate", 10);

        snprintf(buf, sizeof(buf), "viewermenu_%s", ReadString(gcfg, "language", "en"));
        m3x3 = GetResource(buf, NULL);
        if (m3x3 == NULL) m3x3 = GetResource("viewermenu_en", NULL);
        if (m3x3 == NULL) m3x3 = NewBitmap(128, 128);
        bmk_flag = GetResource("bmk_flag", NULL);

        bgnd_p = GetResource("book_background_p", NULL);
        bgnd_l = GetResource("book_background_l", NULL);
        if (orient == 0 || orient == 3) {
            GetThemeRect("book.textarea", &textarea, 0, 0, ScreenWidth(), ScreenHeight(), 0);
        } else {
            GetThemeRect("book.textarea", &textarea, 0, 0, ScreenHeight(), ScreenWidth(), 0);
        }

        if (argc < 2 && QueryNetwork() == 0) {
            Message(ICON_WARNING, "FBReader", "@Cant_open_file", 2000);
            return 0;
        }
        read_styles();
        std::cerr << (HQUpdateSupported() ? "HQUpdate supported" : "HQUpdate not supported") << "\n";
        if (argc < 2 || is_weburl(argv[1])) {
            FileName = "/ebrmain/share/FBReader/web/default.html";
            webpanel = 1;
            is_webpage = 1;
            if (argc >= 2) {
                set_current_url(argv[1]);
                dl_request = 1;
            }
        } else if (argc >= 2 && is_wlnk(argv[1])) {
            FileName = "/ebrmain/share/FBReader/web/default.html";
            webpanel = 1;
            is_webpage = 1;
            f = fopen(argv[1], "r");
            if (f != NULL) {
                memset(buf, 0, sizeof(buf));
                fgets(buf, sizeof(buf)-1, f);
                fclose(f);
            }
            trim_right(buf);
            if (is_weburl(buf)) {
                set_current_url(buf);
                dl_request = 1;
            }
        } else {
            FileName=argv[1];
            bi = GetBookInfo(FileName);
            if (bi->title) book_title = strdup(bi->title);
            if (bi->author) book_author = strdup(bi->author);
        }

        if (is_cached_url(FileName)) {
            webpanel = 1;
            is_webpage = 1;
        }

        ApplyAntialiasing();


        OriginalName = FileName;
        std::cerr << std::endl << "MD5: " << PBMD5::MainFileMD5().ToString() << std::endl;
        DataFile = GetAssociatedFile(OriginalName, 0);
        ZetFile = GetAssociatedFile(OriginalName, 'z');

        f = iv_fopen(DataFile, "rb");
	int state_size = 0;
	if (f != NULL)
	{
		state_size = iv_fread(&docstate, 1, sizeof(tdocstate), f);
	}
	if (f == NULL || (!(state_size == sizeof(tdocstate) || state_size == sizeof(tdocstate) - 8)) || docstate.magic != 0x9751) {
            docstate.magic = 0x9751;
            docstate.position = pack_position(0, 0, 0);
            strcpy(docstate.encoding, "auto");
            docstate.nbmk = 0;
            docstate.text_direction = -1; // direction is undefined
        }
	if (f != NULL)
	{
		iv_fclose(f);
		if (state_size == sizeof(tdocstate) - 8)
		{
		    docstate.text_direction = -1; // direction is undefined
		}
	}
	signal(SIGSEGV, sigsegv_handler);

        if (is_doc(OriginalName)) {
            system("mkdir " CONVTMP);
            if (strcasecmp(docstate.encoding, "@ip_auto") != 0 && strcasecmp(docstate.encoding, "auto") != 0 && strcasecmp(docstate.encoding, "utf-8") != 0) {
                utfopt = "-e";
            }
#ifdef EMULATOR
            sprintf(buf, "./antiword -x db %s \"%s\" >%s", utfopt, FileName, CONVTMP "/index.html");
#else
            sprintf(buf, "/ebrmain/bin/antiword.app -x db %s \"%s\" >%s", utfopt, FileName, CONVTMP "/index.html");
#endif
            fprintf(stderr, "%s\n", buf);
            system(buf);
            FileName = CONVTMP "/index.html";
        }
        if (is_docx(OriginalName)) {
            system("mkdir " CONVTMP);
            chdir(CONVTMP);
            sprintf(buf, "/ebrmain/bin/docx2html.app -o index.html -m index.files \"%s\"", FileName);
            system(buf);
            FileName = CONVTMP "/index.html";
        }

        if ((strcmp(docstate.encoding, "@ip_auto") == 0 || strcmp(docstate.encoding, "auto") == 0) && is_html(OriginalName)) {
            f = iv_fopen(FileName, "rb");
            if (f != NULL) {
                tbuf = (char *) malloc(10001);
                n = iv_fread(tbuf, 1, 10000, f);
                tbuf[n] = 0;
                p = stristr(tbuf, "charset=");
                if (p) {
                    p += 8;
                    pp = strchr(p, '\"');
                    if (pp != NULL && pp-p <= 12) {
                        *pp = 0;
                        for (pp=p; *pp; pp++) {
                            if (*pp>='a' && *pp<='z') *pp -= 0x20;
                        }
                        strcpy(docstate.encoding, p);
                    }
                }
                free(tbuf);
                iv_fclose(f);
            }
        }

        WriteString(fcfg, "encoding", docstate.encoding);
        FBReader::EncodingOverride = (strcmp(docstate.encoding, "@ip_auto") == 0 ? "auto" : docstate.encoding);

        if (docstate.text_direction == 1) // RTL
            FBReader::LanguageOverride = "ar"; // should work for hebrew also, but need test
        WriteInt(fcfg, "textdir", docstate.text_direction);

        WriteInt(fcfg, "preformatted", docstate.preformatted);
	break_override = docstate.preformatted;

        screenbuf = (unsigned char *)malloc(1024*1024/2);
        npages = 0;

        if (argc > 1) printf("opening %s...\n", argv[1]);
        int zargc = 1;
        char **zargv = NULL;
        if (!ZLibrary::init(zargc, zargv)) {
            fprintf(stderr, "Zlibrary::init() failed\n");
            Message(ICON_WARNING, "FBReader", "@Cant_open_file", 2000);
            return 0;
        }

        ZLibrary::run(new FBReader(FileName));
        FBReader& fbreader = FBReader::Instance();

        if (fbreader.isBookReady() == false) {
            fprintf(stderr, "Zlibrary::run() failed\n");
            Message(ICON_WARNING, "FBReader", "@Cant_open_file", 2000);
            return 0;
        }
        bookview = &fbreader.bookTextView();
        footview = &fbreader.footnoteView();

        apply_config(1, 0);

        if (argc >= 3) {
            fprintf(stderr, "jump_ref argv[2] = %s\n", argv[2]);
            jump_ref(argv[2]);
        } else {
            set_position(cpos = docstate.position);
        }
        //printpos(": ", cpos);

        SetRTLBook((is_rtl_book() ? 1 : 0));

        mainApplication->refreshWindow();

        int depth = GetHardwareDepth();
        fprintf(stderr, "HardwareDepth - %d, hwconfig - %llX, controller - %d\n", depth, hwconfig, ((int)((hwconfig >> 8) & 15)));

        CreateToolBar();
        PBMainFrame fbFrame;
        fbFrame.Create(NULL, 0,0,1,1, PBWS_VISIBLE, 0);
        fbFrame.Run();



	cleanup_temps();
	return 0;
} 

