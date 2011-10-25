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

#include "main.h"
#include <stdio.h>
#include <inkview.h>
#include <inkinternal.h>
//#include "TouchZoom.h"
#include <math.h>
#include <sys/wait.h>
#include "pbmainframe.h"

const char* APPCAPTION = "DJVU Viewer";

#ifdef USESYNOPSIS
// Denis // -> Synopsis
#include <synopsis/synopsis.h>
#include "synopsis/content.h"
#include "synopsis/bookmark.h"
#include "synopsis/note.h"
#include "synopsis/TOC.h"

SynopsisContent *Content;
SynopsisNote *Note;
SynopsisBookmark *Bookmark;
TSynopsisWordList List;
SynopsisTOC SynTOC;
static void DrawSynopsisLabels();

SynWordList *acwordlist=NULL;
int acwlistcount = 0;
int acwlistsize = 0;
extern ibitmap star;
extern "C" const ibitmap ac_note, ac_bmk;

//int glb_page;

// Denis //
// Denis // <-
#endif

#ifdef WIN32
#undef WIN32
#include "../libdjvu/miniexp.h"
#include "../libdjvu/ddjvuapi.h"
#define WIN32
#else
#include "../libdjvu/miniexp.h"
#include "../libdjvu/ddjvuapi.h"
#endif

#include "zoomer.h"

extern const ibitmap zoombm;
extern const ibitmap searchbm;
extern const ibitmap hgicon;

#define MAXRESULTS 200

// #define die(x...) { fprintf(stderr, x); exit(1); }

//static const char *def_menutext[9] = {
//	"@Goto_page", "@Exit", "@Search",
//	"@Bookmarks", "@Menu", "@Rotate",
//	"@Dictionary", "@Zoom", "@Contents"
//};
//
//static const char *def_menuaction[9] = {
//	"@KA_goto", "@KA_exit", "@KA_srch",
//	"@KA_obmk", "@KA_none", "@KA_rtte",
//	"@KA_dict", "@KA_zoom", "@KA_cnts"
//};
//
//char *strings3x3[9];

ddjvu_context_t *ctx;
ddjvu_document_t *doc;

tdocstate docstate;

pid_t bgpid = 0;

static int SCALES[19] = { 33, 50, 70, 80, 85, 90, 95, 100, 105, 110, 120, 130, 140, 150, 160, 170, 200, 300, 400};
#define NSCALES ((int)(sizeof(SCALES)/sizeof(int)))

//static int regStatus;
static iconfig *gcfg;
static int ko;

static char *book_title="";
static int orient=0;
static int cpage=1, npages=1;
static int cpagew, cpageh;
int offx, offy, oldoffy;
static int scrx, scry;
int scale=100;
static int offset;
static int calc_optimal_zoom;
static int thx, thy, thw, thh, thix, thiy, thiw, thih, panh, pgbottom;
static int zoom_mode=0;
static char *FileName;
static char *DataFile;
static int ready_sent = 0;
static int bmkrem;
static tocentry* toc;
static int toclen;
static float pagescalex;
static float pagescaley;
static int pageoffsetx;
static int pageoffsety;

static iv_wlist *diclist=NULL;
static int diclen=0, dicsize=0;

static long long bmkpos[32];

static char kbdbuffer[64];

int search_mode=0;
static char *stext;
static int spage, sdir;

static char *keyact0[32], *keyact1[32];

static ibitmap *m3x3;
static ibitmap *bmk_flag;

ddjvu_page_t* last_decoded_page = NULL;
static int last_decoded_cpage = -1;

//static void prev_page();
//static void next_page();

// Denis //

#define EPSX 50
#define EPSY 50
#define MENUMARGIN 150
//#define BOOKMARKMARGIN 100
#define SCROLLPAGEMARGINHEIGHT 150

static int px0, py0;
static bool AddBkMark, Move;

//int touchscale;
// Denis //

void fill_words( int page );

int get_npage()
{
	return npages;
}

int get_cpage()
{
	return npages;
}

static void handle(int wait)
{
	const ddjvu_message_t *msg;
	if (!ctx)
		return;
	if (wait)
		msg = ddjvu_message_wait(ctx);
	while ((msg = ddjvu_message_peek(ctx)))
    {
		switch(msg->m_any.tag)
        {
        case DDJVU_ERROR:
			fprintf(stderr,"ddjvu: %s\n", msg->m_error.message);
			break;
        default:
			break;
        }
		ddjvu_message_pop(ctx);
    }
}

static inline int is_portrait() { return (orient == 0 || orient == 3); }

static void find_off_x(int step)
{
	int sw=ScreenWidth();
	step *= (sw/3);

	//fprintf(stderr, "step=%i offx=%i offy=%i mx=%i ,my=%i pw=%i ph=%i\n", step,offx,offy,marginx,marginy,pw,ph);

	if (step < 0) {
		if (offx <= 0) {
			if (cpage == 1) return;
			cpage--;
			offx = cpagew-sw;
		} else {
			offx += step;
		}
	} else {
		if (offx >= cpagew-sw) {
			if (cpage >= npages) return;
			cpage++;
			offx = 0;
		} else {
			offx += step;
		}
	}

}

static void find_off(int step)
{
	int sw=ScreenWidth();
	int sh=ScreenHeight()-PanelHeight();

	oldoffy = offy;

	offy+=((sh-panh*2)*step);

	if (step > 0 && pgbottom == 1)
 	{
		offy=0;
		offx+=sw;
		if (scale < 200 || offx>=cpagew)
 	 	{
			if (cpage == npages) return;
			offx=0;
			cpage++;
 	 	}
 	 	else
			if (offx>=cpagew-sw)
				offx=cpagew-sw;
 	}
	else if (offy>cpageh-(sh-panh)) {
		offy = cpageh-(sh-panh);
	}

	if (offy<0 && offy>-(sh-panh*2)) {
		offy = 0;
	} else if (offy==-(sh-panh*2))
 	{
		offy=cpageh-(sh-panh);
		offx-=sw;
		if (scale < 200 || offx<=-sw)
 	 	{

			if (cpage == 1) return;
			offx=cpagew-sw;
			cpage--;
 	 	}
 	 	else
			if (offx<=0)
				offx=0;
 	}

	//if (offy <= 24) offy = 0;
	//if (offy >= cpageh-sh-24) offy = cpageh-sh;

}

/* Djvuapi events */

