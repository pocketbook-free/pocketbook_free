#include "pdfviewer.h"
#include "TouchZoom.h"

extern "C" const ibitmap touchzoom;

namespace TouchZoom
{
	const int SC_PREVIEW[] =
	{ 33, 50, -1 };
	const int SC_NORMAL[] =
	{ 75, 80, 85, 90, 95, 100, 105, 110, 120, 130, 140, 150, 170, -1 };
	const int SC_COLUMNS[] =
	{ 200, 300, 400, 500, -1 };
	const int SC_REFLOW[] =
	{ 150, 200, 300, 400, 500, -1 };

	static iv_handler prevhandler;
	static ibitmap* bm_touchzoom = NULL;
	static ibitmap* isaves;
	static int dx, dy, dw, dh, th;
	static int new_scale, new_rscale, new_reflow;
	bool need_fit_scale;
	//  static int pos;
	ifont* menu_arrow_font;
	/*  0:preview  1:fit  2:normal  3:columns  4:reflow  */

	void update_value(int* val, int d, const int* variants)
	{
		int i, n, mind = 9999999, ci = 0, cd;

		for (n = 0; variants[n] != -1; n++)
			;

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

	static bool get_new_mode_from_coord(int x, int y)
	{
		need_fit_scale = false;
		int cw = bm_touchzoom->width / 6 - 36, ch = bm_touchzoom->height / 5 - 48;
		for (int i = 0; i < 6; ++i)
			if (x > dx + 40 + i *(cw + 36) && x < dx + 40 + (i + 1) *(cw + 36)) for (int j = 0; j < 5; ++j)
					if (y > dy + th + 8 + j *(ch + 48) && y < dy + th + 8 + (j + 1) *(ch + 48))
					{
						switch (j)
						{
							case 4:
								if (i > 4) return false;
								new_rscale = SC_REFLOW[i];
								new_reflow = 1;
								break;

							case 3:
								if (i > 3) return false;
								new_scale = SC_COLUMNS[i];
								new_reflow = 0;
								break;

							case 2:
								new_scale = SC_NORMAL[(i + 5) * 13 / 11 + 1];
								new_reflow = 0;
								break;

							case 1:
								new_scale = SC_NORMAL[i * 13 / 11];
								new_reflow = 0;
								break;

							case 0:
								if (i == 0)
									new_scale = SC_PREVIEW[1];
								else if (i == 2)
									new_scale = SC_PREVIEW[0];
								else if (i == 4)
									need_fit_scale = true;
								else
									return false;
								new_reflow = 0;
								break;
						}
						invert_item(dx + 40 + i *(cw + 36) + 16, dy + th + 8 + j *(ch + 48) + 22, cw + 4, ch + 4);
						PartialUpdateBW(dx + 40 + i *(cw + 36) + 16, dy + th + 8 + j *(ch + 48) + 22, cw + 4, ch + 4);
						return true;
					}
		return false;
	}

	static void draw_new_zoomer(int update)
	{
		char buf[80];
		int cw, ch;
		int deltaw = 36;

		iv_windowframe(dx, dy, dw, dh, BLACK, WHITE, "@Zoom", 0);
		DrawBitmap(dx + 40, dy + th + 8, bm_touchzoom);
		cw = bm_touchzoom->width / 6 - deltaw;
		ch = bm_touchzoom->height / 5 - 48;
		//    invert_item(dx+40+(cw + 6)*pos + 3, dy+th+8, cw, ch);

		SetFont(menu_n_font, 0);
		//    snprintf(buf, sizeof(buf), "%s", GetLangText("@Preview"));
		//    DrawTextRect(dx, dy + th + 8, dw, 1, buf, ALIGN_CENTER);
		//    snprintf(buf, sizeof(buf), "%s", GetLangText("@Normal_page"));
		//    DrawTextRect(dx, dy + th + 8 + ch + 48, dw, 1, buf, ALIGN_CENTER);
		//    snprintf(buf, sizeof(buf), "%s", GetLangText("@Reflow"));
		//    DrawTextRect(dx, dy + th + 8 + 48 + (ch + 24) * 3 + 24, dw, 1, buf, ALIGN_CENTER);

		DrawTextRect(dx + 40, dy + th + 8 + ch + 24, cw + deltaw, 1, "4", ALIGN_CENTER);
		DrawTextRect(dx + 40 + (cw + deltaw) * 2, dy + th + 8 + ch + 24, cw + deltaw, 1, (char*)((GetOrientation() + 1) % 4 > 2 ? "6" : "9"),
		ALIGN_CENTER);
		snprintf(buf, sizeof(buf), "%s", GetLangText("@Fit_width"));
		//    DrawTextRect(dx + 40 + (cw + deltaw) * 4 - 30, dy + th + 8 + ch + 24, cw + deltaw + 60, 1, buf, ALIGN_CENTER);

		DrawTextRect(dx + 40, dy + th + 8 + (ch + 48) * 2 - 24, cw + deltaw, 1, "75%", ALIGN_CENTER);
		DrawTextRect(dx + 40 + cw + deltaw, dy + th + 8 + (ch + 48) * 2 - 24, cw + deltaw, 1, "80%", ALIGN_CENTER);
		DrawTextRect(dx + 40 + (cw + deltaw) * 2, dy + th + 8 + (ch + 48) * 2 - 24, cw + deltaw, 1, "85%", ALIGN_CENTER);
		DrawTextRect(dx + 40 + (cw + deltaw) * 3, dy + th + 8 + (ch + 48) * 2 - 24, cw + deltaw, 1, "90%", ALIGN_CENTER);
		DrawTextRect(dx + 40 + (cw + deltaw) * 4, dy + th + 8 + (ch + 48) * 2 - 24, cw + deltaw, 1, "95%", ALIGN_CENTER);
		DrawTextRect(dx + 40 + (cw + deltaw) * 5, dy + th + 8 + (ch + 48) * 2 - 24, cw + deltaw, 1, "100%", ALIGN_CENTER);

		DrawTextRect(dx + 40, dy + th + 8 + (ch + 48) * 3 - 24, cw + deltaw, 1, "105%", ALIGN_CENTER);
		DrawTextRect(dx + 40 + cw + deltaw, dy + th + 8 + (ch + 48) * 3 - 24, cw + deltaw, 1, "120%", ALIGN_CENTER);
		DrawTextRect(dx + 40 + (cw + deltaw) * 2, dy + th + 8 + (ch + 48) * 3 - 24, cw + deltaw, 1, "130%", ALIGN_CENTER);
		DrawTextRect(dx + 40 + (cw + deltaw) * 3, dy + th + 8 + (ch + 48) * 3 - 24, cw + deltaw, 1, "140%", ALIGN_CENTER);
		DrawTextRect(dx + 40 + (cw + deltaw) * 4, dy + th + 8 + (ch + 48) * 3 - 24, cw + deltaw, 1, "150%", ALIGN_CENTER);
		DrawTextRect(dx + 40 + (cw + deltaw) * 5, dy + th + 8 + (ch + 48) * 3 - 24, cw + deltaw, 1, "170%", ALIGN_CENTER);

		DrawTextRect(dx + 40, dy + th + 8 + (ch + 48) * 4 - 24, cw + deltaw, 1, "200%", ALIGN_CENTER);
		DrawTextRect(dx + 40 + cw + deltaw, dy + th + 8 + (ch + 48) * 4 - 24, cw + deltaw, 1, "300%", ALIGN_CENTER);
		DrawTextRect(dx + 40 + (cw + deltaw) * 2, dy + th + 8 + (ch + 48) * 4 - 24, cw + deltaw, 1, "400%", ALIGN_CENTER);
		DrawTextRect(dx + 40 + (cw + deltaw) * 3, dy + th + 8 + (ch + 48) * 4 - 24, cw + deltaw, 1, "500%", ALIGN_CENTER);

		DrawTextRect(dx + 40, dy + th + 8 + (ch + 48) * 5 - 24, cw + deltaw, 1, "150%", ALIGN_CENTER);
		DrawTextRect(dx + 40 + cw + deltaw, dy + th + 8 + (ch + 48) * 5 - 24, cw + deltaw, 1, "200%", ALIGN_CENTER);
		DrawTextRect(dx + 40 + (cw + deltaw) * 2, dy + th + 8 + (ch + 48) * 5 - 24, cw + deltaw, 1, "300%", ALIGN_CENTER);
		DrawTextRect(dx + 40 + (cw + deltaw) * 3, dy + th + 8 + (ch + 48) * 5 - 24, cw + deltaw, 1, "400%", ALIGN_CENTER);
		DrawTextRect(dx + 40 + (cw + deltaw) * 4, dy + th + 8 + (ch + 48) * 5 - 24, cw + deltaw, 1, "500%", ALIGN_CENTER);

		if (update) PartialUpdate(dx, dy, dw + 4, dh + 4);
	}

	static void close_zoomer()
	{
		DrawBitmap(dx, dy, isaves);
		free(isaves);
		isaves = NULL;
		iv_seteventhandler(prevhandler);
		SetKeyboardRate(700, 500);
		PartialUpdate(dx, dy, dw + 4, dh + 4);
		CloseFont(menu_arrow_font);
	}

	static int pointer_handler(int type, int par1, int par2)
	{
		if (type == EVT_POINTERUP)
		{
			if (par1 < dx || par1 > dx + dw + 4 || par2 < dy || par2 > dy + dh + 4) close_zoomer();
			if (get_new_mode_from_coord(par1, par2))
			{
				reflow_mode = new_reflow;
				if (need_fit_scale)
				{
					scale = get_fit_scale();
				}
				else
				{
					scale = new_scale;
				}
				rscale = new_rscale;
				if (new_reflow != reflow_mode) subpage = 0;
				close_zoomer();
				out_page(1);
			}
			return 1;
		}
		return 0;
	}

	static int newzoomer_handler(int type, int par1, int par2)
	{
		//  fprintf(stderr, "&&&&&&&&&&&&& %d %d %d\n", offx, offy, scale);
		if (type == EVT_POINTERDOWN || type == EVT_POINTERUP || type == EVT_POINTERMOVE || type == EVT_POINTERHOLD || type == EVT_POINTERLONG)
		{
			return pointer_handler(type, par1, par2);
		}

		if (type != EVT_KEYPRESS && type != EVT_KEYREPEAT) return 0;
		//    embed::Matrix matr;
		switch (par1)
		{
			case KEY_BACK:
			case KEY_MENU:
				close_zoomer();
				return 1;
		}

		return 0;

	}

	void open_zoomer()
	{
		if (!bm_touchzoom) bm_touchzoom = GetResource("touchzoom", (ibitmap*) &touchzoom);

		th = header_font->height + 4;
		dw = bm_touchzoom->width + 80;
		dh = th + bm_touchzoom->height + menu_n_font->height + 40;
		dx = (ScreenWidth() - dw) / 2;
		dy = (ScreenHeight() - dh) / 2;

		isaves = BitmapFromScreen(dx, dy, dw + 4, dh + 4);
		DimArea(dx + 4, dy + 4, dw, dh, BLACK);

		new_scale = scale;
		new_rscale = rscale;
		new_reflow = reflow_mode;

		prevhandler = iv_seteventhandler(newzoomer_handler);
		if (ivstate.needupdate)
		{
			draw_new_zoomer(0);
			SoftUpdate();
		}
		else
		{
			draw_new_zoomer(1);
		}

	}
}
