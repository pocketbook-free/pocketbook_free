#include "pdfviewer.h"
#include "pbnotes.h"

#ifdef SYNOPSISV2
#include "pbmainframe.h"
#endif

static int slx, sly, slw, slh, slpage = -1, slscale, slorn, watermark = -1;
static int after_hand_move = 0;
static pid_t bgpid = 0;

void bg_monitor()
{

	char buf[64];
	struct stat st;
	int status;

	if (bgpid == 0) return;
	waitpid(-1, &status, WNOHANG);
	sprintf(buf, "/proc/%i/cmdline", bgpid);
	if (stat(buf, &st) == -1)
	{
		bgpid = 0;
		return;
	}
	SetHardTimer("BGPAINT", bg_monitor, 500);

}

void kill_bgpainter()
{

	int status;

	if (bgpid != 0)
	{
		kill(bgpid, SIGKILL);
		usleep(100000);
		waitpid(-1, &status, WNOHANG);
		bgpid = 0;
	}

}

void draw_wait_thumbnail()
{
	if (thw == 0 || thh == 0) return;
	FillArea(thx, thy, thw, thh, WHITE);
	DrawBitmap(thx + (thw - hgicon.width) / 2, thy + (thh - hgicon.height) / 2, &hgicon);
	PartialUpdate(thx, thy, thw, thh);
	thw = thh = 0;
}

void getpagesize(int n, int* w, int* h, double* res, int* marginx, int* marginy)
{

	int mx, my, realscale;
	int pw, ph, tmp;

	if (reflow_mode)
	{
		pw = (int)ceil(doc->getPageMediaWidth(n));
		ph = (int)ceil(doc->getPageMediaHeight(n));
	}
	else
	{
		pw = (int)ceil(doc->getPageCropWidth(n));
		ph = (int)ceil(doc->getPageCropHeight(n));
	}
	int rt = (int)ceil(doc->getPageRotate(n));
	if (rt == 90 || rt == 270)
	{
		tmp = pw;
		pw = ph;
		ph = tmp;
	}
	int sw = ScreenWidth();
	int ush = ScreenHeight() - panelh - 15;

	//fprintf(stderr, "(%i,%i) (%i,%i) %i\n", pw, ph, cw, ch, rt);
	if (reflow_mode)
	{
		realscale = rscale;
	}
	else
	{
//		realscale = (scale < 200) ? scale : (scale * CSCALEADJUST) / 100;
		realscale = (scale < 200) ? scale : scale;
	}
	*w = ((sw * realscale) / 100);
	*h = (((*w) * ph) / pw);
	*res = ((((double)realscale * 72.0) / 100.0) * sw) / pw;

	if (scale > 50 && scale < 200)
	{
		mx = (*w - sw) / 2;
		if (mx < 0) mx = 0;
		if (*h < ush)
		{
			my = 0;
		}
		else
		{
			my = (*h - ush) / 2;
			if (my > mx * 2) my = mx;
			if (my < 0) my = 0;
		}
	}
	else
	{
		mx = my = 0;
	}

	n = *h / ush;
	if (n > 0 && *h - (n * ush) < *h / 10)
	{
		my = (*h - (n * ush)) / 2 + 2;
	}

	*marginx = mx;
	*marginy = my;

}

void find_off(int step)
{
	int sw = ScreenWidth();
	int sh = ScreenHeight();
	int pw, ph, marginx, marginy;
	double res;

	getpagesize(cpage, &pw, &ph, &res, &marginx, &marginy);

	step = (step * (sh - panelh) * 9) / 10;

	//fprintf(stderr, "step=%i offx=%i offy=%i mx=%i ,my=%i pw=%i ph=%i\n", step,offx,offy,marginx,marginy,pw,ph);

	if (step < 0)
	{
		step = -step;
		if (offy <= 1)
		{
			if (scale < 200 || offx <= marginx)
			{
				if (cpage == 1) return;
				cpage--;
				offx = pw - sw;
			}
			else
			{
				offx -= (sw * CSCALEADJUST) / 100;
			}
			offy = ph;
		}
		else
		{
			watermark = offy;
			offy -= step;
		}
	}
	else
	{
		if (offy + (sh - panelh) >= ph - 1)
		{
			if (scale < 200 || offx >= pw - sw - marginx)
			{
				if (cpage >= npages) return;
				cpage++;
				offx = 0;
			}
			else
			{
				offx += (sw * CSCALEADJUST) / 100;
			}
			offy = 1;
		}
		else
		{
			watermark = offy + (sh - panelh);
			offy += step;
		}
	}

}

