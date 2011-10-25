/*
 * Copyright (C) 2008 Alexander Egorov <lunohod@gmx.de>
 *
 * dithering algorithm and jpeg/png handling are from coolreader engine
 * Copyright (C) Vadim Lopatin, 2000-2006
 *
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

/* bmp support is based on:
 *
 * Stand alone BMP library.
 * Copyright (c) 2006 Rene Rebe <rene@exactcode.de>
 *
 * based on:
 *
 * Project:  libtiff tools
 * Purpose:  Convert Windows BMP files in TIFF.
 * Author:   Andrey Kiselev, dron@remotesensing.org
 *
 ******************************************************************************
 * Copyright (c) 2004, Andrey Kiselev <dron@remotesensing.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */


#include <ZLImage.h>

#include "ZLNXImageManager.h"

#define PNG_SKIP_SETJMP_CHECK 1

extern "C" 
{
#include <stdio.h>
#include <jpeglib.h>
//#include <setjmp.h>
}

#include <png.h>
#include <gif_lib.h>
#include <inkview.h>
#include "../view/ZLNXPaintContext.h"

static const short dither_2bpp_8x8[] = {
	0, 32, 12, 44, 2, 34, 14, 46, 
	48, 16, 60, 28, 50, 18, 62, 30, 
	8, 40, 4, 36, 10, 42, 6, 38, 
	56, 24, 52, 20, 58, 26, 54, 22, 
	3, 35, 15, 47, 1, 33, 13, 45, 
	51, 19, 63, 31, 49, 17, 61, 29, 
	11, 43, 7, 39, 9, 41, 5, 37, 
	59, 27, 55, 23, 57, 25, 53, 21, 
};


int Dither2BitColor( int color, int x, int y )
{
	int cl = ((((color>>16) & 255) + ((color>>8) & 255) + ((color) & 255)) * (256/3)) >> 8;
	if (cl<5)
		return 0;
	else if (cl>=250)
		return 3<<6;
	int d = dither_2bpp_8x8[(x&7) | ( (y&7) << 3 )] - 1;

	cl = ( cl + d - 32 );
	if (cl<5)
		return 0;
	else if (cl>=250)
		return 3<<6;
	return cl & 0xc0;
}

int Dither4BitColor( int color, int x, int y )
{
	int cl = ((((color>>16) & 255) + ((color>>8) & 255) + ((color) & 255)) * (256/3)) >> 8;
	if (cl<5)
		return 0;
	else if (cl>=250)
		return 0xf0;
	int d = dither_2bpp_8x8[(x&7) | ( (y&7) << 3 )] - 1;

//	cl = ( cl + d - 32 );
	cl = ( cl + (d >> 1) - 16 );
	if (cl<5)
		return 0;
	else if (cl>=250)
		return 0xf0;
	return cl & 0xf0;
}

typedef struct {
	struct jpeg_source_mgr pub;   /* public fields */
	int len;
	JOCTET *buffer;      /* start of buffer */
	bool start_of_file;    /* have we gotten any data yet? */
} my_jpeg_source_mgr;

struct my_error_mgr {
	struct jpeg_error_mgr pub;	/* "public" fields */
	jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

void my_jpeg_error (j_common_ptr cinfo) {
	my_error_ptr myerr = (my_error_ptr) cinfo->err;

	char buffer[JMSG_LENGTH_MAX];

	/* Create the message */
	(*cinfo->err->format_message) (cinfo, buffer);

	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	//my_error_ptr myerr = (my_error_ptr) cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

void my_init_source (j_decompress_ptr cinfo)
{
	my_jpeg_source_mgr * src = (my_jpeg_source_mgr*) cinfo->src;
	src->start_of_file = true;
}

boolean my_fill_input_buffer (j_decompress_ptr cinfo)
{
	my_jpeg_source_mgr * src = (my_jpeg_source_mgr *) cinfo->src;

	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = src->len; 

	src->start_of_file = false;

	return true;
}

void my_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	my_jpeg_source_mgr * src = (my_jpeg_source_mgr *) cinfo->src;
	src->pub.next_input_byte += num_bytes;
	src->pub.bytes_in_buffer -= num_bytes; 
}

boolean my_resync_to_restart (j_decompress_ptr cinfo, int desired)
{
	return false;
}

void my_term_source (j_decompress_ptr cinfo)
{
	my_jpeg_source_mgr * src = (my_jpeg_source_mgr *) cinfo->src;
	if ( src && src->buffer )
	{
		delete[] src->buffer;
		src->buffer = NULL;
	}
	if(src)
		delete src;
}

