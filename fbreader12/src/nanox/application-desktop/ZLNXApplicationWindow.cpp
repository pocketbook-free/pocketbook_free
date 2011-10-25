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

#include <ZLibrary.h>

#include "ZLNXApplicationWindow.h"

#include "../view/ZLNXViewWidget.h"
#include "../../dialogs/ZLOptionView.h"
#include "../dialogs/ZLNXDialogManager.h"


ZLNXApplicationWindow::ZLNXApplicationWindow(ZLApplication *application) :
	ZLDesktopApplicationWindow(application) {
}

void ZLNXApplicationWindow::init() {
	ZLDesktopApplicationWindow::init();
	switch (myWindowStateOption.value()) {
		case NORMAL:
			break;
		case FULLSCREEN:
			setFullscreen(true);
			break;
		case MAXIMIZED:
			break;
	}
}

ZLNXApplicationWindow::~ZLNXApplicationWindow() {
	//GrClose();
}

void ZLNXApplicationWindow::onButtonPress() {
//		application().doActionByKey(ZLNXKeyUtil::keyName(event));
}

//void ZLNXApplicationWindow::addToolbarItem(ZLApplication::Toolbar::ItemPtr item) {
//}

void ZLNXApplicationWindow::setFullscreen(bool fullscreen) {
}

bool ZLNXApplicationWindow::isFullscreen() const {
	return true;
}

ZLViewWidget *ZLNXApplicationWindow::createViewWidget() {
	ZLNXViewWidget *viewWidget = new ZLNXViewWidget(&application(), (ZLView::Angle)application().AngleStateOption.value());
	return viewWidget;
}

void ZLNXApplicationWindow::close() {
}

void ZLNXApplicationWindow::grabAllKeys(bool) {
}

void ZLNXDialogManager::createApplicationWindow(ZLApplication *application) const {
	ZLNXApplicationWindow *mw = new ZLNXApplicationWindow(application);
}
