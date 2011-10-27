//========================================================================
//
// GfxState.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef GFXSTATE_H
#define GFXSTATE_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "goo/gtypes.h"
#include "goo/FixedPoint.h"
#include "Object.h"
#include "Function.h"

class Array;
class GfxFont;
class PDFRectangle;
class GfxShading;

class Matrix {
public:
  FixedPoint m[6];

  GBool invertTo(Matrix *other);
  void transform(FixedPoint x, FixedPoint y, FixedPoint *tx, FixedPoint *ty);
};

//------------------------------------------------------------------------
// GfxBlendMode
//------------------------------------------------------------------------
 
enum GfxBlendMode {
  gfxBlendNormal,
  gfxBlendMultiply,
  gfxBlendScreen,
  gfxBlendOverlay,
  gfxBlendDarken,
  gfxBlendLighten,
  gfxBlendColorDodge,
  gfxBlendColorBurn,
  gfxBlendHardLight,
  gfxBlendSoftLight,
  gfxBlendDifference,
  gfxBlendExclusion,
  gfxBlendHue,
  gfxBlendSaturation,
  gfxBlendColor,
  gfxBlendLuminosity
};

//------------------------------------------------------------------------
// GfxColorComp
//------------------------------------------------------------------------

// 16.16 fixed point color component
typedef int GfxColorComp;

#define gfxColorComp1 0x10000

static inline GfxColorComp dblToCol(double x) {
  return (GfxColorComp)(x * gfxColorComp1);
}

static inline GfxColorComp fpToCol(FixedPoint x) {
//  return (GfxColorComp)(x * (FixedPoint)gfxColorComp1);
  return x.getRaw();
}

static inline double colToDbl(GfxColorComp x) {
  return (double)x / (double)gfxColorComp1;
}

static inline GfxColorComp byteToCol(Guchar x) {
  // (x / 255) << 16  =  (0.0000000100000001... * x) << 16
  //                  =  ((x << 8) + (x) + (x >> 8) + ...) << 16
  //                  =  (x << 8) + (x) + (x >> 7)
  //                                      [for rounding]
  return (GfxColorComp)((x << 8) + x + (x >> 7));
}

static inline Guchar colToByte(GfxColorComp x) {
  // 255 * x + 0.5  =  256 * x - x + 0x8000
  return (Guchar)(((x << 8) - x + 0x8000) >> 16);
}

//------------------------------------------------------------------------
// GfxColor
//------------------------------------------------------------------------

#define gfxColorMaxComps funcMaxOutputs

struct GfxColor {
  GfxColorComp c[gfxColorMaxComps];
};

//------------------------------------------------------------------------
// GfxGray
//------------------------------------------------------------------------

typedef GfxColorComp GfxGray;

//------------------------------------------------------------------------
// GfxRGB
//------------------------------------------------------------------------

struct GfxRGB {
  GfxColorComp r, g, b;
};

//------------------------------------------------------------------------
// GfxCMYK
//------------------------------------------------------------------------

struct GfxCMYK {
  GfxColorComp c, m, y, k;
};

//------------------------------------------------------------------------
// GfxColorSpace
//------------------------------------------------------------------------

// NB: The nGfxColorSpaceModes constant and the gfxColorSpaceModeNames
// array defined in GfxState.cc must match this enum.
enum GfxColorSpaceMode {
  csDeviceGray,
  csCalGray,
  csDeviceRGB,
  csCalRGB,
  csDeviceCMYK,
  csLab,
  csICCBased,
  csIndexed,
  csSeparation,
  csDeviceN,
  csPattern
};

class GfxColorSpace {
public:

  GfxColorSpace();
  virtual ~GfxColorSpace();
  virtual GfxColorSpace *copy() = 0;
  virtual GfxColorSpaceMode getMode() = 0;

  // Construct a color space.  Returns NULL if unsuccessful.
  static GfxColorSpace *parse(Object *csObj);

  // Convert to gray, RGB, or CMYK.
  virtual void getGray(GfxColor *color, GfxGray *gray) = 0;
  virtual void getRGB(GfxColor *color, GfxRGB *rgb) = 0;
  virtual void getCMYK(GfxColor *color, GfxCMYK *cmyk) = 0;
  virtual void getRGBLine(Guchar *in, unsigned int *out, int length);
  virtual void getGrayLine(Guchar *in, Guchar *out, int length);

  // Return the number of color components.
  virtual int getNComps() = 0;

  // Get this color space's default color.
  virtual void getDefaultColor(GfxColor *color) = 0;

  // Return the default ranges for each component, assuming an image
  // with a max pixel value of <maxImgPixel>.
  virtual void getDefaultRanges(FixedPoint *decodeLow, FixedPoint *decodeRange,
				int maxImgPixel);

  // Returns true if painting operations in this color space never
  // mark the page (e.g., the "None" colorant).
  virtual GBool isNonMarking() { return gFalse; }

  // Return the number of color space modes
  static int getNumColorSpaceModes();

  // Return the name of the <idx>th color space mode.
  static const char *getColorSpaceModeName(int idx);

private:
};

//------------------------------------------------------------------------
// GfxDeviceGrayColorSpace
//------------------------------------------------------------------------

class GfxDeviceGrayColorSpace: public GfxColorSpace {
public:

  GfxDeviceGrayColorSpace();
  virtual ~GfxDeviceGrayColorSpace();
  virtual GfxColorSpace *copy();
  virtual GfxColorSpaceMode getMode() { return csDeviceGray; }

  virtual void getGray(GfxColor *color, GfxGray *gray);
  virtual void getRGB(GfxColor *color, GfxRGB *rgb);
  virtual void getCMYK(GfxColor *color, GfxCMYK *cmyk);
  virtual void getGrayLine(Guchar *in, Guchar *out, int length);
  virtual void getRGBLine(Guchar *in, unsigned int *out, int length);

