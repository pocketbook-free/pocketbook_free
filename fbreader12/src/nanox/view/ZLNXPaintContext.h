/*
 * Copyright (C) 2008 Alexander Egorov <lunohod@gmx.de>
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

#ifndef __ZLNXPAINTCONTEXT_H__
#define __ZLNXPAINTCONTEXT_H__

#include <ZLPaintContext.h>

#include <algorithm>
#include <iostream>
#include <vector>
#include <map>

#include <ft2build.h>
/*#include <freetype/freetype.h>
#include <freetype/ftbitmap.h>
#include <freetype/ftglyph.h>*/
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_BITMAP_H

/*
	//virtual void clear(ZLColor color) = 0;
	//virtual void setFont(const std::string &family, int size, bool bold, bool italic) = 0;
	//virtual void setColor(ZLColor color, LineStyle style = SOLID_LINE) = 0;
	//virtual void setFillColor(ZLColor color, FillStyle style = SOLID_FILL) = 0;
	//virtual int width() const = 0;
	//virtual int height() const = 0;
	//virtual int stringWidth(const char *str, int len, bool rtl) const = 0;
	//virtual int spaceWidth() const = 0;
	//virtual int stringHeight() const = 0;
	//virtual int descent() const = 0;
	//virtual void drawString(int x, int y, const char *str, int len, bool rtl) = 0;
	virtual void drawImage(int x, int y, const ZLImageData &image) = 0;
	virtual void drawImage(int x, int y, const ZLImageData &image, int width, int height, ScalingType type) = 0;
	virtual void drawLine(int x0, int y0, int x1, int y1) = 0;
	virtual void fillRectangle(int x0, int y0, int x1, int y1) = 0;
	virtual void drawFilledCircle(int x, int y, int r) = 0;
	virtual const std::string realFontFamilyName(std::string &fontFamily) const = 0;
	virtual void fillFamiliesList(std::vector<std::string> &families) const = 0;

  */
class ZLNXPaintContext : public ZLPaintContext {

public:
	ZLNXPaintContext();
	~ZLNXPaintContext();
        static bool lock_drawing;
	enum Angle {
		DEGREES0 = 0,
		DEGREES90 = 90,
		DEGREES180 = 180,
		DEGREES270 = 270
	};

	int width() const;
	int height() const;

	void clear(ZLColor color);

	void fillFamiliesList(std::vector<std::string> &families) const;
	void cacheFonts() const;
	const std::string realFontFamilyName(std::string &fontFamily) const;

	void setFont(const std::string &family, int size, bool bold, bool italic);
	void setColor(ZLColor color, LineStyle style = SOLID_LINE);
	void setFillColor(ZLColor color, FillStyle style = SOLID_FILL);
	void InvertSelection(int x, int y, int w, int h);

	int stringWidth(const char *str, int len, bool rtl) const;
	int spaceWidth() const;
	int stringHeight() const;
	int descent() const;
	void drawString(int x, int y, const char *str, int len, bool rtl);

	void drawImage(int x, int y, const ZLImageData &image);
	void drawImage(int x, int y, const ZLImageData &image, int width, int height, ScalingType type);


	void drawLine(int x0, int y0, int x1, int y1);
	void drawLine(int x0, int y0, int x1, int y1, bool fill);
	void fillRectangle(int x0, int y0, int x1, int y1);
	void drawFilledCircle(int x, int y, int r);

	void setSize(int width, int height, int scanline) {
		myWidth = width;
		myHeight = height;
		myScanline = scanline;
	}


	void rotate(Angle rotation) { 
		myRotation = rotation; 
	}
	Angle rotation() const { return myRotation; }

private:
	int myWidth, myHeight, myScanline;
	Angle myRotation;
	bool embold, italize;

	class Font {
		public:
			Font() : 
				familyName(""),
				fileName(""),
				index(0),
				myFace(NULL),
				myFaceSub(NULL),
				isBold(false),
				isItalic(false) {}

			~Font() { if(myFace) FT_Done_Face(myFace); }
	
			std::string familyName;
			std::string fileName;
			int index;

			FT_Face myFace;
			FT_Face myFaceSub;
			bool isBold;
			bool isItalic;

/*			std::map<int, std::map<unsigned long, int> > charWidthCacheAll;
			std::map<int, std::map<unsigned long, FT_BitmapGlyph> > glyphCacheAll;
			
			std::map<int, std::map<FT_UInt, std::map<FT_UInt, int> > > kerningCacheAll;
			std::map<int, std::map<unsigned long, FT_UInt> > glyphIdxCacheAll;*/
			std::map<int, std::map<unsigned long, int> > charWidthCacheAll;
                        std::map<int, std::map<unsigned long, FT_BitmapGlyph> > glyphCacheAll;
                         
                         std::map<int, std::map<FT_UInt, std::map<FT_UInt, int> > > kerningCacheAll;
                         std::map<int, std::map<unsigned long, FT_UInt> > glyphIdxCacheAll;
			
	};
	

	mutable std::vector<std::string> fPath;
	mutable std::map<std::string, std::map<int, Font> > fontCache;
	mutable std::map<unsigned long, int> *charWidthCache;
	mutable std::map<unsigned long, FT_BitmapGlyph> *glyphCache;

	mutable std::map<FT_UInt, std::map<FT_UInt, int> > *kerningCache;
	mutable std::map<unsigned long, FT_UInt> *glyphIdxCache;

	FT_Face	*face;
	FT_Face	*facesub;
	std::string fCurFamily;
	int fCurSize;
	bool fCurItalic;
	bool fCurBold;
	int fColor;
	int tColor;

	FT_Library library;

	std::vector<std::string> myFontFamilies;

	mutable int myStringHeight;
	mutable int mySpaceWidth;
	int myDescent;

	void drawGlyph( FT_Bitmap*  bitmap, FT_Int x, FT_Int y);
	void invert(int x, int y, int w, int h);
//	void setPixelPointer(int x, int y, char **c, int *s);

};

#endif /* __ZLNXPAINTCONTEXT_H__ */
