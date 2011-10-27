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
// OutputDev.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef MYSPLASHOUTPUTDEV_H
#define MYSPLASHOUTPUTDEV_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <poppler-config.h>
#include "goo/gtypes.h"
#include "poppler/SplashOutputDev.h"
#include "splash/SplashFont.h"
#include "CharTypes.h"
#include <inkview.h>

#define ddprintf(x...) fprintf(stderr, x)
// #define ddprintf(x...)

class Dict;
class GooHash;
class GooString;
class GfxState;
struct GfxColor;
class GfxColorSpace;
class GfxImageColorMap;
class GfxFunctionShading;
class GfxAxialShading;
class GfxRadialShading;
class Stream;
class Links;
class Link;
class Catalog;
class Page;
class Function;

//------------------------------------------------------------------------
// OutputDev
//------------------------------------------------------------------------

class MySplashOutputDev: public SplashOutputDev
{
	public:

		// Constructor.
		MySplashOutputDev(SplashColorMode colorModeA, int bitmapRowPadA,
						  GBool reverseVideoA, SplashColorPtr paperColorA,
						  GBool bitmapTopDownA = gTrue,
						  GBool allowAntialiasA = gTrue)

			: SplashOutputDev(colorModeA, bitmapRowPadA, reverseVideoA, paperColorA, bitmapTopDownA, allowAntialiasA)
		{
			reflow = gTrue;
		}

		// Destructor.
		~MySplashOutputDev()
		{
		}

		virtual GBool useDrawChar()
		{
			return gTrue;
		}
		virtual GBool needNonText()
		{
			return gTrue;
		}

		virtual void updateAll(GfxState* state)
		{
			if (reflow)
			{
				SplashOutputDev::updateFont(state);
			}
			else
			{
				SplashOutputDev::updateAll(state);
			}
		}

		virtual void updateLineDash(GfxState* state)
		{
			if (reflow) return;
			SplashOutputDev::updateLineDash(state);
		}

		virtual void updateFont(GfxState* state)
		{
			SplashOutputDev::updateFont(state);
		}

		virtual void stroke(GfxState* state)
		{
			if (! reflow) SplashOutputDev::stroke(state);
		}
		virtual void fill(GfxState* state)
		{
			if (! reflow) SplashOutputDev::fill(state);
		}
		virtual void eoFill(GfxState* state)
		{
			if (! reflow) SplashOutputDev::eoFill(state);
		}
		virtual void tilingPatternFill(GfxState* state, Object* str,
									   int paintType, Dict* resDict,
									   FixedPoint* mat, FixedPoint* bbox,
									   int x0, int y0, int x1, int y1,
									   FixedPoint xStep, FixedPoint yStep)
		{

			if (! reflow) SplashOutputDev::tilingPatternFill(state, str, paintType,
					resDict, mat, bbox, x0, y0, x1, y1, xStep, yStep);
		}
		virtual void clip(GfxState* state)
		{
			if (! reflow) SplashOutputDev::clip(state);
		}
		virtual void eoClip(GfxState* state)
		{
			if (! reflow) SplashOutputDev::eoClip(state);
		}
		virtual void clipToStrokePath(GfxState* state)
		{
			if (! reflow) SplashOutputDev::clipToStrokePath(state);
		}
		virtual void beginStringOp(GfxState* state)
		{
			SplashOutputDev::beginStringOp(state);
			if (reflow && subpage == -1) iv_reflow_bt();
		}
		virtual void endStringOp(GfxState* state)
		{
			SplashOutputDev::endStringOp(state);
			if (reflow && subpage == -1) iv_reflow_et();
		}

		void setup(GBool usereflow, int sp, int dispx, int dispy, int dispw, int disph,
				   FixedPoint x, FixedPoint y, FixedPoint w, FixedPoint h, FixedPoint res);

		virtual void drawChar(GfxState* state, FixedPoint x, FixedPoint y,
							  FixedPoint dx, FixedPoint dy,
							  FixedPoint originX, FixedPoint originY,
							  CharCode code, int nBytes, Unicode* u, int uLen);

		virtual void drawImageMask(GfxState* state, Object* ref, Stream* str,
								   int width, int height, GBool invert,
								   GBool inlineImg);

		virtual void drawImage(GfxState* state, Object* ref, Stream* str,
							   int width, int height, GfxImageColorMap* colorMap,
							   int* maskColors, GBool inlineImg);

		virtual void drawMaskedImage(GfxState* state, Object* ref, Stream* str,
									 int width, int height,
									 GfxImageColorMap* colorMap,
									 Stream* maskStr, int maskWidth, int maskHeight,
									 GBool maskInvert);

		virtual void drawSoftMaskedImage(GfxState* state, Object* ref, Stream* str,
										 int width, int height,
										 GfxImageColorMap* colorMap,
										 Stream* maskStr,
										 int maskWidth, int maskHeight,
										 GfxImageColorMap* maskColorMap);


		int subpageCount()
		{
			return iv_reflow_subpages();
		}


		GBool beginType3Char(GfxState* state, FixedPoint x, FixedPoint y,
							 FixedPoint dx, FixedPoint dy,
							 CharCode code, Unicode* u, int uLen)
		{
			if (reflow) return false;
			return SplashOutputDev::beginType3Char(state, x, y, dx, dy, code, u, uLen);
		}
		void endType3Char(GfxState* state)
		{
			if (reflow) return;
			SplashOutputDev::endType3Char(state);
		}

		void endTextObject(GfxState* state)
		{
			SplashOutputDev::endTextObject(state);
			if (reflow && subpage == -1) iv_reflow_div();
		}

		//iv_wlist *getWordList(int spnum);
		void getWordList(iv_wlist** word_list, int* wlist_len, int spnum);

	private:

		void add_image(FixedPoint* m);
		int get_image(FixedPoint* m);

		GBool reflow;
		FixedPoint ares;
		int subpage;
		FixedPoint dcx, dcy, dcw, dch;
		FixedPoint pagex, pagey, pagew, pageh;

};

#endif
