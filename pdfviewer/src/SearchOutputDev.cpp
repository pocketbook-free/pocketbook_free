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
// OutputDev
//------------------------------------------------------------------------

SearchOutputDev::SearchOutputDev(char* text)
{
	ulen = utf2ucs(text, utext, 32);
	ucs_uppercase(utext);
}

void SearchOutputDev::drawString(GfxState* state, GooString* s)
{

	GfxFont* font;
	CharCode code;
	char* p;
	Unicode* u = NULL;
	FixedPoint dx, dy, originX, originY, tOriginX, tOriginY;
	int clen, len, n, uLen;
	int i;

	if (found == 1) return;

	font = state->getFont();
	p = s->getCString();
	len = s->getLength();
	clen = 0;
	while (len > 0)
	{
		n = font->getNextChar(p, len, &code,
							  &u, &uLen,
							  &dx, &dy, &originX, &originY);

		if (u == NULL) break;
		for (i = 0; i < uLen; i++)
		{
			if (clen >= 4094) break;
			buffer[clen++] = (unsigned short) u[i];
		}
		p += n;
		len -= n;
	}
	buffer[clen] = 0;

	//  len = utf2ucs(s->getCString(), buffer, 4096-1);
	ucs_uppercase(buffer);
	for (i = 0; i <= clen - ulen; i++)
	{
		if (memcmp(&buffer[i], utext, ulen * 2) == 0)
		{
			found = 1;
			break;
		}
	}

	//  fprintf(stderr, "{%s}", s->getCString());

}
