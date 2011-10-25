/*
 * Copyright (C) 2009-2010 Geometer Plus <contact@geometerplus.com>
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

package ua.pocketbook.fb2viewer.android.fbreader;

import ua.pocketbook.reader.R;
import android.app.Activity;

public class TextSearchActivity extends SearchActivity {
	@Override
	void onSuccess() {
		FBReader.Instance.showTextSearchControls(true);
	}

	@Override
	void onFailure() {
		FBReader.Instance.showTextSearchControls(false);
	}

	@Override
	String getFailureMessageResourceKey() {
		return getResources().getString(R.string.textNotFound);
	//	return "textNotFound";
	}

	@Override
	String getWaitMessageResourceKey() {
		return "search";
	}

	@Override
	boolean runSearch(final String pattern) {
		final ua.pocketbook.fb2viewer.fbreader.fbreader.FBReader fbreader =
			(ua.pocketbook.fb2viewer.fbreader.fbreader.FBReader)ua.pocketbook.fb2viewer.fbreader.fbreader.FBReader.Instance();
		fbreader.TextSearchPatternOption.setValue(pattern);
		return fbreader.getTextView().search(pattern, true, false, false, false) != 0;
	}

	@Override
	Activity getParentActivity() {
		return FBReader.Instance;
	}
}