void draw_page_image() {

	int sw, sh, pw, ph, w, h, rowsize;
	ddjvu_page_t *page;
	ddjvu_rect_t prect;
	ddjvu_rect_t rrect;
	ddjvu_format_style_t style;
	ddjvu_render_mode_t mode;
	ddjvu_format_t *fmt;
	unsigned char *data;

	pgbottom=0;
	FillArea(thx, thy, thw, thh, WHITE);
	DrawBitmap(thx+(thw-hgicon.width)/2, thy+(thh-hgicon.height)/2, &hgicon);
	PartialUpdate(thx, thy, thw, thh);

	SetOrientation(orient);
	sw = ScreenWidth();
	sh = ScreenHeight() - PanelHeight();

	if (last_decoded_cpage != cpage - 1)
	{

		if (! (page = ddjvu_page_create_by_pageno(doc, cpage-1))) {
			fprintf(stderr, "Cannot access page %d.\n", cpage);
			return;
		}
		while (! ddjvu_page_decoding_done(page))
			handle(TRUE);
		if (ddjvu_page_decoding_error(page)) {
			fprintf(stderr, "Cannot decode page %d.\n", cpage);
			ddjvu_page_release(page);
			return;
		}

		if (last_decoded_page != 0)
		{
			ddjvu_page_release(last_decoded_page);
		}

		last_decoded_page = page;
		last_decoded_cpage = cpage - 1;

		//printf("Page was decoded\n");

	}
	else
	{
		page = last_decoded_page;

		//printf("Page was taken from cache\n");
	}

	if (calc_optimal_zoom)
	{
		CalculateOptimalZoom(doc, page, &scale, &offset);
	}


	pw = ddjvu_page_get_width(page);
	ph = ddjvu_page_get_height(page);

	scrx = scry = 0;
	w = sw;
	h = sh;

	prect.x = 0;
	prect.y = 0;
	prect.w = cpagew = (sw*scale)/100;
	cpageh = (prect.w * ph) / pw;
	prect.h = (prect.w * (ph - panh - 1)) / pw;

	if (scale > 50 && scale < 200) {
		offx = (cpagew - sw) / 2;
		//if (is_portrait()) offy = (cpageh - sh) / 2;
	}

	rrect.w = sw;
	rrect.h = sh;

	if (prect.w < sw) {
		scrx = (sw-prect.w) / 2;
		offx = 0;
		rrect.w = prect.w;
	} else {
		if (offx>prect.w-sw) offx=prect.w-sw;
		if (offx < 0) offx = 0;
	}
	if (prect.h < sh) {
		scry = (sh-prect.h) / 2;
		offy = 0;
		rrect.h = prect.h;
	} else {
		if (offy>=prect.h-sh) {
			offy=prect.h-sh;
			pgbottom=1;
		}
		if (offy < 0) offy = 0;
	}

	rrect.x = offx;
	rrect.y = offy;

	mode = DDJVU_RENDER_COLOR;
	style = DDJVU_FORMAT_GREY8;

	if (! (fmt = ddjvu_format_create(style, 0, 0))) {
		fprintf(stderr, "Cannot determine pixel style\n");
	}
	ddjvu_format_set_row_order(fmt, 1);
	ddjvu_format_set_y_direction(fmt, 1);
	//ddjvu_format_set_ditherbits(fmt, 8);

	rowsize = rrect.w;
	if (! (data = (unsigned char *)malloc(rowsize * rrect.h))) {
		fprintf(stderr, "Cannot allocate image buffer\n");
	}

	rrect.x += offset;

	if (rrect.x < 0) rrect.x=0;

	if (rrect.x + rrect.w >= prect.w)
	{
		rrect.x = prect.w - rrect.w;
	}

	pagescalex = (float)prect.w / pw;
	pagescaley = (float)prect.h / ph;
	pageoffsetx = rrect.x;
	pageoffsety = rrect.y;

	if (! ddjvu_page_render(page, mode, &prect, &rrect, fmt, rowsize, (char *)data)) {
		fprintf(stderr, "Cannot render image\n");
		//ddjvu_page_release(page);
		return;
	}

	// auto adjust
	int pp, xx, min = 255, max = 0;
	for (pp = 0; pp < rrect.h*rrect.w; pp+=rrect.w*3)
	{
		for (xx = 0; xx < rrect.w; xx+=3)
		{
			if (data[pp+xx] < min) min = data[pp+xx];
			if (data[pp+xx] > max) max = data[pp+xx];
		}
	}

	int c, i;
	if (max-min >= 100)
	{
		fprintf(stderr, "adjust: min=%i max=%i\n", min, max);
		int coeff = (270 * 1000 - 1) / (max - min);

		for (i = 0; i < rrect.w * rrect.h; ++i)
		{
			c = ((data[i] - min) * coeff) / 1000;
			if (c > 255) c = 255;
			data[i] = c;
		}
	}

	w = rrect.w;
	h = rrect.h;
	Stretch(data, IMAGE_GRAY8, w, h, rowsize, scrx, scry, w, h, 0);
	//int grads = (1 << GetHardwareDepth());
	//if (grads > 4) DitherArea(scrx, scry, w, h, grads, DITHER_DIFFUSION);
	int grads = (1 << GetHardwareDepth());
	if (grads < 8)grads = 8;
	DitherArea(scrx, scry, w, h, grads, DITHER_DIFFUSION);

	ddjvu_format_release(fmt);
	free(data);
	//ddjvu_page_release(page);

	thiw = (sw * 100) / cpagew; if (thiw > 100) thiw = 100;
	thih = (sh * 100) / cpageh; if (thih > 100) thih = 100;
	thix = (offx * 100) / cpagew; if (thix < 0) thix = 0; if (thix+thiw>100) thix = 100-thiw;
	thiy = (offy * 100) / cpageh; if (thiy < 0) thiy = 0; if (thiy+thih>100) thiy = 100-thih;

}

static void bg_monitor() {

	char buf[64];
	struct stat st;
	int status;

	if (bgpid == 0) return;
	waitpid(-1, &status, WNOHANG);
	sprintf(buf, "/proc/%i/cmdline", bgpid);
	if (stat(buf, &st) == -1) {
		bgpid = 0;
		return;
	}
	SetHardTimer("BGPAINT", bg_monitor, 500);

}