void find_off_x(int step)
{
	int sw = ScreenWidth();
	int pw, ph, marginx, marginy;
	double res;

	getpagesize(cpage, &pw, &ph, &res, &marginx, &marginy);

	step *= (sw / 3);

	//fprintf(stderr, "step=%i offx=%i offy=%i mx=%i ,my=%i pw=%i ph=%i\n", step,offx,offy,marginx,marginy,pw,ph);

	if (step < 0)
	{
		if (scale < 200 || offx <= marginx)
		{
			if (cpage == 1) return;
			cpage--;
			offx = pw - sw;
		}
		else
		{
			offx += step;
		}
	}
	else
	{
		if (scale < 200 || offx >= pw - sw - marginx)
		{
			if (cpage >= npages) return;
			cpage++;
			offx = 0;
		}
		else
		{
			offx += step;
		}
	}

}

void find_off_xy(int xstep, int ystep)
{

	int sw = ScreenWidth();
	int sh = ScreenHeight();
	int pw, ph, marginx, marginy;
	double res;

	getpagesize(cpage, &pw, &ph, &res, &marginx, &marginy);

	offx += xstep;
	if (offx >= pw - sw) offx = pw - sw;
	if (offx < 0) offx = 0;
	offy += ystep;
	if (offy >= ph - (sh - panelh)) offy = ph - (sh - panelh);
	if (offy <= 0) offy = 1;
	after_hand_move = 1;

}

int is_page_cached(int pagenum, int sc, int orn)
{

	if (reflow_mode) slpage = -1;
	return (pagenum == slpage && sc == slscale && orn == slorn);

}

void display_slice(int pagenum, int sc, double res, GBool withreflow, int x, int y, int  w, int h)
{

	/*

	  void displayPage(OutputDev *out, int page,
		 double hDPI, double vDPI, int rotate,
		 GBool useMediaBox, GBool crop, GBool printing,
		 GBool (*abortCheckCbk)(void *data) = NULL,
		 void *abortCheckCbkData = NULL,
					 GBool (*annotDisplayDecideCbk)(Annot *annot, void *user_data) = NULL,
					 void *annotDisplayDecideCbkData = NULL);

	  void displayPageSlice(OutputDev *out, int page,
		double hDPI, double vDPI, int rotate,
		GBool useMediaBox, GBool crop, GBool printing,
		int sliceX, int sliceY, int sliceW, int sliceH,
		GBool (*abortCheckCbk)(void *data) = NULL,
		void *abortCheckCbkData = NULL,
						  GBool (*annotDisplayDecideCbk)(Annot *annot, void *user_data) = NULL,
						  void *annotDisplayDecideCbkData = NULL);
		  */

	int orn = GetOrientation();

	//doc->displayPageSlice(splashOut, pagenum, res, res, 0, withreflow, !withreflow, gFalse, x, y, w, h);

	slx = x;
	sly = y;
	slw = w;
	slh = h;

	if (is_page_cached(pagenum, sc, orn))
	{
		fprintf(stderr, "+%i:%i (%i,%i,%i,%i)\n", pagenum, sc, x, y, w, h);
		return;
	}

	fprintf(stderr, "-%i:%i (%i,%i,%i,%i)\n", pagenum, sc, x, y, w, h);
	if (! reflow_mode)
	{
		slpage = pagenum;
		slscale = sc;
		slorn = orn;
	}
	doc->displayPage(splashOut, pagenum, res, res, 0, withreflow, !withreflow, gFalse);

}

