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

#include <algorithm>

#include <iostream>
#include <vector>
#include <map>
#include <dirent.h>

#include <ZLUnicodeUtil.h>
#include <ZLImage.h>
#include "../image/ZLNXImageManager.h"

#include "ZLNXPaintContext.h"
#include <inkview.h>
#include <inkinternal.h>
#include <synopsis/synopsis.h>
#include <assert.h>
#include <fribidi/fribidi.h>


using namespace std;

#define FALSE 0
#define TRUE 1

typedef struct char_node_s{
	int junk;
	unsigned short isolated;
	unsigned short initial;
	unsigned short medial;
	unsigned short final;
} char_node;

/* *INDENT-OFF* */
char_node shaping_table[] = {
/* 0x621 */  { FALSE, 0xFE80, 0x0000, 0x0000, 0x0000},
/* 0x622 */  { FALSE, 0xFE81, 0x0000, 0x0000, 0xFE82},
/* 0x623 */  { FALSE, 0xFE83, 0x0000, 0x0000, 0xFE84},
/* 0x624 */  { FALSE, 0xFE85, 0x0000, 0x0000, 0xFE86},
/* 0x625 */  { FALSE, 0xFE87, 0x0000, 0x0000, 0xFE88},
/* 0x626 */  { FALSE, 0xFE89, 0xFE8B, 0xFE8C, 0xFE8A},
/* 0x627 */  { FALSE, 0xFE8D, 0x0000, 0x0000, 0xFE8E},
/* 0x628 */  { FALSE, 0xFE8F, 0xFE91, 0xFE92, 0xFE90},
/* 0x629 */  { FALSE, 0xFE93, 0x0000, 0x0000, 0xFE94},
/* 0x62A */  { FALSE, 0xFE95, 0xFE97, 0xFE98, 0xFE96},
/* 0x62B */  { FALSE, 0xFE99, 0xFE9B, 0xFE9C, 0xFE9A},
/* 0x62C */  { FALSE, 0xFE9D, 0xFE9F, 0xFEA0, 0xFE9E},
/* 0x62D */  { FALSE, 0xFEA1, 0xFEA3, 0xFEA4, 0xFEA2},
/* 0x62E */  { FALSE, 0xFEA5, 0xFEA7, 0xFEA8, 0xFEA6},
/* 0x62F */  { FALSE, 0xFEA9, 0x0000, 0x0000, 0xFEAA},
/* 0x630 */  { FALSE, 0xFEAB, 0x0000, 0x0000, 0xFEAC},
/* 0x631 */  { FALSE, 0xFEAD, 0x0000, 0x0000, 0xFEAE},
/* 0x632 */  { FALSE, 0xFEAF, 0x0000, 0x0000, 0xFEB0},
/* 0x633 */  { FALSE, 0xFEB1, 0xFEB3, 0xFEB4, 0xFEB2},
/* 0x634 */  { FALSE, 0xFEB5, 0xFEB7, 0xFEB8, 0xFEB6},
/* 0x635 */  { FALSE, 0xFEB9, 0xFEBB, 0xFEBC, 0xFEBA},
/* 0x636 */  { FALSE, 0xFEBD, 0xFEBF, 0xFEC0, 0xFEBE},
/* 0x637 */  { FALSE, 0xFEC1, 0xFEC3, 0xFEC4, 0xFEC2},
/* 0x638 */  { FALSE, 0xFEC5, 0xFEC7, 0xFEC8, 0xFEC6},
/* 0x639 */  { FALSE, 0xFEC9, 0xFECB, 0xFECC, 0xFECA},
/* 0x63A */  { FALSE, 0xFECD, 0xFECF, 0xFED0, 0xFECE},
/* 0x63B */  { TRUE , 0x0000, 0x0000, 0x0000, 0x0000},
/* 0x63C */  { TRUE , 0x0000, 0x0000, 0x0000, 0x0000},
/* 0x63D */  { TRUE , 0x0000, 0x0000, 0x0000, 0x0000},
/* 0x63E */  { TRUE , 0x0000, 0x0000, 0x0000, 0x0000},
/* 0x63F */  { TRUE , 0x0000, 0x0000, 0x0000, 0x0000},
/* 0x640 */  { FALSE, 0x0640, 0x0000, 0x0000, 0x0000},
/* 0x641 */  { FALSE, 0xFED1, 0xFED3, 0xFED4, 0xFED2},
/* 0x642 */  { FALSE, 0xFED5, 0xFED7, 0xFED8, 0xFED6},
/* 0x643 */  { FALSE, 0xFED9, 0xFEDB, 0xFEDC, 0xFEDA},
/* 0x644 */  { FALSE, 0xFEDD, 0xFEDF, 0xFEE0, 0xFEDE},
/* 0x645 */  { FALSE, 0xFEE1, 0xFEE3, 0xFEE4, 0xFEE2},
/* 0x646 */  { FALSE, 0xFEE5, 0xFEE7, 0xFEE8, 0xFEE6},
/* 0x647 */  { FALSE, 0xFEE9, 0xFEEB, 0xFEEC, 0xFEEA},
/* 0x648 */  { FALSE, 0xFEED, 0x0000, 0x0000, 0xFEEE},
/* 0x649 */  { FALSE, 0xFEEF, 0x0000, 0x0000, 0xFEF0},
/* 0x64A */  { FALSE, 0xFEF1, 0xFEF3, 0xFEF4, 0xFEF2}
};

bool HebrewTestWord(char *str, int size);
void Transform(char *Data2, int len);
void HebrewInvert(char *Data2, unsigned int size);

struct xxx_link {
	int x1, y1, x2, y2;
	int kind;
	std::string id;
	bool next;
};
extern int imgposx, imgposy;
extern std::vector<xxx_link> xxx_page_links;
extern iv_wlist *xxx_wordlist;
extern int xxx_wlistlen, xxx_wlistsize, xxx_hasimages, xxx_grayscale, xxx_use_update;
extern char *screenbuf;
extern int use_antialiasing;

bool ZLNXPaintContext::lock_drawing = false;

#define ROUND_26_6_TO_INT(valuetoround) (((valuetoround) + 63) >> 6)

static FT_Matrix imatrix = { 65536, 15000, 0, 65536 };

#define setPixelPointer(x, y, c, s) { c=screenbuf+((x)>>1)+(myScanline*(y)); s=((x)&1)<<2; }

extern SynWordList *acwordlist;
extern int acwlistcount, acwlistsize;

extern int maxstrheight;