	void
my_jpeg_src_free (j_decompress_ptr cinfo)
{
	my_jpeg_source_mgr * src = (my_jpeg_source_mgr *) cinfo->src;
	if ( src && src->buffer )
	{
		delete[] src->buffer;
		src->buffer = NULL;
	}
	delete src;
}

struct s_my_png {
	char *p;
	int size;
};

static void mypng_error_func (png_structp png, png_const_charp msg)
{
	longjmp(png_jmpbuf(png), 1);
}

static void mypng_warning_func (png_structp png, png_const_charp msg)
{
	longjmp(png_jmpbuf(png), 1);
}

static void mypng_read_func(png_structp png, png_bytep buf, png_size_t len)
{
	struct s_my_png *obj = (struct s_my_png *) png_get_io_ptr(png);
	size_t bytesRead = obj->size;

	if(bytesRead >= len)
		bytesRead = len;
	else
		longjmp(png_jmpbuf(png), 1);

	memcpy(buf, (const unsigned char*)obj->p, bytesRead);
	obj->p += bytesRead;
	obj->size -= bytesRead;
}


unsigned int ZLNXImageData::width() const {

	return myWidth;

}

unsigned int ZLNXImageData::height() const {

	return myHeight;

}

unsigned int ZLNXImageData::scanline() const {

	return myScanline;

}

void ZLNXImageData::init(unsigned int width, unsigned int height) {

	myWidth = width;
	myHeight = height;
	myScanline = (width+1) / 2;
	if (myImageData) free(myImageData);
	myImageData = (char*)malloc(myHeight * myScanline);
}

void ZLNXImageData::setPosition(unsigned int x, unsigned int y) {

	myX = x;
	myY = y;
	myPosition = myImageData + (x >> 1) + y * myScanline;
	myShift = (x & 1) << 2;
}

void ZLNXImageData::moveX(int delta) {

	setPosition(myX + delta, myY);

}

void ZLNXImageData::moveY(int delta) {

	setPosition(myX, myY + delta);

}

void ZLNXImageData::setPixel(unsigned char r, unsigned char g, unsigned char b) {

	int pixel = (r * 77 + g * 151 + b * 28) >> 8;
	*myPosition &= ~(0xf0 >> myShift);
	*myPosition |= ((pixel & 0xf0) >> myShift);
}

void ZLNXImageData::copyFrom(const ZLImageData &source, unsigned int targetX, unsigned int targetY) {

	char *c;
	char *c_src;
	int s, s_src;

	ZLNXImageData *source_image = (ZLNXImageData *)&source;
	int sW = source_image->width();
	int sH = source_image->height();
	int sL = source_image->scanline();
	char *src = source_image->getImageData();

	for(int j = 0; j < sH; j++)
		for(int i = 0; i < sW; i++) {
			c_src = src + i / 2 + sL * j;
			s_src = (i & 1) << 2;

			c = myImageData + (i + targetX) / 2 + myScanline * (j + (targetY - sH));
			s = ((i + targetX) & 1) << 2;

			*c &= ~(0xf0 >> s);
			*c |= (((*c_src << s_src) & 0xf0) >> s);
		}
}

shared_ptr<ZLImageData> ZLNXImageManager::createData() const {

	return new ZLNXImageData();

}

bool ZLNXImageManager::convertImageDirect(const std::string &stringData, ZLImageData &data) const {

	unsigned char m0, m1;
	m0 = *(stringData.data());
	m1 = *(stringData.data()+1);

	if((m0 == 0xff) && (m1 == 0xd8))
        {
                // We have some problems using this code. disable it
                convertImageDirectJpeg(stringData, data);
        }
        else if(!png_sig_cmp((unsigned char *)stringData.data(), (png_size_t)0, 4) )
		convertImageDirectPng(stringData, data);
	else if(!strncmp(stringData.c_str(), "GIF", 3))
		convertImageDirectGif(stringData, data);
	else if(!strncmp(stringData.c_str(), "BM", 2))
		convertImageDirectBmp(stringData, data);
	else {
		printf("unsupported image format: %d %d\n", m0, m1);
		return false;
	}

	return true;
}

void ZLNXImageManager::convertImageDirectJpeg(const std::string &stringData, ZLImageData &data) const {
	JSAMPARRAY buffer;		/* Output row buffer */
	int row_stride;		/* physical row width in output buffer */

        struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
        jpeg_error_mgr errmgr;

	cinfo.err = jpeg_std_error(&errmgr);
	errmgr.error_exit = my_jpeg_error;	

	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */

		my_jpeg_src_free (&cinfo);
		jpeg_destroy_decompress(&cinfo);
		return;
	}
        jpeg_create_decompress(&cinfo);

	my_jpeg_source_mgr *src;

	src = (my_jpeg_source_mgr *) new my_jpeg_source_mgr;
	cinfo.src = (struct jpeg_source_mgr *) src;

	src->len = stringData.length() + 2;
	src->buffer = new JOCTET[src->len];

	memcpy(src->buffer, (const unsigned char*)stringData.data(), stringData.length());
	src->buffer[stringData.length()] = (JOCTET) 0xFF;
	src->buffer[stringData.length() + 1] = (JOCTET) JPEG_EOI;

	src->pub.init_source = my_init_source;
	src->pub.fill_input_buffer = my_fill_input_buffer;
	src->pub.skip_input_data = my_skip_input_data;
	src->pub.resync_to_restart = my_resync_to_restart;
	src->pub.term_source = my_term_source;
	src->pub.bytes_in_buffer = src->len;
	src->pub.next_input_byte = src->buffer;

