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

#ifndef SEARCHOUTPUTDEV_H
#define SEARCHOUTPUTDEV_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <poppler-config.h>
#include "goo/gtypes.h"
#include "CharTypes.h"
#include <stdio.h>

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

class SearchOutputDev: public OutputDev
{
	public:

		// Constructor.
		SearchOutputDev(char* text);

		// Destructor.
		~SearchOutputDev() {}

		//----- get info about output device

		GBool upsideDown()
		{
			return gTrue;
		}
		GBool useDrawChar()
		{
			return gFalse;
		}
		GBool useTilingPatternFill()
		{
			return gTrue;
		}
		GBool useShadedFills()
		{
			return gTrue;
		}
		GBool useDrawForm()
		{
			return gTrue;
		}
		GBool interpretType3Chars()
		{
			return gFalse;
		}
		GBool needNonText()
		{
			return gFalse;
		}

		//----- initialization and control

		// Set default transform matrix.
		void setDefaultCTM(FixedPoint* ctm) { }

		GBool checkPageSlice(Page* page, FixedPoint hDPI, FixedPoint vDPI,
							 int rotate, GBool useMediaBox, GBool crop,
							 int sliceX, int sliceY, int sliceW, int sliceH,
							 GBool printing, Catalog* catalog,
							 GBool(* abortCheckCbk)(void* data) = NULL,
							 void* abortCheckCbkData = NULL)
		{
			return gTrue;
		}

		void startPage(int pageNum, GfxState* state)
		{

			//  fprintf(stderr, "[StartPage]");

		}

		// End a page.
		void endPage()
		{

			//  fprintf(stderr, "[EndPage]");

		}

		// Dump page contents to display.
		void dump() {}

		//----- coordinate conversion

		// Convert between device and user coordinates.
		void cvtDevToUser(FixedPoint dx, FixedPoint dy, FixedPoint* ux, FixedPoint* uy) { }
		void cvtUserToDev(FixedPoint ux, FixedPoint uy, int* dx, int* dy) { }

		FixedPoint* getDefCTM()
		{
			return defCTM;
		}
		FixedPoint* getDefICTM()
		{
			return defICTM;
		}


		//----- text drawing
		void beginStringOp(GfxState * /*state*/)
		{

			//  fprintf(stderr, "[BSO]");

		}
		void endStringOp(GfxState * /*state*/)
		{

			//  fprintf(stderr, "[ESO]");

		}

		void beginString(GfxState* state, GooString* s)
		{

			//  fprintf(stderr, "[BS]");

		}
		void endString(GfxState* state)
		{

			//  fprintf(stderr, "[ES]");

		}
		void drawChar(GfxState* state, FixedPoint x, FixedPoint y,
					  FixedPoint dx, FixedPoint dy,
					  FixedPoint originX, FixedPoint originY,
					  CharCode code, int nBytes, Unicode* u, int uLen)
		{

			//  fprintf(stderr, "[!DRAWCHAR]");


		}

		void drawString(GfxState* state, GooString* s);

		void endTextObject(GfxState * /*state*/)
		{

			//  fprintf(stderr, "[ETO]");

		}

		int found;

	private:

		void ucs_uppercase(unsigned short* p)
		{

			unsigned short c;

			while ((c = *p) != 0)
			{
				if (c >= 'a' && c <= 'z')
				{
					*p -= 0x20;
				}
				else if (c >= 0xe0 && c <= 0xfe)
				{
					*p -= 0x20;
				}
				else if (c >= 0x430 && c <= 0x44f)
				{
					*p -= 0x20;
				}
				p++;
			}

		}



		FixedPoint defCTM[6];   // default coordinate transform matrix
		FixedPoint defICTM[6];    // inverse of default CTM

		unsigned short utext[32];
		unsigned short buffer[4096];
		int ulen;

};

#endif
