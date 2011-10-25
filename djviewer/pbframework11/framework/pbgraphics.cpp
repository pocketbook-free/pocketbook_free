#include "pbgraphics.h"
#include "pbwindow.h"

#define GETPIXEL1(data, x) ((data[(x)>>3] >> (7 - ((x) & 7))) & 1)
#define GETPIXEL2(data, x) ((data[(x)>>2] >> ((3 - ((x) & 3)) << 1)) & 3)
#define GETPIXEL4(data, x) ((data[(x)>>1] >> (((x) & 1) ? 0 : 4)) & 0xf)
#define GETPIXEL8(data, x) data[x]

PBScreenUpdateType UPDATE_TYPE(int color)
{
	if (color == BLACK || color == WHITE)
		return updateRectBW;
	else
		return updateRect;
}

#define SETPIXEL1(data, x, c) { \
	data[(x)>>3] &= ~(1 << (7 - ((x) & 7))); \
	data[(x)>>3] |= ((c) << (7 - ((x) & 7))); }
#define SETPIXEL2(data, x, c) { \
	data[(x)>>2] &= ~(3 << ((3 - ((x) & 3)) << 1)); \
	data[(x)>>2] |= ((c) << ((3 - ((x) & 3)) << 1)); }
#define SETPIXEL4(data, x, c) { \
	data[(x)>>1] &= ~(((x) & 1) ? 0xf : 0xf0); \
	data[(x)>>1] |= ((c) << (((x) & 1) ? 0 : 4)); }
#define SETPIXEL8(data, x, c) { data[x] = (c); }

PBGraphics::PBGraphics(PBWindow *pParent)
	:m_pParent(pParent)
{
}

PBWindow *PBGraphics::m_pClient = NULL;

PBGraphics::~PBGraphics()
{
}

void PBGraphics::Create(PBWindow *pParent)
{
	m_pParent = pParent;
}

void PBGraphics::DrawSelection(int x, int y, int w, int h, int color)
{
	if (m_pParent && m_pParent->IsVisible())
    {
		PBRect screenRect = m_pParent->ClientToScreen(PBRect(x,y,w,h));
		::DrawSelection(screenRect.GetX(), screenRect.GetY(), screenRect.GetWidth(), screenRect.GetHeight(), color);
		m_pParent->Invalidate(screenRect, UPDATE_TYPE(color));
    }
}

void PBGraphics::DrawPixel(int x, int y, int color)
{
	if (m_pParent && m_pParent->IsVisible())
	{
		CoordsX(&x);
		CoordsY(&y);
		PBPoint screenPoint = m_pParent->ClientToScreen(PBPoint(x,y));
		::DrawPixel(screenPoint.GetX(), screenPoint.GetY(), color);
		m_pParent->Invalidate(PBRect(screenPoint.GetX(), screenPoint.GetY(), 1, 1), UPDATE_TYPE(color));
	}
}

void PBGraphics::DimArea(int x, int y, int w, int h, int color)
{
	if (m_pParent && m_pParent->IsVisible())
    {
		PBRect screenRect = m_pParent->ClientToScreen(PBRect(x,y,w,h));
		::DimArea(screenRect.GetX(), screenRect.GetY(), screenRect.GetWidth(), screenRect.GetHeight(), color);
		m_pParent->Invalidate(screenRect, UPDATE_TYPE(color));
    }
}

void PBGraphics::DrawRect(int x, int y, int w, int h, int color)
{
	if (m_pParent && m_pParent->IsVisible())
    {
		PBRect screenRect = m_pParent->ClientToScreen(PBRect(x,y,w,h));
		::DrawRect(screenRect.GetX(), screenRect.GetY(), screenRect.GetWidth(), screenRect.GetHeight(), color);
		m_pParent->Invalidate(screenRect, UPDATE_TYPE(color));
    }
}

void PBGraphics::StretchBitmap(int x, int y, int w, int h, const ibitmap *src, int flags)
{
	if (m_pParent && m_pParent->IsVisible())
    {
	    PBRect screenRect = m_pParent->ClientToScreen(PBRect(x,y,w,h));
	    ::StretchBitmap(screenRect.GetX(), screenRect.GetY(), screenRect.GetWidth(), screenRect.GetHeight(), src, flags);
		m_pParent->Invalidate(screenRect, updateRect);
    }
}

void PBGraphics::DrawLine(int x1, int y1, int x2, int y2, int color)
{
	if (m_pParent && m_pParent->IsVisible())
	{
		PBRect screenRect = m_pParent->ClientToScreen(PBRect(x1, y1, x2 - x1/* + 1*/, y2 - y1/* + 1*/));
		::DrawLine(screenRect.GetX(), screenRect.GetY(), screenRect.GetRight(), screenRect.GetBottom(), color);
		m_pParent->Invalidate(screenRect, UPDATE_TYPE(color));
	}
}

void PBGraphics::DrawTextRect(int x, int y, int w, int h, const char *s, int flags)
{
	if (m_pParent && m_pParent->IsVisible())
	{
		PBRect screenRect = m_pParent->ClientToScreen(PBRect(x,y,w,h));
		::DrawTextRect(screenRect.GetX(), screenRect.GetY(), screenRect.GetWidth(), screenRect.GetHeight(), s, flags);
		m_pParent->Invalidate(screenRect, updateRectBW);
	}
}

void PBGraphics::DrawBitmap(int x, int y, const ibitmap *b)
{
	if (m_pParent && m_pParent->IsVisible())
	{
		PBPoint screenPoint = m_pParent->ClientToScreen(PBPoint(x,y));
		::DrawBitmap(screenPoint.GetX(), screenPoint.GetY(), b);
		m_pParent->Invalidate(PBRect(screenPoint.GetX(), screenPoint.GetY(), b->width, b->height), updateRect);
	}
}