void get_bitmap_data(unsigned char** data, int* w, int* h, int* row)
{

	unsigned char* cdata;
	int cw, ch, crow;

	cdata = (unsigned char*)(splashOut->getBitmap()->getDataPtr());
	cw = splashOut->getBitmap()->getWidth();
	ch = splashOut->getBitmap()->getHeight();
	crow = splashOut->getBitmap()->getRowSize();

	cdata += (USE4 ? slx / 2 : slx) + sly * crow;
	cw -= slx;
	ch -= sly;
	if (cw > slw) cw = slw;
	if (ch > slh) ch = slh;

	if (data) *data = cdata;
	if (w) *w = cw;
	if (h) *h = ch;
	if (row) *row = crow;

}

static int center_image(int sw, int* rw)
{

	unsigned char* data, *p, mask;
	int* arl, *arr;
	int w, h, row;
	int x1, x2, y, pxinbyte, npts, n, n2, sum, mins;
	int c, c0;
	int xleft, xright, ytop, ybottom;

	get_bitmap_data(&data, &w, &h, &row);

	if (USE4)
	{
		mask = 0xcc;
		pxinbyte = 2;
	}
	else
	{
		mask = 0xc0;
		pxinbyte = 1;
	}

	npts = (w - sw) + 8;
	xleft = npts;
	xright = w - npts;
	ytop = h / 24;
	ybottom = h - h / 24;

	arl = (int*)malloc((npts + 1) * sizeof(int));
	arr = (int*)malloc((npts + 1) * sizeof(int));

	n = 0;
	for (x1 = 0; x1 < npts; x1 += pxinbyte)
	{
		p = data + (x1 + 4) / pxinbyte + row * ytop;
		sum = 0;
		c0 = mask;
		for (y = ytop; y < ybottom; y++)
		{
			c = (*p & mask);
			if (c != c0) sum++;
			c0 = c;
			p += row;
		}
		arl[n++] = sum;
	}
	arr[n] = 0;

	n = 0;
	for (x2 = w; x2 > w - npts; x2 -= pxinbyte)
	{
		p = data + (x2 - 6) / pxinbyte + row * ytop;
		sum = 0;
		c0 = mask;
		for (y = ytop; y < ybottom; y++)
		{
			c = (*p & mask);
			if (c != c0) sum++;
			c0 = c;
			p += row;
		}
		arr[n++] = sum;
	}
	arr[n] = 0;
	n2 = n;

	//fprintf(stderr, "w=%i h=%i (%i,%i)=%i\n", w, h, x2, x1, x1+(x2-x1)/2);

	if (rw)
	{
		for (x1 = 0, n = 0; x1 < npts; x1 += pxinbyte)
		{
			if (arl[n++] > 4) break;
		}
		for (x2 = w, n = 0; x2 > w - npts; x2 -= pxinbyte)
		{
			if (arr[n++] > 4) break;
		}
		*rw = x2 - x1;
	}

	n = sum = 0;
	mins = -99999999;
	for (x1 = x2 = 0; x1 < npts; x1 += pxinbyte)
	{
		sum += (arr[--n2] * (1000 + n2)) / 1000;
		if (mins < sum)
		{
			mins = sum;
			x2 = x1;
		}
		sum -= (arl[n++] * (1000 + n)) / 1000;
	}

	free(arl);
	free(arr);

	return x2;

}

int get_fit_scale()
{

	int sw, sh, pw, ph, marginx, marginy, rw;
	double res;

	scale = 100;
	sw = ScreenWidth();
	sh = ScreenHeight();
	getpagesize(cpage, &pw, &ph, &res, &marginx, &marginy);
	splashOut->setup(gFalse, 0, 0, 0, sw, sh - panelh, 0, 0, 0, 0, res);
	display_slice(cpage, scale, res, gFalse, 0, 0, pw, ph);
	center_image(pw / 2, &rw);
	return (sw * 99) / rw;

}

