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

#ifndef PDFVIEWER_H
#define PDFVIEWER_H

#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <sys/wait.h>
#include <inkview.h>
#include <inkinternal.h>

#include "goo/gmem.h"
#include "goo/GooString.h"
#include "goo/GooList.h"
#include "goo/GooHash.h"
#include "goo/FixedPoint.h"
#include "GlobalParams.h"
#include "Object.h"
#include "Stream.h"
#include "GfxState.h"
#include "GfxFont.h"
#include "PDFDoc.h"
#include "Outline.h"
#include "Link.h"
#include "OutputDev.h"
#include "TextOutputDev.h"
#include "SplashOutputDev.h"
#include "splash/SplashBitmap.h"
#include "splash/Splash.h"

#include "MySplashOutputDev.h"
#include "SearchOutputDev.h"

#define USE4 1

#define MAXRESULTS 200
#define CSCALEADJUST 95

#define CACHEDIR "/var/cache"

#define EPSX 50
#define EPSY 50
#define MENUMARGIN 150
#define BOOKMARKMARGIN 100
#define SCROLLPAGEMARGINHEIGHT 150

typedef struct tdocstate_s
{

	int magic;
	int page;
	int offx;
	int offy;
	short scale;
	short rscale;
	int orient;
	int subpage;
	int nbmk;
	int bmk[30];

} tdocstate;

struct sresult
{
	int x;
	int y;
	int w;
	int h;
};

extern "C" const ibitmap zoombm;
extern "C" const ibitmap searchbm;
extern "C" const ibitmap hgicon;

extern const int SC_PREVIEW[], SC_NORMAL[], SC_COLUMNS[], SC_REFLOW[], SC_DIRECT[];

extern int cpage, npages, subpage, nsubpages;
extern int scale, rscale;
extern int reflow_mode;
extern int orient;
extern int clock_left;
extern int offx, offy;
extern int scrx, scry;
extern int flowpage, flowscale, flowwidth, flowheight, savereflow;
extern int thx, thy, thw, thh, thix, thiy, thiw, thih, panelh;
extern int search_mode;
extern int zoom_mode;
extern struct sresult results[MAXRESULTS];
extern int nresults;

extern char* book_title;
extern PDFDoc* doc;
extern MySplashOutputDev* splashOut;
extern TextOutputDev* textout;
extern SearchOutputDev* searchout;
extern tdocstate docstate;

extern ibitmap* m3x3;
extern ibitmap* bmk_flag;

static inline int is_portrait()
{
	return (orient == 0 || orient == 3);
}

void getpagesize(int n, int* w, int* h, double* res, int* marginx, int* marginy);
void find_off(int step);
void find_off_x(int step);
void find_off_xy(int xstep, int ystep);
int get_fit_scale();
void draw_wait_thumbnail();
void display_slice(int pagenum, int sc, double res, GBool withreflow, int x, int y, int  w, int h);
void get_bitmap_data(unsigned char** data, int* w, int* h, int* row);
void out_page(int full);
void draw_bmk_flag(int update);
void open_dictionary();
void open_mini_zoomer();
void bg_monitor();
void kill_bgpainter();
void update_value(int* val, int d, const int* variants);
void area_selector();

#endif