  virtual int getNComps() { return 1; }
  virtual void getDefaultColor(GfxColor *color);

private:
};

//------------------------------------------------------------------------
// GfxCalGrayColorSpace
//------------------------------------------------------------------------

class GfxCalGrayColorSpace: public GfxColorSpace {
public:

  GfxCalGrayColorSpace();
  virtual ~GfxCalGrayColorSpace();
  virtual GfxColorSpace *copy();
  virtual GfxColorSpaceMode getMode() { return csCalGray; }

  // Construct a CalGray color space.  Returns NULL if unsuccessful.
  static GfxColorSpace *parse(Array *arr);

  virtual void getGray(GfxColor *color, GfxGray *gray);
  virtual void getRGB(GfxColor *color, GfxRGB *rgb);
  virtual void getCMYK(GfxColor *color, GfxCMYK *cmyk);
  virtual void getGrayLine(Guchar *in, Guchar *out, int length);
  virtual void getRGBLine(Guchar *in, unsigned int *out, int length);

  virtual int getNComps() { return 1; }
  virtual void getDefaultColor(GfxColor *color);

  // CalGray-specific access.
  FixedPoint getWhiteX() { return whiteX; }
  FixedPoint getWhiteY() { return whiteY; }
  FixedPoint getWhiteZ() { return whiteZ; }
  FixedPoint getBlackX() { return blackX; }
  FixedPoint getBlackY() { return blackY; }
  FixedPoint getBlackZ() { return blackZ; }
  FixedPoint getGamma() { return gamma; }

private:

  FixedPoint whiteX, whiteY, whiteZ;    // white point
  FixedPoint blackX, blackY, blackZ;    // black point
  FixedPoint gamma;			    // gamma value
};

//------------------------------------------------------------------------
// GfxDeviceRGBColorSpace
//------------------------------------------------------------------------

class GfxDeviceRGBColorSpace: public GfxColorSpace {
public:

  GfxDeviceRGBColorSpace();
  virtual ~GfxDeviceRGBColorSpace();
  virtual GfxColorSpace *copy();
  virtual GfxColorSpaceMode getMode() { return csDeviceRGB; }

  virtual void getGray(GfxColor *color, GfxGray *gray);
  virtual void getRGB(GfxColor *color, GfxRGB *rgb);
  virtual void getCMYK(GfxColor *color, GfxCMYK *cmyk);
  virtual void getGrayLine(Guchar *in, Guchar *out, int length);
  virtual void getRGBLine(Guchar *in, unsigned int *out, int length);

  virtual int getNComps() { return 3; }
  virtual void getDefaultColor(GfxColor *color);

private:
};

//------------------------------------------------------------------------
// GfxCalRGBColorSpace
//------------------------------------------------------------------------

class GfxCalRGBColorSpace: public GfxColorSpace {
public:

  GfxCalRGBColorSpace();
  virtual ~GfxCalRGBColorSpace();
  virtual GfxColorSpace *copy();
  virtual GfxColorSpaceMode getMode() { return csCalRGB; }

  // Construct a CalRGB color space.  Returns NULL if unsuccessful.
  static GfxColorSpace *parse(Array *arr);

  virtual void getGray(GfxColor *color, GfxGray *gray);
  virtual void getRGB(GfxColor *color, GfxRGB *rgb);
  virtual void getCMYK(GfxColor *color, GfxCMYK *cmyk);
  virtual void getGrayLine(Guchar *in, Guchar *out, int length);
  virtual void getRGBLine(Guchar *in, unsigned int *out, int length);

  virtual int getNComps() { return 3; }
  virtual void getDefaultColor(GfxColor *color);

  // CalRGB-specific access.
  FixedPoint getWhiteX() { return whiteX; }
  FixedPoint getWhiteY() { return whiteY; }
  FixedPoint getWhiteZ() { return whiteZ; }
  FixedPoint getBlackX() { return blackX; }
  FixedPoint getBlackY() { return blackY; }
  FixedPoint getBlackZ() { return blackZ; }
  FixedPoint getGammaR() { return gammaR; }
  FixedPoint getGammaG() { return gammaG; }
  FixedPoint getGammaB() { return gammaB; }
  FixedPoint *getMatrix() { return mat; }

private:

  FixedPoint whiteX, whiteY, whiteZ;    // white point
  FixedPoint blackX, blackY, blackZ;    // black point
  FixedPoint gammaR, gammaG, gammaB;    // gamma values
  FixedPoint mat[9];		    // ABC -> XYZ transform matrix
};

//------------------------------------------------------------------------
// GfxDeviceCMYKColorSpace
//------------------------------------------------------------------------

class GfxDeviceCMYKColorSpace: public GfxColorSpace {
public:

  GfxDeviceCMYKColorSpace();
  virtual ~GfxDeviceCMYKColorSpace();
  virtual GfxColorSpace *copy();
  virtual GfxColorSpaceMode getMode() { return csDeviceCMYK; }

  virtual void getGray(GfxColor *color, GfxGray *gray);
  virtual void getRGB(GfxColor *color, GfxRGB *rgb);
  virtual void getCMYK(GfxColor *color, GfxCMYK *cmyk);

  virtual int getNComps() { return 4; }
  virtual void getDefaultColor(GfxColor *color);

private:
};

//------------------------------------------------------------------------
// GfxLabColorSpace
//------------------------------------------------------------------------

class GfxLabColorSpace: public GfxColorSpace {
public:

  GfxLabColorSpace();
  virtual ~GfxLabColorSpace();
  virtual GfxColorSpace *copy();
  virtual GfxColorSpaceMode getMode() { return csLab; }

  // Construct a Lab color space.  Returns NULL if unsuccessful.
  static GfxColorSpace *parse(Array *arr);

  virtual void getGray(GfxColor *color, GfxGray *gray);
  virtual void getRGB(GfxColor *color, GfxRGB *rgb);
  virtual void getCMYK(GfxColor *color, GfxCMYK *cmyk);

