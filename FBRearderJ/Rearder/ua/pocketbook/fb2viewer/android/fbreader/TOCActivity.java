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

import java.util.ArrayList;
import java.util.List;

import android.os.Bundle;
import android.view.*;
import android.widget.*;
import android.content.Context;
import android.content.Intent;
import android.app.ListActivity;
import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;


import ua.pocketbook.reader.R;
import ua.pocketbook.reader.ReaderActivity;
import ua.pocketbook.utils.BookStruct;
import ua.pocketbook.utils.BooksDatabase;
import ua.pocketbook.utils.NoteStruct;


import ua.pocketbook.fb2viewer.fbreader.bookmodel.TOCTree;
import ua.pocketbook.fb2viewer.fbreader.fbreader.FBReader;
import ua.pocketbook.fb2viewer.zlibrary.core.application.ZLApplication;
import ua.pocketbook.fb2viewer.zlibrary.core.resources.ZLResource;
import ua.pocketbook.fb2viewer.zlibrary.core.tree.ZLTree;
import ua.pocketbook.fb2viewer.zlibrary.text.view.ZLTextWordCursor;

public class TOCActivity extends ListActivity {

	//Action
        public static final String TOC = "fbreader_toc"; 

	private TOCAdapter myAdapter;
	private ZLTree mySelectedItem;
	private FBReader fbreader;

	@Override
	public void onCreate(Bundle bundle) {
		super.onCreate(bundle);

		Thread.setDefaultUncaughtExceptionHandler(new ua.pocketbook.fb2viewer.zlibrary.ui.android.library.UncaughtExceptionHandler(this));
		try{
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		
		
		

		fbreader = (FBReader)ZLApplication.Instance();
		
		
		TOCTree root = fbreader.Model.TOCTree;
		//TODO Active Contents
		root = doActive(root);
		
		myAdapter = new TOCAdapter(root);
		final ZLTextWordCursor cursor = fbreader.BookTextView.getStartCursor();
		int index = cursor.getParagraphIndex();	
		if (cursor.isEndOfParagraph()) {
			++index;
		}
		TOCTree treeToSelect = null;
		// TODO: process multi-model texts
		for (TOCTree tree : root) {
			final TOCTree.Reference reference = tree.getReference();
			if (reference == null) {
				continue;
			}else{
				if(reference.isNoteStruct)
					continue;
			}
			if (reference.ParagraphIndex > index) {
				break;
			}
			treeToSelect = tree;
		}
		myAdapter.selectItem(treeToSelect);
		mySelectedItem = treeToSelect;
		}catch(Exception e){
			finish();
		}
	}

	
	//private static final int PROCESS_TREE_ITEM_ID = 0;
	//private static final int READ_BOOK_ITEM_ID = 1;

/*	@Override
	public boolean onContextItemSelected(MenuItem item) {
		final int position = ((AdapterView.AdapterContextMenuInfo)item.getMenuInfo()).position;
		final TOCTree tree = (TOCTree)myAdapter.getItem(position);
		switch (item.getItemId()) {
			case PROCESS_TREE_ITEM_ID:
				myAdapter.runTreeItem(tree);
				return true;
			case READ_BOOK_ITEM_ID:
				myAdapter.openBookText(tree);
				return true;
		}
		return super.onContextItemSelected(item);
	}*/

	private final class TOCAdapter extends ZLTreeAdapter {
		private final TOCTree myRoot;

		TOCAdapter(TOCTree root) {
			super(getListView(), root);
			myRoot = root;
		}

	/*	@Override
		public void onCreateContextMenu(ContextMenu menu, View view, ContextMenu.ContextMenuInfo menuInfo) {
			final int position = ((AdapterView.AdapterContextMenuInfo)menuInfo).position;
			final TOCTree tree = (TOCTree)getItem(position);
			if (tree.hasChildren()) {
				menu.setHeaderTitle(tree.getText());
				final ZLResource resource = ZLResource.resource("tocView");
				menu.add(0, PROCESS_TREE_ITEM_ID, 0, resource.getResource(isOpen(tree) ? "collapseTree" : "expandTree").getValue());
				menu.add(0, READ_BOOK_ITEM_ID, 0, resource.getResource("readText").getValue());
			}
		}*/

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			final TOCTree tree = (TOCTree)getItem(position);
			final View view;
			
			if(tree.myReference != null && tree.myReference.isNoteStruct)
				view =LayoutInflater.from(parent.getContext()).inflate(R.layout.toc_tree_item_activecontents, parent, false);
			else
				view = //(convertView != null) ? convertView :
				LayoutInflater.from(parent.getContext()).inflate(R.layout.toc_tree_item, parent, false);
			
			
			view.setBackgroundColor((tree == mySelectedItem) ? 0xff808080 : 0);
			//view.setBackgroundColor(0);
			setIcon((ImageView)view.findViewById(R.id.toc_tree_item_icon), tree);
			((TextView)view.findViewById(R.id.toc_tree_item_text)).setText(tree.getText());
			
