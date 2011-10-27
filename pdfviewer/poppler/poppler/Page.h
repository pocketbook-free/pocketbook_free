//========================================================================
//
// Page.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef PAGE_H
#define PAGE_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "Object.h"

class Dict;
class XRef;
class OutputDev;
class Links;
class Catalog;
class Annots;
class Annot;
class Gfx;
class FormPageWidgets;
class Form;

//------------------------------------------------------------------------

class PDFRectangle {
public:
  FixedPoint x1, y1, x2, y2;

  PDFRectangle() { x1 = y1 = x2 = y2 = 0; }
  PDFRectangle(FixedPoint x1A, FixedPoint y1A, FixedPoint x2A, FixedPoint y2A)
    { x1 = x1A; y1 = y1A; x2 = x2A; y2 = y2A; }
  GBool isValid() { return x1 != 0 || y1 != 0 || x2 != 0 || y2 != 0; }
  void clipTo(PDFRectangle *rect);
};

//------------------------------------------------------------------------
// PageAttrs
//------------------------------------------------------------------------

class PageAttrs {
public:

  // Construct a new PageAttrs object by merging a dictionary
  // (of type Pages or Page) into another PageAttrs object.  If
  // <attrs> is NULL, uses defaults.
  PageAttrs(PageAttrs *attrs, Dict *dict);

  // Destructor.
  ~PageAttrs();

  // Accessors.
  PDFRectangle *getMediaBox() { return &mediaBox; }
  PDFRectangle *getCropBox() { return &cropBox; }
  GBool isCropped() { return haveCropBox; }
  PDFRectangle *getBleedBox() { return &bleedBox; }
  PDFRectangle *getTrimBox() { return &trimBox; }
  PDFRectangle *getArtBox() { return &artBox; }
  int getRotate() { return rotate; }
  GooString *getLastModified()
    { return lastModified.isString()
	? lastModified.getString() : (GooString *)NULL; }
  Dict *getBoxColorInfo()
    { return boxColorInfo.isDict() ? boxColorInfo.getDict() : (Dict *)NULL; }
  Dict *getGroup()
    { return group.isDict() ? group.getDict() : (Dict *)NULL; }
  Stream *getMetadata()
    { return metadata.isStream() ? metadata.getStream() : (Stream *)NULL; }
  Dict *getPieceInfo()
    { return pieceInfo.isDict() ? pieceInfo.getDict() : (Dict *)NULL; }
  Dict *getSeparationInfo()
    { return separationInfo.isDict()
	? separationInfo.getDict() : (Dict *)NULL; }
  Dict *getResourceDict()
    { return resources.isDict() ? resources.getDict() : (Dict *)NULL; }

private:

  GBool readBox(Dict *dict, const char *key, PDFRectangle *box);

  PDFRectangle mediaBox;
  PDFRectangle cropBox;
  GBool haveCropBox;
  PDFRectangle bleedBox;
  PDFRectangle trimBox;
  PDFRectangle artBox;
  int rotate;
  Object lastModified;
  Object boxColorInfo;
  Object group;
  Object metadata;
  Object pieceInfo;
  Object separationInfo;
  Object resources;
};

//------------------------------------------------------------------------
// Page
//------------------------------------------------------------------------

class Page {
public:

  // Constructor.
  Page(XRef *xrefA, int numA, Dict *pageDict, PageAttrs *attrsA, Form *form);

  // Destructor.
  ~Page();

  // Is page valid?
  GBool isOk() { return ok; }

  // Get page parameters.
  int getNum() { return num; }
  PDFRectangle *getMediaBox() { return attrs->getMediaBox(); }
  PDFRectangle *getCropBox() { return attrs->getCropBox(); }
  GBool isCropped() { return attrs->isCropped(); }
  FixedPoint getMediaWidth() 
    { return attrs->getMediaBox()->x2 - attrs->getMediaBox()->x1; }
  FixedPoint getMediaHeight()
    { return attrs->getMediaBox()->y2 - attrs->getMediaBox()->y1; }
  FixedPoint getCropWidth() 
    { return attrs->getCropBox()->x2 - attrs->getCropBox()->x1; }
  FixedPoint getCropHeight()
    { return attrs->getCropBox()->y2 - attrs->getCropBox()->y1; }
  PDFRectangle *getBleedBox() { return attrs->getBleedBox(); }
  PDFRectangle *getTrimBox() { return attrs->getTrimBox(); }
  PDFRectangle *getArtBox() { return attrs->getArtBox(); }
  int getRotate() { return attrs->getRotate(); }
  GooString *getLastModified() { return attrs->getLastModified(); }
  Dict *getBoxColorInfo() { return attrs->getBoxColorInfo(); }
  Dict *getGroup() { return attrs->getGroup(); }
  Stream *getMetadata() { return attrs->getMetadata(); }
  Dict *getPieceInfo() { return attrs->getPieceInfo(); }
  Dict *getSeparationInfo() { return attrs->getSeparationInfo(); }

