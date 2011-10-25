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

#ifndef __ZLNXAPPLICATIONWINDOW_H__
#define __ZLNXAPPLICATIONWINDOW_H__

#include <vector>
#include <map>
#include <assert.h>

#include "../../desktop/application/ZLDesktopApplicationWindow.h"


class ZLNXApplicationWindow : public ZLDesktopApplicationWindow { 

	public:
		ZLNXApplicationWindow(ZLApplication *application);
		~ZLNXApplicationWindow();

		void onButtonPress();

		void addToolbarItem(ZLToolbar::ItemPtr item) {assert(0);};
		void setToggleButtonState(const ZLToolbar::ToggleButtonItem &toggleButton) {assert(0);};
		void processAllEvents() {/*assert(0);*/};


	private:
		ZLNXApplicationWindow *myWindow;
		ZLViewWidget *createViewWidget();
		void init();
		void close();

		void grabAllKeys(bool grab);
//		void addToolbarItem(ZLApplication::Toolbar::ItemPtr item);
		void setCaption(const std::string &caption) { }

		bool isFullscreen() const;
		void setFullscreen(bool fullscreen);
};

#endif /* __ZLNXAPPLICATIONWINDOW_H__ */