void PBGraphics::DrawBitmapBW(int x, int y, const ibitmap *b)
{
	if (m_pParent && m_pParent->IsVisible())
	{
		PBPoint screenPoint = m_pParent->ClientToScreen(PBPoint(x,y));
		::DrawBitmap(screenPoint.GetX(), screenPoint.GetY(), b);
		m_pParent->Invalidate(PBRect(screenPoint.GetX(), screenPoint.GetY(), b->width, b->height), updateRectBW);
	}
}

void PBGraphics::FillArea(int x, int y, int w, int h, int color)
{
	if (m_pParent && m_pParent->IsVisible())
	{
		PBRect screenRect = m_pParent->ClientToScreen(PBRect(x,y,w,h));
		::FillArea(screenRect.GetX(), screenRect.GetY(), screenRect.GetWidth(), screenRect.GetHeight(), color);
		m_pParent->Invalidate(screenRect, UPDATE_TYPE(color));
	}
}

void PBGraphics::DrawBitmapRect(int x, int y, int w, int h, ibitmap *b, int flags)
{
	if (m_pParent && m_pParent->IsVisible())
	{
		PBRect screenRect = m_pParent->ClientToScreen(PBRect(x,y,w,h));
		::DrawBitmapRect(screenRect.GetX(), screenRect.GetY(), screenRect.GetWidth(), screenRect.GetHeight(), b, flags);
		m_pParent->Invalidate(screenRect, updateRect);
	}
}

void PBGraphics::DrawBitmapRectBW(int x, int y, int w, int h, ibitmap *b, int flags)
{
	if (m_pParent && m_pParent->IsVisible())
	{
		PBRect screenRect = m_pParent->ClientToScreen(PBRect(x,y,w,h));
		::DrawBitmapRect(screenRect.GetX(), screenRect.GetY(), screenRect.GetWidth(), screenRect.GetHeight(), b, flags);
		m_pParent->Invalidate(screenRect, updateRectBW);
	}
}

void PBGraphics::RotateBitmap(ibitmap **pbm, int rotate)
{
	int w, h, sl, ml, depth, x, y;
	unsigned char *data, *mask, *dp, *mp;

	if (*pbm == NULL || rotate == ROTATE0) return;

	if (rotate == ROTATE180)
	{
		MirrorBitmap(*pbm, YMIRROR);
		return;
	}

	if (rotate != ROTATE90 && rotate != ROTATE270) return;

	ibitmap *bm = *pbm;
	ibitmap *rbm;
	int rw, rh, rsl, rml;
	int len;
	unsigned char *rdata, *rmask, *rdp, *rmp;

	w = bm->width;
	h = bm->height;
	depth = bm->depth & 0xff;
	sl = bm->scanline;
	ml = (bm->width + 7) >> 3;
	data = bm->data;
	mask = ((bm->depth & 0x8000) == 0) ? NULL : bm->data + h * sl;

	rw = bm->height;
	rh = bm->width;
	rsl = (rw + depth - 1) * depth / 8;
	rml = (rw + 7) / 8;

	len = sizeof(ibitmap *) + rh * rw;
	if (mask) len += rh * ((rw + 7) / 8);

	rbm = (ibitmap *) malloc(len);

	rbm->width = rw;
	rbm->height = rh;
	rbm->depth = bm->depth;
	rbm->scanline = rsl;

	rdata = rbm->data;
	rmask = (mask ? rdata + rh * rsl : NULL);

	if (rotate == ROTATE90)
	{
		for (y=0; y<h; y++) {

			dp = data + y * sl;
			mp = mask + y * ml;

			for (x = 0; x < w; x++) {

				rdp = rdata + x * rsl;
				rmp = rmask + x * rml;

				switch (depth){
					case 1:
					SETPIXEL1(rdp, rw - y - 1, GETPIXEL1(dp, x));
					break;
					case 2:
					SETPIXEL2(rdp, rw - y - 1, GETPIXEL2(dp, x));
					break;
					case 4:
					SETPIXEL4(rdp, rw - y - 1, GETPIXEL4(dp, x));
					break;
					case 8:
					SETPIXEL8(rdp, rw - y - 1, GETPIXEL8(dp, x));
					break;
					case 24:
					break;
				}

				if (mask) {
					SETPIXEL1(rmp, rw - y - 1, GETPIXEL1(mp, x));
				}

			}
		}
	}

	if (rotate == ROTATE270)
	{
		for (y=0; y<h; y++) {

			dp = data + y * sl;
			mp = mask + y * ml;

			for (x = 0; x < w; x++) {

				rdp = rdata + (rh - x - 1) * rsl;
				rmp = rmask + (rh - x - 1) * rml;

				switch (depth){
					case 1:
					SETPIXEL1(rdp, y, GETPIXEL1(dp, x));
					break;
					case 2:
					SETPIXEL2(rdp, y, GETPIXEL2(dp, x));
					break;
					case 4:
					SETPIXEL4(rdp, y, GETPIXEL4(dp, x));
					break;
					case 8:
					SETPIXEL8(rdp, y, GETPIXEL8(dp, x));
					break;
					case 24:
					break;
				}

				if (mask) {
					SETPIXEL1(rmp, y, GETPIXEL1(mp, x));
				}

			}
		}
	}

	free(*pbm);
	*pbm = rbm;

}

// Curves and Circles