ZLNXPaintContext::ZLNXPaintContext() {
	myWidth = 800;
	myHeight = 600;
	myScanline = myWidth >> 1;

	myStringHeight = -1;
	mySpaceWidth = -1;
	myDescent = 0;

	fCurFamily = "";
	fCurSize = 0;
	fCurItalic = false;
	fCurBold = false;

	FT_Error error;

	error = FT_Init_FreeType( &library );

	//fPath.push_back("/mnt/fbreader/fonts/");
	//fPath.push_back("/mnt/FBREADER/FONTS/");
	////fPath.push_back("/mnt/CRENGINE/FONTS/");
	////fPath.push_back("/mnt/crengine/fonts/");
	//fPath.push_back("/root/fbreader/fonts/");
	//fPath.push_back("/root/crengine/fonts/");
	//fPath.push_back("/root/fonts/truetype/");
	fPath.push_back(USERFONTDIR);
	fPath.push_back(SYSTEMFONTDIR);

	cacheFonts();
}

ZLNXPaintContext::~ZLNXPaintContext() {

	for(std::map<std::string, std::map<int, Font> >::iterator x = fontCache.begin();
			x != fontCache.end();
			x++) {

		for(std::map<int, Font>::iterator y = x->second.begin();
				y != x->second.end();
				y++) {

			std::map<int, std::map<unsigned long, FT_BitmapGlyph> >::iterator piter = y->second.glyphCacheAll.begin();
			while(piter != y->second.glyphCacheAll.end()) {
				std::map<unsigned long, FT_BitmapGlyph>::iterator piter2 = piter->second.begin();
				while(piter2 != piter->second.end()) {
					FT_Bitmap_Done(library, (FT_Bitmap*)&(piter2->second->bitmap));			
					piter2++;
				}
				piter++;
			}

			if(y->second.myFace != NULL)
				FT_Done_Face(y->second.myFace);
		}
	}

	if(library)
		FT_Done_FreeType(library);
}


void ZLNXPaintContext::fillFamiliesList(std::vector<std::string> &families) const {
}

void ZLNXPaintContext::cacheFonts() const {
	DIR *dir_p;
	struct dirent *dp;
	int idx;
	bool bold, italic;
	int bi_hash;

	FT_Error error;
	FT_Face lFace;

	for(std::vector<std::string>::iterator it = fPath.begin();
			it != fPath.end();
			it++) {

		//fprintf(stderr, "-- %s\n", it->c_str());
		dir_p = NULL;
		dir_p = opendir(it->c_str());	
		if(dir_p == NULL)
			continue;

		while((dp = readdir(dir_p)) != NULL) {
			idx = strlen(dp->d_name);

			if(idx <= 4)
				continue;

			idx -= 4;
			if (strcasecmp(dp->d_name + idx, ".ttf") != 0 &&
			    strcasecmp(dp->d_name + idx, ".otf") != 0 &&
			    strcasecmp(dp->d_name + idx, ".bdf") != 0 &&
//			    strcasecmp(dp->d_name + idx, ".pcf") != 0 &&
			    strcasecmp(dp->d_name + idx, ".pfb") != 0 &&
			    strcasecmp(dp->d_name + idx, ".pfm") != 0 &&
			    strcasecmp(dp->d_name + idx, ".fon") != 0 &&
			    strcasecmp(dp->d_name + idx, ".fnt") != 0
			)
			    continue;

			std::string fFullName = it->c_str();
			fFullName += "/";
			fFullName += dp->d_name;					

			for(int i = 0 ;; i++) {
				error = FT_New_Face(library, fFullName.c_str(), i, &lFace);
				if(error)
					break;

				//if(FT_IS_SCALABLE(lFace)) {
					bold = lFace->style_flags & FT_STYLE_FLAG_BOLD;				
					italic = lFace->style_flags & FT_STYLE_FLAG_ITALIC;				
					bi_hash = (bold?2:0) + (italic?1:0);

					std::string fNameWithIndex = lFace->family_name;
					if (i > 0) {
						fNameWithIndex += ":";
						fNameWithIndex += ('0'+i);
					}

					//fprintf(stderr, "    %s:%i %i %i\n", dp->d_name, i, bold, italic);
					Font *fc = &(fontCache[fNameWithIndex])[bi_hash];

					if(fc->fileName.length() == 0) {
						fc->familyName = fNameWithIndex;
						fc->fileName = fFullName;
						fc->index = i;
						fc->isBold = bold;
						fc->isItalic = italic;
					}
				//}
				FT_Done_Face(lFace);
			}
		}

/*		
		cout << "---------------------" << endl;

		for(std::map<std::string, std::map<int, Font> >::iterator x = fontCache.begin();
				x != fontCache.end();
				x++) {
			cout << "family: " << x->first << endl;

			for(std::map<int, Font>::iterator y = x->second.begin();
					y != x->second.end();
					y++) {
				cout << "	hash: " << y->first << endl;
				cout << "	file: " << y->second.fileName << endl;
				cout << "	index: " << y->second.index << endl;
				cout << "	b:	"	<< y->second.isBold << endl;
				cout << "	i:	"	<< y->second.isItalic << endl;
				cout << endl;
			}
		}
*/		

		closedir(dir_p);
	}
}

const std::string ZLNXPaintContext::realFontFamilyName(std::string &fontFamily) const {
	return fontFamily;
}