	(void) jpeg_read_header(&cinfo, true);

	cinfo.out_color_space = JCS_RGB;
	//    cinfo.out_color_space = JCS_GRAYSCALE;

	(void) jpeg_start_decompress(&cinfo);

	data.init(cinfo.output_width, cinfo.output_height);

        if (ZLNXPaintContext::lock_drawing) {
		//(void) jpeg_finish_decompress(&cinfo);
		jpeg_destroy_decompress(&cinfo);
		return;
	}

	row_stride = cinfo.output_width * cinfo.output_components;
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

	ZLNXImageData &nxdata = (ZLNXImageData &)data;
	char *idata = nxdata.getImageData();
	int iL = nxdata.scanline();

	unsigned char *p;
	char *c;	
	int pixel, s, j;
	while (cinfo.output_scanline < cinfo.output_height) {

		p = (unsigned char*)buffer[0];
		j = cinfo.output_scanline;

		(void) jpeg_read_scanlines(&cinfo, buffer, 1);

		for(int i = 0; i < cinfo.output_width; i++) {			
			pixel = Dither4BitColor( 
					p[0] << 16 | p[1] << 8 | p[2] << 0,
					i, j);

			c = idata + i / 2 + iL * j;
			s = (i & 1) << 2;

			*c &= ~(0xf0 >> s);
			*c |= (pixel >> s);

			p += 3;
		}
	}

	(void) jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
}

void ZLNXImageManager::convertImageDirectPng(const std::string &stringData, ZLImageData &data) const {

//fprintf(stderr, "convertImageDirectPng\n");

	struct s_my_png my_png;
	my_png.p = (char*)stringData.data();
	my_png.size = stringData.length();

	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	unsigned int *row = NULL;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
			(png_voidp)&my_png, mypng_error_func, mypng_warning_func);
	if ( !png_ptr )
		return;

	if (setjmp( png_ptr->jmpbuf )) {
		if (png_ptr)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
		}
		if ( row )
			delete row;
		return;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		mypng_error_func(png_ptr, "cannot create png info struct");
	png_set_read_fn(png_ptr,
			(voidp)&my_png, mypng_read_func);
	png_read_info( png_ptr, info_ptr );


	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	png_get_IHDR(png_ptr, info_ptr, &width, &height,
			&bit_depth, &color_type, &interlace_type,
			NULL, NULL);

	data.init(width, height);

        if (ZLNXPaintContext::lock_drawing) {
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
		return;
	}

	row = new unsigned int[ width ];

	// SET TRANSFORMS
	if (color_type & PNG_COLOR_MASK_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_gray_1_2_4_to_8(png_ptr);

	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);

	if (bit_depth == 16)
		png_set_strip_16(png_ptr);

	png_set_invert_alpha(png_ptr);

	if (bit_depth < 8)
		png_set_packing(png_ptr);

	png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);

	//if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	//png_set_swap_alpha(png_ptr);

	if (color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	int number_passes = png_set_interlace_handling(png_ptr);
	//if (color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
	//    color_type == PNG_COLOR_TYPE_GRAY_ALPHA)

	//if (color_type == PNG_COLOR_TYPE_RGB ||
	//    color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	png_set_bgr(png_ptr);

	ZLNXImageData &nxdata = (ZLNXImageData &)data;
	char *idata = nxdata.getImageData();
	int iL = nxdata.scanline();

	char *c;	
	unsigned int *p;
	int pixel, s;
	for(int pass = 0; pass < number_passes; pass++) {
		for(int y = 0; y < height; y++) {

			png_read_rows(png_ptr, (unsigned char **)&row, png_bytepp_NULL, 1);

			p = row;
			for(int i = 0; i < width; i++) {			

				int v = *p;
				v = (v & 0x80000000) ? 0xffffffff : v;
				pixel = Dither4BitColor(v, i, y);

				c = idata + i / 2 + iL * y;
				s = (i & 1) << 2;

				*c &= ~(0xf0 >> s);
				*c |= (pixel >> s);

				p++;
			}
		}
	}

	png_read_end(png_ptr, info_ptr);

	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
}

struct gif_info {
	int cpos;
	int length;
	const char *data;
};

int readGif(GifFileType *gf, GifByteType *buf, int len) {
	int length;

	struct gif_info *gi = (struct gif_info *)gf->UserData;

	if(gi->cpos == gi->length)
		return 0;

	length = len;
	if(gi->cpos + length > gi->length)
		length  = gi->length - gi->cpos;


	memcpy(buf, gi->data + gi->cpos, length);
	gi->cpos += length;

	return length;
}


