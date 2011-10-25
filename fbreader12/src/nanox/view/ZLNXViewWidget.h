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

#ifndef __ZLNXVIEWWIDGET_H__
#define __ZLNXVIEWWIDGET_H__

#include <ZLView.h>
#include <ZLViewWidget.h>
#include <ZLApplication.h>

extern char *buf;

class ZLNXViewWidget : public ZLViewWidget {
	public:
	ZLNXViewWidget(ZLApplication *application, ZLView::Angle initialAngle);
		~ZLNXViewWidget();

		void setSize(int width, int height, int scanline);
		int width() const;
		int height() const;

		// virtual
		void setScrollbarEnabled(ZLView::Direction /*direction*/, bool /*enabled*/) {};
		void setScrollbarPlacement(ZLView::Direction /*direction*/, bool /*standard*/) {};
		void setScrollbarParameters(ZLView::Direction /*direction*/, size_t /*full*/, size_t /*from*/, size_t /*to*/) {};


	private:
		void updateCoordinates(int &x, int &y);

		void trackStylus(bool track);
		void repaint();

		ZLApplication *myApplication;
};

#endif /* __ZLNXVIEWWIDGET_H__ */