void ZLNXPaintContext::setFont(const std::string &family, int size, bool bold, bool italic) {


	//fprintf(stderr, "setFont: %s, %d, %d, %d\n", family.c_str(), size, bold?1:0, italic?1:0);
	FT_Error error;
	char buf[256], *p;
	int aindex;

	if((family == fCurFamily) && (bold == fCurBold) && (italic == fCurItalic) && (size == fCurSize)) {
		return;
	}

	fCurFamily = family;
	fCurBold = bold;
	fCurItalic = italic;

	std::string defFont("Liberation Sans");

	int bi_hash = (bold?2:0) + (italic?1:0);
	std::map<std::string, std::map<int, Font> >::iterator it = fontCache.find(family);
	std::map<std::string, std::map<int, Font> >::iterator it2 = fontCache.find(defFont);
	if (it == fontCache.end())
		it = it2;

	Font *fc = &((it->second)[bi_hash]);

	/*
	   cout << "	hash: " << bi_hash << endl;
	   cout << "	family: " << fc->familyName << endl;
	   cout << "	file: " << fc->fileName << endl;
	   cout << "	index: " << fc->index << endl;
	   cout << "	b:	"	<< fc->isBold << endl;
	   cout << "	i:	"	<< fc->isItalic << endl;
	   cout << endl;
	   */

	if(fc->fileName.size() == 0)
		fc = &((it->second)[0]);

	if(fc->myFace == NULL) {
		aindex = fc->index;
		snprintf(buf, sizeof(buf), "%s", fc->fileName.c_str());
		p = strrchr(buf, ':');
		if (p != NULL) {
			*(p++) = 0;
			aindex = atoi(p);
		}
		error = FT_New_Face(library, buf, aindex, &fc->myFace);
		if(error) {
			return;
		}
	}

	if (fc->myFaceSub == NULL) {
		Font *fc2 = &((it2->second)[bi_hash]);
		if(fc2->myFace == NULL) {
			error = FT_New_Face(library, fc2->fileName.c_str(), fc2->index, &fc2->myFace);
			if(error) {
				return;
			}
		}
		fc->myFaceSub = fc2->myFace;
	}

	embold = (bold && ! fc->isBold);
	italize = (italic && ! fc->isItalic);

	face = &(fc->myFace);
	facesub = &(fc->myFaceSub);
	if(size >= 6)
		fCurSize = size;
	else
		fCurSize = 6; 

	//FT_Set_Char_Size( *face, fCurSize * 64, 0, 160, 0 );
	FT_Set_Pixel_Sizes( *face, fCurSize, 0);
	FT_Set_Pixel_Sizes( *facesub, fCurSize, 0);

	charWidthCache = &(fc->charWidthCacheAll[fCurSize]);
	glyphCache = &(fc->glyphCacheAll[fCurSize]);
	glyphIdxCache = &(fc->glyphIdxCacheAll[fCurSize]);
	kerningCache = &(fc->kerningCacheAll[fCurSize]);

	myStringHeight = (fCurSize * 160 / 72) / 2;
	myDescent = (abs((*face)->size->metrics.descender) + 63 ) >> 6;
	mySpaceWidth = -1;

}

void ZLNXPaintContext::setColor(ZLColor color, LineStyle /*style*/) {

	tColor = ((color.Red * 77 + color.Green * 151 + color.Blue * 28) >> 8);
}

void ZLNXPaintContext::setFillColor(ZLColor color, FillStyle /*style*/) {

	fColor = ((color.Red * 77 + color.Green * 151 + color.Blue * 28) >> 8);
	//fColor &= 3;
	//fColor <<= 6;
	fColor &= 0xf0;
}

void ZLNXPaintContext::InvertSelection(int x, int y, int w, int h)
{
	InvertArea(x + imgposx, y + imgposy - myStringHeight + 5 * h / 6, w, h / 6);
}

static int char_index(FT_Face f, int c) {

	int idx = FT_Get_Char_Index(f, c);
	if (idx == 0 && f->charmap->encoding_id == 0) {
		if (c >= 0x410 && c <= 0x4ff) {
			idx = c - 0x350;
		} else {
			switch (c) {
				case 0x401: idx = 0xa8; break;
				case 0x404: idx = 0xaa; break;
				case 0x407: idx = 0xaf; break;
				case 0x406: idx = 0xb2; break;
				case 0x456: idx = 0xb3; break;
				case 0x451: idx = 0xb8; break;
				case 0x454: idx = 0xba; break;
				case 0x457: idx = 0xbf; break;
				default: idx = c; break;
			}
		}
		idx = FT_Get_Char_Index(f, idx);
		if (idx == 0) {
			switch (c) {
				case 0x401: idx = 0xc5; break;
				case 0x404: idx = 0xc5; break;
				case 0x407: idx = 0x49; break;
				case 0x406: idx = 0x49; break;
				case 0x456: idx = 0x69; break;
				case 0x451: idx = 0xe5; break;
				case 0x454: idx = 0xe5; break;
				case 0x457: idx = 0x69; break;
				default: idx = c; break;
			}
			idx = FT_Get_Char_Index(f, idx);
		}

	}
	return idx;

}

int ZLNXPaintContext::stringWidth(const char *str, int len, bool rtl) const {
	int w = 0;
	int ch_w;
	char *p = (char *)str;
	unsigned long         codepoint;
	unsigned char         in_code;
	int                   expect = 0;
	FT_Face uface;
	FT_UInt glyph_idx = 0;
	FT_UInt previous;
	FT_Bool use_kerning;
	FT_Vector delta; 
	int kerning = 0;

	use_kerning = (*face)->face_flags & FT_FACE_FLAG_KERNING;

	char* mystr = NULL;
	int slen = len;

	if (rtl){
		mystr = (char *) malloc(sizeof(char) * len * 2);
		memset(mystr, 2*len, 0);
		strncpy(mystr, str, len);
		mystr[len] = 0;

		Transform(mystr, len);

		HebrewInvert(mystr, strlen(mystr));

		p = mystr;
		slen = strlen(mystr);
	}else{
		p = (char *)str;
	}

	while ( *p && slen-- > 0)
	{
		in_code = *p++ ;

		if ( in_code >= 0xC0 )
		{
			if ( in_code < 0xE0 )           /*  U+0080 - U+07FF   */
			{
				expect = 1;
				codepoint = in_code & 0x1F;
			}
			else if ( in_code < 0xF0 )      /*  U+0800 - U+FFFF   */
			{
				expect = 2;
				codepoint = in_code & 0x0F;
			}
			else if ( in_code < 0xF8 )      /* U+10000 - U+10FFFF */
			{
				expect = 3;
				codepoint = in_code & 0x07;
			}
			continue;
		}
		else if ( in_code >= 0x80 )
		{
			--expect;

			if ( expect >= 0 )
			{
				codepoint <<= 6;
				codepoint  += in_code & 0x3F;
			}
			if ( expect >  0 )
				continue;

			expect = 0;
		}
		else                              /* ASCII, U+0000 - U+007F */
			codepoint = in_code;

		if (codepoint == 0xad) continue;

		uface = *face;
		if(glyphIdxCache->find(codepoint) != glyphIdxCache->end()) {
			glyph_idx = (*glyphIdxCache)[codepoint];
			if (glyph_idx & 0x80000000) uface = *facesub;
		} else {
			glyph_idx = char_index(uface, codepoint);
			if (glyph_idx == 0) {
				uface = *facesub;
				glyph_idx = char_index(uface, codepoint) | 0x80000000;
			}
			(*glyphIdxCache)[codepoint] = glyph_idx;
		}
		glyph_idx &= ~0x80000000;

		if ( use_kerning && previous && glyph_idx ) { 
			if((kerningCache->find(glyph_idx) != kerningCache->end()) &&
				((*kerningCache)[glyph_idx].find(previous) != (*kerningCache)[glyph_idx].end())) {
				
				kerning = ((*kerningCache)[glyph_idx])[previous];
			} else {

				FT_Get_Kerning( uface, previous, glyph_idx, FT_KERNING_DEFAULT, &delta ); 
				kerning = delta.x >> 6;

				int *k = &((*kerningCache)[glyph_idx])[previous];
				*k = kerning;
			}
		} else {
			kerning = 0;
		}

		if(charWidthCache->find(codepoint) != charWidthCache->end()) {
			w += (*charWidthCache)[codepoint] + kerning;
		} else {
			if(!FT_Load_Glyph(uface, glyph_idx,  FT_LOAD_DEFAULT)) {
				ch_w = ROUND_26_6_TO_INT(uface->glyph->advance.x); // or face->glyph->metrics->horiAdvance >> 6
				w += ch_w + kerning;
				charWidthCache->insert(std::make_pair(codepoint, ch_w));
			} 
			//	else
			//		printf("glyph %d not found\n", glyph_idx);
		}
		previous = glyph_idx;
	}

	return w;
}