void PBGraphics::DrawCurve(int x, int y, int radius, int corns, int color)
{

	if (m_pParent && m_pParent->IsVisible())
	{
		PBRect screenRect = m_pParent->ClientToScreen(PBRect(x - radius, y - radius, 2 * radius, 2 * radius));
		m_pParent->Invalidate(screenRect, UPDATE_TYPE(color));
		int x = screenRect.GetX() + radius, y = screenRect.GetY() + radius;

		int dx, dy, sum;
		int outx = radius;

		dx = sum = radius;
		dy = 0;

		while (dx >= 0)
		{
			sum -= dy;
			if (sum < 0)
			{
				if ((corns & FILLCORN) > 0)
				{
					outx = dx - 1;
				}
				while (sum < 0 && dx >= 0)
				{
					sum += dx;
					dx--;
					if ((corns & FILLCORN) == 0)
					{
						if ((corns & RIGHTBOTTOM) > 0)
							::DrawPixel(x + dx, y + dy, color);
						if ((corns & RIGHTTOP) > 0)
							::DrawPixel(x + dx, y - dy, color);
						if ((corns & LEFTBOTTOM) > 0)
							::DrawPixel(x - dx, y + dy, color);
						if ((corns & LEFTTOP) > 0)
							::DrawPixel(x - dx, y - dy, color);
					}
				}
			}
			else
			{
				outx = dx;
				if ((corns & FILLCORN) == 0)
				{
					if ((corns & RIGHTBOTTOM) > 0)
						::DrawPixel(x + dx, y + dy, color);
					if ((corns & RIGHTTOP) > 0)
						::DrawPixel(x + dx, y - dy, color);
					if ((corns & LEFTBOTTOM) > 0)
						::DrawPixel(x - dx, y + dy, color);
					if ((corns & LEFTTOP) > 0)
						::DrawPixel(x - dx, y - dy, color);
				}
			}
			if ((corns & FILLCORN) > 0)
			{
				if ((corns & RIGHTBOTTOM) > 0)
					::DrawLine(x, y + dy, x + outx, y + dy, color);
				if ((corns & RIGHTTOP) > 0)
					::DrawLine(x, y - dy, x + outx, y - dy, color);
				if ((corns & LEFTBOTTOM) > 0)
					::DrawLine(x - outx, y + dy, x, y + dy, color);
				if ((corns & LEFTTOP) > 0)
					::DrawLine(x - outx, y - dy, x, y - dy, color);
			}
			dy++;
		}
	}

}

void PBGraphics::DrawCurve(int x, int y, int radius, int corns, int thickness, int color)
{
	if (m_pParent && m_pParent->IsVisible())
	{
		PBRect screenRect = m_pParent->ClientToScreen(PBRect(x - radius, y - radius, 2 * radius, 2 * radius));
		m_pParent->Invalidate(screenRect, UPDATE_TYPE(color));
		int x = screenRect.GetX() + radius, y = screenRect.GetY() + radius;

		int outdx,outsum, indx, insum, dy;
		int outx, inx;

		if (thickness < 1) return;
		if (thickness > radius)
			thickness = radius + 1;
		outdx = outsum = radius;
		dy = 0;
		indx = insum = radius - thickness + 1;

		while (outdx > 0)
		{
			outsum -= dy;
			if (outsum < 0)
			{
				outsum += outdx;
				outdx--;
				outx = outdx;
				while (outsum < 0 && outdx > 0)
				{
					outsum += outdx;
					outdx--;
				}
			}
			else
			{
				outx = outdx;
			}
			if (dy < radius - thickness + 2)
			{
				insum -= dy;
				if (insum < 0 && indx > 0)
				{
					insum += indx;
					indx--;
					while (insum < 0 && indx > 0)
					{
						insum += indx;
						indx--;
					}
					inx = indx;
				}
				else
				{
					inx = indx;
				}
			}
			else
			{
				inx = 0;
			}

			if (inx > 0)
			{
				if ((corns & RIGHTBOTTOM) > 0)
					::DrawLine(x + outx, y + dy, x + inx, y + dy, color);
				if ((corns & RIGHTTOP) > 0)
					::DrawLine(x + outx, y - dy, x + inx, y - dy, color);
				if ((corns & LEFTBOTTOM) > 0)
					::DrawLine(x - outx, y + dy, x - inx, y + dy, color);
				if ((corns & LEFTTOP) > 0)
					::DrawLine(x - outx, y - dy, x - inx, y - dy, color);
			}
			else
			{
				if ((corns & RIGHTBOTTOM) > 0)
					::DrawLine(x, y + dy, x + outx, y + dy, color);
				if ((corns & RIGHTTOP) > 0)
					::DrawLine(x, y - dy, x + outx, y - dy, color);
				if ((corns & LEFTBOTTOM) > 0)
					::DrawLine(x - outx, y + dy, x, y + dy, color);
				if ((corns & LEFTTOP) > 0)
					::DrawLine(x - outx, y - dy, x, y - dy, color);
			}
			dy++;
		}
	}
}

void PBGraphics::DrawCircle(int x, int y, int radius, int color)
{
	DrawCurve(x, y, radius, 0xf, color);
/*
	int dx, dy, sum;

	dx = sum = r;
	dy = 0;

	while (dx > 0)
	{
		sum -= dy;
		if (sum < 0)
		{
			while (sum < 0 && dx > 0)
			{
				sum += dx;
				dx--;
				DrawPixel(x + dx, y + dy, color);
				DrawPixel(x + dx, y - dy, color);
				DrawPixel(x - dx, y + dy, color);
				DrawPixel(x - dx, y - dy, color);
			}
		}
		else
		{
			DrawPixel(x + dx, y + dy, color);
			DrawPixel(x + dx, y - dy, color);
			DrawPixel(x - dx, y + dy, color);
			DrawPixel(x - dx, y - dy, color);
		}
		dy++;
	}
*/
}

