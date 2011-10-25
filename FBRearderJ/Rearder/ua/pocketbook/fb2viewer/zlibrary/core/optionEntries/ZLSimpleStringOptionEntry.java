/*
 * Copyright (C) 2007-2010 Geometer Plus <contact@geometerplus.com>
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

package ua.pocketbook.fb2viewer.zlibrary.core.optionEntries;

import ua.pocketbook.fb2viewer.zlibrary.core.dialogs.ZLStringOptionEntry;
import ua.pocketbook.fb2viewer.zlibrary.core.options.ZLStringOption;

public class ZLSimpleStringOptionEntry extends ZLStringOptionEntry {
	private final ZLStringOption myOption;
	
	public ZLSimpleStringOptionEntry(ZLStringOption option) {
		myOption = option;
	}
	
	public String initialValue() {
		return myOption.getValue();
	}

	public void onAccept(String value) {
		myOption.setValue(value);
	}
}
