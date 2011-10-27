#include "pdfviewer.h"

extern "C" const ibitmap pdfmode;

const int SC_PREVIEW[] = { 33, 50, -1 };
const int SC_NORMAL[] = { 75, 80, 85, 90, 95, 100, 105, 110, 115, 120, 130, 140, 150, 160, 170, 180, -1 };
const int SC_COLUMNS[] = { 200, 300, 400, 500, -1 };
const int SC_REFLOW[] = { 150, 200, 300, 400, 500, -1 };
const int SC_DIRECT[] = { 33, 50, 75, 80, 85, 90, 95, 100, 105, 110, 115, 120, 130, 140, 150, 160, 170, 180, 200, 250, 300, 350, 400, 450, 500, -1 };

static iv_handler prevhandler;
static ibitmap* bm_pdfmode = NULL;
static ibitmap* isaves;
static ifont* zoom_font = NULL;
static int dx, dy, dw, dh, th;
static int new_scale, new_rscale, new_reflow;
static int pos;

/*  0:preview  1:fit  2:normal  3:columns  4:reflow  */

void update_value(int* val, int d, const int* variants)
{

	int i, n, mind = 9999999, ci = 0, cd;

	for (n = 0; variants[n] != -1; n++) ;

	for (i = 0; i < n; i++)
	{
		cd = *val - variants[i];
		if (cd < 0) cd = -cd;
		if (cd < mind)
		{
			mind = cd;
			ci = i;
		}
	}

	ci += d;
	if (ci < 0) ci = n - 1;
	if (ci >= n) ci = 0;
	*val = variants[ci];

}

static void invert_item(int x, int y, int w, int h)
{

	InvertAreaBW(x, y + 1, w, h - 2);
	InvertAreaBW(x + 1, y, w - 2, 1);
	InvertAreaBW(x + 1, y + h - 1, w - 2, 1);
	
}

static void draw_new_zoomer(int update)
{

	char buf[80];
	int cw, ch, n;

	iv_windowframe(dx, dy, dw, dh, BLACK, WHITE, "@Zoom", 0);
	DrawBitmap(dx + 14, dy + th + 8, bm_pdfmode);
	cw = bm_pdfmode->width / 5;
	ch = bm_pdfmode->height;
	invert_item(dx + 14 + cw * pos, dy + th + 8, cw, ch);

	switch (pos)
	{
		case 0:
			update_value(&new_scale, 0, SC_PREVIEW);
			if (orient == 0 || orient == 3)
			{
				n = (new_scale == 50) ? 4 : 9;
			}
			else
			{
				n = (new_scale == 50) ? 2 : 6;
			}
			snprintf(buf, sizeof(buf),
					 "%s: %i%c %s", GetLangText("@Preview"), n, 0x16, GetLangText("@pages"));
			break;
		case 1:
			snprintf(buf, sizeof(buf),
					 "%s", GetLangText("@Fit_width"));
			break;
		case 2:
			if (new_scale >= 200) new_scale = 100;
			update_value(&new_scale, 0, SC_NORMAL);
			snprintf(buf, sizeof(buf),
					 "%s: %i%%%c", GetLangText("@Normal_page"), new_scale, 0x16);
			break;
		case 3:
			if (new_scale < 200) new_scale = 200;
			update_value(&new_scale, 0, SC_COLUMNS);
			n = (new_scale + 50) / 100;
			snprintf(buf, sizeof(buf),
					 "%s: %i%c", GetLangText("@Columns"), n, 0x16);
			break;
		case 4:
			update_value(&new_rscale, 0, SC_REFLOW);
			snprintf(buf, sizeof(buf),
					 "%s: %i%%%c", GetLangText("@Reflow"), new_rscale, 0x16);
			break;
	}
	SetFont(zoom_font, 0);
	DrawTextRect(dx + 2, dy + th + 8 + ch + 3, dw - 4, 1, buf, ALIGN_CENTER);
	if (update == 1) PartialUpdate(dx, dy, dw + 4, dh + 4);
	if (update == 2) PartialUpdateBW(dx, dy, dw + 4, dh + 4);

}

static void close_zoomer(int update)
{

	DrawBitmap(dx, dy, isaves);
	if (update) PartialUpdate(dx, dy, isaves->width, isaves->height);
	free(isaves);
	isaves = NULL;
	iv_seteventhandler(prevhandler);
	SetKeyboardRate(700, 500);

}

static void change_value(int d)
{

	switch (pos)
	{
		case 0:
			update_value(&new_scale, d, SC_PREVIEW);
			break;
		case 1:
			break;
		case 2:
			update_value(&new_scale, d, SC_NORMAL);
			break;
		case 3:
			update_value(&new_scale, d, SC_COLUMNS);
			break;
		case 4:
			update_value(&new_rscale, d, SC_REFLOW);
			break;
	}

}

static int newzoomer_handler(int type, int par1, int par2)
{

	if (type != EVT_KEYPRESS && type != EVT_KEYREPEAT) return 0;

	switch (par1)
	{

		case KEY_LEFT:
			if (--pos < 0) pos = 4;
			new_reflow = (pos == 4) ? 1 : 0;
			new_scale = scale;
			draw_new_zoomer(2);
			break;

		case KEY_RIGHT:
			if (++pos > 4) pos = 0;
			new_reflow = (pos == 4) ? 1 : 0;
			new_scale = scale;
			draw_new_zoomer(2);
			break;

		case KEY_UP:
		case KEY_DOWN:
			change_value((par1 == KEY_UP) ? 1 : -1);
			draw_new_zoomer(2);
			break;

		case KEY_OK:
			close_zoomer(0);
			// ... for "fit width", calculate new zoom
			reflow_mode = new_reflow;
			if (pos == 1)
			{
				scale = get_fit_scale();
			}
			else
			{
				scale = new_scale;
			}
			rscale = new_rscale;
			if (new_reflow != reflow_mode) subpage = 0;
			out_page(1);
			break;

		case KEY_BACK:
		case KEY_MENU:
			close_zoomer(1);
			return 1;

	}
	return 0;

}

void open_mini_zoomer()
{

	if (!bm_pdfmode) bm_pdfmode = GetResource("pdfmode", (ibitmap*)&pdfmode);
	if (!zoom_font) zoom_font = OpenFont(DEFAULTFONT, 20, 0);

	th = header_font->height + 4;
	dw = bm_pdfmode->width + 28;
	dh = th + bm_pdfmode->height + zoom_font->height + 20;
	dx = (ScreenWidth() - dw) / 2;
	dy = (ScreenHeight() - dh) / 2;

	isaves = BitmapFromScreen(dx, dy, dw + 4, dh + 4);
	DimArea(dx + 4, dy + 4, dw, dh, BLACK);

	new_scale = scale;
	new_rscale = rscale;
	new_reflow = reflow_mode;
	if (reflow_mode)
	{
		pos = 4;
	}
	else if (scale <= 50)
	{
		pos = 0;
	}
	else if (scale >= 200)
	{
		pos = 3;
	}
	else
	{
		pos = 2;
	}

	prevhandler = iv_seteventhandler(newzoomer_handler);
	//if (ivstate.needupdate)
	//{
	//	draw_new_zoomer(0);
	//	SoftUpdate();
	//}
	//else
	//{
	//	draw_new_zoomer(1);
	//}
	draw_new_zoomer(0);
}