void ZLNXImageManager::convertImageDirectGif(const std::string &stringData, ZLImageData &data) const {

//fprintf(stderr, "convertImageDirectGif\n");

	static std::map<std::string, ImageData> imgCache;

        if(stringData.length() < 500 && ! ZLNXPaintContext::lock_drawing) {
		std::map<std::string, ImageData>::const_iterator it = imgCache.find(stringData);
		if(it != imgCache.end()) {
			data.init(it->second.w, it->second.h);
			memcpy(((ZLNXImageData&)data).getImageData(), it->second.d, it->second.len);
			return;
		}
	}

    GifFileType *GifFile;

	struct gif_info gi;

	gi.length = stringData.length();
	gi.cpos = 0;
	gi.data = stringData.c_str();

	if((GifFile = DGifOpen((void*)&gi, readGif)) == NULL) {
		return;
	}
	
	data.init(GifFile->SWidth, GifFile->SHeight);

        if (ZLNXPaintContext::lock_drawing) {
		DGifCloseFile(GifFile);
		return;
	}

	ZLNXImageData &nxdata = (ZLNXImageData &)data;
	char *idata = nxdata.getImageData();
	int iL = nxdata.scanline();

    int 
		ColorMapSize = 0,
		BackGround = 0,
		InterlacedOffset[] = { 0, 4, 2, 1 }, /* The way Interlaced image should. */
	    InterlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */

    int	i, j, Error, NumFiles, Size, Row, Col, Width, Height, ExtCode, Count;
    GifRecordType RecordType;
    GifByteType *Extension;
    GifRowType *ScreenBuffer;
	ColorMapObject *ColorMap;

	int transparent = -1;
		
    ScreenBuffer = (GifRowType *) malloc(GifFile->SHeight * sizeof(GifRowType *));

    Size = GifFile->SWidth * sizeof(GifPixelType);/* Size in bytes one row.*/
    ScreenBuffer[0] = (GifRowType) malloc(Size); /* First row. */

    for (i = 0; i < GifFile->SWidth; i++)  /* Set its color to BackGround. */
		ScreenBuffer[0][i] = GifFile->SBackGroundColor;
    for (i = 1; i < GifFile->SHeight; i++) {
		/* Allocate the other rows, and set their color to background too: */
		ScreenBuffer[i] = (GifRowType) malloc(Size);
		memcpy(ScreenBuffer[i], ScreenBuffer[0], Size);
    }

    do {
		if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR) {
			memset(((ZLNXImageData&)data).getImageData(), 0x02, GifFile->SWidth * GifFile->SHeight / 4);
			DGifCloseFile(GifFile);
			return;			
		}

		if(RecordType == EXTENSION_RECORD_TYPE) {
			/* Skip any extension blocks in file: */
			if (DGifGetExtension(GifFile, &ExtCode, &Extension) == GIF_ERROR) {
				break;
			}
			if(ExtCode == GRAPHICS_EXT_FUNC_CODE)
				if(Extension[0] == 4 ) {
					int flag = Extension[1];
					transparent = (flag & 0x1) ? Extension[4] : -1;
				}

			while (Extension != NULL) {
				if (DGifGetExtensionNext(GifFile, &Extension) == GIF_ERROR) {
					break;
				}
/*			printf("ExtCode: %d, %d\n", ExtCode, GRAPHICS_EXT_FUNC_CODE);
				if(ExtCode == GRAPHICS_EXT_FUNC_CODE)
					if(Extension[0] == 4 ) {
						int flag = Extension[1];
						transparent = (flag & 0x1) ? Extension[4] : -1;
						}
*/
			}
		}
	} while((RecordType != IMAGE_DESC_RECORD_TYPE) && (RecordType != TERMINATE_RECORD_TYPE));
		
	DGifGetImageDesc(GifFile);

	Row = GifFile->Image.Top; /* Image Position relative to Screen. */
	Col = GifFile->Image.Left;
	Width = GifFile->Image.Width;
	Height = GifFile->Image.Height;

	if (GifFile->Image.Interlace) {
		/* Need to perform 4 passes on the images: */
		for (Count = i = 0; i < 4; i++)
			for (j = Row + InterlacedOffset[i]; j < Row + Height;
					j += InterlacedJumps[i])
				DGifGetLine(GifFile, &ScreenBuffer[j][Col], Width);
	} else
		for (i = 0; i < Height; i++)
			DGifGetLine(GifFile, &ScreenBuffer[Row++][Col], Width);

    /* Lets display it - set the global variables required and do it: */
    BackGround = GifFile->SBackGroundColor;
    ColorMap = (GifFile->Image.ColorMap
		? GifFile->Image.ColorMap
		: GifFile->SColorMap);
    ColorMapSize = ColorMap->ColorCount;
	
	char *c;	
	int p;
	int pixel, s;

    int MinIntensity, MaxIntensity, AvgIntensity;
    unsigned long ValueMask;
    GifColorType *ColorMapEntry = ColorMap->Colors;


	Width = GifFile->SWidth;
	Height = GifFile->SHeight;
	
	for (j = 0; j < Height; j++) {
		for (i = 0; i < Width; i++) {
			p = ScreenBuffer[j][i];
			//			p = ColorMapEntry[p].Red * 30 +
			//				ColorMapEntry[p].Green * 59 +
			//				ColorMapEntry[p].Blue * 11 > AvgIntensity;			

			if (p == transparent) {

				pixel = 0xf0;

			} else {

			unsigned int x = (ColorMapEntry[p].Red * 77 + ColorMapEntry[p].Green * 151 +
				ColorMapEntry[p].Blue * 28) >> 8;

/*
			if(x < 64)
				pixel = 0x00;
			else if(x <= 128)
				pixel = 0x40;
			else if(x <= 192)
				pixel = 0x80;
			else
				pixel = 0xc0;

*/
			pixel = x & 0xf0;

			}

			c = idata + i / 2 + iL * j;
			s = (i & 1) << 2;

			*c &= ~(0xf0 >> s);
			*c |= (pixel >> s);
		}
	}

	if(stringData.length() < 500) {
		ImageData id;

		ZLNXImageData &nxdata = (ZLNXImageData &)data;
		id.w = nxdata.width();
		id.h = nxdata.height();
		id.len = id.h * nxdata.scanline();
		id.d = (char*)malloc(id.len);
		memcpy(id.d, nxdata.getImageData(), id.len);

		imgCache.insert(std::pair<std::string, ImageData>(stringData, id));
	}

    for (i = GifFile->SHeight - 1 ; i >= 0 ; i--)
		free( ScreenBuffer[ i ] );
    free( ScreenBuffer );

	DGifCloseFile(GifFile);
}