static int draw_pages() {

	int sw, sh, pw, ph, nx, ny, boxx, boxy, boxw, boxh, xx, yy, n, rowsize, w, h, size;
	ddjvu_page_t *page;
	ddjvu_rect_t prect;
	ddjvu_rect_t rrect;
	ddjvu_format_t *fmt;
	unsigned char *data;
	pid_t pid;

	// free as much memory as we can
	if (last_decoded_page != 0)
	{
		ddjvu_page_release(last_decoded_page);
		last_decoded_page = NULL;
		last_decoded_cpage = -1;
	}

	sw = ScreenWidth();
	sh = ScreenHeight()-thh;

	if (is_portrait() && scale == 50) {
		nx = ny = 2;
	} else if (is_portrait() && scale == 33) {
		nx = ny = 3;
	} else if (!is_portrait() && scale == 50) {
		nx = 2; ny = 1;
	} else if (!is_portrait() && scale == 33) {
		nx = 3; ny = 2;
	} else {
		return 1;
	}
	boxw = sw/nx;
	boxh = (sh-30)/ny;

	for (yy=0; yy<ny; yy++) {
		for (xx=0; xx<nx; xx++) {
			if (cpage+yy*nx+xx > npages) break;
			boxx = xx*boxw;
			boxy = yy*boxh;
			DrawRect(boxx+6, boxy+boxh-4, boxw-8, 2, DGRAY);
			DrawRect(boxx+boxw-4, boxy+6, 2, boxh-8, DGRAY);
			DrawRect(boxx+4, boxy+4, boxw-8, boxh-8, BLACK);
		}
	}



#ifndef EMULATOR

	if ((pid = fork()) == 0) {

#else

		FullUpdate();

#endif

		if (! (fmt = ddjvu_format_create(DDJVU_FORMAT_GREY8, 0, 0))) {
			fprintf(stderr, "Cannot determine pixel style\n");
		}
		ddjvu_format_set_row_order(fmt, 1);
		ddjvu_format_set_y_direction(fmt, 1);

		w = boxw-10;
		h = boxh-10;
		size = w*h;
		data = (unsigned char *) malloc(size);

		for (yy=0; yy<ny; yy++) {
			for (xx=0; xx<nx; xx++) {

				n = cpage+yy*nx+xx;
				if (n > npages) break;
				boxx = xx*boxw;
				boxy = yy*boxh;
				w = boxw-10;
				h = boxh-10;
				memset(data, 0xff, size);

				if (! (page = ddjvu_page_create_by_pageno(doc, n-1))) {
					fprintf(stderr, "Cannot access page %d.\n", cpage);
					return 1;
				}
				while (! ddjvu_page_decoding_done(page))
					handle(TRUE);
				if (ddjvu_page_decoding_error(page)) {
					fprintf(stderr, "Cannot decode page %d.\n", cpage);
					ddjvu_page_release(page);
					break;
				}

				pw = ddjvu_page_get_width(page);
				ph = ddjvu_page_get_height(page);

				prect.x = 0;
				prect.y = 0;
				prect.w = w;
				prect.h = h;

				rrect.x = 0;
				rrect.y = 0;
				rrect.w = w;
				rrect.h = h;

				rowsize = w;

				if (! ddjvu_page_render(page, DDJVU_RENDER_BLACK, &prect, &rrect, fmt, rowsize, (char *)data)) {
					fprintf(stderr, "Cannot render image\n");
					ddjvu_page_release(page);
					return 1;
				}

				ddjvu_page_release(page);

				Stretch(data, IMAGE_GRAY8, w, h, rowsize, boxx+5, boxy+5, w, h, 0);
				PartialUpdate(boxx+5, boxy+5, boxw, boxh);

			}
		}

		ddjvu_format_release(fmt);
		free(data);


#ifndef EMULATOR

		exit(0);

	} else {

		bgpid = pid;

	}

#endif

	SetHardTimer("BGPAINT", bg_monitor, 500);
	return cpage+nx*ny > npages ? npages+1-cpage : nx*ny;


}

static void kill_bgpainter() {

#ifndef EMULATOR

	int status;

	if (bgpid != 0) {
		kill(bgpid, SIGKILL);
		usleep(100000);
		waitpid(-1, &status, WNOHANG);
		bgpid = 0;
	}

#endif

}

static void draw_zoomer() {
	DrawBitmap(ScreenWidth()-zoombm.width-10, ScreenHeight()-zoombm.height-35, &zoombm);
}

static void draw_bmk_flag(int update)
{
#ifdef USESYNOPSIS
	char from[30];
	char to[30];

	TSynopsisItem::PositionToChar(cpage, from);
	TSynopsisItem::PositionToChar(cpage+1, to);

	if (SynTOC.IsBookmarkInside(from, to))
	{
		int x = ScreenWidth()-bmk_flag->width;
		int y = 0;
		DrawBitmap(x, y, bmk_flag);
		if (update)
			PartialUpdate(x, y, bmk_flag->width, bmk_flag->height);
	}
#else
	int i, x, y, bmkset=0;
	if (! bmk_flag) return;
	for (i=0; i<docstate.nbmk; i++) {
		if (docstate.bmk[i] == cpage) bmkset = 1;
	}
	if (! bmkset) return;
	x = ScreenWidth()-bmk_flag->width;
	y = 0;
	DrawBitmap(x, y, bmk_flag);
	if (update)
		PartialUpdate(x, y, bmk_flag->width, bmk_flag->height);
#endif
}

static void draw_searchpan() {
	DrawBitmap(ScreenWidth()-searchbm.width, ScreenHeight()-searchbm.height-PanelHeight(), &searchbm);
}

void out_page(int full) {

	char buf[48];
	int i, n=1, h;

	ClearScreen();

	if (scale>50)
		draw_page_image();
	else
		n = draw_pages();
	if (zoom_mode) {
		draw_zoomer();
	}
	if (n == 1) {
		sprintf(buf, "    %i / %i    %i%%", cpage, npages, scale);
	} else {
		sprintf(buf, "    %i-%i / %i    %i%%", cpage, cpage+n-1, npages, scale);
	}

	h = DrawPanel(NULL, buf, book_title, (cpage * 100) / npages);
	
	thx = 4;
	thy = ScreenHeight()-h+4;
	thh = h-6;
	thw = (thh*6)/8;
	FillArea(thx, thy, thw, thh, WHITE);
	thx+=1; thy+=1; thw-=2; thh-=2;
	if (1/*scale >= 200*/) {
		FillArea(thx+(thw*thix)/100, thy+(thh*thiy)/100, (thw*thiw)/100, (thh*thih)/100, BLACK);
	}
	draw_bmk_flag(0);

	if (search_mode) {
		fill_words(cpage-1);
		for (i=0; i<diclen; i++) {
			if (utfcasestr(diclist[i].word, stext) != NULL) {
				InvertArea(diclist[i].x1, diclist[i].y1, diclist[i].x2-diclist[i].x1, diclist[i].y2-diclist[i].y1);
			}
		}
		draw_searchpan();
	}

	if (full) {
		FullUpdate();
	} else {
		PartialUpdate(0, 0, ScreenWidth(), ScreenHeight());
	}
}

static void page_selected(int page) {

	if (page < 1) page = 1;
	if (page > npages) page = npages;
	cpage = page;
	offx = offy = 0;
	out_page(1);

}

static void rotate_handler(int n) {

	if (n == -1 || ko == 0) {
		SetGlobalOrientation(n);
	} else {
		SetOrientation(n);
	}
	orient = GetOrientation();
	//offx = offy = 0;
	out_page(1);

}

int ornevt_handler(int n) {

	orient = n;
	SetOrientation(n);
	out_page(1);
	return 0;
}

static void draw_jump_info(char *text) {

	int x, y, w, h;

	SetFont(menu_n_font, BLACK);
	w = StringWidth("999%")+20;
	if (StringWidth(text) > w-10) w = StringWidth(text) + 20;
	h = (menu_n_font->height*3) / 2;
	x = ScreenWidth()-w-5;
	y = ScreenHeight()-h-30;
	FillArea(x+1, y+1, w-2, h-2, WHITE);
	DrawRect(x+1, y, w-2, h, BLACK);
	DrawRect(x, y+1, w, h-2, BLACK);
	DrawTextRect(x, y, w, h, text, ALIGN_CENTER | VALIGN_MIDDLE);
	PartialUpdateBW(x, y, w, h);

}

