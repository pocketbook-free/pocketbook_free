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

package ua.pocketbook.fb2viewer.fbreader.bookmodel;


import ua.pocketbook.fb2viewer.zlibrary.core.tree.ZLTree;
import ua.pocketbook.fb2viewer.zlibrary.text.model.ZLTextModel;

public class TOCTree extends ZLTree<TOCTree> {
	private String myText;
	public Reference myReference;

	protected TOCTree() {
		super();
	}

	public TOCTree(TOCTree parent) {
		super(parent);
	}
	
	public TOCTree(int level ,TOCTree parent, int position) {
		super(level, parent, position);
	}

	public final String getText() {
		return myText;
	}

	public final void setText(String text) {
		myText = text;
	}
	
	public Reference getReference() {
		return myReference;
	}
	
	public void setReference(ZLTextModel model, int reference) {
		myReference = new Reference(reference, model);
	}

	public static class Reference {
		public final int ParagraphIndex;
		public final ZLTextModel Model;
		public final boolean isNoteStruct;
		public final boolean isNotes;

		public int myBookmarkId;
		
		public Reference(final int paragraphIndex, final ZLTextModel model) {
			ParagraphIndex = paragraphIndex;
			Model = model;
			isNoteStruct = false;
			isNotes = false;
		}
		
		public Reference(final int paragraphIndex, final ZLTextModel model, boolean bookmark, int id, boolean note ) {
			ParagraphIndex = paragraphIndex;
			Model = model;
			isNoteStruct = bookmark;
			myBookmarkId = id;
			isNotes = note;
		}
	}
}