int last_bit_set (int v)
{
	unsigned int i;
	for (i = sizeof (int) * 8 - 1; i > 0; --i) {
		if (v & (1L << i))
			return i;
	}
	return 0;
}

static void
rearrangePixels(unsigned char* buf, uint32 width, uint32 bit_count, int row, ZLImageData &data)
{
  char tmp;
  uint32 i;

	ZLNXImageData &nxdata = (ZLNXImageData &)data;
	char *idata = nxdata.getImageData();
	int iL = nxdata.scanline();

	char *c;	
	int pixel, s;
  
	printf("bit_count: %d\n", bit_count);
  switch(bit_count) {
    
  case 16:    /* FIXME: need a sample file */
    break;
    
  case 24:
    for (i = 0; i < width; i++, buf += 3) {

		unsigned int x = (buf[2] * 77 + buf[1] * 151 + buf[0] * 28) >> 8;


/*
		if(x < 64)
			pixel = 0x00;
		else if(x <= 128)
			pixel = 0x40;
		else if(x <= 192)
			pixel = 0x80;
		else
			pixel = 0xc0;

*/
		pixel = x & 0xf0;

		c = idata + i / 2 + iL * row;
		s = (i & 1) << 2;

		*c &= ~(0xf0 >> s);
		*c |= (pixel >> s);

    }
    break;
  
  case 32:
    {
      unsigned char* buf1 = buf;
      for (i = 0; i < width; i++, buf += 4) {

		unsigned int x = (buf[2] * 77 + buf[1] * 151 + buf[0] * 28) >> 8;

/*
		if(x < 64)
			pixel = 0x00;
		else if(x <= 128)
			pixel = 0x40;
		else if(x <= 192)
			pixel = 0x80;
		else
			pixel = 0xc0;
*/
		pixel = x & 0xf0;

		c = idata + i / 2 + iL * row;
		s = (i & 1) << 2;

		*c &= ~(0xf0 >> s);
		*c |= (pixel >> s);
      }
    }
    break;
    
  default:
    break;
  }
}