static void zoom_timer() { out_page(1); }

static void do_zoom(int add) {

	char buf[16];
	int i;

	calc_optimal_zoom = 0;

	for (i=0; i<NSCALES; i++) {
		if (SCALES[i] == scale) break;
	}
	if (i == NSCALES) {
		scale = 100;
	} else {
		if (i+add >= 0 && i+add < NSCALES) {
			i+=add;
			scale = SCALES[i];
		} else {
			return;
		}
	}
	sprintf(buf, "%i%%", scale);
	draw_jump_info(buf);
	SetHardTimer("ZOOM", zoom_timer, 1000);

}

void turn_page(int n) {

	offx = 0;
	offy = 0;
	if (n > 0 && cpage < npages) {
		cpage += n;
		if (cpage < 1) cpage = 1;
		out_page(1);
	}
	if (n < 0 && cpage > 1) {
		cpage += n;
		if (cpage > npages) cpage = npages;
		out_page(1);
	}

}

void jump_pages(int n) {

	char buf[16];

	cpage += n;
	if (cpage < 1) cpage = 1;
	if (cpage > npages) cpage = npages;
	offx = offy = 0;
	sprintf(buf, "%i", cpage);
	draw_jump_info(buf);

}

void readTocRecurse(miniexp_t exp, int offset, int only_calculate)
{
	if ( !miniexp_listp( exp ) )
        return;

    int l = miniexp_length( exp );
    int i;
    for ( i = offset; i < l; ++i )
    {
        miniexp_t cur = miniexp_nth( i, exp );

        if ( miniexp_consp( cur ) && ( miniexp_length( cur ) > 0 ) &&
             miniexp_stringp( miniexp_nth( 0, cur ) ) && miniexp_stringp( miniexp_nth( 1, cur ) ) )
        {
            const char* name = miniexp_to_str( miniexp_nth( 0, cur ) );
            const char* dst = miniexp_to_str( miniexp_nth( 1, cur ) );

            if (*dst != 0 && dst[0] == '#' )
            {
                int page = atoi(&dst[1]);

                if (!only_calculate)
                {
                    toc[toclen].level = offset - 1;
                    toc[toclen].page = page;
                    toc[toclen].position = page;
                    toc[toclen].text = strdup(name);
                }

                ++toclen;
            }

            if ((miniexp_length(cur) > 2))
            {
                readTocRecurse( cur, offset+1, only_calculate );
            }
        }
    }

}

static void toc_handler(long long page)
{
    if (page < 1) page = 1;
    if (page > npages) page = npages;
    cpage = page;
    offx = offy = 0;
    out_page(1);
}

static void BuildToc()
{
    miniexp_t outline;
    while ( ( outline = ddjvu_document_get_outline( doc) ) == miniexp_dummy)
        handle(1 );

    if ( miniexp_listp( outline ) && ( miniexp_length( outline ) > 0 ) && miniexp_symbolp( miniexp_nth( 0, outline ) ) &&
         ( !strcmp(miniexp_to_name( miniexp_nth( 0, outline ) ), "bookmarks" ) ))
    {
        readTocRecurse( outline, 1, 1 );

        toc = (tocentry *) malloc(sizeof(tocentry) * toclen);

        toclen = 0;

        readTocRecurse( outline, 1, 0 );

        ddjvu_miniexp_release( doc, outline );
    }

}

static void FreeToc()
{
    int i;
    for (i = 0; i < toclen; ++i)
    {
        free(toc[i].text);
    }

    if (toc != 0) free(toc);
}

void open_contents()
{
    if (toclen == 0)
    {
		Message(ICON_INFORMATION, APPCAPTION, "@No_contents", 2000);
    }
    else
    {
        OpenContents(toc, toclen, 1, toc_handler);
    }
}

void search_timer()
{
	if (stext == NULL || ! search_mode) return;

	if (spage < 1 || spage > npages) {
		HideHourglass();
		Message(ICON_INFORMATION, GetLangText("@Search"), GetLangText("@No_more_matches"), 2000);
		return;
	}

	miniexp_t r = miniexp_nil;
	while ((r = ddjvu_document_get_pagetext(doc, spage-1, "page"))==miniexp_dummy)
		handle(1);

	if (r != miniexp_nil && (r = miniexp_nth(5, r)) && miniexp_stringp(r)) {
		const char *s = miniexp_to_str(r);
		//fprintf(stderr, "\n[%s]\n", s);
		if (utfcasestr((char *)s, stext) != NULL) {
			cpage = spage;
			out_page(1);
			return;
		}
	}

	spage += sdir;
	SetHardTimer("SEARCH", search_timer, 1);

}		

static void do_search(char *text, int frompage, int dir) {

	search_mode = 1;
	stext = strdup(text);
	spage = frompage;
	sdir = dir;
	ShowHourglass();
	search_timer();

}

static void stop_search() {

	ClearTimer(search_timer);
	free(stext);
	stext = NULL;
	search_mode = 0;
	SetEventHandler(PBMainFrame::main_handler/*main_handler*/);

}

static int search_handler(int type, int par1, int par2) {

	static int x0, y0;

	if (type == EVT_SHOW) {
		//out_page(0);
	}

	if (type == EVT_POINTERDOWN) {
		x0 = par1;
		y0 = par2;
	}

	if (type == EVT_POINTERUP) {
		if (par1-x0 > EPSX) {
			prev_page();
			return 1;
		}
		if (x0-par1 > EPSX) {
			next_page();
			return 1;
		}
		if (par1 > ScreenWidth()-searchbm.width && par2 > ScreenHeight()-PanelHeight()-searchbm.height) {
			if (par1 < ScreenWidth()-searchbm.width/2) {
				do_search(stext, spage-1, -1);
			} else {
				do_search(stext, spage+1, +1);
			}
			return 1;
		} else {
			stop_search();
		}

	}

	if (type == EVT_KEYPRESS) {

		if (par1 == KEY_OK || par1 == KEY_BACK) {
			stop_search();
		}
		if (par1 == KEY_LEFT) {
			do_search(stext, spage-1, -1);
		}
		if (par1 == KEY_RIGHT) {
			do_search(stext, spage+1, +1);
		}

	}

	return 0;

}

static void search_enter(char *text) {
	if (text == NULL || text[0] == 0) return;
	SetEventHandler(search_handler);
	do_search(text, cpage, +1);
}

void start_search() { OpenKeyboard(GetLangText("@Search"), kbdbuffer, 30, 0, search_enter); }