int ZLNXPaintContext::spaceWidth() const {
	if (mySpaceWidth == -1) {
		mySpaceWidth = stringWidth(" ", 1, false);
	}
	return mySpaceWidth;
}

int ZLNXPaintContext::stringHeight() const {
	if (myStringHeight == -1) {
		//FIXME
		//		myStringHeight = (*face)->size->metrics.height >> 6;
		//		printf("myStringHeight: %d\n", myStringHeight);
	}
	return myStringHeight;
}

int ZLNXPaintContext::descent() const {
	return myDescent;
}

bool HebrewTestWord(char *str, int size){

	int unic1=0;
	int i=0;
	while (i<size-2){
		if ((((str[i] & 0xE0) ^ 0xC0) == 0)&&(((str[i+1] & 0xC0) ^ 0x80) == 0)){
				unic1=(str[i] & 0x1F)<<6;
				unic1=unic1 | (str[i+1] & 0x3F);
		}
		if ((((str[i] & 0xF0) ^ 0xE0) == 0)&&(((str[i+1] & 0xC0) ^ 0x80) == 0)&&(((str[i+2] & 0xC0) ^ 0x80) == 0)){
			unic1=(str[i] & 0x0F)<<12;
			unic1=unic1 | ((str[i+1] & 0x3F)<<6);
			unic1=unic1 | (str[i+2] & 0x3F);
		}
		if (((unic1>=0x2010) && (unic1<=0x2046)) ||
				((unic1>=0x0591)&&(unic1<=0x06FF)) || ((unic1>=0x0750)&&(unic1<=0x077F)) ||
				((unic1>=0xFB1D)&&(unic1<=0xFDFF)) || ((unic1>=0xFE70)&&(unic1<=0xFEFF))) {
			return true;
		} else {
			unic1=0;
			i++;
		}
	}
	return false;
}

