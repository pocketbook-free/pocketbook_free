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

#include <stdio.h>
#include "ZLNXDialogManager.h"

shared_ptr<ZLDialog> ZLNXDialogManager::createDialog(const ZLResourceKey &key) const {
}

shared_ptr<ZLOptionsDialog> ZLNXDialogManager::createOptionsDialog(const ZLResourceKey &id, shared_ptr<ZLRunnable> applyAction, bool showApplyButton) const {
}

void ZLNXDialogManager::informationBox(const ZLResourceKey &key, const std::string &message) const {
	printf("informationBox: %s\n", message.c_str());
}

void ZLNXDialogManager::errorBox(const ZLResourceKey &key, const std::string &message) const {
	printf("errorBox: %s\n", message.c_str());
}

int ZLNXDialogManager::questionBox(const ZLResourceKey &key, const std::string &message, const ZLResourceKey &button0, const ZLResourceKey &button1, const ZLResourceKey &button2) const {
	printf("questionBox: %s\n", message.c_str());
}

bool ZLNXDialogManager::selectionDialog(const ZLResourceKey &key, ZLTreeHandler &handler) const {
}

void ZLNXDialogManager::wait(const ZLResourceKey &key, ZLRunnable &runnable) const {
	printf("wait\n");
	runnable.run();
}

bool ZLNXDialogManager::isClipboardSupported(ClipboardType) const {
	return true;
}

void ZLNXDialogManager::setClipboardText(const std::string &text, ClipboardType type) const {
}