int add_word(miniexp_t word, int page_height)
{
    int size = miniexp_length( word );
    if ( size >= 6 )
    {
        int xmin = miniexp_to_int( miniexp_nth( 1, word ) );
        int ymin = miniexp_to_int( miniexp_nth( 2, word ) );
        int xmax = miniexp_to_int( miniexp_nth( 3, word ) );
        int ymax = miniexp_to_int( miniexp_nth( 4, word ) );

        xmin = xmin * pagescalex - pageoffsetx+offx+scrx;
        ymin = (page_height - ymin) * pagescaley + pageoffsety-offy+scry;
        xmax = xmax * pagescalex - pageoffsetx+offx+scrx;
        ymax = (page_height - ymax) * pagescaley + pageoffsety-offy+scry;
		if (xmax < xmin) { int tmp = xmax; xmax = xmin; xmin = tmp; }
		if (ymax < ymin) { int tmp = ymax; ymax = ymin; ymin = tmp; }
		ymin -= 2;
		ymax += 2;

        if (ymax > offy && ymin < ScreenHeight() + offy - thh &&  xmin > offx && xmax < ScreenWidth() + offx)
        {
			if (diclen+2 >= dicsize) {
				dicsize += 50;
				diclist = (iv_wlist *) realloc(diclist, dicsize * sizeof(iv_wlist));
			}
            diclist[diclen].word = strdup(miniexp_to_str( miniexp_nth( 5, word ) ));
            diclist[diclen].x1 = xmin-offx;
            diclist[diclen].y1 = ymin-offy;
            diclist[diclen].x2 = xmax-offx;
            diclist[diclen].y2 = ymax-offy;
			//fprintf(stderr, "[%i %i %i %i %s]\n", diclist[diclen].x1, diclist[diclen].y1, diclist[diclen].x2-diclist[diclen].x1, diclist[diclen].y2-diclist[diclen].y1, diclist[diclen].word);
            ++diclen;
            diclist[diclen].word = NULL;
        }
    }

    return 1;
}

static void make_wordlist_recursive(miniexp_t exp, int height) {

	int i;

	if (exp == NULL) return;
	int l = miniexp_length(exp);
	for (i=0; i<l; i++) {
		miniexp_t cur = miniexp_nth( i, exp );
		if (miniexp_listp(cur) && (miniexp_length(cur) > 0) && miniexp_symbolp(miniexp_nth(0, cur))) {
			const char *s = miniexp_to_name(miniexp_nth(0, cur));
			if (strcmp(s, "word") == 0) {
				add_word(cur, height);
			} else {
				make_wordlist_recursive(cur, height);
			}
		}
	}
}


void fill_words( int page )
{
    miniexp_t r;
    int i;

    if (diclist != NULL) {
		for (i=0; i<diclen; i++) free(diclist[i].word);
		free(diclist);
		diclist = NULL;
		diclen = dicsize = 0;
    }

    while ( ( r = ddjvu_document_get_pagetext( doc, page, 0 ) ) == miniexp_dummy )
        handle( 1 );

    if ( r == miniexp_nil )
        return;


	//    int height = d->m_pages.at( page )->height();

    ddjvu_pageinfo_t in;
    ddjvu_document_get_pageinfo(doc,page,&in);
    make_wordlist_recursive(r, in.height);

}

void show_hide_panel() {
	SetPanelType((PanelHeight() == 0) ? 1 : 0);
	panh = PanelHeight();
	out_page(1);
}

//void open_quickmenu() { OpenMenu3x3(m3x3, (const char **)strings3x3, menu_handler); }
void prev_page() { turn_page(-1); }
void next_page() { turn_page(+1); }
void jump_pr10() { jump_pages(-10); }
void jump_nx10() { jump_pages(+10); }
void stop_jump() { out_page(1); }
void open_pageselector()  { OpenPageSelector(page_selected); }
void first_page() { page_selected(1); }
void last_page() { page_selected(npages); }
void new_note() { CreateNote(FileName, book_title, cpage); }
void save_page_note() { CreateNoteFromPage(FileName, book_title, cpage); }
void open_notes() { OpenNotepad(NULL); }
void open_dictionary()
{
    if (scale > 50) fill_words(cpage-1);
    OpenDictionaryView(diclist, NULL);

}
void zoom_in() { do_zoom(+1); }
void zoom_out() { do_zoom(-1); }
void open_rotate() { OpenRotateBox(rotate_handler); }
void main_menu() { OpenMainMenu(); }
void exit_reader() { CloseApp(); }
void open_mp3() { OpenPlayer(); }
void mp3_pause() { TogglePlaying(); }
void volume_up() { int r = GetVolume(); SetVolume(r+3); }
void volume_down() { int r = GetVolume(); SetVolume(r-3); }

void on_close_zoomer(ZoomerParameters* params)
{
    if (scale != params->zoom || offset != params->offset)
    {
        scale = params->zoom;
        offset = params->offset;
        calc_optimal_zoom = params->optimal_zoom;
        out_page(1);
    }
}

void apply_scale_factor(int zoom_type, int new_scale)
{
	//if (scale != new_scale)
	{
		scale = new_scale;
		calc_optimal_zoom = zoom_type == ZoomTypeFitWidth ? 1 : 0;
		out_page(1);
	}
}

int get_zoom_param(int& zoom_type, int& zoom_param)
{
	if (calc_optimal_zoom)
	{
		zoom_type = ZoomTypeFitWidth;
		zoom_param = 100;
	}
	else if (scale <= 50)
	{
		zoom_type = ZoomTypePreview;
		zoom_param = scale;
	}
	else if (scale < 200)
	{
		zoom_type = ZoomTypeNormal;
		zoom_param = scale;
	}
	else if (scale >= 200)
	{
		zoom_type = ZoomTypeColumns;
		zoom_param = scale;
	}

	return scale;
}

void open_new_zoomer()
{
    ZoomerParameters params = {0};
	params.cpage = cpage;
    params.zoom = scale;
    params.doc = doc;
    params.page = last_decoded_page;
    params.orient = orient;
    params.optimal_zoom = calc_optimal_zoom;

//    if (QueryTouchpanel() == 0){
        ShowZoomer(&params, on_close_zoomer);
//    }
//	else {
//        TouchZoom::open_zoomer();
//    }
}

void handle_navikey(int key) {

	int pageinc=1;

	switch (scale) {
	case 50: pageinc = is_portrait() ? 4 : 2; break;
	case 33: pageinc = is_portrait() ? 9 : 6; break;
	}

	switch (key) {

	case KEY_LEFT:
		if (scale >= 200) {
			find_off_x(-1);
			out_page(1);
		} else {
			turn_page(-pageinc);
		}
		break;

		case KEY_RIGHT:
		if (scale >= 200) {
			find_off_x(+1);
			out_page(1);
		} else {
			turn_page(pageinc);
		}
		break;

		case PB_KEY_UP:
		if (scale<=50) {
			turn_page(-1);
		} else {
			find_off(-1);
			out_page(1);
		}
		break;

		case PB_KEY_DOWN:
		if (scale<=50) {
			turn_page(1);
		} else {
			find_off(1);
			out_page(1);
		}
		break;

		case KEY_PREV:
		case KEY_PREV2:
		if (scale >= 200 || orient == 1 || orient == 2) {
			find_off(-1);
			out_page(1);
		} else {
			turn_page(-pageinc);
		}
		break;

		case KEY_NEXT:
		case KEY_NEXT2:
		if (scale >= 200 || orient == 1 || orient == 2) {
			find_off(+1);
			out_page(1);
		} else {
			turn_page(pageinc);
		}
		break;

	}

}