static void draw_page_image()
{

	int sw, sh, pw, ph, x, y, w, h, dx, row, orn, i, grads;
	FixedPoint tx, ty, tw, th, cw, mw;
	int marginx, marginy;
	double res;
	unsigned char* data;

	orn = GetOrientation();
	if (! is_page_cached(cpage, scale, orn))
	{
		draw_wait_thumbnail();
	}
	ClearScreen();

	SetOrientation(orient);
	sw = ScreenWidth();
	sh = ScreenHeight();

	getpagesize(cpage, &pw, &ph, &res, &marginx, &marginy);

	scrx = scry = dx = 0;

	if (! after_hand_move)
	{
		if (pw - sw - marginx < offx) offx = pw - sw - marginx;
		if (offx < marginx) offx = marginx;
		//if (offy < marginy) offy = marginy;
		//if (ph-(sh-panelh-15)-marginy < offy) offy = ph-(sh-panelh-15)-marginy;
		if (ph - (sh - panelh) < offy) offy = ph - (sh - panelh);
		if (offy < 0) offy = 1;
		if (offy == 0) offy = marginy;
	}

	if (! reflow_mode)
	{

		if (scale > 50 && scale <= 99)
		{
			scrx = (sw - pw) / 2;
			offx = 0;
		}
		splashOut->setup(gFalse, 0, 0, 0, sw, sh - panelh, 0, 0, 0, 0, res);
		if (scale > 100 && scale <= 199 && ! after_hand_move)
		{
			display_slice(cpage, scale, res, gFalse, 0, offy, pw, sh - panelh);
			//doc->displayPageSlice(splashOut, cpage, res, res, 0, gFalse, gTrue/*gFalse*/, gFalse, 0, offy, pw, sh);
			dx = center_image(sw, NULL);
			offx = dx;
			if (dx < 0) dx = 0;
		}
		else
		{
			display_slice(cpage, scale, res, gFalse, offx, offy, sw, sh - panelh);
			//doc->displayPageSlice(splashOut, cpage, res, res, 0, gFalse, gTrue/*gFalse*/, gFalse, offx, offy, sw, sh);
		}
		nsubpages = 1;

	}
	else
	{

		w = sw - sw / 17;
		h = sh - panelh;
		x = w / 34;
		y = 0;
		tw = (doc->getPageMediaWidth(cpage) / pw) * w;
		tx = tw / 34.0;
		th = (FixedPoint)((h * 72.0) / res);
		ty = doc->getPageMediaHeight(cpage);
		//fprintf(stderr, "pw=%i ph=%i cw=%i mw=%i tx=%i ty=%i tw=%i th=%i\n", (int)pw, (int)ph, (int)cw, (int)mw, (int)tx, (int)ty, (int)tw, (int)th);

		if (flowpage != cpage || flowscale != rscale || flowwidth != w || flowheight != h)
		{
			splashOut->setup(gTrue, -1, x, y, w, h, tx, ty, tw, th, res);
			//doc->displayPageSlice(splashOut, cpage, res, res, 0, gTrue/*gFalse*/, /*gTrue*/gFalse, gFalse, 0, 0, sw, sh);
			display_slice(cpage, scale, res, gTrue, 0, 0, sw, sh);
			flowpage = cpage;
			flowscale = rscale;
			flowwidth = w;
			flowheight = h;
		}
		nsubpages = splashOut->subpageCount();
		if (subpage >= nsubpages) subpage = nsubpages - 1;
		splashOut->setup(gTrue, subpage, x, y, w, h, tx, ty, tw, th, res);
		//doc->displayPageSlice(splashOut, cpage, res, res, 0, gTrue/*gFalse*/, /*gTrue*/gFalse, gFalse, 0, 0, sw, sh);
		display_slice(cpage, scale, res, gTrue, 0, 0, sw, sh);

	}

	get_bitmap_data(&data, &w, &h, &row);

	Stretch(data, USE4 ? IMAGE_GRAY4 : IMAGE_GRAY8, w, h, row, scrx - dx, scry, w, h, 0);
	grads = (1 << GetHardwareDepth());
	if (grads > 4) DitherArea(scrx - dx, scry, w, h, grads, DITHER_DIFFUSION);

	if (watermark >= 0)
	{
		y = watermark - offy;
		if (y > 0 && y < sh - panelh)
		{
			for (x = 0; x < sw; x += 4)
			{
				DrawPixel(x, y, DGRAY);
				DrawPixel(x + 1, y, DGRAY);
			}
		}
		watermark = -1;
	}

	//scrx -= dx;

	if (search_mode)
	{
		for (i = 0; i < nresults; i++)
		{
			InvertArea(results[i].x - offx, results[i].y - offy, results[i].w, results[i].h);
		}
	}

	thiw = (sw * 100) / pw;
	if (thiw > 100) thiw = 100;
	thih = (sh * 100) / ph;
	if (thih > 100) thih = 100;
	thix = (offx * 100) / pw;
	if (thix < 0) thix = 0;
	if (thix + thiw > 100) thix = 100 - thiw;
	thiy = (offy * 100) / ph;
	if (thiy < 0) thiy = 0;
	if (thiy + thih > 100) thiy = 100 - thih;
	after_hand_move = 0;

}