			/*if(tree.myReference != null && tree.myReference.isNoteStruct){
				((TextView)view.findViewById(R.id.toc_tree_item_text)).setTextSize(16);
				if(tree.myReference.isNotes){					
					((TextView)view.findViewById(R.id.toc_tree_item_text)).setBackgroundResource(R.drawable.main_menu_note);
				}else{
					((TextView)view.findViewById(R.id.toc_tree_item_text)).setBackgroundResource(R.drawable.main_menu_bookmark);
				}
			}else{
				((TextView)view.findViewById(R.id.toc_tree_item_text)).setBackgroundDrawable(null);
			}*/
			
			return view;
		}

		void openBookText(TOCTree tree) {
			final TOCTree.Reference reference = tree.getReference();
			if (reference != null) {
				finish();
				final FBReader fbreader = (FBReader)ZLApplication.Instance();
				fbreader.BookTextView.gotoPosition(reference.ParagraphIndex, 0, 0);
				fbreader.showBookTextView();
				ReaderActivity.instance.handler.sendEmptyMessage(ReaderActivity.MSG_PAGE_RECOUNT);
			}
		}

		@Override
		protected boolean runTreeItem(ZLTree tree) {
			if (super.runTreeItem(tree)) {
				return true;
			}
			
			try{
				TOCTree.Reference ref = ((TOCTree)tree).myReference;
				if(ref.isNoteStruct){
					NoteStruct note = ReaderActivity.bookContentResolver.getNote(ref.myBookmarkId);
					if(ref.isNotes){//note
					
						Intent intent = new Intent(Intent.ACTION_VIEW);    
				        intent.setClassName("ua.pocketbook.vinotes", "ua.pocketbook.vinotes.TextNote");
				        intent.putExtra(BooksDatabase.Note._ID, note.getId());
				        
				        ReaderActivity.instance.startActivity(intent);
					}else{// bookmark	
					((FBReader)FBReader.Instance()).BookTextView.gotoPosition(note.getPosition());
					 finish();	
					}
					
				
				return true;
				}				
				
			}catch(Exception e){}
			
			openBookText((TOCTree)tree);
			
			return true;
		}
	}
	
	
	private TOCTree doActive(TOCTree root) {
		
		
		clearTree(root);
		
		List<NoteStruct> bookmarks = getBookmarks();		
		for(NoteStruct bm: bookmarks)
			addBookmark(root, bm, false);
		
		List<NoteStruct> notes = getNotes();

		for(NoteStruct bm: notes)
			addBookmark(root, bm, true);
		
		
		
		
		
		
		return root;
	}

	
	


	private void clearTree(TOCTree root) {
		
		for(TOCTree tree: root){
			if(tree.getReference() != null && tree.getReference().isNoteStruct == true){
				tree.removeSelf();
				clearTree(root);
				return;
			}	
		}
		
	}
	
	
	private void addBookmark(TOCTree root, NoteStruct bm, boolean isNote) {
		try{
		
		String sPos = bm.getPosition();
		int paragraphIndex = Integer.parseInt(sPos.substring(0, sPos.indexOf(',')))+1;
		TOCTree curentTree = null;
		
		
	//	add = false;
		for (TOCTree tree : root) {
			final TOCTree.Reference reference = tree.getReference();
			
			if (reference != null  && reference.ParagraphIndex > paragraphIndex) {
				int pos = curentTree.getPosition();
								
				TOCTree bmTree = new TOCTree(curentTree.Level, curentTree.Parent, pos);
				bmTree.setText(bm.getNote().substring(0, 150)+"...");				
				bmTree.myReference = new TOCTree.Reference(paragraphIndex, fbreader.Model.BookTextModel, true, bm.getId(), isNote);
				//add = true;				
				return;				
			}	
			
			curentTree = tree;
			
		}
		
			TOCTree bmTree = new TOCTree(root);
			bmTree.setText(bm.getNote().substring(0, 150)+"...");				
			bmTree.myReference = new TOCTree.Reference(paragraphIndex, fbreader.Model.BookTextModel, true, bm.getId(), isNote);
		
		
		
		}catch(Exception e){}
	}


	private List<NoteStruct> getBookmarks() {
		
		int book_id = ReaderActivity.bookContentResolver.getIdBookByFileName(ReaderActivity.instance.bookPath).getId();
		BookStruct bookStruct = ReaderActivity.bookContentResolver.getBook(book_id);		
		ArrayList<NoteStruct>  bms =ReaderActivity.bookContentResolver.getBookmarks( bookStruct.getId());
		//ArrayList<NoteStruct>  nts =ReaderActivity.bookContentResolver.getNotes(BooksDatabase.NoteType.note, bookStruct.getId());
		//bms.addAll(nts);
		 
		return bms;
		
	}
	
	private List<NoteStruct> getNotes() {
		
		int book_id = ReaderActivity.bookContentResolver.getIdBookByFileName(ReaderActivity.instance.bookPath).getId();
		BookStruct bookStruct = ReaderActivity.bookContentResolver.getBook(book_id);		
		//ArrayList<NoteStruct>  bms =ReaderActivity.bookContentResolver.getNotes(BooksDatabase.NoteType.bookmark, bookStruct.getId());
		ArrayList<NoteStruct>  nts = ReaderActivity.bookContentResolver.getNotes(bookStruct.getId());
		//bms.addAll(nts);
		 
		return nts;
		//return new ArrayList<NoteStruct>();
		
	}
}