  virtual int getNComps() { return 3; }
  virtual void getDefaultColor(GfxColor *color);

  virtual void getDefaultRanges(FixedPoint *decodeLow, FixedPoint *decodeRange,
				int maxImgPixel);

  // Lab-specific access.
  FixedPoint getWhiteX() { return whiteX; }
  FixedPoint getWhiteY() { return whiteY; }
  FixedPoint getWhiteZ() { return whiteZ; }
  FixedPoint getBlackX() { return blackX; }
  FixedPoint getBlackY() { return blackY; }
  FixedPoint getBlackZ() { return blackZ; }
  FixedPoint getAMin() { return aMin; }
  FixedPoint getAMax() { return aMax; }
  FixedPoint getBMin() { return bMin; }
  FixedPoint getBMax() { return bMax; }

private:

  FixedPoint whiteX, whiteY, whiteZ;    // white point
  FixedPoint blackX, blackY, blackZ;    // black point
  FixedPoint aMin, aMax, bMin, bMax;    // range for the a and b components
  FixedPoint kr, kg, kb;		    // gamut mapping mulitpliers
};

//------------------------------------------------------------------------
// GfxICCBasedColorSpace
//------------------------------------------------------------------------

class GfxICCBasedColorSpace: public GfxColorSpace {
public:

  GfxICCBasedColorSpace(int nCompsA, GfxColorSpace *altA,
			Ref *iccProfileStreamA);
  virtual ~GfxICCBasedColorSpace();
  virtual GfxColorSpace *copy();
  virtual GfxColorSpaceMode getMode() { return csICCBased; }

  // Construct an ICCBased color space.  Returns NULL if unsuccessful.
  static GfxColorSpace *parse(Array *arr);

  virtual void getGray(GfxColor *color, GfxGray *gray);
  virtual void getRGB(GfxColor *color, GfxRGB *rgb);
  virtual void getCMYK(GfxColor *color, GfxCMYK *cmyk);

  virtual void getRGBLine(Guchar *in, unsigned int *out, int length);
  virtual int getNComps() { return nComps; }
  virtual void getDefaultColor(GfxColor *color);

  virtual void getDefaultRanges(FixedPoint *decodeLow, FixedPoint *decodeRange,
				int maxImgPixel);

  // ICCBased-specific access.
  GfxColorSpace *getAlt() { return alt; }

private:

  int nComps;			// number of color components (1, 3, or 4)
  GfxColorSpace *alt;		// alternate color space
  FixedPoint rangeMin[4];		// min values for each component
  FixedPoint rangeMax[4];		// max values for each component
  Ref iccProfileStream;		// the ICC profile
};

//------------------------------------------------------------------------
// GfxIndexedColorSpace
//------------------------------------------------------------------------

class GfxIndexedColorSpace: public GfxColorSpace {
public:

  GfxIndexedColorSpace(GfxColorSpace *baseA, int indexHighA);
  virtual ~GfxIndexedColorSpace();
  virtual GfxColorSpace *copy();
  virtual GfxColorSpaceMode getMode() { return csIndexed; }

  // Construct a Lab color space.  Returns NULL if unsuccessful.
  static GfxColorSpace *parse(Array *arr);

  virtual void getGray(GfxColor *color, GfxGray *gray);
  virtual void getRGB(GfxColor *color, GfxRGB *rgb);
  virtual void getCMYK(GfxColor *color, GfxCMYK *cmyk);
  virtual void getRGBLine(Guchar *in, unsigned int *out, int length);

  virtual int getNComps() { return 1; }
  virtual void getDefaultColor(GfxColor *color);

  virtual void getDefaultRanges(FixedPoint *decodeLow, FixedPoint *decodeRange,
				int maxImgPixel);

  // Indexed-specific access.
  GfxColorSpace *getBase() { return base; }
  int getIndexHigh() { return indexHigh; }
  Guchar *getLookup() { return lookup; }
  GfxColor *mapColorToBase(GfxColor *color, GfxColor *baseColor);

private:

  GfxColorSpace *base;		// base color space
  int indexHigh;		// max pixel value
  Guchar *lookup;		// lookup table
};

//------------------------------------------------------------------------
// GfxSeparationColorSpace
//------------------------------------------------------------------------

class GfxSeparationColorSpace: public GfxColorSpace {
public:

  GfxSeparationColorSpace(GooString *nameA, GfxColorSpace *altA,
			  Function *funcA);
  virtual ~GfxSeparationColorSpace();
  virtual GfxColorSpace *copy();
  virtual GfxColorSpaceMode getMode() { return csSeparation; }

  // Construct a Separation color space.  Returns NULL if unsuccessful.
  static GfxColorSpace *parse(Array *arr);

  virtual void getGray(GfxColor *color, GfxGray *gray);
  virtual void getRGB(GfxColor *color, GfxRGB *rgb);
  virtual void getCMYK(GfxColor *color, GfxCMYK *cmyk);

  virtual int getNComps() { return 1; }
  virtual void getDefaultColor(GfxColor *color);

  virtual GBool isNonMarking() { return nonMarking; }

  // Separation-specific access.
  GooString *getName() { return name; }
  GfxColorSpace *getAlt() { return alt; }
  Function *getFunc() { return func; }

private:

  GooString *name;		// colorant name
  GfxColorSpace *alt;		// alternate color space
  Function *func;		// tint transform (into alternate color space)
  GBool nonMarking;
};

//------------------------------------------------------------------------
// GfxDeviceNColorSpace
//------------------------------------------------------------------------

class GfxDeviceNColorSpace: public GfxColorSpace {
public:

  GfxDeviceNColorSpace(int nCompsA, GfxColorSpace *alt, Function *func);
  virtual ~GfxDeviceNColorSpace();
  virtual GfxColorSpace *copy();
  virtual GfxColorSpaceMode getMode() { return csDeviceN; }

