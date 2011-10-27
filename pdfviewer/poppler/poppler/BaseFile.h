//========================================================================
//
// BaseFile.h
//
// Copyright 1999 Derek B. Noonburg assigned by Michael Meeks.
//
//========================================================================

#ifndef BASEFILE_H
#define BASEFILE_H

#include <stdio.h>
#include <stdarg.h>
#include <inkview.h>

#include "Error.h"

typedef FILE * BaseFile;

static inline BaseFile
bxpdfopen (GooString *fileName1)
{
  GooString *fileName2;
  // try to open file
  fileName2 = NULL;
  BaseFile file;

#ifdef VMS
  if (!(file = iv_fopen(fileName->getCString(), "rb", "ctx=stm"))) {
    error(-1, "Couldn't open file '%s'", fileName->getCString());
    return NULL;
  }
#else
  if (!(file = iv_fopen(fileName1->getCString(), "rb"))) {
    fileName2 = fileName1->copy();
    fileName2->lowerCase();
    if (!(file = iv_fopen(fileName2->getCString(), "rb"))) {
      fileName2->upperCase();
      if (!(file = iv_fopen(fileName2->getCString(), "rb"))) {
	error(-1, "Couldn't open file '%s'", fileName1->getCString());
	delete fileName2;
	return NULL;
      }
    }
    delete fileName2;
  }
#endif
  return file;
}

static inline void
bfclose (BaseFile file)
{
  iv_fclose (file);
}

static inline size_t
bfread (void *ptr, size_t size, size_t nmemb, BaseFile file)
{
  return iv_fread (ptr, size, nmemb, file);
}

static inline int
bfseek (BaseFile file, long offset, int whence)
{
  return iv_fseek (file, offset, whence);
}

static inline void
brewind (BaseFile file)
{
  iv_rewind (file);
}

static inline long
bftell (BaseFile file)
{
  return iv_ftell (file);
}*/

#endif /* BASEFILE_H */