static int draw_pages()
{

	int sw, sh, pw, ph, nx, ny, boxx, boxy, boxw, boxh, xx, yy, n, row, marginx, marginy, w, h;
	double res;
	unsigned char* data;
	pid_t pid;

	sw = ScreenWidth();
	sh = ScreenHeight();

	if (is_portrait() && scale == 50)
	{
		nx = ny = 2;
	}
	else if (is_portrait() && scale == 33)
	{
		nx = ny = 3;
	}
	else if (!is_portrait() && scale == 50)
	{
		nx = 2;
		ny = 1;
	}
	else if (!is_portrait() && scale == 33)
	{
		nx = 3;
		ny = 2;
	}
	else
	{
		return 1;
	}
	boxw = sw / nx;
	boxh = (sh - PanelHeight()) / ny;

	for (yy = 0; yy < ny; yy++)
	{
		for (xx = 0; xx < nx; xx++)
		{
			if (cpage + yy * nx + xx > npages) break;
			boxx = xx * boxw;
			boxy = yy * boxh;
			DrawRect(boxx + 6, boxy + boxh - 4, boxw - 8, 2, DGRAY);
			DrawRect(boxx + boxw - 4, boxy + 6, 2, boxh - 8, DGRAY);
			DrawRect(boxx + 4, boxy + 4, boxw - 8, boxh - 8, BLACK);
		}
	}

#ifndef EMULATOR

	if ((pid = fork()) == 0)
	{

#else

	FullUpdate();

#endif

		for (yy = 0; yy < ny; yy++)
		{
			for (xx = 0; xx < nx; xx++)
			{
				n = cpage + yy * nx + xx;
				if (n > npages) break;
				boxx = xx * boxw;
				boxy = yy * boxh;
				getpagesize(n, &pw, &ph, &res, &marginx, &marginy);
				splashOut->setup(gFalse, 0, 0, 0, boxw - 10, boxh - 10, 0, 0, 0, 0, res);
				display_slice(n, scale, res, gFalse, 5, (scale == 33 && !is_portrait()) ? 15 : 5, boxw - 10, boxh - 10);
				//doc->displayPageSlice(splashOut, n, res, res, 0, gFalse, gTrue/*gFalse*/, gFalse,
				//  5, (scale==33 && !is_portrait()) ? 15 : 5, boxw-10, boxh-10);
				get_bitmap_data(&data, &w, &h, &row);
				Stretch(data, USE4 ? IMAGE_GRAY4 : IMAGE_GRAY8, w, h, row,
						boxx + 5, boxy + 5, w, h, 0);
				//DitherArea(boxx+5, boxy+5, boxw-10, boxh-10, 2, DITHER_DIFFUSION);
				PartialUpdate(boxx + 5, boxy + 5, boxw, boxh);
			}
		}

#ifndef EMULATOR
		exit(0);

	}
	else
	{

		bgpid = pid;

	}

#endif

	return cpage + nx * ny > npages ? npages + 1 - cpage : nx * ny;


}