  // Construct a DeviceN color space.  Returns NULL if unsuccessful.
  static GfxColorSpace *parse(Array *arr);

  virtual void getGray(GfxColor *color, GfxGray *gray);
  virtual void getRGB(GfxColor *color, GfxRGB *rgb);
  virtual void getCMYK(GfxColor *color, GfxCMYK *cmyk);

  virtual int getNComps() { return nComps; }
  virtual void getDefaultColor(GfxColor *color);

  virtual GBool isNonMarking() { return nonMarking; }

  // DeviceN-specific access.
  GooString *getColorantName(int i) { return names[i]; }
  GfxColorSpace *getAlt() { return alt; }
  Function *getTintTransformFunc() { return func; }

private:

  int nComps;			// number of components
  GooString			// colorant names
    *names[gfxColorMaxComps];
  GfxColorSpace *alt;		// alternate color space
  Function *func;		// tint transform (into alternate color space)
  GBool nonMarking;
};

//------------------------------------------------------------------------
// GfxPatternColorSpace
//------------------------------------------------------------------------

class GfxPatternColorSpace: public GfxColorSpace {
public:

  GfxPatternColorSpace(GfxColorSpace *underA);
  virtual ~GfxPatternColorSpace();
  virtual GfxColorSpace *copy();
  virtual GfxColorSpaceMode getMode() { return csPattern; }

  // Construct a Pattern color space.  Returns NULL if unsuccessful.
  static GfxColorSpace *parse(Array *arr);

  virtual void getGray(GfxColor *color, GfxGray *gray);
  virtual void getRGB(GfxColor *color, GfxRGB *rgb);
  virtual void getCMYK(GfxColor *color, GfxCMYK *cmyk);

  virtual int getNComps() { return 0; }
  virtual void getDefaultColor(GfxColor *color);

  // Pattern-specific access.
  GfxColorSpace *getUnder() { return under; }

private:

  GfxColorSpace *under;		// underlying color space (for uncolored
				//   patterns)
};

//------------------------------------------------------------------------
// GfxPattern
//------------------------------------------------------------------------

class GfxPattern {
public:

  GfxPattern(int typeA);
  virtual ~GfxPattern();

  static GfxPattern *parse(Object *obj);

  virtual GfxPattern *copy() = 0;

  int getType() { return type; }

private:

  int type;
};

//------------------------------------------------------------------------
// GfxTilingPattern
//------------------------------------------------------------------------

class GfxTilingPattern: public GfxPattern {
public:

  static GfxTilingPattern *parse(Object *patObj);
  virtual ~GfxTilingPattern();

  virtual GfxPattern *copy();

  int getPaintType() { return paintType; }
  int getTilingType() { return tilingType; }
  FixedPoint *getBBox() { return bbox; }
  FixedPoint getXStep() { return xStep; }
  FixedPoint getYStep() { return yStep; }
  Dict *getResDict()
    { return resDict.isDict() ? resDict.getDict() : (Dict *)NULL; }
  FixedPoint *getMatrix() { return matrix; }
  Object *getContentStream() { return &contentStream; }

private:

  GfxTilingPattern(int paintTypeA, int tilingTypeA,
		   FixedPoint *bboxA, FixedPoint xStepA, FixedPoint yStepA,
		   Object *resDictA, FixedPoint *matrixA,
		   Object *contentStreamA);

  int paintType;
  int tilingType;
  FixedPoint bbox[4];
  FixedPoint xStep, yStep;
  Object resDict;
  FixedPoint matrix[6];
  Object contentStream;
};

//------------------------------------------------------------------------
// GfxShadingPattern
//------------------------------------------------------------------------

class GfxShadingPattern: public GfxPattern {
public:

  static GfxShadingPattern *parse(Object *patObj);
  virtual ~GfxShadingPattern();

  virtual GfxPattern *copy();

  GfxShading *getShading() { return shading; }
  FixedPoint *getMatrix() { return matrix; }

private:

  GfxShadingPattern(GfxShading *shadingA, FixedPoint *matrixA);

  GfxShading *shading;
  FixedPoint matrix[6];
};

//------------------------------------------------------------------------
// GfxShading
//------------------------------------------------------------------------

class GfxShading {
public:

  GfxShading(int typeA);
  GfxShading(GfxShading *shading);
  virtual ~GfxShading();

  static GfxShading *parse(Object *obj);

  virtual GfxShading *copy() = 0;

  int getType() { return type; }
  GfxColorSpace *getColorSpace() { return colorSpace; }
  GfxColor *getBackground() { return &background; }
  GBool getHasBackground() { return hasBackground; }
  void getBBox(FixedPoint *xMinA, FixedPoint *yMinA, FixedPoint *xMaxA, FixedPoint *yMaxA)
    { *xMinA = xMin; *yMinA = yMin; *xMaxA = xMax; *yMaxA = yMax; }
  GBool getHasBBox() { return hasBBox; }

protected:

  GBool init(Dict *dict);

  int type;
  GfxColorSpace *colorSpace;
  GfxColor background;
  GBool hasBackground;
  FixedPoint xMin, yMin, xMax, yMax;
  GBool hasBBox;
};

//------------------------------------------------------------------------
// GfxFunctionShading
//------------------------------------------------------------------------

class GfxFunctionShading: public GfxShading {
public:

  GfxFunctionShading(FixedPoint x0A, FixedPoint y0A,
		     FixedPoint x1A, FixedPoint y1A,
		     FixedPoint *matrixA,
		     Function **funcsA, int nFuncsA);
  GfxFunctionShading(GfxFunctionShading *shading);
  virtual ~GfxFunctionShading();

  static GfxFunctionShading *parse(Dict *dict);

  virtual GfxShading *copy();