void ZLNXImageManager::convertImageDirectBmp(const std::string &stringData, ZLImageData &data) const {

//fprintf(stderr, "convertImageDirectBmp\n");


/*	int w, h;
	char *p;
	char *c;	
	int pixel, s;

	typedef union {
		char c[4];
		int i;
	} cInt;
	
	cInt ci;


	strncpy(ci.c, stringData.c_str() + 18, 4);
	w = ci.i;
	strncpy(ci.c, stringData.c_str() + 22, 4);
	h = ci.i;

	data.init(w, h);

        if (ZLNXPaintContext::lock_drawing) {
		return;
	}

	ZLNXImageData &nxdata = (ZLNXImageData &)data;
	char *idata = data.getImageData();
	int iL = data.scanline();

	p = (char*)stringData.c_str() + 54;

	for (int j = h; j > 0; j--) {
		for (int i = 0; i < w; i++) {
			
//			pixel = Dither4BitColor((p[0] << 16 | p[1] << 8 | p[2] << 0), i, j);


			unsigned int x = (buf[2] * 77 + buf[1] * 151 + buf[0] * 28) >> 8;


//			if(x < 64)
//				pixel = 0x00;
//			else if(x <= 128)
//				pixel = 0x40;
//			else if(x <= 192)
//				pixel = 0x80;
//			else
//				pixel = 0xc0;

			pixel = x & 0xf0;

			c = idata + i / 2 + iL * j;
			s = (i & 1) << 2;

			*c &= ~(0xf0 >> s);
			*c |= (pixel >> s);

			p += 3;
		}
		p += (w % 4);
	}
*/

	int xres, yres, w, h, spp, bps;
	char *idata;
	int iL;

	const char *p = stringData.c_str();
	struct stat instat;

	BMPFileHeader file_hdr;
	BMPInfoHeader info_hdr;
	enum BMPType bmp_type;

	uint32  clr_tbl_size, n_clr_elems = 3;
	unsigned char *clr_tbl;

	uint32	row, stride;

	unsigned char* xdata = 0;

	memcpy(file_hdr.bType, p, 2);

	ZLNXImageData &nxdata = (ZLNXImageData &)data;

	if(file_hdr.bType[0] != 'B' || file_hdr.bType[1] != 'M') {
		fprintf(stderr, "File is not a BMP\n");
		goto bad;
	}

	/* -------------------------------------------------------------------- */
	/*      Read the BMPFileHeader. We need iOffBits value only             */
	/* -------------------------------------------------------------------- */
	memcpy(&file_hdr.iOffBits, p+10, 4);

	file_hdr.iSize = stringData.length();

	/* -------------------------------------------------------------------- */
	/*      Read the BMPInfoHeader.                                         */
	/* -------------------------------------------------------------------- */

	memcpy(&info_hdr.iSize, p+BFH_SIZE, 4);

	if (info_hdr.iSize == BIH_WIN4SIZE)
		bmp_type = BMPT_WIN4;
	else if (info_hdr.iSize == BIH_OS21SIZE)
		bmp_type = BMPT_OS21;
	else if (info_hdr.iSize == BIH_OS22SIZE || info_hdr.iSize == 16)
		bmp_type = BMPT_OS22;
	else
		bmp_type = BMPT_WIN5;

	if (bmp_type == BMPT_WIN4 || bmp_type == BMPT_WIN5 || bmp_type == BMPT_OS22) {
		p = stringData.c_str() + BFH_SIZE + 4;
		memcpy(&info_hdr.iWidth, p, 4);
		p += 4;
		memcpy(&info_hdr.iHeight, p, 4);
		p += 4;
		memcpy(&info_hdr.iPlanes, p, 2);
		p += 2;
		memcpy(&info_hdr.iBitCount, p, 2);
		p += 2;
		memcpy(&info_hdr.iCompression, p, 4);
		p += 4;
		memcpy(&info_hdr.iSizeImage, p, 4);
		p += 4;
		memcpy(&info_hdr.iXPelsPerMeter, p, 4);
		p += 4;
		memcpy(&info_hdr.iYPelsPerMeter, p, 4);
		p += 4;
		memcpy(&info_hdr.iClrUsed, p, 4);
		p += 4;
		memcpy(&info_hdr.iClrImportant, p, 4);
		p += 4;
		memcpy(&info_hdr.iRedMask, p, 4);
		p += 4;
		memcpy(&info_hdr.iGreenMask, p, 4);
		p += 4;
		memcpy(&info_hdr.iBlueMask, p, 4);
		p += 4;
		memcpy(&info_hdr.iAlphaMask, p, 4);
		p += 4;
		n_clr_elems = 4;
		xres = ((double)info_hdr.iXPelsPerMeter * 2.54 + 0.05) / 100;
		yres = ((double)info_hdr.iYPelsPerMeter * 2.54 + 0.05) / 100;
	}

	if (bmp_type == BMPT_OS22) {
		/* 
		 * FIXME: different info in different documents
		 * regarding this!
		 */
		n_clr_elems = 3;
	}

	if (bmp_type == BMPT_OS21) {
		int16  iShort;

		memcpy(&iShort, p, 2);
		p += 2;
		info_hdr.iWidth = iShort;
		memcpy(&iShort, p, 2);
		p += 2;
		info_hdr.iHeight = iShort;
		memcpy(&iShort, p, 2);
		p += 2;
		info_hdr.iPlanes = iShort;
		memcpy(&iShort, p, 2);
		p += 2;
		info_hdr.iBitCount = iShort;
		info_hdr.iCompression = BMPC_RGB;
		n_clr_elems = 3;
	}

	if (info_hdr.iBitCount != 1  && info_hdr.iBitCount != 4  &&
			info_hdr.iBitCount != 8  && info_hdr.iBitCount != 16 &&
			info_hdr.iBitCount != 24 && info_hdr.iBitCount != 32) {
		fprintf(stderr, "Cannot process BMP file with bit count %d\n",
				info_hdr.iBitCount);
		//   close(fd);
		return;
	}

	w = info_hdr.iWidth;
	h = (info_hdr.iHeight > 0) ? info_hdr.iHeight : -info_hdr.iHeight;

	data.init(w, h);
	idata = nxdata.getImageData();
	iL = nxdata.scanline();

	switch (info_hdr.iBitCount)
	{
		case 1:
		case 4:
		case 8:
			spp = 1;
			bps = info_hdr.iBitCount;

			/* Allocate memory for colour table and read it. */
			if (info_hdr.iClrUsed)
				clr_tbl_size = ((uint32)(1 << bps) < info_hdr.iClrUsed) ?
					1 << bps : info_hdr.iClrUsed;
			else
				clr_tbl_size = 1 << bps;
			clr_tbl = (unsigned char *)
				_TIFFmalloc(n_clr_elems * clr_tbl_size);
			if (!clr_tbl) {
				fprintf(stderr, "Can't allocate space for color table\n");
				goto bad;
			}

			/*printf ("n_clr_elems: %d, clr_tbl_size: %d\n",
			  n_clr_elems, clr_tbl_size); */

			p = stringData.c_str() + BFH_SIZE + info_hdr.iSize;
			memcpy(clr_tbl, p, n_clr_elems * clr_tbl_size);

			/*for(clr = 0; clr < clr_tbl_size; ++clr) {
			  printf ("%d: r: %d g: %d b: %d\n",
			  clr,
			  clr_tbl[clr*n_clr_elems+2],
			  clr_tbl[clr*n_clr_elems+1],
			  clr_tbl[clr*n_clr_elems]);
			  }*/
			break;

		case 16:
		case 24:
			spp = 3;
			bps = info_hdr.iBitCount / spp;
			break;

		case 32:
			spp = 3;
			bps = 8;
			break;

		default:
			break;
	}

	stride = (w * spp * bps + 7) / 8;
	/*printf ("w: %d, h: %d, spp: %d, bps: %d, colorspace: %d\n",
	 *w, *h, *spp, *bps, info_hdr.iCompression); */

	// detect old style bitmask images
	if (info_hdr.iCompression == BMPC_RGB && info_hdr.iBitCount == 16)
	{
		/*printf ("implicit non-RGB image\n"); */
		info_hdr.iCompression = BMPC_BITFIELDS;
		info_hdr.iBlueMask = 0x1f;
		info_hdr.iGreenMask = 0x1f << 5;
		info_hdr.iRedMask = 0x1f << 10;
	}

	/* -------------------------------------------------------------------- */
	/*  Read uncompressed image data.                                       */
	/* -------------------------------------------------------------------- */

	switch (info_hdr.iCompression) {
		case BMPC_BITFIELDS:
			// we convert those to RGB for easier use
			bps = 8;
			stride = (w * spp * bps + 7) / 8;
		case BMPC_RGB:
			{
				uint32 file_stride = ((w * info_hdr.iBitCount + 7) / 8 + 3) / 4 * 4;

				/*printf ("bitcount: %d, stride: %d, file stride: %d\n",
				  info_hdr.iBitCount, stride, file_stride);

				  printf ("red mask: %x, green mask: %x, blue mask: %x\n",
				  info_hdr.iRedMask, info_hdr.iGreenMask, info_hdr.iBlueMask); */

				xdata = (unsigned char*)_TIFFmalloc (stride * h);

				if (!xdata) {
					fprintf(stderr, "Can't allocate space for image buffer\n");
					goto bad1;
				}

				for (row = 0; row < h; row++) {
					uint32 offset;

					if (info_hdr.iHeight > 0)
						offset = file_hdr.iOffBits + (h - row - 1) * file_stride;
					else
						offset = file_hdr.iOffBits + row * file_stride;

					//	if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
					//	  fprintf(stderr, "scanline %lu: Seek error\n", (unsigned long) row);
					//	}
					p = stringData.c_str() + offset;

					memcpy(xdata + stride*row, p, stride);

					// convert to RGB
					if (info_hdr.iCompression == BMPC_BITFIELDS)
					{

						unsigned char* row_ptr = xdata + stride*row;
						unsigned char* r16_ptr = row_ptr + file_stride - 2;
						unsigned char* rgb_ptr = row_ptr + stride - 3;

						int r_shift = last_bit_set (info_hdr.iRedMask) - 7;
						int g_shift = last_bit_set (info_hdr.iGreenMask) - 7;
						int b_shift = last_bit_set (info_hdr.iBlueMask) - 7;

						for (int i=0 ; rgb_ptr >= row_ptr; r16_ptr -= 2, rgb_ptr -= 3, i++)
						{
							int val = (r16_ptr[0] << 0) + (r16_ptr[1] << 8);
							if (r_shift > 0)
								rgb_ptr[0] = (val & info_hdr.iRedMask) >> r_shift;
							else
								rgb_ptr[0] = (val & info_hdr.iRedMask) << -r_shift;
							if (g_shift > 0)
								rgb_ptr[1] = (val & info_hdr.iGreenMask) >> g_shift;
							else
								rgb_ptr[1] = (val & info_hdr.iGreenMask) << -g_shift;
							if (b_shift > 0)
								rgb_ptr[2] = (val & info_hdr.iBlueMask) >> b_shift;
							else
								rgb_ptr[2] = (val & info_hdr.iBlueMask) << -b_shift;
						

							char *c;	
							int pixel, s;
							unsigned int x = (rgb_ptr[0] * 77 + rgb_ptr[1] * 151 + rgb_ptr[2] * 28) >> 8;

							/*
							if(x < 64)
								pixel = 0x00;
							else if(x <= 128)
								pixel = 0x40;
							else if(x <= 192)
								pixel = 0x80;
							else
								pixel = 0xc0;
							*/
							pixel = x & 0xf0;

							c = idata + i / 2 + iL * row;
							s = (i & 1) << 2;

							*c &= ~(0xf0 >> s);
							*c |= (pixel >> s);
						}
					}
					else if(info_hdr.iBitCount == 8) {
						unsigned char *b = xdata + stride * row;
						for (int i = 0; i < w; i++, b++) {
							char *c;	
							int pixel, s;
							unsigned int x = (clr_tbl[*b*n_clr_elems+2] * 77 + clr_tbl[*b*n_clr_elems+1] * 151 + clr_tbl[*b*n_clr_elems] * 28) >> 8;

							/*
							if(x < 64)
								pixel = 0x00;
							else if(x <= 128)
								pixel = 0x40;
							else if(x <= 192)
								pixel = 0x80;
							else
								pixel = 0xc0;
							*/
							pixel = x & 0xf0;

							c = idata + i / 2 + iL * row;
							s = (i & 1) << 2;

							*c &= ~(0xf0 >> s);
							*c |= (pixel >> s);
						}
					}
					else
						rearrangePixels(xdata + stride*row, w, info_hdr.iBitCount, row, data);
				}
			}
			break;

			/* -------------------------------------------------------------------- */
			/*  Read compressed image data.                                         */
			/* -------------------------------------------------------------------- */
		case BMPC_RLE4:
		case BMPC_RLE8:
			{
				uint32		i, j, k, runlength, x;
				uint32		compr_size, uncompr_size;
				unsigned char   *comprbuf;
				unsigned char   *uncomprbuf;

				//printf ("RLE%s compressed\n", info_hdr.iCompression == BMPC_RLE4 ? "4" : "8");

				compr_size = file_hdr.iSize - file_hdr.iOffBits;
				uncompr_size = w * h;

				comprbuf = (unsigned char *) _TIFFmalloc( compr_size );
				if (!comprbuf) {
					fprintf (stderr, "Can't allocate space for compressed scanline buffer\n");
					goto bad1;
				}
				uncomprbuf = (unsigned char *) _TIFFmalloc( uncompr_size );
				if (!uncomprbuf) {
					fprintf (stderr, "Can't allocate space for uncompressed scanline buffer\n");
					goto bad1;
				}


				p = stringData.c_str() + file_hdr.iOffBits;
				memcpy(comprbuf, p, compr_size);
				i = j = x = 0;

				while( j < uncompr_size && i < compr_size ) {
					if ( comprbuf[i] ) {
						runlength = comprbuf[i++];
						for ( k = 0;
								runlength > 0 && j < uncompr_size && i < compr_size && x < w;
								++k, ++x) {
							if (info_hdr.iBitCount == 8)
								uncomprbuf[j++] = comprbuf[i];
							else {
								if ( k & 0x01 )
									uncomprbuf[j++] = comprbuf[i] & 0x0F;
								else
									uncomprbuf[j++] = (comprbuf[i] & 0xF0) >> 4;
							}
							runlength--;
						}
						i++;
					} else {
						i++;
						if ( comprbuf[i] == 0 ) {         /* Next scanline */
							i++;
							x = 0;;
						}
						else if ( comprbuf[i] == 1 )    /* End of image */
							break;
						else if ( comprbuf[i] == 2 ) {  /* Move to... */
							i++;
							if ( i < compr_size - 1 ) {
								j += comprbuf[i] + comprbuf[i+1] * w;
								i += 2;
							}
							else
								break;
						} else {                         /* Absolute mode */
							runlength = comprbuf[i++];
							for ( k = 0; k < runlength && j < uncompr_size && i < compr_size; k++, x++)
							{
								if (info_hdr.iBitCount == 8)
									uncomprbuf[j++] = comprbuf[i++];
								else {
									if ( k & 0x01 )
										uncomprbuf[j++] = comprbuf[i++] & 0x0F;
									else
										uncomprbuf[j++] = (comprbuf[i] & 0xF0) >> 4;
								}
							}
							/* word boundary alignment */
							if (info_hdr.iBitCount == 4)
								k /= 2;
							if ( k & 0x01 )
								i++;
						}
					}
				}

				_TIFFfree(comprbuf);
				xdata = (unsigned char *) _TIFFmalloc( uncompr_size );
				if (!xdata) {
					fprintf (stderr, "Can't allocate space for final uncompressed scanline buffer\n");
					goto bad1;
				}

				// TODO: suboptimal, later improve the above to yield the corrent orientation natively
				for (row = 0; row < h; ++row) {
					memcpy (xdata + row * w, uncomprbuf + (h - 1 - row) * w, w);
					rearrangePixels(xdata + row * w, w, info_hdr.iBitCount, row, data);
				}

				_TIFFfree(uncomprbuf);
				bps = 8;
			}
			break;
	} /* switch */

	/* export the table */
//	*color_table = clr_tbl;
//	*color_table_size = clr_tbl_size;
//	*color_table_elements = n_clr_elems;
	goto bad;

bad1:
	if (clr_tbl)
		_TIFFfree(clr_tbl);
	clr_tbl = NULL;

bad:
	return;
}

