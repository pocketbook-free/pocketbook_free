#ifndef TOUCHZOOM_H_
#define TOUCHZOOM_H_

#include "pdfviewer.h"

namespace TouchZoom
{
	const int SC_DIRECT[] = { 33, 50, 75, 80, 90, 100, 110, 120, 130, 140, 150, 170, 200, 250, 300, 350, 400, 450, 500, -1 };
	void update_value(int* val, int d, const int* variants);
	void open_zoomer();
}

#endif /*TOUCHZOOM_H_*/
