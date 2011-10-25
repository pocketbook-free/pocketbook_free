/*
 * Copyright (C) 2008 Alexander Egorov <lunohod@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "ZLNXViewWidget.h"
#include "ZLNXPaintContext.h"

void ZLNXViewWidget::updateCoordinates(int &x, int &y) {
	switch (rotation()) {
		default:
			break;
		case ZLView::DEGREES90:
		{
			int tmp = x;
			x = height() - y;
			y = tmp;
			break;
		}
		case ZLView::DEGREES180:
			x = width() - x;
			y = height() - y;
			break;
		case ZLView::DEGREES270:
		{
			int tmp = x;
			x = y;
			y = width() - tmp;
			break;
		}
	}
}

void ZLNXViewWidget::setSize(int width, int height, int scanline) {

	ZLNXPaintContext &NXContext = (ZLNXPaintContext&)view()->context();
	NXContext.setSize(width, height, scanline);

}


int ZLNXViewWidget::width() const {
	ZLNXPaintContext &NXContext = (ZLNXPaintContext&)view()->context();
	return NXContext.width();
}

int ZLNXViewWidget::height() const {
	ZLNXPaintContext &NXContext = (ZLNXPaintContext&)view()->context();
	return NXContext.height();
}

ZLNXViewWidget::ZLNXViewWidget(ZLApplication *application, ZLView::Angle initialAngle) : ZLViewWidget(initialAngle) {
	myApplication = application;
}

ZLNXViewWidget::~ZLNXViewWidget() {
}

void ZLNXViewWidget::repaint()	{
	ZLNXPaintContext &NXContext = (ZLNXPaintContext&)view()->context();
		//printf("repaint\n");
//	memset(buf, 0xff, 800*600/4);
	view()->paint();
}

void ZLNXViewWidget::trackStylus(bool track) {
}