void PBGraphics::DrawCircle(int x, int y, int radius, int color, bool fill)
{
	if (fill)
	{
		DrawCurve(x, y, radius, 0x1f, color);
/*
		int dx, dy, sum;
		int outx;
		PBPoint screenRect = m_pParent->ClientToScreen(PBPoint(0, 0));
		int l = screenRect.GetX(), t = screenRect.GetY();

		dx = sum = radius;
		dy = 0;

		while (dx >= 0)
		{
			sum -= dy;
			if (sum < 0)
			{
				while (sum < 0 && dx >= 0)
				{
					sum += dx;
					dx--;
				}
				
			}
			::DrawLine(l + x, t + y + dy, l + x + dx, t + y + dy, color);
			::DrawLine(l + x, t + y - dy, l + x + dx, t + y - dy, color);
			::DrawLine(l + x - dx, t + y + dy, l + x, t + y + dy, color);
			::DrawLine(l + x - dx, t + y - dy, l + x, t + y - dy, color);
			dy++;
		}
*/
	}
	else
	{
		DrawCurve(x, y, radius, 0xf, color);
	}
}

void PBGraphics::DrawCircle(int x, int y, int radius, int thickness, int color)
{
	DrawCurve(x, y, radius, 0xf, thickness, color);
/*
	int outdx,outsum, indx, insum, dy;
	int outx, inx;

	if (bold < 1) return;
	outdx = outsum = r;
	dy = 0;
	indx = insum = r - bold + 1;

	while (outdx > 0)
	{
		outsum -= dy;
		if (outsum < 0)
		{
			outsum += outdx;
			outdx--;
			outx = outdx;
			while (outsum < 0 && outdx > 0)
			{
				outsum += outdx;
				outdx--;
			}
		}
		else
		{
			outx = outdx;
		}
		if (dy < r - bold + 2)
		{
			insum -= dy;
			if (insum < 0 && indx > 0)
			{
				insum += indx;
				indx--;
				while (insum < 0 && indx > 0)
				{
					insum += indx;
					indx--;
				}
				inx = indx;
			}
			else
			{
				inx = indx;
			}
		}
		else
		{
			inx = 0;
		}

		if (inx > 0)
		{
			DrawLine(x + outx, y + dy, x + inx, y + dy, color);
			DrawLine(x + outx, y - dy, x + inx, y - dy, color);
			DrawLine(x - outx, y + dy, x - inx, y + dy, color);
			DrawLine(x - outx, y - dy, x - inx, y - dy, color);
		}
		else
		{
			DrawLine(x - outx, y + dy, x + outx, y + dy, color);
			DrawLine(x - outx, y - dy, x + outx, y - dy, color);
		}
		dy++;
	}
*/
}

void PBGraphics::DrawShadow(int x, int y, int w, int h, int radius, int shadowsize, int color)
{
	int dx, dy, sum;

	dx = sum = radius;
	dy = 0;

	int indx, indy, insum;
	indx = insum = radius;
	indy = 0;

	if (shadowsize < 1) return;
	if (shadowsize > radius)
		shadowsize = radius;

	int x1;
	int y1, y2;
//	color = DGRAY;
	while (dx >= 0)
	{
		sum -= dy;
		if (sum < 0)
		{
			while (sum < 0 && dx >= 0)
			{
				sum += dx;
				dx--;
				if (dx >= 0)
				{
					if (radius <= dx + shadowsize)
					{
						DrawLine(x + w - radius + dx + shadowsize, y + radius - dy + shadowsize, x + w - radius + dx + shadowsize, y + h - radius + dy + shadowsize, color);
					}
					else
					{
						while (dx + shadowsize != indx)
						{
							if (insum < 0)
							{
								insum += indx;
								indx--;
							}
							else
							{
								insum -= indy;
								if (insum < 0)
								{
									insum += indx;
									indx--;
								}
								else
								{
									indy++;
								}
							}
						}
						x1 = x + w - radius + shadowsize + dx;
						y1 = y + radius - dy + shadowsize;
						y2 = y + radius - indy + 1;
						if (y1 <= y2)
						{
							DrawLine(x1, y1, x1, y2, color);
						}
						x1 = x + w - radius + shadowsize + dx;
						y1 = y + h;
						y2 = y + h - radius + indy - 2;
						DrawLine(x1, y1, x1, y2, color);
						if (insum > 0)
							indy++;
					}
				}
			}
			if (radius - shadowsize <= dy)
			{
				DrawLine(x + radius - dx + shadowsize, y + h - radius + dy + shadowsize, x + w - radius + dx + shadowsize, y + h - radius + dy + shadowsize, color);
			}
		}
		if (radius - shadowsize <= dy)
		{
			DrawLine(x + radius - dx + shadowsize, y + h - radius + dy + shadowsize, x + w - radius + dx + shadowsize, y + h - radius + dy + shadowsize, color);
		}
		dy++;
	}

	while (indx >= 0)
	{
		insum -= indy;
		if (insum < 0)
		{
			while (insum < 0 && indx >= 0)
			{
				insum += indx;
				indx--;
				if (indx >= 0)
				{
					x1 = x + w - radius + indx;
					y1 = y + h;
					y2 = y + h - radius + indy - 2;
					DrawLine(x1, y1, x1, y2, color);
				}
			}
		}
		indy++;
	}

/*
	dx = sum = radius;
	dy = 0;

	indx = insum = radius;
	indy = 0;

	PartialUpdate(0, 0, ScreenWidth(), ScreenHeight());

	while (dx >= 0)
	{
		sum -= dy;
		if (sum < 0)
		{
			while (sum < 0 && dx >= 0)
			{
				sum += dx;
				dx--;
				if (dx >= 0)
				{
					if (indx <= shadowsize && indx >= 0)
					{
						while (shadowsize != indx)
						{
							if (insum < 0)
							{
								insum += indx;
								indx--;
							}
							else
							{
								insum -= indy;
								if (insum < 0)
								{
									insum += indx;
									indx--;
								}
								else
								{
									indy++;
								}
							}
						}
						x1 = x + radius - dx + shadowsize;
						y1 = y + h - radius + dy + shadowsize;
						y2 = y + h - radius + indy;
						if (y1 >= y2)
							DrawLine(x1, y1, x1, y2, LGRAY);
						if (insum > 0)
							indy++;
					}
					else
					{
						while (shadowsize != indx)
						{
							if (insum < 0)
							{
								insum += indx;
								indx--;
							}
							else
							{
								insum -= indy;
								if (insum < 0)
								{
									insum += indx;
									indx--;
								}
								else
								{
									indy++;
								}
							}
						}
						if (insum > 0)
							indy++;
					}
				}
			}
		}
//		if (radius >= dx + shadowsize)
//		{
//
//			DrawLine(x + radius - dx + shadowsize, y + h - radius + dy + shadowsize, x + w - radius + dx + shadowsize, y + h - radius + dy + shadowsize, color);
//		}
		dy++;
		PartialUpdate(0, 0, ScreenWidth(), ScreenHeight());
	}
*/
}