  void getDomain(FixedPoint *x0A, FixedPoint *y0A, FixedPoint *x1A, FixedPoint *y1A)
    { *x0A = x0; *y0A = y0; *x1A = x1; *y1A = y1; }
  FixedPoint *getMatrix() { return matrix; }
  int getNFuncs() { return nFuncs; }
  Function *getFunc(int i) { return funcs[i]; }
  void getColor(FixedPoint x, FixedPoint y, GfxColor *color);

private:

  FixedPoint x0, y0, x1, y1;
  FixedPoint matrix[6];
  Function *funcs[gfxColorMaxComps];
  int nFuncs;
};

//------------------------------------------------------------------------
// GfxAxialShading
//------------------------------------------------------------------------

class GfxAxialShading: public GfxShading {
public:

  GfxAxialShading(FixedPoint x0A, FixedPoint y0A,
		  FixedPoint x1A, FixedPoint y1A,
		  FixedPoint t0A, FixedPoint t1A,
		  Function **funcsA, int nFuncsA,
		  GBool extend0A, GBool extend1A);
  GfxAxialShading(GfxAxialShading *shading);
  virtual ~GfxAxialShading();

  static GfxAxialShading *parse(Dict *dict);

  virtual GfxShading *copy();

  void getCoords(FixedPoint *x0A, FixedPoint *y0A, FixedPoint *x1A, FixedPoint *y1A)
    { *x0A = x0; *y0A = y0; *x1A = x1; *y1A = y1; }
  FixedPoint getDomain0() { return t0; }
  FixedPoint getDomain1() { return t1; }
  GBool getExtend0() { return extend0; }
  GBool getExtend1() { return extend1; }
  int getNFuncs() { return nFuncs; }
  Function *getFunc(int i) { return funcs[i]; }
  void getColor(FixedPoint t, GfxColor *color);

private:

  FixedPoint x0, y0, x1, y1;
  FixedPoint t0, t1;
  Function *funcs[gfxColorMaxComps];
  int nFuncs;
  GBool extend0, extend1;
};

//------------------------------------------------------------------------
// GfxRadialShading
//------------------------------------------------------------------------

class GfxRadialShading: public GfxShading {
public:

  GfxRadialShading(FixedPoint x0A, FixedPoint y0A, FixedPoint r0A,
		   FixedPoint x1A, FixedPoint y1A, FixedPoint r1A,
		   FixedPoint t0A, FixedPoint t1A,
		   Function **funcsA, int nFuncsA,
		   GBool extend0A, GBool extend1A);
  GfxRadialShading(GfxRadialShading *shading);
  virtual ~GfxRadialShading();

  static GfxRadialShading *parse(Dict *dict);

  virtual GfxShading *copy();

  void getCoords(FixedPoint *x0A, FixedPoint *y0A, FixedPoint *r0A,
		 FixedPoint *x1A, FixedPoint *y1A, FixedPoint *r1A)
    { *x0A = x0; *y0A = y0; *r0A = r0; *x1A = x1; *y1A = y1; *r1A = r1; }
  FixedPoint getDomain0() { return t0; }
  FixedPoint getDomain1() { return t1; }
  GBool getExtend0() { return extend0; }
  GBool getExtend1() { return extend1; }
  int getNFuncs() { return nFuncs; }
  Function *getFunc(int i) { return funcs[i]; }
  void getColor(FixedPoint t, GfxColor *color);

private:

  FixedPoint x0, y0, r0, x1, y1, r1;
  FixedPoint t0, t1;
  Function *funcs[gfxColorMaxComps];
  int nFuncs;
  GBool extend0, extend1;
};

//------------------------------------------------------------------------
// GfxGouraudTriangleShading
//------------------------------------------------------------------------

struct GfxGouraudVertex {
  FixedPoint x, y;
  GfxColor color;
};

class GfxGouraudTriangleShading: public GfxShading {
public:

  GfxGouraudTriangleShading(int typeA,
			    GfxGouraudVertex *verticesA, int nVerticesA,
			    int (*trianglesA)[3], int nTrianglesA,
			    Function **funcsA, int nFuncsA);
  GfxGouraudTriangleShading(GfxGouraudTriangleShading *shading);
  virtual ~GfxGouraudTriangleShading();

  static GfxGouraudTriangleShading *parse(int typeA, Dict *dict, Stream *str);

  virtual GfxShading *copy();

  int getNTriangles() { return nTriangles; }
  void getTriangle(int i, FixedPoint *x0, FixedPoint *y0, GfxColor *color0,
		   FixedPoint *x1, FixedPoint *y1, GfxColor *color1,
		   FixedPoint *x2, FixedPoint *y2, GfxColor *color2);

private:

  GfxGouraudVertex *vertices;
  int nVertices;
  int (*triangles)[3];
  int nTriangles;
  Function *funcs[gfxColorMaxComps];
  int nFuncs;
};

//------------------------------------------------------------------------
// GfxPatchMeshShading
//------------------------------------------------------------------------

struct GfxPatch {
  FixedPoint x[4][4];
  FixedPoint y[4][4];
  GfxColor color[2][2];
};

class GfxPatchMeshShading: public GfxShading {
public:

  GfxPatchMeshShading(int typeA, GfxPatch *patchesA, int nPatchesA,
		      Function **funcsA, int nFuncsA);
  GfxPatchMeshShading(GfxPatchMeshShading *shading);
  virtual ~GfxPatchMeshShading();

  static GfxPatchMeshShading *parse(int typeA, Dict *dict, Stream *str);

  virtual GfxShading *copy();

  int getNPatches() { return nPatches; }
  GfxPatch *getPatch(int i) { return &patches[i]; }

private:

  GfxPatch *patches;
  int nPatches;
  Function *funcs[gfxColorMaxComps];
  int nFuncs;
};

//------------------------------------------------------------------------
// GfxImageColorMap
//------------------------------------------------------------------------

class GfxImageColorMap {
public:

  // Constructor.
  GfxImageColorMap(int bitsA, Object *decode, GfxColorSpace *colorSpaceA);

  // Destructor.
  ~GfxImageColorMap();

  // Return a copy of this color map.
  GfxImageColorMap *copy() { return new GfxImageColorMap(this); }

  // Is color map valid?
  GBool isOk() { return ok; }

  // Get the color space.
  GfxColorSpace *getColorSpace() { return colorSpace; }

  // Get stream decoding info.
  int getNumPixelComps() { return nComps; }
  int getBits() { return bits; }

  // Get decode table.
  FixedPoint getDecodeLow(int i) { return decodeLow[i]; }
  FixedPoint getDecodeHigh(int i) { return decodeLow[i] + decodeRange[i]; }

  // Convert an image pixel to a color.
  void getGray(Guchar *x, GfxGray *gray);
  void getRGB(Guchar *x, GfxRGB *rgb);
  void getRGBLine(Guchar *in, unsigned int *out, int length);
  void getGrayLine(Guchar *in, Guchar *out, int length);
  void getCMYK(Guchar *x, GfxCMYK *cmyk);
  void getColor(Guchar *x, GfxColor *color);

private:

  GfxImageColorMap(GfxImageColorMap *colorMap);

  GfxColorSpace *colorSpace;	// the image color space
  int bits;			// bits per component
  int nComps;			// number of components in a pixel
  GfxColorSpace *colorSpace2;	// secondary color space
  int nComps2;			// number of components in colorSpace2
  GfxColorComp *		// lookup table
    lookup[gfxColorMaxComps];
  Guchar *byte_lookup;
  Guchar *tmp_line;
  FixedPoint			// minimum values for each component
    decodeLow[gfxColorMaxComps];
  FixedPoint			// max - min value for each component
    decodeRange[gfxColorMaxComps];
  GBool ok;
};

//------------------------------------------------------------------------
// GfxSubpath and GfxPath
//------------------------------------------------------------------------

class GfxSubpath {
public:

  // Constructor.
  GfxSubpath(FixedPoint x1, FixedPoint y1);

  // Destructor.
  ~GfxSubpath();

  // Copy.
  GfxSubpath *copy() { return new GfxSubpath(this); }

  // Get points.
  int getNumPoints() { return n; }
  FixedPoint getX(int i) { return x[i]; }
  FixedPoint getY(int i) { return y[i]; }
  GBool getCurve(int i) { return curve[i]; }

  // Get last point.
  FixedPoint getLastX() { return x[n-1]; }
  FixedPoint getLastY() { return y[n-1]; }

  // Add a line segment.
  void lineTo(FixedPoint x1, FixedPoint y1);

  // Add a Bezier curve.
  void curveTo(FixedPoint x1, FixedPoint y1, FixedPoint x2, FixedPoint y2,
	       FixedPoint x3, FixedPoint y3);

  // Close the subpath.
  void close();
  GBool isClosed() { return closed; }

  // Add (<dx>, <dy>) to each point in the subpath.
  void offset(FixedPoint dx, FixedPoint dy);

private:

  FixedPoint *x, *y;		// points
  GBool *curve;			// curve[i] => point i is a control point
				//   for a Bezier curve
  int n;			// number of points
  int size;			// size of x/y arrays
  GBool closed;			// set if path is closed

  GfxSubpath(GfxSubpath *subpath);
};

class GfxPath {
public:

  // Constructor.
  GfxPath();

  // Destructor.
  ~GfxPath();

  // Copy.
  GfxPath *copy()
    { return new GfxPath(justMoved, firstX, firstY, subpaths, n, size); }

  // Is there a current point?
  GBool isCurPt() { return n > 0 || justMoved; }

  // Is the path non-empty, i.e., is there at least one segment?
  GBool isPath() { return n > 0; }

  // Get subpaths.
  int getNumSubpaths() { return n; }
  GfxSubpath *getSubpath(int i) { return subpaths[i]; }

  // Get last point on last subpath.
  FixedPoint getLastX() { return subpaths[n-1]->getLastX(); }
  FixedPoint getLastY() { return subpaths[n-1]->getLastY(); }

  // Move the current point.
  void moveTo(FixedPoint x, FixedPoint y);

  // Add a segment to the last subpath.
  void lineTo(FixedPoint x, FixedPoint y);

  // Add a Bezier curve to the last subpath
  void curveTo(FixedPoint x1, FixedPoint y1, FixedPoint x2, FixedPoint y2,
	       FixedPoint x3, FixedPoint y3);

  // Close the last subpath.
  void close();

  // Append <path> to <this>.
  void append(GfxPath *path);

  // Add (<dx>, <dy>) to each point in the path.
  void offset(FixedPoint dx, FixedPoint dy);

private:

  GBool justMoved;		// set if a new subpath was just started
  FixedPoint firstX, firstY;	// first point in new subpath
  GfxSubpath **subpaths;	// subpaths
  int n;			// number of subpaths
  int size;			// size of subpaths array

  GfxPath(GBool justMoved1, FixedPoint firstX1, FixedPoint firstY1,
	  GfxSubpath **subpaths1, int n1, int size1);
};

//------------------------------------------------------------------------
// GfxState
//------------------------------------------------------------------------

class GfxState {
public:

  // Construct a default GfxState, for a device with resolution <hDPI>
  // x <vDPI>, page box <pageBox>, page rotation <rotateA>, and
  // coordinate system specified by <upsideDown>.
  GfxState(FixedPoint hDPIA, FixedPoint vDPIA, PDFRectangle *pageBox,
	   int rotateA, GBool upsideDown);

  // Destructor.
  ~GfxState();

  // Copy.
  GfxState *copy() { return new GfxState(this); }