  // Get resource dictionary.
  Dict *getResourceDict() { return attrs->getResourceDict(); }

  // Get annotations array.
  Object *getAnnots(Object *obj) { return annots.fetch(xref, obj); }

  // Return a list of links.
  Links *getLinks(Catalog *catalog);

  // Return a list of annots. Ownership is transferred to the caller.
  Annots *getAnnots(Catalog *catalog);

  // Get contents.
  Object *getContents(Object *obj) { return contents.fetch(xref, obj); }

  // Get thumb.
  Object *getThumb(Object *obj) { return thumb.fetch(xref, obj); }
  GBool loadThumb(unsigned char **data, int *width, int *height, int *rowstride);

  // Get transition.
  Object *getTrans(Object *obj) { return trans.fetch(xref, obj); }

  // Get form.
  FormPageWidgets *getPageWidgets() { return pageWidgets; }

  // Get duration, the maximum length of time, in seconds,
  // that the page is displayed before the presentation automatically
  // advances to the next page
  FixedPoint getDuration() { return duration; }

  // Get actions
  Object *getActions(Object *obj) { return actions.fetch(xref, obj); }

  Gfx *createGfx(OutputDev *out, FixedPoint hDPI, FixedPoint vDPI,
		 int rotate, GBool useMediaBox, GBool crop,
		 int sliceX, int sliceY, int sliceW, int sliceH,
		 GBool printing, Catalog *catalog,
		 GBool (*abortCheckCbk)(void *data),
		 void *abortCheckCbkData,
		 GBool (*annotDisplayDecideCbk)(Annot *annot, void *user_data),
		 void *annotDisplayDecideCbkData);

  // Display a page.
  void display(OutputDev *out, FixedPoint hDPI, FixedPoint vDPI,
	       int rotate, GBool useMediaBox, GBool crop,
	       GBool printing, Catalog *catalog,
	       GBool (*abortCheckCbk)(void *data) = NULL,
	       void *abortCheckCbkData = NULL,
               GBool (*annotDisplayDecideCbk)(Annot *annot, void *user_data) = NULL,
               void *annotDisplayDecideCbkData = NULL);

  // Display part of a page.
  void displaySlice(OutputDev *out, FixedPoint hDPI, FixedPoint vDPI,
		    int rotate, GBool useMediaBox, GBool crop,
		    int sliceX, int sliceY, int sliceW, int sliceH,
		    GBool printing, Catalog *catalog,
		    GBool (*abortCheckCbk)(void *data) = NULL,
		    void *abortCheckCbkData = NULL,
                    GBool (*annotDisplayDecideCbk)(Annot *annot, void *user_data) = NULL,
                    void *annotDisplayDecideCbkData = NULL);

  void display(Gfx *gfx);

  void makeBox(FixedPoint hDPI, FixedPoint vDPI, int rotate,
	       GBool useMediaBox, GBool upsideDown,
	       FixedPoint sliceX, FixedPoint sliceY, FixedPoint sliceW, FixedPoint sliceH,
	       PDFRectangle *box, GBool *crop);

  void processLinks(OutputDev *out, Catalog *catalog);

  // Get the page's default CTM.
  void getDefaultCTM(FixedPoint *ctm, FixedPoint hDPI, FixedPoint vDPI,
		     int rotate, GBool useMediaBox, GBool upsideDown);

private:

  XRef *xref;			// the xref table for this PDF file
  int num;			// page number
  PageAttrs *attrs;		// page attributes
  Object annots;		// annotations array
  Object contents;		// page contents
  FormPageWidgets *pageWidgets; 			// the form for that page
  Object thumb;			// page thumbnail
  Object trans;			// page transition
  Object actions;		// page addiction actions
  FixedPoint duration;              // page duration
  GBool ok;			// true if page is valid
};

#endif