static void move_back() { handle_navikey(KEY_PREV); }

static void move_forward() { handle_navikey(KEY_NEXT); }
/*
static int act_on_press(char *a0, char *a1) {

	if (a0 == NULL || a1 == NULL || strcmp(a1, "@KA_none") == 0) return 1;
	if (strcmp(a0, "@KA_prev") == 0 || strcmp(a0, "@KA_next") == 0) {
		if (strcmp(a1, "@KA_pr10") == 0) return 1;
		if (strcmp(a1, "@KA_nx10") == 0) return 1;
	}
	return 0;

}

static const struct {
	const char *action;
	void (*f1)();
	void (*f2)();
	void (*f3)();
} KA[] = {
{ "@KA_menu", open_quickmenu, NULL, NULL },
{ "@KA_prev", move_back, move_back, NULL },
{ "@KA_next", move_forward, move_forward, NULL },
{ "@KA_pgup", prev_page, prev_page, NULL },
{ "@KA_pgdn", next_page, next_page, NULL },
{ "@KA_pr10", jump_pr10, jump_pr10, stop_jump },
{ "@KA_nx10", jump_nx10, jump_nx10, stop_jump },
{ "@KA_goto", open_pageselector, NULL, NULL },
{ "@KA_frst", first_page, NULL, NULL },
{ "@KA_last", last_page, NULL, NULL },
//{ "@KA_prse", prev_section, NULL, NULL },
//{ "@KA_nxse", next_section, NULL, NULL },
{ "@KA_obmk", open_bookmarks, NULL, NULL },
{ "@KA_nbmk", new_bookmark, NULL, NULL },
{ "@KA_nnot", new_note, NULL, NULL },
{ "@KA_savp", save_page_note, NULL, NULL },
{ "@KA_onot", open_notes, NULL, NULL },
//{ "@KA_olnk", open_links, NULL, NULL },
//{ "@KA_blnk", back_link, NULL, NULL },
{ "@KA_cnts", open_contents, NULL, NULL },
{ "@KA_srch", start_search, NULL, NULL },
{ "@KA_dict", open_dictionary, NULL, NULL },
{ "@KA_zoom", open_new_zoomer, NULL, NULL },
{ "@KA_cnts", open_contents, NULL, NULL },
{ "@KA_zmin", zoom_in, NULL, NULL },
{ "@KA_zout", zoom_out, NULL, NULL },
{ "@KA_hidp", show_hide_panel, NULL, NULL },
{ "@KA_rtte", open_rotate, NULL, NULL },
{ "@KA_mmnu", main_menu, NULL, NULL },
{ "@KA_exit", exit_reader, NULL, NULL },
{ "@KA_mp3o", open_mp3, NULL, NULL },
{ "@KA_mp3p", mp3_pause, NULL, NULL },
{ "@KA_volp", volume_up, NULL, NULL },
{ "@KA_volm", volume_down, NULL, NULL },
{ NULL, NULL, NULL, NULL }
};
*/
//void menu_handler(int pos) {
//
//	char buf[32], *act;
//	int i;
//
//	if (pos < 0) return;
//	sprintf(buf, "qmenu.pdfviewer.%i.action", pos);
//	act = GetThemeString(buf, (char *)def_menuaction[pos]);
//
//	for (i=0; KA[i].action != NULL; i++) {
//		if (strcmp(act, KA[i].action) != 0) continue;
//		if (KA[i].f1 != NULL) (KA[i].f1)();
//		if (KA[i].f3 != NULL) (KA[i].f3)();
//		break;
//	}
//
//}