void HebrewInvert(char *Data2, unsigned int size){
	
	unsigned int i=0;
	int unic1=0;
	int unic2=0;
	unsigned int k=0;
	unsigned int bk=0;

//	while (((Data2[k]=='(') ||
//		(Data2[k]=='[') ||
//		(Data2[k]=='<') ||
//		(Data2[k]=='{') ||
//		(Data2[k]=='\"'))
//		 && (k<size-1)){
//		k++;
//	}
//	/*
//	if ( ((Data2[k]>='0') && (Data2[k]<='9')) ||
//		((Data2[k]>='A') && (Data2[k]<='Z')) ||
//		((Data2[k]>='a') && (Data2[k]<='z')) ||
//		(size<=2) ||
//		(k>size-2)
//		){
//		return;
//	}*/

	/*k=0;
	while ((Data2[0]=='(' || Data2[0]=='[' || Data2[0]=='<' || Data2[0]=='{') && (k<size) )
	{
		if ((Data2[0]=='(') ){
			memcpy(&Data2[0], &Data2[1], size-1);
			Data2[size-1]=')';
			k++;
			size--;
		}

		if ((Data2[0]=='[')){
			memcpy(&Data2[0], &Data2[1], size-1);
			Data2[size-1]=']';
			k++;
			size--;
		}

		if ((Data2[0]=='<')){
			memcpy(&Data2[0], &Data2[1], size-1);
			Data2[size-1]='>';
			k++;
			size--;
		}

		if ((Data2[0]=='{')){
			memcpy(&Data2[0], &Data2[1], size-1);
			Data2[size-1]='}';
			k++;
			size--;
		}
	}
	
	bk=0;
	while (
		((Data2[size+bk-1]==')') ||
		(Data2[size+bk-1]==']') ||
		(Data2[size+bk-1]=='>') ||
		(Data2[size+bk-1]=='}') ||
		(Data2[size+bk-1]==':') ||
		(Data2[size+bk-1]==';') ||
		(Data2[size+bk-1]=='.') ||
		(Data2[size+bk-1]==',') ||
		(Data2[size+bk-1]=='!') ||
		(Data2[size+bk-1]=='?'))
			&& (bk<size)
		){
		if ((Data2[size+bk-1]==')') && (bk<size)){
			for (i=size+bk-1; i>bk; i--)
				Data2[i]=Data2[i-1];
			Data2[bk]='(';
			bk++;
			size--;
		}
		if ((Data2[size+bk-1]==']') && (bk<size)){
			for (i=size+bk-1; i>bk; i--)
				Data2[i]=Data2[i-1];
			Data2[bk]='[';
			bk++;
			size--;
		}
		if ((Data2[size+bk-1]=='>') && (bk<size)){
			for (i=size+bk-1; i>bk; i--)
				Data2[i]=Data2[i-1];
			Data2[bk]='<';
			bk++;
			size--;
		}
		if ((Data2[size+bk-1]=='}') && (bk<size)){
			for (i=size+bk-1; i>bk; i--)
				Data2[i]=Data2[i-1];
			Data2[bk]='{';
			bk++;
			size--;
		}
		if ((Data2[size+bk-1]==':') && (bk<size)){
			for (i=size+bk-1; i>bk; i--)
				Data2[i]=Data2[i-1];
			Data2[bk]=':';
			bk++;
			size--;
		}
		if ((Data2[size+bk-1]==';') && (bk<size)){
			for (i=size+bk-1; i>bk; i--)
				Data2[i]=Data2[i-1];
			Data2[bk]=';';
			bk++;
			size--;
		}
		if ((Data2[size+bk-1]=='.') && (bk<size)){
			for (i=size+bk-1; i>bk; i--)
				Data2[i]=Data2[i-1];
			Data2[bk]='.';
			bk++;
			size--;
		}
		if ((Data2[size+bk-1]==',') && (bk<size)){
			for (i=size+bk-1; i>bk; i--)
				Data2[i]=Data2[i-1];
			Data2[bk]=',';
			bk++;
			size--;
		}
		if ((Data2[size+bk-1]=='!') && (bk<size)){
			for (i=size+bk-1; i>bk; i--)
				Data2[i]=Data2[i-1];
			Data2[bk]='!';
			bk++;
			size--;
		}
		if ((Data2[size+bk-1]=='?') && (bk<size)){
			for (i=size+bk-1; i>bk; i--)
				Data2[i]=Data2[i-1];
			Data2[bk]='?';
			bk++;
			size--;
		}
	}
*/
	i=bk;
	if (size>1){
		if ((((Data2[i] & 0xE0) ^ 0xC0) == 0)&&(((Data2[i+1] & 0xC0) ^ 0x80) == 0)){
			unic1=(Data2[i] & 0x1F)<<6;
			unic1=unic1 | (Data2[i+1] & 0x3F);
		}
		if ((((Data2[i] & 0xF0) ^ 0xE0) == 0)&&(((Data2[i+1] & 0xC0) ^ 0x80) == 0)&&(((Data2[i+2] & 0xC0) ^ 0x80) == 0)){
			unic1=(Data2[i] & 0x1F)<<12;
			unic1=unic1 | ((Data2[i+1] & 0x3F)<<6);
			unic1=unic1 | (Data2[i+2] & 0x3F);
		}
	}

	if (((unic1>=0x0591)&&(unic1<=0x06FF)) || 
		((unic1>=0x0750)&&(unic1<=0x077F)) ||
		((unic1>=0xFB1D)&&(unic1<=0xFDFF)) ||
		((unic1>=0xFE70)&&(unic1<=0xFEFF)) ||
   		((unic1>=0xA1) && (unic1<=0xBF)) ||
   		((unic1>=0x2010) && (unic1<=0x2046)) ||
		(Data2[i]>0)){
 		int unic1size, unic2size;
		while ((size>2)&&(size<1000)){
			char buf1[5], buf2[5];
			
			unic1=0;
			unic1size=0;
			unic2size=0;
			
		   	if (Data2[i]>0){
		   		unic1size=1;
		   	}
			if ((((Data2[i] & 0xE0) ^ 0xC0) == 0)&&(((Data2[i+1] & 0xC0) ^ 0x80) == 0)){
				unic1=(Data2[i] & 0x1F)<<6;
				unic1=unic1 | (Data2[i+1] & 0x3F);
				if (((unic1>=0x0591)&&(unic1<=0x06FF)) || 
					((unic1>=0x0750)&&(unic1<=0x077F)) ||
			   		((unic1>=0xA1) && (unic1<=0xBF))){
					unic1size=2;
				}
			}
			if ((((Data2[i] & 0xF0) ^ 0xE0) == 0)&&(((Data2[i+1] & 0xC0) ^ 0x80) == 0)&&(((Data2[i+2] & 0xC0) ^ 0x80) == 0)){
				unic1=(Data2[i] & 0x0F)<<12;
				unic1=unic1 | ((Data2[i+1] & 0x3F)<<6);
				unic1=unic1 | (Data2[i+2] & 0x3F);
			   	if (((unic1>=0xFB1D)&&(unic1<=0xFDFF)) ||
					((unic1>=0xFE70)&&(unic1<=0xFEFF)) ||
			   		((unic1>=0x2010) && (unic1<=0x2046))
			   		){
					unic1size=3;
				}
			}
			unic2=0;
		   	if (Data2[i+size-1]>0){
		   		unic2size=1;
		   	}
			if ((((Data2[i+size-2] & 0xE0) ^ 0xC0) == 0)&&(((Data2[i+size-1] & 0xC0) ^ 0x80) == 0)){
				unic2=(Data2[i+size-2] & 0x1F)<<6;
				unic2=unic2 | (Data2[i+size-1] & 0x3F);
				if (((unic2>=0x0591)&&(unic2<=0x06FF)) || 
					((unic2>=0x0750)&&(unic2<=0x077F)) ||
			   		((unic2>=0xA1) && (unic2<=0xBF))){
					unic2size=2;
				}
			}
			if ((((Data2[i+size-3] & 0xF0) ^ 0xE0) == 0)&&
				(((Data2[i+size-2] & 0xC0) ^ 0x80) == 0)&&
				(((Data2[i+size-1] & 0xC0) ^ 0x80) == 0)){
				unic2=(Data2[i+size-3] & 0x0F)<<12;
				unic2=unic2 | ((Data2[i+size-2] & 0x3F)<<6);
				unic2=unic2 | (Data2[i+size-1] & 0x3F);
			   	if (((unic2>=0xFB1D)&&(unic2<=0xFDFF)) ||
					((unic2>=0xFE70)&&(unic2<=0xFEFF)) ||
			   		((unic2>=0x2010) && (unic2<=0x2046))
			   		){
					unic2size=3;
				}
			}
			if ((unic1size==0)||(unic2size==0)){
				//fprintf(stderr, "Denis - ZLTextView.cpp - HebrewInvert - Not Support Symbols - Invert was break\n\t unic1=%d unic2=%d\n", unic1, unic2);
				return;
			}
			if (unic1size>unic2size){
				memcpy(&buf1[0], &Data2[i], unic1size);
				memcpy(&buf2[0], &Data2[i+size-unic2size], unic2size);
				memcpy(&Data2[i+unic2size], &Data2[i+unic2size+(unic1size-unic2size)], size-unic2size-unic1size);
				memcpy(&Data2[i], &buf2[0], unic2size);
				memcpy(&Data2[i+size-unic1size], &buf1[0], unic1size);
				i+=unic2size;
				size-=(unic1size+unic2size);
			} else {
				if (unic1size<unic2size){
					memcpy(&buf1[0], &Data2[i], unic1size);
					memcpy(&buf2[0], &Data2[i+size-unic2size], unic2size);
					unsigned int l;
					for (l=0; l<size-unic1size-unic2size; l++)
						Data2[i+size-unic1size-l-1]=Data2[i+size-unic1size-l-(unic2size-unic1size)-1];
					memcpy(&Data2[i], &buf2[0], unic2size);
					memcpy(&Data2[i+size-unic1size], &buf1[0], unic1size);
					i+=unic2size;
					size-=(unic1size+unic2size);
				} else {
					memcpy(&buf1[0], &Data2[i], unic1size);
					memcpy(&buf2[0], &Data2[i+size-unic2size], unic2size);
					memcpy(&Data2[i], &buf2[0], unic2size);
					memcpy(&Data2[i+size-unic1size], &buf1[0], unic1size);					
					i+=unic2size;
					size-=(unic1size+unic2size);
				}
			}

		}
	} else {
		if (size>3){
			//fprintf(stderr, "Denis - ZLTextView.cpp - HebrewInvert - Not Support Symbols - Invert was break\n\t unic1=%d\n", unic1);
		}
	}
	return;
}