  // Accessors.
  FixedPoint getHDPI() { return hDPI; }
  FixedPoint getVDPI() { return vDPI; }
  FixedPoint *getCTM() { return ctm; }
  void getCTM(Matrix *m) { memcpy (m->m, ctm, sizeof m->m); }
  FixedPoint getX1() { return px1; }
  FixedPoint getY1() { return py1; }
  FixedPoint getX2() { return px2; }
  FixedPoint getY2() { return py2; }
  FixedPoint getPageWidth() { return pageWidth; }
  FixedPoint getPageHeight() { return pageHeight; }
  int getRotate() { return rotate; }
  GfxColor *getFillColor() { return &fillColor; }
  GfxColor *getStrokeColor() { return &strokeColor; }
  void getFillGray(GfxGray *gray)
    { fillColorSpace->getGray(&fillColor, gray); }
  void getStrokeGray(GfxGray *gray)
    { strokeColorSpace->getGray(&strokeColor, gray); }
  void getFillRGB(GfxRGB *rgb)
    { fillColorSpace->getRGB(&fillColor, rgb); }
  void getStrokeRGB(GfxRGB *rgb)
    { strokeColorSpace->getRGB(&strokeColor, rgb); }
  void getFillCMYK(GfxCMYK *cmyk)
    { fillColorSpace->getCMYK(&fillColor, cmyk); }
  void getStrokeCMYK(GfxCMYK *cmyk)
    { strokeColorSpace->getCMYK(&strokeColor, cmyk); }
  GfxColorSpace *getFillColorSpace() { return fillColorSpace; }
  GfxColorSpace *getStrokeColorSpace() { return strokeColorSpace; }
  GfxPattern *getFillPattern() { return fillPattern; }
  GfxPattern *getStrokePattern() { return strokePattern; }
  GfxBlendMode getBlendMode() { return blendMode; }
  FixedPoint getFillOpacity() { return fillOpacity; }
  FixedPoint getStrokeOpacity() { return strokeOpacity; }
  GBool getFillOverprint() { return fillOverprint; }
  GBool getStrokeOverprint() { return strokeOverprint; }
  Function **getTransfer() { return transfer; }
  FixedPoint getLineWidth() { return lineWidth; }
  void getLineDash(FixedPoint **dash, int *length, FixedPoint *start)
    { *dash = lineDash; *length = lineDashLength; *start = lineDashStart; }
  int getFlatness() { return flatness; }
  int getLineJoin() { return lineJoin; }
  int getLineCap() { return lineCap; }
  FixedPoint getMiterLimit() { return miterLimit; }
  GBool getStrokeAdjust() { return strokeAdjust; }
  GBool getAlphaIsShape() { return alphaIsShape; }
  GBool getTextKnockout() { return textKnockout; }
  GfxFont *getFont() { return font; }
  FixedPoint getFontSize() { return fontSize; }
  FixedPoint *getTextMat() { return textMat; }
  FixedPoint getCharSpace() { return charSpace; }
  FixedPoint getWordSpace() { return wordSpace; }
  FixedPoint getHorizScaling() { return horizScaling; }
  FixedPoint getLeading() { return leading; }
  FixedPoint getRise() { return rise; }
  int getRender() { return render; }
  GfxPath *getPath() { return path; }
  void setPath(GfxPath *pathA);
  FixedPoint getCurX() { return curX; }
  FixedPoint getCurY() { return curY; }
  void getClipBBox(FixedPoint *xMin, FixedPoint *yMin, FixedPoint *xMax, FixedPoint *yMax)
    { *xMin = clipXMin; *yMin = clipYMin; *xMax = clipXMax; *yMax = clipYMax; }
  void getUserClipBBox(FixedPoint *xMin, FixedPoint *yMin, FixedPoint *xMax, FixedPoint *yMax);
  FixedPoint getLineX() { return lineX; }
  FixedPoint getLineY() { return lineY; }

  // Is there a current point/path?
  GBool isCurPt() { return path->isCurPt(); }
  GBool isPath() { return path->isPath(); }

  // Transforms.
  void transform(FixedPoint x1, FixedPoint y1, FixedPoint *x2, FixedPoint *y2)
    { *x2 = ctm[0] * x1 + ctm[2] * y1 + ctm[4];
      *y2 = ctm[1] * x1 + ctm[3] * y1 + ctm[5]; }
  void transformDelta(FixedPoint x1, FixedPoint y1, FixedPoint *x2, FixedPoint *y2)
    { *x2 = ctm[0] * x1 + ctm[2] * y1;
      *y2 = ctm[1] * x1 + ctm[3] * y1; }
  void textTransform(FixedPoint x1, FixedPoint y1, FixedPoint *x2, FixedPoint *y2)
    { *x2 = textMat[0] * x1 + textMat[2] * y1 + textMat[4];
      *y2 = textMat[1] * x1 + textMat[3] * y1 + textMat[5]; }
  void textTransformDelta(FixedPoint x1, FixedPoint y1, FixedPoint *x2, FixedPoint *y2)
    { *x2 = textMat[0] * x1 + textMat[2] * y1;
      *y2 = textMat[1] * x1 + textMat[3] * y1; }
  FixedPoint transformWidth(FixedPoint w);
  FixedPoint getTransformedLineWidth()
    { return transformWidth(lineWidth); }
  FixedPoint getTransformedFontSize();
  void getFontTransMat(FixedPoint *m11, FixedPoint *m12, FixedPoint *m21, FixedPoint *m22);