static void find_pointer_off(int dx, int dy)
{
	int sw=ScreenWidth();
	int sh=ScreenHeight();

	offy+=dy;
	offx+=dx;

	if (offx<0)
		offx=0;
	if (offx>cpagew-sw)
		offx=cpagew-sw;
	if (offy<0)
		offy=0;
	if (offy>cpageh-(sh-panh))
		offy=cpageh-(sh-panh);
}
/*
static int pointer_handler(int type, int par1, int par2){

	int i;

	if (type == EVT_POINTERDOWN){
		px0=par1;
		py0=par2;
		AddBkMark=false;
		Move=false;
		return 1;
	} else if (type == EVT_POINTERUP){
		if (AddBkMark) return 1;
		if (!Move){
			if ((par1>ScreenWidth()/2-MENUMARGIN && par1<ScreenWidth()/2+MENUMARGIN)
				&& (par2>ScreenHeight()/2-MENUMARGIN && par2<ScreenHeight()/2+MENUMARGIN)){
				for (i=0; KA[i].action != NULL; i++) {
					if (strcmp("@KA_menu", KA[i].action) != 0) continue;
					if (KA[i].f1 != NULL) (KA[i].f1)();
					break;
				}
				return 1;
			}
			if (par1>ScreenWidth()-BOOKMARKMARGIN && par2<BOOKMARKMARGIN){
				for (i=0; KA[i].action != NULL; i++) {
					if (strcmp("@KA_obmk", KA[i].action) != 0) continue;
					if (KA[i].f1 != NULL) (KA[i].f1)();
					break;
				}
				return 1;
			}
		} else {
			if (py0>ScreenHeight()-SCROLLPAGEMARGINHEIGHT && par2>ScreenHeight()-SCROLLPAGEMARGINHEIGHT && par1-px0>EPSX) {
				for (i=0; KA[i].action != NULL; i++) {
					if (strcmp("@KA_prev", KA[i].action) != 0) continue;
					if (KA[i].f1 != NULL) (KA[i].f1)();
					break;
				}
				return 1;					
			} else if (py0>ScreenHeight()-SCROLLPAGEMARGINHEIGHT && par2>ScreenHeight()-SCROLLPAGEMARGINHEIGHT && px0-par1>EPSX){
				for (i=0; KA[i].action != NULL; i++) {
					if (strcmp("@KA_next", KA[i].action) != 0) continue;
					if (KA[i].f1 != NULL) (KA[i].f1)();
					break;
				}
				return 1;
			} else if ((abs(px0-par1)>EPSX || abs(py0-par2)>EPSY)&& scale>100){
				find_pointer_off(px0-par1, py0-par2);
				out_page(1);			
				return 1;
			}
		}
		return 1;
	} else if (type == EVT_POINTERMOVE){
		if (abs(par1-px0) > 10 || abs(par2-py0) > 10) Move=true;
		return 1;
	} else if (type == EVT_POINTERHOLD){

		return 1;
	} else if (type == EVT_POINTERLONG){
		if (par1>ScreenWidth()-BOOKMARKMARGIN && par2<BOOKMARKMARGIN){
			AddBkMark = true;
			new_bookmark();
		}
		return 1;
	}
	return 0;
}
*/
/*
static int key_handler(int type, int par1, int par2) {

	char *act0, *act1, *act=NULL;
	int i;

	if (type == EVT_KEYPRESS) kill_bgpainter();

	act0 = keyact0[par1];
	act1 = keyact1[par1];

	if (par1 >= 0x20) {

		// aplhanumeric keys
		return 0;

	} else if (par1 == KEY_BACK) {

		CloseApp();
		return 0;

	} else if (par1 == KEY_POWER)  {

		return 0;

	} else if ((par1==KEY_UP || par1== KEY_DOWN || par1==KEY_LEFT || par1==KEY_RIGHT)

		&& (type == EVT_KEYPRESS || (type == EVT_KEYRELEASE && par2 == 0))
		) {
		if (act_on_press(act0, act1)) {
			if (type == EVT_KEYPRESS) handle_navikey(par1);
		} else {
			if (type == EVT_KEYRELEASE) handle_navikey(par1);
		}
		return 0;

	} else {

		if (type == EVT_KEYPRESS && act_on_press(act0, act1)) {
			act = act0;
		} else if (type == EVT_KEYRELEASE && par2 == 0 && !act_on_press(act0, act1)) {
			act = act0;
		} else if (type == EVT_KEYREPEAT) {
			act = act1;
		} else if (type == EVT_KEYRELEASE && par2 > 0) {
			act = act1;
			par2 = -1;
		}
		if (act == NULL) return 1;

		for (i=0; KA[i].action != NULL; i++) {
			if (strcmp(act, KA[i].action) != 0) continue;
			if (par2 == -1) {
				if (KA[i].f3 != NULL) (KA[i].f3)();
			} else if (par2 <= 1) {
				if (KA[i].f1 != NULL) (KA[i].f1)();
				if (act == act0 && KA[i].f3 != NULL) (KA[i].f3)();
			} else {
				if (KA[i].f2 != NULL) (KA[i].f2)();
			}
			break;
		}
		return 1;

	}

}
*/
/*
int main_handler(int type, int par1, int par2) {

	if (type == EVT_INIT)
		SetOrientation(orient);

	if (type == EVT_EXIT)
		save_settings();

	if (type == EVT_SHOW) {
		out_page(1);
		if (! ready_sent) BookReady(FileName);
		ready_sent = 1;
	}

	if (type == EVT_KEYPRESS || type == EVT_KEYREPEAT || type == EVT_KEYRELEASE) {
		return key_handler(type, par1, par2);
	}

	if (type == EVT_POINTERDOWN || type == EVT_POINTERUP || type == EVT_POINTERMOVE || type == EVT_POINTERHOLD || type == EVT_POINTERLONG){
		return pointer_handler(type, par1, par2);
	}

	if (type == EVT_ORIENTATION) {
		orient = par1;
		SetOrientation(orient);
		out_page(1);
	}

	if (type == EVT_SNAPSHOT) {
		fprintf(stderr, "EVT_SNAPSHOT\n");
		DrawPanel((ibitmap *)PANELICON_LOAD, "@snapshot_info", NULL, -1);
		PageSnapshot();
	}

	if (type == EVT_PANEL_TEXT) {
		if (par1 < 5000) {
			open_pageselector();
		} else {
			open_new_zoomer();
		}
		return 0;
	}

	if (type == EVT_PANEL_PROGRESS) {
		open_contents();
		return 0;
	}

	if (type == EVT_PREVPAGE) {
		prev_page();
		return 0;
	}

	if (type == EVT_NEXTPAGE) {
		next_page();
		return 0;
	}

	if (type == EVT_OPENDIC) {
		open_dictionary();
		return 0;
	}

	return 0;

}
*/
void main_show()
{
	out_page(1);
	if (! ready_sent) BookReady(FileName);
	ready_sent = 1;
}

void save_settings()
{
	if (docstate.magic != 0x9751)
		return;
	fprintf(stderr, "djviewer: saving settings\n");
	docstate.page = cpage;
	docstate.offx = offx;
	docstate.offy = offy;

	docstate.scale = scale | (abs(offset) << 16);
	docstate.orient = orient;

	if (offset > 0)
	{
		docstate.scale |= 1 << 31;
	}

	if (calc_optimal_zoom)
	{
		docstate.scale |= 1 << 30;
	}

	FILE *f = iv_fopen(DataFile, "wb");
	if (f != NULL) {
		iv_fwrite(&docstate, 1, sizeof(tdocstate), f);
		iv_fclose(f);
	}
	fprintf(stderr, "djviewer: saving settings done\n");
}

#define die(x...) { \
fprintf(stderr, x); \
		fprintf(stderr, "\n"); \
		Message(ICON_WARNING, APPCAPTION, "@Cant_open_file", 2000); \
		exit(1); \
	}

int main(int argc, char **argv)
{
	fprintf(stderr, "Starting DjVu reader, date of compilation: %s %s\n", __DATE__, __TIME__);

	//char buf[1024];
	//int i;
	bookinfo *bi;
	FILE *f = NULL;

	OpenScreen();

	bmk_flag = GetResource("bmk_flag", NULL);

//	m3x3 = GetResource("pdfviewer_menu", NULL);
//	if (m3x3 == NULL) m3x3 = NewBitmap(128, 128);
//
//	for (i=0; i<9; i++) {
//		sprintf(buf, "qmenu.pdfviewer.%i.text", i);
//		strings3x3[i] = GetThemeString(buf, (char *)def_menutext[i]);
//	}
//
//	GetKeyMapping(keyact0, keyact1);

	if (argc < 2)
		die("Missing filename");

	FileName=argv[1];
	bi = GetBookInfo(FileName);
	if (bi->title) book_title = strdup(bi->title);

	/* Create context and document */
	if (! (ctx = ddjvu_context_create(argv[0])))
		die("Cannot create djvu context.");
	if (! (doc = ddjvu_document_create_by_filename(ctx, FileName, TRUE)))
		die("Cannot open djvu document '%s'.", FileName);
	while (! ddjvu_document_decoding_done(doc))
		handle(TRUE);

	npages = ddjvu_document_get_pagenum(doc);

	DataFile = GetAssociatedFile(FileName, 0);
	f = fopen(DataFile, "rb");

	if (f == NULL || fread(&docstate, 1, sizeof(tdocstate), f) != sizeof(tdocstate) || docstate.magic != 0x9751) {
		docstate.magic = 0x9751;
		docstate.page = 1;
		docstate.offx = 0;
		docstate.offy = 0;
		docstate.scale = 100;
		docstate.orient = 0;
		docstate.nbmk = 0;
	}
	if (f != NULL) fclose(f);

	cpage = docstate.page;
	offx = docstate.offx;
	offy = docstate.offy;
	scale = docstate.scale & 0xFFFF;
	offset = (docstate.scale >> 16) & 0x3FFF;

	if (!(docstate.scale >> 31)) offset =-offset;
	if (((docstate.scale >> 30) & 0x1)) calc_optimal_zoom = 1;

	gcfg = GetGlobalConfig();
	//regStatus = isRegistered();
	ko = ReadInt(gcfg, "keeporient", 0);
	if (GetGlobalOrientation() == -1 || ko == 0) {
		orient = GetOrientation();
	} else {
		orient = docstate.orient & 0x7f;
		SetOrientation(orient);
	}
	panh = PanelHeight();

	if (argc >= 3 && argv[2][0] == '=') cpage = atoi(argv[2]+1);
	if (cpage < 1) cpage = 1;
	if (cpage > npages) cpage = npages;

	BuildToc();

	//InkViewMain(main_handler);
	PBMainFrame fbFrame;
	fbFrame.Create(NULL, 0,0,1,1, PBWS_VISIBLE, 0);
	fbFrame.Run();


	FreeToc();

	free(book_title);

    if (diclist != NULL)
    {
		int i;
		for (i=0; i<diclen; i++) free(diclist[i].word);
		free(diclist);
    }

	if (last_decoded_cpage != 0) ddjvu_page_release(last_decoded_page);

	return 0;
}