void PBGraphics::DrawFrame(int x, int y, int w, int h, int radius, int thickness, int color)
{
	if (radius > w / 2)
		radius = w / 2;
	if (radius > h / 2)
		radius = h / 2;
	if (thickness > w / 2)
		thickness = w / 2;
	if (thickness > h / 2)
		thickness = h / 2;

	DrawCurve(x + radius, y + radius + 1, radius, LEFTTOP, thickness, color);
	DrawCurve(x + w - radius - 1, y + radius + 1, radius, RIGHTTOP, thickness, color);
	DrawCurve(x + w - radius - 1, y + h - radius - 2, radius, RIGHTBOTTOM, thickness, color);
	DrawCurve(x + radius, y + h - radius - 2, radius, LEFTBOTTOM, thickness, color);

//	FillArea(x + radius, y, w - 2 * radius, thickness, color);
//	FillArea(x + w - thickness, y + radius, thickness, h - 2 * radius, color);
//	FillArea(x + radius, y + h - thickness, w - 2 * radius, thickness, color);
//	FillArea(x, y + radius, thickness, h - 2 * radius, color);

/*
	int outdx,outsum, indx, insum, dy;
	int outx, inx;

	if (thickness < 1) return;
	if (thickness > radius)
		thickness = radius + 1;
	outdx = outsum = radius;
	dy = 0;
	indx = insum = radius - thickness + 1;

	while (outdx > 0)
	{
		outsum -= dy;
		if (outsum < 0)
		{
			outsum += outdx;
			outdx--;
			outx = outdx;
			while (outsum < 0 && outdx > 0)
			{
				outsum += outdx;
				outdx--;
			}
		}
		else
		{
			outx = outdx;
		}
		if (dy < radius - thickness + 2)
		{
			insum -= dy;
			if (insum < 0 && indx > 0)
			{
				insum += indx;
				indx--;
				while (insum < 0 && indx > 0)
				{
					insum += indx;
					indx--;
				}
				inx = indx;
			}
			else
			{
				inx = indx;
			}
		}
		else
		{
			inx = 0;
		}

		if (inx > 0)
		{
			if ((corns & RIGHTBOTTOM) > 0)
				DrawLine(x + outx, y + dy, x + inx, y + dy, color);
			if ((corns & RIGHTTOP) > 0)
				DrawLine(x + outx, y - dy, x + inx, y - dy, color);
			if ((corns & LEFTBOTTOM) > 0)
				DrawLine(x - outx, y + dy, x - inx, y + dy, color);
			if ((corns & LEFTTOP) > 0)
				DrawLine(x - outx, y - dy, x - inx, y - dy, color);
		}
		else
		{
			if ((corns & RIGHTBOTTOM) > 0)
				DrawLine(x, y + dy, x + outx, y + dy, color);
			if ((corns & RIGHTTOP) > 0)
				DrawLine(x, y - dy, x + outx, y - dy, color);
			if ((corns & LEFTBOTTOM) > 0)
				DrawLine(x - outx, y + dy, x, y + dy, color);
			if ((corns & LEFTTOP) > 0)
				DrawLine(x - outx, y - dy, x, y - dy, color);
		}
		dy++;
	}
*/
//	DrawSelection(x, y, w, h, color);
}


void PBGraphics::DrawFrame(int x, int y, int w, int h, int radius, int thickness, int shadowsize, int color)
{
	DrawFrame(x, y, w - shadowsize, h - shadowsize, radius, thickness, color);
//	DrawShadow(x, y, w - shadowsize, h - shadowsize, radius, shadowsize, color);
}

// Font functions

ifont *PBGraphics::OpenFont(const char *name, int size, int /*aa*/)
{
	return ::OpenFont(name, size, 1);
}

void PBGraphics::SetFont(const ifont *font, int color)
{
	::SetFont((ifont*)font, color);
}

// Hourglass

void PBGraphics::ShowHourglass()
{
	::ShowHourglass();
}

void PBGraphics::ShowHourglassAt(int x, int y)
{
	PBPoint screenPoint = m_pParent->ClientToScreen(PBPoint(x,y));
	::ShowHourglassAt(screenPoint.GetX(), screenPoint.GetY());
}

void PBGraphics::HideHourglass()
{
	::HideHourglass();
}

// Menu functions

void PBGraphics::MenuHandler(int index)
{
	if (index > 0)
	{
		PBGraphics::m_pClient->OnMenu(index);
		PBGraphics::m_pClient = NULL;
	}
}