  // Change state parameters.
  void setCTM(FixedPoint a, FixedPoint b, FixedPoint c,
	      FixedPoint d, FixedPoint e, FixedPoint f);
  void concatCTM(FixedPoint a, FixedPoint b, FixedPoint c,
		 FixedPoint d, FixedPoint e, FixedPoint f);
  void shiftCTM(FixedPoint tx, FixedPoint ty);
  void setFillColorSpace(GfxColorSpace *colorSpace);
  void setStrokeColorSpace(GfxColorSpace *colorSpace);
  void setFillColor(GfxColor *color) { fillColor = *color; }
  void setStrokeColor(GfxColor *color) { strokeColor = *color; }
  void setFillPattern(GfxPattern *pattern);
  void setStrokePattern(GfxPattern *pattern);
  void setBlendMode(GfxBlendMode mode) { blendMode = mode; }
  void setFillOpacity(FixedPoint opac) { fillOpacity = opac; }
  void setStrokeOpacity(FixedPoint opac) { strokeOpacity = opac; }
  void setFillOverprint(GBool op) { fillOverprint = op; }
  void setStrokeOverprint(GBool op) { strokeOverprint = op; }
  void setTransfer(Function **funcs);
  void setLineWidth(FixedPoint width) { lineWidth = width; }
  void setLineDash(FixedPoint *dash, int length, FixedPoint start);
  void setFlatness(int flatness1) { flatness = flatness1; }
  void setLineJoin(int lineJoin1) { lineJoin = lineJoin1; }
  void setLineCap(int lineCap1) { lineCap = lineCap1; }
  void setMiterLimit(FixedPoint limit) { miterLimit = limit; }
  void setStrokeAdjust(GBool sa) { strokeAdjust = sa; }
  void setAlphaIsShape(GBool ais) { alphaIsShape = ais; }
  void setTextKnockout(GBool tk) { textKnockout = tk; }
  void setFont(GfxFont *fontA, FixedPoint fontSizeA);
  void setFontSize(FixedPoint fontSize1) { fontSize = fontSize1; }
  void setTextMat(FixedPoint a, FixedPoint b, FixedPoint c,
		  FixedPoint d, FixedPoint e, FixedPoint f)
    { textMat[0] = a; textMat[1] = b; textMat[2] = c;
      textMat[3] = d; textMat[4] = e; textMat[5] = f; }
  void setCharSpace(FixedPoint space)
    { charSpace = space; }
  void setWordSpace(FixedPoint space)
    { wordSpace = space; }
  void setHorizScaling(FixedPoint scale)
    { horizScaling = (FixedPoint)0.01 * scale; }
  void setLeading(FixedPoint leadingA)
    { leading = leadingA; }
  void setRise(FixedPoint riseA)
    { rise = riseA; }
  void setRender(int renderA)
    { render = renderA; }

  // Add to path.
  void moveTo(FixedPoint x, FixedPoint y)
    { path->moveTo(curX = x, curY = y); }
  void lineTo(FixedPoint x, FixedPoint y)
    { path->lineTo(curX = x, curY = y); }
  void curveTo(FixedPoint x1, FixedPoint y1, FixedPoint x2, FixedPoint y2,
	       FixedPoint x3, FixedPoint y3)
    { path->curveTo(x1, y1, x2, y2, curX = x3, curY = y3); }
  void closePath()
    { path->close(); curX = path->getLastX(); curY = path->getLastY(); }
  void clearPath();

  // Update clip region.
  void clip();
  void clipToStrokePath();

  // Text position.
  void textSetPos(FixedPoint tx, FixedPoint ty) { lineX = tx; lineY = ty; }
  void textMoveTo(FixedPoint tx, FixedPoint ty)
    { lineX = tx; lineY = ty; textTransform(tx, ty, &curX, &curY); }
  void textShift(FixedPoint tx, FixedPoint ty);
  void shift(FixedPoint dx, FixedPoint dy);

  // Push/pop GfxState on/off stack.
  GfxState *save();
  GfxState *restore();
  GBool hasSaves() { return saved != NULL; }

  // Misc
  GBool parseBlendMode(Object *obj, GfxBlendMode *mode);

private:

  FixedPoint hDPI, vDPI;		// resolution
  FixedPoint ctm[6];		// coord transform matrix
  FixedPoint px1, py1, px2, py2;	// page corners (user coords)
  FixedPoint pageWidth, pageHeight;	// page size (pixels)
  int rotate;			// page rotation angle

  GfxColorSpace *fillColorSpace;   // fill color space
  GfxColorSpace *strokeColorSpace; // stroke color space
  GfxColor fillColor;		// fill color
  GfxColor strokeColor;		// stroke color
  GfxPattern *fillPattern;	// fill pattern
  GfxPattern *strokePattern;	// stroke pattern
  GfxBlendMode blendMode;	// transparency blend mode
  FixedPoint fillOpacity;		// fill opacity
  FixedPoint strokeOpacity;		// stroke opacity
  GBool fillOverprint;		// fill overprint
  GBool strokeOverprint;	// stroke overprint
  Function *transfer[4];	// transfer function (entries may be: all
				//   NULL = identity; last three NULL =
				//   single function; all four non-NULL =
				//   R,G,B,gray functions)

  FixedPoint lineWidth;		// line width
  FixedPoint *lineDash;		// line dash
  int lineDashLength;
  FixedPoint lineDashStart;
  int flatness;			// curve flatness
  int lineJoin;			// line join style
  int lineCap;			// line cap style
  FixedPoint miterLimit;		// line miter limit
  GBool strokeAdjust;		// stroke adjustment
  GBool alphaIsShape;		// alpha is shape
  GBool textKnockout;		// text knockout

  GfxFont *font;		// font
  FixedPoint fontSize;		// font size
  FixedPoint textMat[6];		// text matrix
  FixedPoint charSpace;		// character spacing
  FixedPoint wordSpace;		// word spacing
  FixedPoint horizScaling;		// horizontal scaling
  FixedPoint leading;		// text leading
  FixedPoint rise;			// text rise
  int render;			// text rendering mode

  GfxPath *path;		// array of path elements
  FixedPoint curX, curY;		// current point (user coords)
  FixedPoint lineX, lineY;		// start of current text line (text coords)

  FixedPoint clipXMin, clipYMin,	// bounding box for clip region
         clipXMax, clipYMax;

  GfxState *saved;		// next GfxState on stack

  GfxState(GfxState *state);
};

#endif