#ifdef USESYNOPSIS
// Denis // -> Synopsis

/*
static long long pack_position(int para, int word, int letter)
{
	return ((long long)para << 40) | ((long long)word << 16) | ((long long)letter);
}

static void unpack_position(long long pos, int *para, int *word, int *letter)
{
	*para = (pos >> 40) & 0xffffff;
	*word = (pos >> 16) & 0xffffff;
	*letter = pos & 0xffff;
}

static long long page_to_position(int page)
{
	return page;
//	if (page < 1) page = 1;
//	if (page > npages) page = npages;
//	return pagelist[page-1];
}
*/
static int position_to_page(long long position)
{
	return position;
//	int i;
//
//	for (i=1; i<=npages; i++)
//	{
//		if (position < pagelist[i])
//		{
//		 return i;
//		}
//	}
//	return npages;
}

static void synopsis_toc_selected(char *pos)
{
	long long position = 0;
	TSynopsisItem::PositionToLong(pos, &position);
	turn_page(position - cpage);
	SetEventHandler(PBMainFrame::main_handler);
}

int SynopsisNote::GetPage()
{
	return position_to_page(this->GetLongPosition());
}

void SynopsisNote::GetNoteBookmarks(int y1, int y2, char *sbookmark, char *ebookmark)
{
	char from[30];
	char to[30];

	TSynopsisItem::PositionToChar(cpage, from);
	TSynopsisItem::PositionToChar(cpage+1, to);

	strcpy(sbookmark, from);
	strcpy(ebookmark, to);
}

int SynopsisBookmark::GetPage()
{
	return position_to_page(this->GetLongPosition());
}

int SynopsisContent::GetPage()
{
	return position_to_page(this->GetLongPosition());
}

static void PrepareSynTOC(int opentoc)
{
	if (SynTOC.GetHeader() == NULL)
	{
		SynTOC.SetHeaderName(book_title);
		SynTOC.SetFilePath(FileName);
		SynTOC.SetHeaderName(book_title);
		if (strrchr(FileName, '/') != NULL)
		{
			SynTOC.SetFileName(strrchr(FileName, '/') + 1);
		}
		else
		{
			SynTOC.SetFileName(FileName);
		}
		SynTOC.LoadTOC();
	}

	if (SynTOC.GetHeader() == NULL)
	{
		if (toclen == 0 && opentoc == 1)
		{
			Message(ICON_INFORMATION, APPCAPTION, "@No_contents", 2000);
			return;
		}

		for (int i=0; i<toclen; i++)
		{
			Content = new SynopsisContent(toc[i].level, toc[i].position, toc[i].text);
			SynTOC.AddTOCItem(Content);
		}
	}
}

void new_synopsis_note()
{
	PrepareSynTOC(0);

	// Synopsis words
	SynWordList* synWordList = new SynWordList[2];
	memset(synWordList, 0, sizeof(SynWordList) * 2);

	// добавляем две пустышки с координатами верхнего левого и нижнего правого углов
	synWordList[0].word = strdup("");
	synWordList[0].x1 = 0;
	synWordList[0].y1 = 0;
	synWordList[0].x2 = 0;
	synWordList[0].y2 = 0;

	synWordList[1].word = strdup("");
	synWordList[1].x1 = ScreenWidth();
	synWordList[1].y1 = ScreenHeight()-PanelHeight();
	synWordList[1].x2 = ScreenWidth();
	synWordList[1].y2 = ScreenHeight()-PanelHeight();


	List.Clear();
	List.Add(synWordList, 2, 1);

	delete[] synWordList;


	ibitmap *bm1=NULL, *bm2=NULL;
	bm1 = BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight());

//	if (cpage < npages/*one_page_forward(-1)*/)
//	{
//		move_forward();
//		List.Add(acwordlist, acwlistcount, 2);
//		bm2 = BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight());
//		move_back();/*one_page_back(-1);*/
//	}

	SynopsisNote *pNote = new SynopsisNote();
	pNote->Create((TSynopsisTOC*)&SynTOC, &List, bm1, bm2, false);
}


void new_synopsis_bookmark()
{
	PrepareSynTOC(0);
	List.Clear();
	//List.Add(acwordlist, acwlistcount, 1);
	Bookmark = new SynopsisBookmark();
	Bookmark->Create((TSynopsisTOC*)&SynTOC, &List, cpage/*(((long long)acwordlist[0].pnum << 40) | ((long long)acwordlist[0].wnum << 16))*/);
	SendEvent(GetEventHandler(), EVT_SHOW, 0, 0);
}

void open_synopsis_contents()
{
	char pos[25];
	PrepareSynTOC(1);
//	restore_current_position();
	SynTOC.SetBackTOCHandler(synopsis_toc_selected);
	TSynopsisItem::PositionToChar(cpage/*get_end_position()*/, pos);

	SynopsisContent Location(0, pos, NULL);
	SynTOC.Open(&Location);
}

// Denis // <-
#else

static void bmk_added(int page) {

	char buf[256];

	sprintf(buf,"%s %i", GetLangText("@Add_Bmk_P"), page);
	draw_bmk_flag(1);

}

static void bmk_handler(int action, int page, long long pos) {

	switch (action) {
	case BMK_ADDED: bmk_added(page); break;
	case BMK_REMOVED: bmkrem = 1; break;
	case BMK_SELECTED: page_selected(page); break;
	case BMK_CLOSED: if (bmkrem) out_page(0); break;
	}

}

void open_bookmarks() {

	int i;

	bmkrem = 0;
	for (i=0; i<docstate.nbmk; i++) bmkpos[i] = docstate.bmk[i];
	OpenBookmarks(cpage, cpage, docstate.bmk, bmkpos, &docstate.nbmk, 30, bmk_handler);

}

void new_bookmark() {

	int i;

	bmkrem = 0;
	for (i=0; i<docstate.nbmk; i++) bmkpos[i] = docstate.bmk[i];
	SwitchBookmark(cpage, cpage, docstate.bmk, bmkpos, &docstate.nbmk, 30, bmk_handler);
	if (bmkrem) out_page(0);

}

#endif
