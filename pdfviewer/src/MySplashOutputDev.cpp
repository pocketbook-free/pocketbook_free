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

//========================================================================
//
// OutputDev.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#include "pdfviewer.h"

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif


//------------------------------------------------------------------------
// MySplashOutputDev
//------------------------------------------------------------------------

static FixedPoint V70 = (FixedPoint)70.0;

void MySplashOutputDev::setup(GBool usereflow, int sp,
							  int dispx, int dispy, int dispw, int disph,
							  FixedPoint x, FixedPoint y, FixedPoint w, FixedPoint h,
							  FixedPoint res)
{

	int xx, yy, ww, hh;

	reflow = usereflow;
	subpage = sp;
	ares =  res;
	if (! reflow) return;

	if (subpage == -1)
	{
		pagex = x;
		pagey = y;
		pagew = w;
		pageh = h;
		dcx = (FixedPoint)dispx;
		dcy = (FixedPoint)dispy;
		dcw = (FixedPoint)dispw;
		dch = (FixedPoint)disph;

		xx = (int)(x.getRaw() >> 8);
		yy = (int)(y.getRaw() >> 8);
		ww = (int)(w.getRaw() >> 8);
		hh = (int)(h.getRaw() >> 8);
		//fprintf(stderr, "---[%i,%i,%i,%i:%i,%i]---\n", (int)x,(int)y,(int)w,(int)h,(int)dcw,(int)dch);
		iv_reflow_start(xx, yy, ww, hh, (int)res);
	}
	else
	{
		iv_reflow_render(subpage);
	}

}

void MySplashOutputDev::drawChar(GfxState* state, FixedPoint x, FixedPoint y,
								 FixedPoint dx, FixedPoint dy,
								 FixedPoint originX, FixedPoint originY,
								 CharCode code, int nBytes, Unicode* u, int uLen)
{

	static Unicode lastu = 0xffffffff;
	static CharCode lastcode = 0xffffffff;

	//if (u) ddprintf("%c", (*u >= ' ' && *u <= 0x7f) ? *u : '.');

	if (! reflow)
	{
		SplashOutputDev::drawChar(state, x, y, dx, dy, originX, originY, code, nBytes, u, uLen);
		return;
	}

	checkFontUpdate(state);
	FixedPoint fs = getCurrentFont()->getSize();
	if (fs >= 51)
	{
		state->setFontSize((state->getFontSize() / fs) * 50);
		updateFont(state);
		checkFontUpdate(state);
	}

	if (subpage == -1)
	{
		FixedPoint csize = ((getCurrentFont()->getSize()) * V70) / ares;
		int c = (u != NULL) ? *u : code;
		if ((u != NULL) && (lastu == *u) && (code != lastcode)) c = '-';
		int xx = (int)(x.getRaw() >> 8);
		int yy = (int)(y.getRaw() >> 8);
		int ww = (int)(dx.getRaw() >> 8);
		int hh = (int)(csize.getRaw() >> 8);
		iv_reflow_addchar(c, xx, yy, ww, hh);
		lastu = (u == NULL) ? 0 : *u;
		lastcode = code;
	}
	else
	{
		int cx, cy;
		if (! iv_reflow_getchar(&cx, &cy)) return;
		FixedPoint cxx = FixedPoint::make(cx << 8);
		FixedPoint cyy = FixedPoint::make(cy << 8);
		SplashOutputDev::drawChar(state, cxx, cyy,
								  dx, dy, originX, originY, code, nBytes, u, uLen);
	}

}

void MySplashOutputDev::add_image(FixedPoint* m)
{

	int xx = (int)(((m[4] / dcw) * pagew).getRaw() >> 8);
	int yy = (int)((pagey - (m[5] / dch) * pageh).getRaw() >> 8);
	int ww = (int)(((m[0] / dcw) * pagew).getRaw() >> 8);
	int hh = (int)((((-m[3]) / dch) * pageh).getRaw() >> 8);

	if (ww < 0)
	{
		ww = -ww;
		xx -= ww;
	}
	if (hh < 0)
	{
		hh = -hh;
		yy -= hh;
	}

	//fprintf(stderr, "(A:%i,%i,%i,%i)\n", (int)m[4],(int)m[5],(int)m[0],(int)m[3]);

	iv_reflow_addimage(xx, yy, ww, hh, 0);

}

int MySplashOutputDev::get_image(FixedPoint* m)
{

	int x, y, sc;

	if (! iv_reflow_getimage(&x, &y, &sc)) return 0;

	m[4] = (FixedPoint::make(x << 8) / pagew) * dcw;
	m[5] = ((pagey - FixedPoint::make(y << 8)) / pageh) * dch;

	if (sc != 1000)
	{
		m[0] = (m[0] / (FixedPoint)1000) * (FixedPoint)sc;
		m[3] = (m[3] / (FixedPoint)1000) * (FixedPoint)sc;
	}

	if (m[0] < 0) m[4] -= m[0];
	if (m[3] > 0) m[5] -= m[3];

	m[1] = m[2] = 0;

	//fprintf(stderr, "(R:%i,%i,%i=%i,%i,%i,%i)\n", x>>8,y>>8,sc,(int)m[4],(int)m[5],(int)m[0],(int)m[3]);

	return 1;

}

static void eat(Stream* str, int n)
{

	int i;

	str->reset();
	for (i = 0; i < n; ++i)
		str->getChar();
	str->close();

}