void PBGraphics::OpenMenu(PBWindow *client, imenu *menu, int pos, int x, int y)
{
	m_pClient = client;
	PBPoint screenPoint = m_pParent->ClientToScreen(PBPoint(x,y));
	::OpenMenu(menu, pos, screenPoint.GetX(), screenPoint.GetY(), PBGraphics::MenuHandler);
}

// Panel

int PBGraphics::DrawPanel(const ibitmap *icon, const char *text, const char *title, int percent)
{
	PBRect screenRect = PBRect(0, ScreenHeight() - PanelHeight(), ScreenWidth(), PanelHeight());
	m_pParent->Invalidate(screenRect, updateRect);
	return ::DrawPanel(icon, text, title, percent);
}

// private methods

void PBGraphics::CoordsX(int *x)
{
	if (*x < 0) *x = 0;
	if (*x > m_pParent->GetClientWidth()) *x = m_pParent->GetClientWidth();
}

void PBGraphics::CoordsX(int *x1, int *x2)
{
	if (*x1 < 0) *x1 = 0;
	if (*x2 < 0) *x2 = 0;

	if (*x1 > m_pParent->GetClientWidth()) *x1 = m_pParent->GetClientWidth();
	if (*x2 > m_pParent->GetClientWidth()) *x2 = m_pParent->GetClientWidth();

	if (*x1 > *x2)
	{
		int swap;
		swap = *x1;
		*x1 = *x2;
		*x2 = swap;
	}
}

void PBGraphics::CoordsY(int *y)
{
	if (*y < 0) *y = 0;
	if (*y > m_pParent->GetClientHeight()) *y = m_pParent->GetClientHeight();
}


void PBGraphics::CoordsY(int *y1, int *y2)
{
	if (*y1 < 0) *y1 = 0;
	if (*y2 < 0) *y2 = 0;

	if (*y1 > m_pParent->GetClientHeight()) *y1 = m_pParent->GetClientHeight();
	if (*y2 > m_pParent->GetClientHeight()) *y2 = m_pParent->GetClientHeight();

	if (*y1 > *y2)
	{
		int swap;
		swap = *y1;
		*y1 = *y2;
		*y2 = swap;
	}
}

// http://rooparam.blogspot.com/2009/09/midpoint-circle-algorithm.html
// вспомогательная функция для заливки прямоугольной области со скругленными углами
// принимает размеры прямоугольника, радиус закругления
// x1, y1 - координаты точки на закругленной части. Эти кординаты вычисляются только для
// одного октанта, для остальных вычисляются путем зеркального отражения по соответствующим осям
// всего получается 8 точек по одной для каждого октанта.
void circle_draw_helper(int x, int y, int w, int h, int r, int x1, int y1, int lineColor)
{
	int xc = x + w - r - 1;
	int yc = y + h - r - 1;
	::DrawPixel(xc+x1, yc+y1, lineColor);      // Ist      Octant
	::DrawPixel(xc+y1, yc+x1, lineColor);      // IInd     Octant

	xc = x + r;
	yc = y + h - r - 1;
	::DrawPixel(xc-y1, yc+x1, lineColor);      // IIIrd    Octant
	::DrawPixel(xc-x1, yc+y1, lineColor);      // IVth     Octant

	xc = x + r;
	yc = y + r;
	::DrawPixel(xc-x1, yc-y1, lineColor);      // Vth      Octant
	::DrawPixel(xc-y1, yc-x1, lineColor);      // VIth     Octant

	xc = x + w - r - 1;
	yc = y + r;
	::DrawPixel(xc+y1, yc-x1, lineColor);      // VIIth    Octant
	::DrawPixel(xc+x1, yc-y1, lineColor);      // VIIIth   Octant
}

// вспомогательная функция для отрисовки прямоугольной области со скругленными углами,
// если толщина линии больше одного пиксела.
// В эту функцию последовательно передаются все точки, содержащиеся в одном октанте окружности.
// Переданная точка путем зеркального отражения по соответствующим осям вычисляется для остальных октантов.
// Затем все 8 точек отрисовываются.
// x1, y1 - координаты точки. x2, y2 - координаты точки, для окружности с радиусом на 1 меньше, чем первая точка.
// Таким образом получаем нужную нам толщину линии.
// Радиус передается всегда один и тот же - радиус закругления переданный в функцию DrawRectEx.
// Чтобы получить именно прямоугольник с закругленными углами а не  просто круг, полученная окружность делится на
// 4 сектора, каждый сектов перемещается в нужный угол прямоугольника.
// x, y, w, h - размеры прямоугольника
void circle_draw_helper(int x, int y, int w, int h, int r, int x1, int y1, int x2, int y2, int lineColor)
{
	int xc = x + w - r - 1;
	int yc = y + h - r - 1;
	::DrawLine(xc+x1, yc+y1, xc+x2, yc+y2, lineColor);      // Ist      Octant rigth-bottom
	::DrawLine(xc+y1, yc+x1, xc+y2, yc+x2, lineColor);      // IInd     Octant

	xc = x + r;
	yc = y + h - r - 1;
	::DrawLine(xc-y1, yc+x1, xc-y2, yc+x2, lineColor);      // IIIrd    Octant left-bottom
	::DrawLine(xc-x1, yc+y1, xc-x2, yc+y2, lineColor);      // IVth     Octant

	xc = x + r;
	yc = y + r;
	::DrawLine(xc-x1, yc-y1, xc-x2, yc-y2, lineColor);      // Vth      Octant left-top
	::DrawLine(xc-y1, yc-x1, xc-y2, yc-x2, lineColor);      // VIth     Octant

	xc = x + w - r - 1;
	yc = y + r;
	::DrawLine(xc+y1, yc-x1, xc+y2, yc-x2, lineColor);      // VIIth    Octant right-top
	::DrawLine(xc+x1, yc-y1, xc+x2, yc-y2, lineColor);      // VIIIth   Octant
}