void Transform(char *Data2, int len){
	
	unsigned short *srcucs, *destucs;
	int y;
	int x = 0;

	srcucs = (unsigned short *) malloc(sizeof(unsigned short) * (len + 1));
	
	utf2ucs(Data2, srcucs, len + 1);

	destucs = (unsigned short *) malloc(sizeof(unsigned short) * (len + 1));
	
	for (y = 0; y < len; y++){
		
		int have_previous = TRUE, have_next = TRUE;
/* If it's not in our range, skip it. */
		if ((srcucs[y] < 0x621) || (srcucs[y] > 0x64A)){
			destucs[x++] = srcucs[y];
			continue;
		}

/* The character wasn't included in the unicode shaping table. */
		if (shaping_table[(srcucs[y] - 0x621)].junk){
			destucs[x++] = srcucs[y];
			continue;
		}

		if (((srcucs[y - 1] < 0x621) || (srcucs[y - 1] > 0x64A)) ||
		      (!(shaping_table[(srcucs[y - 1] - 0x621)].initial) &&
		       !(shaping_table[(srcucs[y - 1] - 0x621)].medial))){
			have_previous = FALSE;
		}

		if (((srcucs[y + 1] < 0x621) || (srcucs[y + 1] > 0x64A)) ||
		      (!(shaping_table[(srcucs[y + 1] - 0x621)].medial) &&
		       !(shaping_table[(srcucs[y + 1] - 0x621)].final)
		       && (srcucs[y + 1] != 0x640))){
		      have_next = FALSE;
		}

		if (srcucs[y] == 0x644){
			if (have_next){
				if ((srcucs[y + 1] == 0x622) ||
					(srcucs[y + 1] == 0x623) ||
					(srcucs[y + 1] == 0x625) || (srcucs[y + 1] == 0x627)){
					if (have_previous){
						if (srcucs[y + 1] == 0x622){
							destucs[x++] = 0xFEF6;
						} else if (srcucs[y + 1] == 0x623){
							destucs[x++] = 0xFEF8;
						} else if (srcucs[y + 1] == 0x625){
							destucs[x++] = 0xFEFA;
						} else {
/* srcucs[y+1] = 0x627 */
							destucs[x++] = 0xFEFC;
						}
					} else {
						if (srcucs[y + 1] == 0x622){
							destucs[x++] = 0xFEF5;
						} else if (srcucs[y + 1] == 0x623){
							destucs[x++] = 0xFEF7;
						} else if (srcucs[y + 1] == 0x625){
							destucs[x++] = 0xFEF9;
						} else {
/* srcucs[y+1] = 0x627 */
							destucs[x++] = 0xFEFB;
						}
					}
					y++;
					continue;
				}
			}
		}

/* Medial */
		if ((have_previous) && (have_next)
			&& (shaping_table[(srcucs[y] - 0x621)].medial)){
/*            		if (shaping_table[(srcucs[y] - 0x621)].medial){ */
				destucs[x++] = shaping_table[(srcucs[y] - 0x621)].medial;
/*              	} else {
				destucs[y] = srcucs[y];
			} */
			continue;
		} else if ((have_previous) && (shaping_table[(srcucs[y] - 0x621)].final)){
/* Final */
			destucs[x++] = shaping_table[(srcucs[y] - 0x621)].final;
			continue;
		} else if ((have_next) && (shaping_table[(srcucs[y] - 0x621)].initial)){
/* Initial */
			destucs[x++] = shaping_table[(srcucs[y] - 0x621)].initial;
			continue;
		} else {
/* Isolated */
			if (shaping_table[(srcucs[y] - 0x621)].isolated){
				destucs[x++] = shaping_table[(srcucs[y] - 0x621)].isolated;
			} else {
				destucs[x++] = srcucs[y];
			}
			continue;
		}
	}

	destucs[x] = 0x0;

//	fprintf(stderr, "Transform - loop end len = %d x = %d\n", len, x);

	ucs2utf(destucs, Data2, 2 * len);
	
	free(destucs);
	free(srcucs);

//	fprintf(stderr, "out Data2 = %s\n", Data2);

}

void ReplaceBrackets(char* str)
{
	while (*str != '\0')
	{
		switch (*str)
		{
		case '(': *str = ')'; break;
		case ')': *str = '('; break;
		case '[': *str = ']'; break;
		case ']': *str = '['; break;
		case '{': *str = '}'; break;
		case '}': *str = '{'; break;
		case '<': *str = '>'; break;
		case '>': *str = '<'; break;
		}
		str++;
	}
}

