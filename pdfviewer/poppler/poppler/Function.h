//========================================================================
//
// Function.h
//
// Copyright 2001-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef FUNCTION_H
#define FUNCTION_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "goo/gtypes.h"
#include "Object.h"

class Dict;
class Stream;
struct PSObject;
class PSStack;

//------------------------------------------------------------------------
// Function
//------------------------------------------------------------------------

#define funcMaxInputs        32
#define funcMaxOutputs       32
#define sampledFuncMaxInputs 16

class Function {
public:

  Function();

  virtual ~Function();

  // Construct a function.  Returns NULL if unsuccessful.
  static Function *parse(Object *funcObj);

  // Initialize the entries common to all function types.
  GBool init(Dict *dict);

  virtual Function *copy() = 0;

  // Return the function type:
  //   -1 : identity
  //    0 : sampled
  //    2 : exponential
  //    3 : stitching
  //    4 : PostScript
  virtual int getType() = 0;

  // Return size of input and output tuples.
  int getInputSize() { return m; }
  int getOutputSize() { return n; }

  FixedPoint getDomainMin(int i) { return domain[i][0]; }
  FixedPoint getDomainMax(int i) { return domain[i][1]; }
  FixedPoint getRangeMin(int i) { return range[i][0]; }
  FixedPoint getRangeMax(int i) { return range[i][1]; }
  GBool getHasRange() { return hasRange; }

  // Transform an input tuple into an output tuple.
  virtual void transform(FixedPoint *in, FixedPoint *out) = 0;

  virtual GBool isOk() = 0;

protected:

  int m, n;			// size of input and output tuples
  FixedPoint			// min and max values for function domain
    domain[funcMaxInputs][2];
  FixedPoint			// min and max values for function range
    range[funcMaxOutputs][2];
  GBool hasRange;		// set if range is defined
};

//------------------------------------------------------------------------
// IdentityFunction
//------------------------------------------------------------------------

class IdentityFunction: public Function {
public:

  IdentityFunction();
  virtual ~IdentityFunction();
  virtual Function *copy() { return new IdentityFunction(); }
  virtual int getType() { return -1; }
  virtual void transform(FixedPoint *in, FixedPoint *out);
  virtual GBool isOk() { return gTrue; }

private:
};

//------------------------------------------------------------------------
// SampledFunction
//------------------------------------------------------------------------

class SampledFunction: public Function {
public:

  SampledFunction(Object *funcObj, Dict *dict);
  virtual ~SampledFunction();
  virtual Function *copy() { return new SampledFunction(this); }
  virtual int getType() { return 0; }
  virtual void transform(FixedPoint *in, FixedPoint *out);
  virtual GBool isOk() { return ok; }

  int getSampleSize(int i) { return sampleSize[i]; }
  FixedPoint getEncodeMin(int i) { return encode[i][0]; }
  FixedPoint getEncodeMax(int i) { return encode[i][1]; }
  FixedPoint getDecodeMin(int i) { return decode[i][0]; }
  FixedPoint getDecodeMax(int i) { return decode[i][1]; }
  FixedPoint *getSamples() { return samples; }

private:

  SampledFunction(SampledFunction *func);

  int				// number of samples for each domain element
    sampleSize[funcMaxInputs];
  FixedPoint			// min and max values for domain encoder
    encode[funcMaxInputs][2];
  FixedPoint			// min and max values for range decoder
    decode[funcMaxOutputs][2];
  FixedPoint			// input multipliers
    inputMul[funcMaxInputs];
  int idxMul[funcMaxInputs];	// sample array index multipliers
  FixedPoint *samples;		// the samples
  int nSamples;			// size of the samples array
  FixedPoint *sBuf;			// buffer for the transform function
  GBool ok;
};

//------------------------------------------------------------------------
// ExponentialFunction
//------------------------------------------------------------------------

class ExponentialFunction: public Function {
public:

  ExponentialFunction(Object *funcObj, Dict *dict);
  virtual ~ExponentialFunction();
  virtual Function *copy() { return new ExponentialFunction(this); }
  virtual int getType() { return 2; }
  virtual void transform(FixedPoint *in, FixedPoint *out);
  virtual GBool isOk() { return ok; }

  FixedPoint *getC0() { return c0; }
  FixedPoint *getC1() { return c1; }
  FixedPoint getE() { return e; }

private:

  ExponentialFunction(ExponentialFunction *func);

  FixedPoint c0[funcMaxOutputs];
  FixedPoint c1[funcMaxOutputs];
  FixedPoint e;
  GBool ok;
};

//------------------------------------------------------------------------
// StitchingFunction
//------------------------------------------------------------------------

class StitchingFunction: public Function {
public:

  StitchingFunction(Object *funcObj, Dict *dict);
  virtual ~StitchingFunction();
  virtual Function *copy() { return new StitchingFunction(this); }
  virtual int getType() { return 3; }
  virtual void transform(FixedPoint *in, FixedPoint *out);
  virtual GBool isOk() { return ok; }

  int getNumFuncs() { return k; }
  Function *getFunc(int i) { return funcs[i]; }
  FixedPoint *getBounds() { return bounds; }
  FixedPoint *getEncode() { return encode; }
  FixedPoint *getScale() { return scale; }

private:

  StitchingFunction(StitchingFunction *func);

  int k;
  Function **funcs;
  FixedPoint *bounds;
  FixedPoint *encode;
  FixedPoint *scale;
  GBool ok;
};

//------------------------------------------------------------------------
// PostScriptFunction
//------------------------------------------------------------------------

class PostScriptFunction: public Function {
public:

  PostScriptFunction(Object *funcObj, Dict *dict);
  virtual ~PostScriptFunction();
  virtual Function *copy() { return new PostScriptFunction(this); }
  virtual int getType() { return 4; }
  virtual void transform(FixedPoint *in, FixedPoint *out);
  virtual GBool isOk() { return ok; }

  GooString *getCodeString() { return codeString; }

private:

  PostScriptFunction(PostScriptFunction *func);
  GBool parseCode(Stream *str, int *codePtr);
  GooString *getToken(Stream *str);
  void resizeCode(int newSize);
  void exec(PSStack *stack, int codePtr);

  GooString *codeString;
  PSObject *code;
  int codeSize;
  GBool ok;
};

#endif