// вспомогательная функция для заливки прямоугольной области со скругленными углами
// принимает размеры прямоугольника, радиус закругления
// В эту функцию последовательно передаются все точки, содержащиеся в одном октанте окружности.
// x1, y1 - координаты точки на закругленной части. Эти кординаты вычисляются только для
// одного октанта, для остальных вычисляются путем зеркального отражения по соответствующим осям
// всего получается 8 точек по одной для каждого октанта. Между этими точками проводятся линии
// цветом заливки. Таким образом заливается область внутри круга.
// Чтобы получить именно прямоугольник с закругленными углами а не  просто круг, полученная окружность делится на
// 4 сектора, каждый сектов перемещается в нужный угол прямоугольника.
// x, y, w, h - размеры прямоугольника
void circle_fill_helper(int x, int y, int w, int h, int r, int x1, int y1, int fillColor)
{
	// Ist and IInd Octant - rigth-bottom
	int xc = x + w - r - 1;
	int yc = y + h - r - 1;
	int p1x = xc+x1;
	int p1y = yc+y1;
	int p3x = xc+y1;
	int p3y = yc+x1;

	// IIIrd and IVth Octant - left-bottom
	xc = x + r;
	yc = y + h - r - 1;
	int p2x = xc-x1;
	int p2y = yc+y1;
	int p4x = xc-y1;
	int p4y = yc+x1;

	::DrawLine(p1x, p1y, p2x, p2y, fillColor);
	::DrawLine(p3x, p3y, p4x, p4y, fillColor);

	// Vth and VIth Octant - left-top
	xc = x + r;
	yc = y + r;
	p1x = xc-x1;
	p1y = yc-y1;
	p3x = xc-y1;
	p3y = yc-x1;

	// VIIth and VIIIth Octant - right-top
	xc = x + w - r - 1;
	yc = y + r;
	p2x = xc+x1;
	p2y =  yc-y1;
	p4x = xc+y1;
	p4y = yc-x1;

	::DrawLine(p1x, p1y, p2x, p2y, fillColor);
	::DrawLine(p3x, p3y, p4x, p4y, fillColor);
}

typedef std::map<int, std::pair<PBPoint, PBPoint> > TInvertCircleData;

void circle_invert_helper(int x, int y, int w, int h, int r, int x1, int y1, TInvertCircleData & data)
{
	// Ist and IInd Octant - rigth-bottom
	int xc = x + w - r - 1;
	int yc = y + h - r - 1;
	int p1x = xc+x1;
	int p1y = yc+y1;
	int p3x = xc+y1;
	int p3y = yc+x1;

	// IIIrd and IVth Octant - left-bottom
	xc = x + r;
	yc = y + h - r - 1;
	int p2x = xc-x1;
	int p2y = yc+y1;
	int p4x = xc-y1;
	int p4y = yc+x1;

	TInvertCircleData::iterator it = data.find(p1y);
	if (it == data.end() || (*it).second.first.GetX() > p2x)
		data[p1y] = std::pair<PBPoint, PBPoint>(PBPoint(p2x, p2y), PBPoint(p1x, p1y));

	it = data.find(p3y);
	if (it == data.end() || (*it).second.first.GetX() > p4x)
		data[p3y] = std::pair<PBPoint, PBPoint>(PBPoint(p4x, p4y), PBPoint(p3x, p3y));

	// Vth and VIth Octant - left-top
	xc = x + r;
	yc = y + r;
	p1x = xc-x1;
	p1y = yc-y1;
	p3x = xc-y1;
	p3y = yc-x1;

	// VIIth and VIIIth Octant - right-top
	xc = x + w - r - 1;
	yc = y + r;
	p2x = xc+x1;
	p2y = yc-y1;
	p4x = xc+y1;
	p4y = yc-x1;

	it = data.find(p1y);
	if (it == data.end() || (*it).second.first.GetX() > p1x)
		data[p1y] = std::pair<PBPoint, PBPoint>(PBPoint(p1x, p1y), PBPoint(p2x, p2y));

	it = data.find(p3y);
	if (it == data.end() || (*it).second.first.GetX() > p3x)
		data[p3y] = std::pair<PBPoint, PBPoint>(PBPoint(p3x, p3y), PBPoint(p4x, p4y));
}

// рисует круг с радиусом r. Затем делит его на четверти и растаскивает их по углам
// прямоугольника (x, y, w, h - прямоугольник).
// Это вспомогательная функция. Нужно использовать функцию DrawRectEx
void PBGraphics::drawCircleEx(int x, int y, int w, int h, int r, int thickness, int lineColor)
{
	if (thickness == 1)
	{
		int p = 1 - r;
		int x1 = 0, y1 = r;
		do
		{
			circle_draw_helper(x, y, w, h, r, x1, y1, lineColor);

			++x1;
			if (p < 0)
				p += 1 + (x1<<1);
			else
			{
				--y1;
				p += 1 - ((y1-x1) << 1);
			}
		}
		while(y1 >= x1);
	}
	else
	{
		int r_save = r;

		while (thickness-1)
		{
			int p1 = 1 - r;
			int x1 = 0;
			int y1 = r;

			int p2 = 1 - (r - 1);
			int x2 = 0;
			int y2 = (r - 1);

			do
			{
				circle_draw_helper(x, y, w, h, r_save, x1, y1, x2, y2, lineColor);

				++x1;
				if (p1 < 0)
				{
					p1 += 1 + (x1<<1);
				}
				else
				{
					--y1;
					p1 += 1 - ((y1-x1) << 1);
				}

				if (y2 >= x2)
				{
					++x2;
					if (p2 < 0)
					{
						p2 += 1 + (x2<<1);
					}
					else
					{
						--y2;
						p2 += 1 - ((y2-x2) << 1);
					}
				}
			}
			while(y1 >= x1);

			thickness--;
			r--;
		}
	}
}