void ZLNXPaintContext::drawString(int x, int y, const char *str, int len, bool rtl){

	if (lock_drawing)
		return;

	const char *p = str;
	int slen = len;
	char *mystr = NULL;

//	std::string stdstr(str, len);
//	if (stdstr.find('[3]') != std::string::npos || tColor != 0)
//	{
//		int ddd = 0;
//		ddd++;
//	}

	if (rtl){
		mystr = (char *) malloc(sizeof(char) * len * 2);
		memset(mystr, 2*len, 0);
		strncpy(mystr, str, len);
		mystr[len] = 0;

		Transform(mystr, len);
		ReplaceBrackets(mystr);

		HebrewInvert(mystr, strlen(mystr));

		p = mystr;
		slen = strlen(mystr);
	}else{
		p = (char *)str;
	}


	FT_BitmapGlyph glyph;
	FT_BitmapGlyph *pglyph;
	FT_Vector     spen;                    /* untransformed origin  */
	FT_Vector     pen;                    /* untransformed origin  */

	FT_Face uface;
	FT_UInt glyph_idx = 0;
	FT_UInt previous;
	FT_Bool use_kerning;
	FT_Vector delta; 
	int current_aa;
	
	use_kerning = (*face)->face_flags & FT_FACE_FLAG_KERNING;
	current_aa = (use_antialiasing && xxx_use_update!=2) ? 1 : 0;

	unsigned long         codepoint, codepoint2;
	unsigned char         in_code;
	int                   expect;
	int kerning;

	int nch = 0;

	spen.x = pen.x = x;
	spen.y = pen.y = y;

	int wlen = slen;


	while ( *p && wlen--)
	{
		in_code = *p++ ;

		if ( in_code >= 0xC0 )
		{
			if ( in_code < 0xE0 )           /*  U+0080 - U+07FF   */
			{
				expect = 1;
				codepoint = in_code & 0x1F;
			}
			else if ( in_code < 0xF0 )      /*  U+0800 - U+FFFF   */
			{
				expect = 2;
				codepoint = in_code & 0x0F;
			}
			else if ( in_code < 0xF8 )      /* U+10000 - U+10FFFF */
			{
				expect = 3;
				codepoint = in_code & 0x07;
			}
			continue;
		}
		else if ( in_code >= 0x80 )
		{
			--expect;

			if ( expect >= 0 )
			{
				codepoint <<= 6;
				codepoint  += in_code & 0x3F;
			}
			if ( expect >  0 )
				continue;

			expect = 0;
		}
		else                              /* ASCII, U+0000 - U+007F */
			codepoint = in_code;

		if (codepoint == 0xad) continue;
		nch++;

		uface = *face;
		if(glyphIdxCache->find(codepoint) != glyphIdxCache->end()) {
			glyph_idx = (*glyphIdxCache)[codepoint];
			if (glyph_idx & 0x80000000) uface = *facesub;
		} else {
			glyph_idx = char_index(uface, codepoint);
			if (glyph_idx == 0) {
				uface = *facesub;
				glyph_idx = char_index(uface, codepoint) | 0x80000000;
			}
			(*glyphIdxCache)[codepoint] = glyph_idx;
		}
		glyph_idx &= ~0x80000000;

		if ( use_kerning && previous && glyph_idx ) { 
			if((kerningCache->find(glyph_idx) != kerningCache->end()) &&
				((*kerningCache)[glyph_idx].find(previous) != (*kerningCache)[glyph_idx].end())) {
				
				kerning = ((*kerningCache)[glyph_idx])[previous];
			} else {

				FT_Get_Kerning(uface, previous, glyph_idx, FT_KERNING_DEFAULT, &delta ); 
				kerning = delta.x >> 6;

				int *k = &((*kerningCache)[glyph_idx])[previous];
				*k = kerning;
			}
			pen.x += kerning;		
		}

		codepoint2 = (codepoint << 3);
		if (embold)  codepoint2 |= 1;
		if (italize) codepoint2 |= 2;
		if (current_aa) codepoint2 |= 4;

		if(glyphCache->find(codepoint2) != glyphCache->end()) { 
			pglyph = &(*glyphCache)[codepoint2];
		} else {
			if(FT_Load_Glyph(uface, glyph_idx,  FT_LOAD_DEFAULT)){
				continue;
			}
			if (embold) {
				if ((uface)->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
				    FT_Outline_Embolden(&uface->glyph->outline, 63);
			}
			if (italize) {
				if ((uface)->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
				    FT_Outline_Transform(&uface->glyph->outline, &imatrix);
			}
			if (use_antialiasing == 2) {
				if ((uface)->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
				    FT_Outline_Embolden(&uface->glyph->outline, 15);
			}
			FT_Render_Glyph(uface->glyph,
				current_aa ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO);

			FT_Get_Glyph(uface->glyph, (FT_Glyph*)&glyph);

			glyph->root.advance.x = uface->glyph->advance.x;	  			

			(*glyphCache)[codepoint2] = glyph;
			pglyph = &glyph;
		}

		drawGlyph( &(*pglyph)->bitmap,
				pen.x + (*pglyph)->left,
				pen.y - (*pglyph)->top);


/*		if(!mark) {
			drawLine(pen.x + (*pglyph)->left, y+1, pen.x + ((*pglyph)->root.advance.x >> 6), y+1);
			mark = true;
		}
*/

		/* increment pen position */
		pen.x += (*pglyph)->root.advance.x >> 6;
		previous = glyph_idx;
	}

	if (tColor >= 120) {
		invert(spen.x, spen.y-myStringHeight+myStringHeight/4, pen.x-spen.x, myStringHeight);
	}

	if (xxx_wlistlen+3 >= xxx_wlistsize) {
		xxx_wlistsize = xxx_wlistsize + (xxx_wlistsize >> 1) + 64;
		xxx_wordlist = (iv_wlist *) realloc(xxx_wordlist, xxx_wlistsize * sizeof(iv_wlist));
	}
	if (nch >= 1) {
		iv_wlist *wp = &(xxx_wordlist[xxx_wlistlen++]);
		wp->x1 = imgposx+spen.x;
		wp->x2 = imgposx+pen.x;
		wp->y1 = imgposy+spen.y-myStringHeight+myStringHeight/4;
		wp->y2 = imgposy+spen.y+myStringHeight/4;

		wp->word = (char *) malloc(len+1);
		memcpy(wp->word, str, len);

		if ( rtl /*HebrewTestWord(wp->word, slen)*/){
			HebrewInvert(wp->word, len);
		}

		wp->word[len] = 0;
		xxx_wordlist[xxx_wlistlen].word = NULL;
	}
	
	if (mystr)
		free(mystr);

	if (maxstrheight < myStringHeight + myStringHeight / 4)
		maxstrheight = myStringHeight + myStringHeight / 4;

}

void ZLNXPaintContext::drawImage(int x, int y, const ZLImageData &image, int width, int height, ScalingType type)
{
	// TODO - scaling support.
	drawImage(x, y, image);
}

void ZLNXPaintContext::drawImage(int x, int y, const ZLImageData &image) {

	if (lock_drawing) return;
	fprintf(stderr, "[IM:%i,%i,%i,%i]", x, y, image.width(), image.height());

	char *c;
	char *c_src;
	int s, s_src;
	ZLNXImageData &nximage = (ZLNXImageData &)image;
	char *src = nximage.getImageData();
	int iW = nximage.width();
	int iH = nximage.height();
	int scanline = nximage.scanline();
	int jW, jH, pix, xx, yy, slxx, slyy, step/*, grays*/;

	if (x < 0) x = 0;
	jW = iW;
	jH = iH;
	if (iW > myWidth - x) {
		jW = myWidth - x;
		jH = (iH * (myWidth - x)) / iW;
	}
	if (jH > myHeight) {
		jW = (jW * myHeight) / jH;
		jH = myHeight;
	}
	if (y - jH < 0) y = jH;

	step = (iW * 1024) / jW;

	//grays = 0;
	for(yy = 0; yy < jH; yy++) {
		slyy = ((yy * step) >> 10) * scanline;
		for(xx = 0; xx < jW; xx++) {
			slxx = (xx * step) >> 10;
			c_src = src + (slxx >> 1) + slyy;
			s_src = (slxx & 1) << 2;
			setPixelPointer(x + xx, (y - jH) + yy, c, s);
			*c &= ~(0xf0 >> s);
			pix = ((*c_src << s_src) & 0xf0);
			//if (pix != 0 && pix != 0xf0) grays++;
			*c |= (pix >> s);
		}
	}
	xxx_hasimages = 1;
	/*if (grays >= (jW * jH) / 5)*/ xxx_grayscale = 1;

	if (maxstrheight < myStringHeight + myStringHeight / 4 + iH)
		maxstrheight = myStringHeight + myStringHeight / 4 + iH;

}

void ZLNXPaintContext::drawLine(int x0, int y0, int x1, int y1) {
	if (lock_drawing) return;
	drawLine(x0, y0, x1, y1, false);
}

void ZLNXPaintContext::drawLine(int x0, int y0, int x1, int y1, bool fill) {

	if (lock_drawing) return;

	int i, j;
	int k, s;
	char *c = screenbuf;
	bool done = false;

	if (x0 < 0 || y0 < 0 || x1 < 0 || y1 < 0) return;
	if (x0 >= myWidth || y0 >= myHeight || x1 >= myWidth || y1 >= myHeight) return;
	
	if(x1 != x0) {
		k = (y1 - y0) / (x1 - x0);
		j = y0;
		i = x0;

		do {
			if(i == x1)
				done = true;

			setPixelPointer(i, j, c, s);

			*c &= ~(0xf0 >> s);

			if(fill)
				*c |= (fColor >> s);


			j += k;

			if(x1 > x0)
				i++;
			else 
				i--;

		} while(!done);

	} else {
		i = x0;
		j = y0;
//		s = (i & 3) << 1;

		do {
			if(j == y1)
				done = true;

			setPixelPointer(i, j, c, s);
			*c &= ~(0xf0 >> s);

			if(fill)
				*c |= (fColor >> s);

			if(y1 > y0)
				j++;
			else if(y1 < y0)
				j--;

		} while(!done);
	}
}

void ZLNXPaintContext::fillRectangle(int x0, int y0, int x1, int y1) {
	//printf("fillRectangle\n");

	if (lock_drawing) return;

	int j;

	j = y0;
	do {
		drawLine(x0, j, x1, j, true);

		if(y1 > y0)
			j++;
		else if(y1 < y0)
			j--;
	} while(( y1 > y0) && ( j <= y1 )  ||
			(j <= y0));
}

void ZLNXPaintContext::drawFilledCircle(int x, int y, int r) {
	//printf("drawFilledCircle\n");
}

void ZLNXPaintContext::clear(ZLColor color) {
	int i;
	for (i=0; i<acwlistcount; i++) 
		if (acwordlist[i].word)
			free(acwordlist[i].word);
	if (acwordlist)
		free(acwordlist);
	acwordlist = NULL;
	acwlistsize = acwlistcount = 0;	

	if (lock_drawing) return;
	memset(screenbuf, 0xff, 1024*1024/2);
	//xxx_page_links.clear();

	for (i=0; i<xxx_wlistlen; i++)
		free(xxx_wordlist[i].word);
	free(xxx_wordlist);
	xxx_wordlist = NULL;
	xxx_wlistsize = xxx_wlistlen = 0;

	xxx_hasimages = 0;
	xxx_grayscale = 0;
	maxstrheight = 0;
}

int ZLNXPaintContext::width() const {
	return myWidth;
}

int ZLNXPaintContext::height() const {
	return myHeight;
}

void ZLNXPaintContext::drawGlyph( FT_Bitmap*  bitmap, FT_Int x, FT_Int y)
{
	if (lock_drawing) return;
	FT_Int  i, j, p, q;
	FT_Int  x_max = x + bitmap->width;
	FT_Int  y_max = y + bitmap->rows;
	char *c = screenbuf;
	int mode = bitmap->pixel_mode;
	unsigned char *data = bitmap->buffer;
	unsigned char val;
	int s;

	for ( i = x, p = 0; i < x_max; i++, p++ ) {
		if (i < 0 || i >= myWidth) continue;
		for ( j = y, q = 0; j < y_max; j++, q++ ) {
			if (j < 0 || j >= myHeight ) continue;

			setPixelPointer(i, j, c, s);
			if (mode == FT_PIXEL_MODE_MONO) {
				val = data[q * bitmap->pitch + (p >> 3)];
				val = (val & (0x80 >> (p & 7))) ? 255 : 0;
			} else {
				val = data[q * bitmap->pitch + p];
			}
/*
			if(val >= 192) {
				*c &= ~(0xc0 >> s);
			} else if (val >= 128) {
				*c &= ~(0x80 >> s);
			} else if (val >= 64) {
				*c &= ~(0x40 >> s);
			}
*/
			if (val >= 32) {
				*c = (*c & ~(0xf0 >> s)) | (((255 - val) & 0xf0) >> s);
			}
		}
	}
}

void ZLNXPaintContext::invert(int x, int y, int w, int h)
{
	char *c;
	int s, xx, yy;

	for (yy=y; yy<y+h; yy++) {
		for (xx=x; xx<x+w; xx++) {
			setPixelPointer(xx, yy, c, s);
			*c ^= (0xf0 >> s);
		}
	}
}


/*
void ZLNXPaintContext::setPixelPointer(int x, int y, char **c, int *s)
{
	switch (rotation()) {
		default:
		{
			*c = screenbuf + x / 4 + myWidth * y / 4;
			*s =  (x & 3) << 1;
			break;
		}
		case DEGREES90:
		{
			*c = buf + x * myHeight / 4 + (myHeight - y - 1) / 4;
			*s = (3 - y & 3) * 2;
			break;
		}
		case DEGREES180:
		{
//			*c = buf + (myWidth - x + 3) / 4 + (3 + myWidth * (myHeight- y)) / 4;
			*c = buf + x / 4 + myWidth * y / 4;
			*s =  (x & 3) << 1;
//			*s =  (3 - x & 3) << 1;

//			*c = buf +  (myWidth - x) * myHeight / 4 + y / 4;
//			*s = (y & 3) << 1;
			break;
		}
		case DEGREES270:
		{
			*c = buf +  (myWidth - x) * myHeight / 4 + y / 4;
			*s = (y & 3) << 1;
			break;
		}
	}
}
*/