void MySplashOutputDev::drawImageMask(GfxState* state, Object* ref, Stream* str,
									  int width, int height, GBool invert,
									  GBool inlineImg)
{

	if (! reflow)
	{
		SplashOutputDev::drawImageMask(state, ref, str, width, height, invert, inlineImg);
		return;
	}

	FixedPoint* m = state->getCTM();
	int n = height * ((width + 7) / 8);
	if (subpage == -1)
	{
		//ddprintf("IM(%i,%i,%i,%i,%i,%i)", (int)m[0],(int)m[1],(int)m[2],(int)m[3],(int)m[4],(int)m[5]);
		add_image(m);
		if (inlineImg) eat(str, n);
	}
	else
	{
		if (get_image(m))
		{
			SplashOutputDev::drawImageMask(state, ref, str, width, height, invert, inlineImg);
		}
		else
		{
			if (inlineImg) eat(str, n);
		}
	}

}

void MySplashOutputDev::drawImage(GfxState* state, Object* ref, Stream* str,
								  int width, int height, GfxImageColorMap* colorMap,
								  int* maskColors, GBool inlineImg)
{

	if (! reflow)
	{
		SplashOutputDev::drawImage(state, ref, str, width, height, colorMap, maskColors, inlineImg);
		return;
	}

	FixedPoint* m = state->getCTM();
	int n = height * ((width * colorMap->getNumPixelComps() * colorMap->getBits() + 7) / 8);
	if (subpage == -1)
	{
		//ddprintf("I(%i,%i,%i,%i,%i,%i)", (int)m[0],(int)m[1],(int)m[2],(int)m[3],(int)m[4],(int)m[5]);
		add_image(m);
		if (inlineImg) eat(str, n);
	}
	else
	{
		if (get_image(m))
		{
			SplashOutputDev::drawImage(state, ref, str, width, height, colorMap, maskColors, inlineImg);
		}
		else
		{
			if (inlineImg) eat(str, n);
		}
	}

}

void MySplashOutputDev::drawMaskedImage(GfxState* state, Object* ref, Stream* str,
										int width, int height,
										GfxImageColorMap* colorMap,
										Stream* maskStr, int maskWidth, int maskHeight,
										GBool maskInvert)
{


	if (! reflow)
	{
		SplashOutputDev::drawMaskedImage(state, ref, str, width, height, colorMap, maskStr, maskWidth, maskHeight, maskInvert);
		return;
	}

	FixedPoint* m = state->getCTM();
	if (subpage == -1)
	{
		//ddprintf("MI(%i,%i,%i,%i,%i,%i)", (int)m[0],(int)m[1],(int)m[2],(int)m[3],(int)m[4],(int)m[5]);
		add_image(m);
	}
	else
	{
		if (get_image(m))
		{
			SplashOutputDev::drawMaskedImage(state, ref, str, width, height, colorMap, maskStr, maskWidth, maskHeight, maskInvert);
		}
	}

}

void MySplashOutputDev::drawSoftMaskedImage(GfxState* state, Object* ref, Stream* str,
	int width, int height,
	GfxImageColorMap* colorMap,
	Stream* maskStr,
	int maskWidth, int maskHeight,
	GfxImageColorMap* maskColorMap)
{

	if (! reflow)
	{
		SplashOutputDev::drawSoftMaskedImage(state, ref, str, width, height, colorMap, maskStr, maskWidth, maskHeight, maskColorMap);
		return;
	}

	FixedPoint* m = state->getCTM();
	if (subpage == -1)
	{
		//ddprintf("SMI(%i,%i,%i,%i,%i,%i)", (int)m[0],(int)m[1],(int)m[2],(int)m[3],(int)m[4],(int)m[5]);
		add_image(m);
	}
	else
	{
		if (get_image(m))
		{
			SplashOutputDev::drawSoftMaskedImage(state, ref, str, width, height, colorMap, maskStr, maskWidth, maskHeight, maskColorMap);
		}
	}

}
void MySplashOutputDev::getWordList(iv_wlist** word_list, int* wlist_len, int spnum)
{

	iv_wlist* wl;
	int i, x, y, w, h, csp, nwords, cn;
	FixedPoint fx, fy, fw, fh;
	char* s;

	nwords = iv_reflow_words();
	cn = 0;
	for (i = 0; i < nwords; i++)
	{
		s = iv_reflow_getword(i, &csp, &x, &y, &w, &h);
		if (spnum != csp) continue;
		cn++;
		//fprintf(stderr, "%i %i %i %i %s\n", x, y, w, h, s);
	}
	wl = (iv_wlist*) malloc((cn + 1) * sizeof(iv_wlist));
	cn = 0;
	for (i = 0; i < nwords; i++)
	{
		s = iv_reflow_getword(i, &csp, &x, &y, &w, &h);
		if (spnum != csp) continue;
		fx = FixedPoint::make(x << 8);
		fy = FixedPoint::make(y << 8);
		fw = FixedPoint::make(w << 8);
		fh = FixedPoint::make(h << 8);
		wl[cn].x1 = (int)((fx / pagew) * dcw);
		wl[cn].y1 = (int)(dcy + ((pagey - (fy + fh)) / pageh) * dch);
		wl[cn].x2 = (int)(((fx + fw) / pagew) * dcw);
		wl[cn].y2 = (int)(dcy + ((pagey - fy) / pageh) * dch);

		wl[cn].word = strdup(s);
		cn++;
	}
	wl[cn].word = NULL;
	*word_list = wl;
	*wlist_len = cn;
	return;
}





