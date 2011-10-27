#include "pdfviewer.h"

static int ax, ay, aw, ah, adx, ady, ascale, pw, ph, sw, sh;
static int mx, my, mw, mh;
static iv_handler prevhandler;

static void invert_square(int x, int y, int w, int h)
{

	InvertAreaBW(x, y, w, 2);
	InvertAreaBW(x, y + h - 2, w, 2);
	InvertAreaBW(x, y + 2, 2, h - 4);
	InvertAreaBW(x + w - 2, y + 2, 2, h - 4);

}

static void paint_area(int update)
{

	int x, y, w, h, rscale;

	if (update) invert_square(mx, my, mw, mh);


	rscale = (ascale * CSCALEADJUST) / 100;
	x = ax + (adx * aw) / pw;
	y = ay + (ady * ah) / ph;
	w = (aw * 100) / rscale;
	h = (w * sh) / sw;
	if (x + w >= sw) w = sw - x - 1;
	if (y + h >= sh) h = sh - y - 1;
	invert_square(x, y, w, h);

	if (update)
	{
		if (mx > x)
		{
			mw += (mx - x);
			mx = x;
		}
		if (my > y)
		{
			mh += (my - y);
			my = y;
		}
		if ((x - mx) + w > mw) mw = (x - mx) + w;
		if ((y - my) + h > mh) mh = (y - my) + h;
		PartialUpdateBW(mx, my, mw, mh);
	}

	mx = x;
	my = y;
	mw = w;
	mh = h;

}

static int area_handler(int type, int par1, int par2)
{

	int d, ssc;

	if (type != EVT_KEYPRESS && type != EVT_KEYREPEAT) return 0;
	d = (type == EVT_KEYREPEAT) ? 35 : 15;

	switch (par1)
	{

		case KEY_LEFT:
			adx -= (sw * d) / 100;
			if (adx < 0) adx = 0;
			break;

		case KEY_RIGHT:
			adx += (sw * d) / 100;
			if (adx + sw > pw) adx = pw - sw;
			if (adx < 0) adx = 0;
			break;

		case KEY_UP:
			ady -= (sh * d) / 100;
			if (ady < 0) ady = 0;
			break;

		case KEY_DOWN:
			ady += (sh * d) / 100;
			if (ady + sh > ph) ady = ph - sh;
			if (ady < 0) ady = 0;
			break;

		case KEY_PLUS:
			ssc = ascale;
			if (ascale > 200) ascale -= 50;
			if (ascale < 200) ascale = 200;
			goto recalc_p;

		case KEY_MINUS:
			ssc = ascale;
			if (ascale < 500) ascale += 50;
			if (ascale > 500) ascale = 500;
		recalc_p:
			pw = (pw * ascale) / ssc;
			ph = (ph * ascale) / ssc;
			adx = (adx * ascale) / ssc;
			ady = (ady * ascale) / ssc;
			if (adx + sw > pw) adx = pw - sw;
			if (ady + sh > ph) ady = ph - sh;
			if (adx < 0) adx = 0;
			if (ady < 0) ady = 0;
			break;

		case KEY_OK:
			scale = ascale;
			offx = adx;
			offy = ady;
			SetEventHandler(prevhandler);
			return 1;

		case KEY_BACK:
		case KEY_MENU:
		case KEY_MUSIC:
			SetEventHandler(prevhandler);
			return 1;

	}
	paint_area(1);
	return 0;

}


void area_selector()
{

	int marginx, marginy, row;
	double res;
	unsigned char* data;

	sw = ScreenWidth();
	sh = ScreenHeight() - panelh;

	getpagesize(cpage, &pw, &ph, &res, &marginx, &marginy);
	display_slice(cpage, scale, res, gFalse, 0, 0, pw, ph);

	ascale = scale;
	adx = offx;
	ady = offy;

	aw = sw;
	ah = (aw * ph) / pw;
	if (ah > sh - panelh)
	{
		ah = sh - panelh;
		aw = (ah * pw) / ph;
		if (aw > sw) aw = sw;

	}
	ax = (sw - aw) / 2;
	ay = ((sh - panelh) - ah) / 2;

	FillArea(0, 0, sw, sh, WHITE);
	get_bitmap_data(&data, &pw, &ph, &row);
	Stretch(data, USE4 ? IMAGE_GRAY4 : IMAGE_GRAY8, pw, ph, row, ax, ay, aw, ah, 0);

	paint_area(0);
	SoftUpdate();

	prevhandler = SetEventHandler(area_handler);
	SetKeyboardRate(700, 240);

}