static void draw_zoomer()
{
	DrawBitmap(ScreenWidth() - zoombm.width - 10, ScreenHeight() - zoombm.height - 35, &zoombm);
}

static void draw_searchpan()
{
	DrawBitmap(ScreenWidth() - searchbm.width, ScreenHeight() - searchbm.height - PanelHeight(), &searchbm);
}


void draw_bmk_flag(int update)
{
#ifdef USESYNOPSIS
	DrawSynopsisBookmarks((long long)cpage << 40);
	return;
#endif
	int i, x, y, bmkset = 0;
	if (! bmk_flag) return;
	for (i = 0; i < docstate.nbmk; i++)
	{
		if (docstate.bmk[i] == cpage) bmkset = 1;
	}
	if (! bmkset) return;
	x = ScreenWidth() - bmk_flag->width;
	y = 0;
	DrawBitmap(x, y, bmk_flag);
	if (update) PartialUpdate(x, y, bmk_flag->width, bmk_flag->height);
}

void out_page(int full)
{
    char buf[32];
    int n = 1, h;

    if (reflow_mode || scale > 50)
    {
            draw_page_image();
    }
    else
    {
            ClearScreen();
            n = draw_pages();
            SetHardTimer("BGPAINT", bg_monitor, 500);
    }

    if (zoom_mode)
    {
            draw_zoomer();
    }
    if (search_mode)
    {
            draw_searchpan();
    }
    if (n == 1)
    {
            if (clock_left)
            {
                    sprintf(buf, "    %i%%    %i / %i", reflow_mode ? rscale : scale, cpage, npages);
            }
            else
            {
                    sprintf(buf, "    %i / %i    %i%%", cpage, npages, reflow_mode ? rscale : scale);
            }
    }
    else
    {
            if (clock_left)
            {
                    sprintf(buf, "    %i%%    %i-%i / %i", scale, cpage, cpage + n - 1, npages);
            }
            else
            {
                    sprintf(buf, "    %i-%i / %i    %i%%", cpage, cpage + n - 1, npages, scale);
            }
    }

#ifdef SYNOPSISV2
    if (PBPDFToolBar::GetThis()->GetTool() == 0)
	    PBMainFrame::GetThis()->DrawPen();
#endif

#ifdef PBBER
    h = DrawPanel(NULL, buf, GetLangText("@BeratorBig"), (cpage * 100) / npages);
#else
    h = DrawPanel(NULL, buf, book_title, (cpage * 100) / npages);
#endif
#ifdef SYNOPSISV2
//	if (GetEventHandler() != PBMainFrame::main_handler)
    if (PBPDFToolBar::GetThis()->GetTool() != 0)
            return;
#endif
    thh = h - 6;
    thw = (thh * 6) / 8;
    thx = clock_left ? ScreenWidth() - (thw + 4) : 4;
    thy = ScreenHeight() - h + 4;
    FillArea(thx, thy, thw, thh, WHITE);
    thx += 1;
    thy += 1;
    thw -= 2;
    thh -= 2;
    if ((scale >= 200 || (scale >= 100 && !is_portrait())) && !reflow_mode)
    {
            FillArea(thx + (thw * thix) / 100, thy + (thh * thiy) / 100, (thw * thiw) / 100, (thh * thih) / 100, BLACK);
    }

    draw_bmk_flag(0);

	if (full == 1)
	{
		FullUpdate();
	}
	else if (full != -1)
	{
		PartialUpdate(0, 0, ScreenWidth(), ScreenHeight());
	}
}