// вспомогательная функция.
// рисует круг с радиусом r. Затем делит его на четверти и растаскивает их по углам
// прямоугольника (x, y, w, h - прямоугольник). Затем закрашивает область между четвертями.
// середина остается незакрашенной. Нужно использовать функцию FillRectEx
void PBGraphics::fillCircleEx(int x, int y, int w, int h, int r, int fillColor)
{
	int p = 1 - r;
	int x1 = 0, y1 = r;
	do
	{
		  circle_fill_helper(x, y, w, h, r, x1, y1, fillColor);

		  ++x1;
		  if (p < 0)
			 p += 1 + (x1<<1);
		  else
		  {
			   --y1;
			   p += 1 - ((y1-x1) << 1);
		  }
	}
	while(y1 >= x1);
}


// вспомогательная функция.
// рисует круг с радиусом r. Затем делит его на четверти и растаскивает их по углам
// прямоугольника (x, y, w, h - прямоугольник). Затем инвертирует область между четвертями.
// середина остается незатронутой. Нужно использовать функцию InvertRectEx
void PBGraphics::invertCircleEx(int x, int y, int w, int h, int r)
{
	int p = 1 - r;
	int x1 = 0;
	int y1 = r;
	TInvertCircleData data;

	do
	{
		circle_invert_helper(x, y, w, h, r, x1, y1, data);

		++x1;
		if (p < 0)
			p += 1 + (x1<<1);
		else
		{
			--y1;
			p += 1 - ((y1-x1) << 1);
		}
	}
	while(y1 >= x1);

	for (TInvertCircleData::iterator it = data.begin(); it != data.end(); it++)
	{
		const PBPoint& p1 = (*it).second.first;
		const PBPoint& p2 = (*it).second.second;
		assert(p1.GetY() == p2.GetY());
		assert(p1.GetX() < p2.GetX());

		::InvertAreaBW(p1.GetX(), p1.GetY(), p2.GetX() - p1.GetX() + 1, 1);
	}
}

void PBGraphics::DrawRectEx(int x, int y, int w, int h, int cornerRadius, int thickness, int lineColor, int fillColor)
{
	assert(thickness > 0);

	if (fillColor > 0)
	{
		FillRectEx(x, y, w, h, cornerRadius, fillColor);
	}

	if (cornerRadius > 0 && m_pParent && m_pParent->IsVisible())
	{
		PBRect screenRect = m_pParent->ClientToScreen(PBRect(x,y,w,h));
		drawCircleEx(screenRect.GetX(), screenRect.GetY(), screenRect.GetWidth(), screenRect.GetHeight(), cornerRadius, thickness, lineColor);

		::FillArea(screenRect.GetX()+cornerRadius, screenRect.GetY(), screenRect.GetWidth()-cornerRadius*2, thickness, lineColor);
		::FillArea(screenRect.GetX()+cornerRadius, screenRect.GetY()+screenRect.GetHeight()-thickness, screenRect.GetWidth()-cornerRadius*2, thickness, lineColor);

		::FillArea(screenRect.GetX(), screenRect.GetY()+cornerRadius, thickness, screenRect.GetHeight()-cornerRadius*2, lineColor);
		::FillArea(screenRect.GetX()+screenRect.GetWidth()-thickness, screenRect.GetY()+cornerRadius, thickness, screenRect.GetHeight()-cornerRadius*2, lineColor);

		m_pParent->Invalidate(screenRect, UPDATE_TYPE(lineColor));
	}
	else
	{
		DrawRect(x, y, w, h, lineColor);
	}
}

void PBGraphics::FillRectEx(int x, int y, int w, int h, int cornerRadius, int fillColor)
{
	if (cornerRadius > 0 && m_pParent && m_pParent->IsVisible())
	{
		PBRect screenRect = m_pParent->ClientToScreen(PBRect(x,y,w,h));

		fillCircleEx(screenRect.GetX(), screenRect.GetY(), screenRect.GetWidth(), screenRect.GetHeight(), cornerRadius, fillColor);

		::FillArea(screenRect.GetX(), screenRect.GetY()+cornerRadius, screenRect.GetWidth(), screenRect.GetHeight()-cornerRadius*2, fillColor);

		m_pParent->Invalidate(screenRect, UPDATE_TYPE(fillColor));
	}
	else
	{
		FillArea(x, y, w, h, fillColor);
	}
}

void PBGraphics::InvertRectEx(int x, int y, int w, int h, int cornerRadius)
{
	if (cornerRadius > 0 && m_pParent && m_pParent->IsVisible())
	{
		PBRect screenRect = m_pParent->ClientToScreen(PBRect(x,y,w,h));

		//fillCircleEx(screenRect.GetX(), screenRect.GetY(), screenRect.GetWidth(), screenRect.GetHeight(), cornerRadius, DGRAY);
		invertCircleEx(screenRect.GetX(), screenRect.GetY(), screenRect.GetWidth(), screenRect.GetHeight(), cornerRadius);

		::InvertAreaBW(screenRect.GetX(), screenRect.GetY()+cornerRadius+1, screenRect.GetWidth(), screenRect.GetHeight()-cornerRadius*2 - 2);

		m_pParent->Invalidate(screenRect, UPDATE_TYPE(DGRAY)); // BW update
	}
}
