//========================================================================
//
// FixedPoint.h
//
// Fixed point type, with C++ operators.
//
// Copyright 2004 Glyph & Cog, LLC
//
//========================================================================

#ifndef FIXEDPOINT_H
#define FIXEDPOINT_H

#include <poppler/poppler-config.h>

#if USE_FIXEDPOINT

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <stdio.h>
#include <stdlib.h>
#include "gtypes.h"

#define fixptShift 16
#define fixptMaskL ((1 << fixptShift) - 1)
#define fixptMaskH (~fixptMaskL)

typedef long long FixPtInt64;

class FixedPoint {
public:

  FixedPoint() { val = 0; }
  FixedPoint(const FixedPoint &x) { val = x.val; }
  FixedPoint(double x) { val = (int)(x * (1 << fixptShift) + 0.5); }
  FixedPoint(int x) { val = x << fixptShift; }
  FixedPoint(long x) { val = x << fixptShift; }

  inline operator float()
    { return (float) val * ((float)1 / (float)(1 << fixptShift)); }
  inline operator double()
    { return (double) val * (1.0 / (double)(1 << fixptShift)); }
  inline operator int()
    { return val >> fixptShift; }

  int getRaw() { return val; }

  inline FixedPoint operator =(FixedPoint x) { val = x.val; return *this; }

  inline int operator ==(FixedPoint x) { return val == x.val; }
  inline int operator ==(double x) { return *this == (FixedPoint)x; }
  inline int operator ==(int x) { return *this == (FixedPoint)x; }
  inline int operator ==(long x) { return *this == (FixedPoint)x; }

  inline int operator !=(FixedPoint x) { return val != x.val; }
  inline int operator !=(double x) { return *this != (FixedPoint)x; }
  inline int operator !=(int x) { return *this != (FixedPoint)x; }
  inline int operator !=(long x) { return *this != (FixedPoint)x; }

  inline int operator <(FixedPoint x) { return val < x.val; }
  inline int operator <(double x) { return *this < (FixedPoint)x; }
  inline int operator <(int x) { return *this < (FixedPoint)x; }
  inline int operator <(long x) { return *this < (FixedPoint)x; }

  inline int operator <=(FixedPoint x) { return val <= x.val; }
  inline int operator <=(double x) { return *this <= (FixedPoint)x; }
  inline int operator <=(int x) { return *this <= (FixedPoint)x; }
  inline int operator <=(long x) { return *this <= (FixedPoint)x; }

  inline int operator >(FixedPoint x) { return val > x.val; }
  inline int operator >(double x) { return *this > (FixedPoint)x; }
  inline int operator >(int x) { return *this > (FixedPoint)x; }
  inline int operator >(long x) { return *this > (FixedPoint)x; }

  inline int operator >=(FixedPoint x) { return val >= x.val; }
  inline int operator >=(double x) { return *this >= (FixedPoint)x; }
  inline int operator >=(int x) { return *this >= (FixedPoint)x; }
  inline int operator >=(long x) { return *this >= (FixedPoint)x; }

  inline FixedPoint operator -() { return make(-val); }

  inline FixedPoint operator +(FixedPoint x) { return make(val + x.val); }
  inline FixedPoint operator +(double x) { return *this + (FixedPoint)x; }
  inline FixedPoint operator +(int x) { return *this + (FixedPoint)x; }
  inline FixedPoint operator +(long x) { return *this + (FixedPoint)x; }

  inline FixedPoint operator +=(FixedPoint x) { val = val + x.val; return *this; }
  inline FixedPoint operator +=(double x) { return *this += (FixedPoint)x; }
  inline FixedPoint operator +=(int x) { return *this += (FixedPoint)x; }
  inline FixedPoint operator +=(long x) { return *this += (FixedPoint)x; }

  inline FixedPoint operator -(FixedPoint x) { return make(val - x.val); }
  inline FixedPoint operator -(double x) { return *this - (FixedPoint)x; }
  inline FixedPoint operator -(int x) { return *this - (FixedPoint)x; }
  inline FixedPoint operator -(long x) { return *this - (FixedPoint)x; }

  inline FixedPoint operator -=(FixedPoint x) { val = val - x.val; return *this; }
  inline FixedPoint operator -=(double x) { return *this -= (FixedPoint)x; }
  inline FixedPoint operator -=(int x) { return *this -= (FixedPoint)x; }
  inline FixedPoint operator -=(long x) { return *this -= (FixedPoint)x; }

  inline FixedPoint operator *(FixedPoint x) { return make(mul(val, x.val)); }
  inline FixedPoint operator *(double x) { return *this * (FixedPoint)x; }
  inline FixedPoint operator *(int x) { return *this * (FixedPoint)x; }
  inline FixedPoint operator *(long x) { return *this * (FixedPoint)x; }
  
  inline FixedPoint operator *=(FixedPoint x) { val = mul(val, x.val); return *this; }
  inline FixedPoint operator *=(double x) { return *this *= (FixedPoint)x; }
  inline FixedPoint operator *=(int x) { return *this *= (FixedPoint)x; }
  inline FixedPoint operator *=(long x) { return *this *= (FixedPoint)x; }

  inline FixedPoint operator /(FixedPoint x) { return make(div(val, x.val)); }
  inline FixedPoint operator /(double x) { return *this / (FixedPoint)x; }
  inline FixedPoint operator /(int x) { return *this / (FixedPoint)x; }
  inline FixedPoint operator /(long x) { return *this / (FixedPoint)x; }

  inline FixedPoint operator /=(FixedPoint x) { val = div(val, x.val); return *this; }
  inline FixedPoint operator /=(double x) { return *this /= (FixedPoint)x; }
  inline FixedPoint operator /=(int x) { return *this /= (FixedPoint)x; }
  inline FixedPoint operator /=(long x) { return *this /= (FixedPoint)x; }

  static inline FixedPoint abs(FixedPoint x) { return make(::abs(x.val)); }

  static inline int floor(FixedPoint x) { return x.val >> fixptShift; }

  static inline int ceil(FixedPoint x)
    { return (x.val & fixptMaskL) ? ((x.val >> fixptShift) + 1)
	                          : (x.val >> fixptShift); }

  static inline int round(FixedPoint x)
    { return (x.val + (1 << (fixptShift - 1))) >> fixptShift; }

  static FixedPoint sqrt(FixedPoint x);

  static FixedPoint pow(FixedPoint x, FixedPoint y);

  // Compute *result = x/y; return false if there is an underflow or
  // overflow.
  static GBool divCheck(FixedPoint x, FixedPoint y, FixedPoint *result);

  static inline FixedPoint make(int valA) { FixedPoint x; x.val = valA; return x; }

  inline int mul(int x, int y){
#if 1 //~tmp
    return ((FixPtInt64)x * y) >> fixptShift;
#else
    int ah0, ah, bh, al, bl;
    ah0 = x & fixptMaskH;
    ah = x >> fixptShift;
    al = x - ah0;
    bh = y >> fixptShift;
    bl = y - (bh << fixptShift);
    return ah0 * bh + ah * bl + al * bh + ((al * bl) >> fixptShift);
#endif
  }

inline int div(int x, int y) {
#if 1 //~tmp
    FixPtInt64 r = ((FixPtInt64)(x ^ y) << 32) | 0x7fffffffffffffffLL;
    if (y != 0) r = ((FixPtInt64)x << fixptShift) / y;
    return r;
#else
#endif
  }

private:

  int val;			// 16.16 fixed point
};

#endif // USE_FIXEDPOINT

#endif
