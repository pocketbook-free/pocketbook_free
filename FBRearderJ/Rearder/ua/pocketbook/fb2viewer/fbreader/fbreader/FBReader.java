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

package ua.pocketbook.fb2viewer.fbreader.fbreader;

import ua.pocketbook.fb2viewer.fbreader.bookmodel.BookModel;
import ua.pocketbook.fb2viewer.fbreader.library.Book;
import ua.pocketbook.fb2viewer.zlibrary.core.application.ZLApplication;
import ua.pocketbook.fb2viewer.zlibrary.core.application.ZLKeyBindings;
import ua.pocketbook.fb2viewer.zlibrary.core.dialogs.ZLDialogManager;
import ua.pocketbook.fb2viewer.zlibrary.core.filesystem.ZLFile;
import ua.pocketbook.fb2viewer.zlibrary.core.options.ZLBooleanOption;
import ua.pocketbook.fb2viewer.zlibrary.core.options.ZLIntegerRangeOption;
import ua.pocketbook.fb2viewer.zlibrary.core.options.ZLStringOption;
import ua.pocketbook.fb2viewer.zlibrary.text.hyphenation.ZLTextHyphenator;
import ua.pocketbook.fb2viewer.zlibrary.text.view.ZLTextPosition;
import ua.pocketbook.fb2viewer.zlibrary.ui.android.dialogs.ZLAndroidDialogManager;
import ua.pocketbook.reader.FB2BookmarksActivity;
import ua.pocketbook.reader.R;
import ua.pocketbook.reader.ReaderActivity;
import ua.pocketbook.reader.ReaderControll;
import ua.pocketbook.utils.BookStruct;
import ua.pocketbook.utils.BooksDatabase;
import ua.pocketbook.utils.NoteStruct;
import android.content.Intent;
import android.os.Message;
import android.widget.Toast;

public final class FBReader extends ZLApplication implements ReaderControll{
	public final ZLStringOption BookSearchPatternOption =
		new ZLStringOption("BookSearch", "Pattern", "");
	public final ZLStringOption TextSearchPatternOption =
		new ZLStringOption("TextSearch", "Pattern", "");
	public final ZLStringOption BookmarkSearchPatternOption =
		new ZLStringOption("BookmarkSearch", "Pattern", "");

	public final ZLBooleanOption UseSeparateBindingsOption = 
		new ZLBooleanOption("KeysOptions", "UseSeparateBindings", false);

	public final ZLIntegerRangeOption LeftMarginOption =
		new ZLIntegerRangeOption("Options", "LeftMargin", 0, 1000, 4);
	public final ZLIntegerRangeOption RightMarginOption =
		new ZLIntegerRangeOption("Options", "RightMargin", 0, 1000, 4);
	public final ZLIntegerRangeOption TopMarginOption =
		new ZLIntegerRangeOption("Options", "TopMargin", 0, 1000, 0);
	public final ZLIntegerRangeOption BottomMarginOption =
		new ZLIntegerRangeOption("Options", "BottomMargin", 0, 1000, 4);

	public final ZLIntegerRangeOption ScrollbarTypeOption =
		new ZLIntegerRangeOption("Options", "ScrollbarType", 0, 2, FBView.SCROLLBAR_HIDE);

	final ZLBooleanOption SelectionEnabledOption =
		new ZLBooleanOption("Options", "IsSelectionEnabled", true);

	final ZLStringOption ColorProfileOption =
		new ZLStringOption("Options", "ColorProfile", ColorProfile.DAY);

	private final ZLKeyBindings myBindings = new ZLKeyBindings("Keys");

	public final FBView BookTextView;
	final FBView FootnoteView;
	final ZLAndroidDialogManager dialogManager =
		(ZLAndroidDialogManager)ZLAndroidDialogManager.Instance();

	public BookModel Model;

	private final String myArg0;

	public FBReader(String[] args) {
		myArg0 = (args.length > 0) ? args[0] : null;
		addAction(ActionCode.QUIT, new QuitAction(this));

		addAction(ActionCode.INCREASE_FONT, new ChangeFontSizeAction(this, +2));
		addAction(ActionCode.DECREASE_FONT, new ChangeFontSizeAction(this, -2));
	//	addAction(ActionCode.ROTATE, new RotateAction(this));

		//addAction(ActionCode.SHOW_LIBRARY, new ShowLibraryAction(this));
		addAction(ActionCode.SHOW_LIBRARY, new ShowBookShelf(this));
	//	addAction(ActionCode.SHOW_OPTIONS, new ShowOptionsDialogAction(this));
	//	addAction(ActionCode.SHOW_PREFERENCES, new PreferencesAction(this));
	//	addAction(ActionCode.SHOW_BOOK_INFO, new BookInfoAction(this));
		addAction(ActionCode.SHOW_CONTENTS, new ShowTOCAction(this));
	//	addAction(ActionCode.SHOW_BOOKMARKS, new ShowBookmarksAction(this));
	//	addAction(ActionCode.SHOW_NETWORK_LIBRARY, new ShowNetworkLibraryAction(this));
		
		addAction(ActionCode.SEARCH, new SearchAction(this));
		addAction(ActionCode.FIND_NEXT, new FindNextAction(this));
		addAction(ActionCode.FIND_PREVIOUS, new FindPreviousAction(this));
		addAction(ActionCode.CLEAR_FIND_RESULTS, new ClearFindResultsAction(this));
		
		addAction(ActionCode.SCROLL_TO_HOME, new ScrollToHomeAction(this));
		//addAction(ActionCode.SCROLL_TO_START_OF_TEXT, new DummyAction(this));
		//addAction(ActionCode.SCROLL_TO_END_OF_TEXT, new DummyAction(this));
		addAction(ActionCode.VOLUME_KEY_SCROLL_FORWARD, new VolumeKeyScrollingAction(this, false));
		addAction(ActionCode.VOLUME_KEY_SCROLL_BACKWARD, new VolumeKeyScrollingAction(this, true));
		addAction(ActionCode.TRACKBALL_SCROLL_FORWARD, new TrackballScrollingAction(this, true));
		addAction(ActionCode.TRACKBALL_SCROLL_BACKWARD, new TrackballScrollingAction(this, false));
		addAction(ActionCode.CANCEL, new CancelAction(this));
		//addAction(ActionCode.GOTO_NEXT_TOC_SECTION, new DummyAction(this));
		//addAction(ActionCode.GOTO_PREVIOUS_TOC_SECTION, new DummyAction(this));
		//addAction(ActionCode.COPY_SELECTED_TEXT_TO_CLIPBOARD, new DummyAction(this));
		//addAction(ActionCode.OPEN_SELECTED_TEXT_IN_DICTIONARY, new DummyAction(this));
		//addAction(ActionCode.CLEAR_SELECTION, new DummyAction(this));
		addAction(ActionCode.FOLLOW_HYPERLINK, new FollowHyperlinkAction(this));

	//	addAction(ActionCode.SWITCH_TO_DAY_PROFILE, new SwitchProfileAction(this, ColorProfile.DAY));
	//	addAction(ActionCode.SWITCH_TO_NIGHT_PROFILE, new SwitchProfileAction(this, ColorProfile.NIGHT));

		BookTextView = new FBView(this);
		FootnoteView = new FBView(this);

		setView(BookTextView);
		
	}

	public void initWindow() {
		super.initWindow();		
		
		
		ZLDialogManager.Instance().wait("loadingBook", new Runnable() {
			public void run() { 
				Book book = createBookForFile(ZLFile.createFileByPath(myArg0));
				openBookInternal(book);	
				//ReaderActivity.instance.ready = true;
				
				
				ReaderActivity.instance.handler.sendEmptyMessage(ReaderActivity.MSG_PAGE_RECOUNT);
			}
		});
	}
	
	public void openBook(final Book book) {
		ZLDialogManager.Instance().wait("loadingBook", new Runnable() {
			public void run() { 
				openBookInternal(book); 					
			}
		});
	}

	private ColorProfile myColorProfile;

	public ColorProfile getColorProfile() {
		/*if (myColorProfile == null) {
			myColorProfile = ColorProfile.get(getColorProfileName());
		}
		return myColorProfile;*/
		if(ReaderActivity.readerPreferences.isNightReading()){
			myColorProfile = ColorProfile.get(ColorProfile.NIGHT);
		}else{
			myColorProfile = ColorProfile.get(ColorProfile.DAY);
		}	
		return myColorProfile;
	}

	public String getColorProfileName() {
		return ColorProfileOption.getValue();
	}

	public void setColorProfileName(String name) {
		ColorProfileOption.setValue(name);
		myColorProfile = null;
	}
	
	
	public void setColorProfile(){
		if(ReaderActivity.readerPreferences.isNightReading()){
			setColorProfileName(ColorProfile.NIGHT);
		}else{
			setColorProfileName(ColorProfile.DAY);
		}		
	}

	public ZLKeyBindings keyBindings() {
		return myBindings;
	}

	public FBView getTextView() {
		return (FBView)getCurrentView();
	}

	void tryOpenFootnote(String id) {
		if (Model != null) {
			BookModel.Label label = Model.getLabel(id);
			if (label != null) {
				if (label.ModelId == null) {
					BookTextView.gotoPosition(label.ParagraphIndex, 0, 0);
					ReaderActivity.instance.handler.sendEmptyMessage(ReaderActivity.MSG_PAGE_RECOUNT);
				} else {
					FootnoteView.setModel(Model.getFootnoteModel(label.ModelId));
					setView(FootnoteView);
					FootnoteView.gotoPosition(label.ParagraphIndex, 0, 0);
					
					Message msg = ReaderActivity.instance.handler.obtainMessage(ReaderActivity.MSG_PAGE);
						msg.arg1 = 0;
						msg.arg2 = 0;		
					ReaderActivity.instance.handler.sendMessage(msg);
				}
				repaintView();
			}
		}
	}

	public void clearTextCaches() {
		BookTextView.clearCaches();
		FootnoteView.clearCaches();
	}
	
	void openBookInternal(Book book) {
		if (book != null) {
			onViewChanged();

			if (Model != null) {
				Model.Book.storePosition(BookTextView.getStartCursor());
			}
			BookTextView.setModel(null);
			FootnoteView.setModel(null);
			clearTextCaches();

			Model = null;
			System.gc();
			System.gc();
			Model = BookModel.createModel(book);
			if (Model != null) {
				ZLTextHyphenator.Instance().load(book.getLanguage());
				BookTextView.setModel(Model.BookTextModel);
				String pos;
				
				if(ReaderActivity.instance.bookmark != null && !ReaderActivity.instance.bookmark.equals("")){
					pos = ReaderActivity.instance.bookmark;
					ReaderActivity.instance.bookmark = null;
				}else{
					BookStruct bookStruct = ReaderActivity.bookContentResolver.getIdBookByFileName(ReaderActivity.instance.bookPath);
					pos = bookStruct.getReaderposition();
				}
								
				setView(BookTextView);
				BookTextView.gotoPosition(pos);			
				
			}
		}
		repaintView();		
	}

/*	public void gotoBookmark(Bookmark bookmark) {
		final String modelId = bookmark.getModelId();
		if (modelId == null) {
			BookTextView.gotoPosition(bookmark);
			setView(BookTextView);
		} else {
			FootnoteView.setModel(Model.getFootnoteModel(modelId));
			FootnoteView.gotoPosition(bookmark);
			setView(FootnoteView);
		}
		repaintView();
	}*/
	
	public void showBookTextView() {
		setView(BookTextView);		
	}
	
	private Book createBookForFile(ZLFile file) {
		if (file == null) {
			return null;
		}
		Book book = Book.getByFile(file);
		if (book != null) {
			return book;
		}
		if (file.isArchive()) {
			for (ZLFile child : file.children()) {
				book = Book.getByFile(child);
				if (book != null) {
					return book;
				}
			}
		}
		return null;
	}

	@Override
	public void openFile(ZLFile file) {
		final Book book = createBookForFile(file);
		if (book != null) {
			openBook(book);
			
		}
	}

	public void onWindowClosing() {
		if ((Model != null) && (BookTextView != null)) {
			
			// запоминает позицию в книге
			Model.Book.storePosition(BookTextView.getStartCursor());
		}
	}
	
	//@Override
	public void createBookmark() {
		NoteStruct bookmark = new NoteStruct();
		BookStruct bookStruct = ReaderActivity.bookContentResolver.getIdBookByFileName(ReaderActivity.instance.bookPath);
		
		bookmark.setTitle(bookStruct.getName());
		bookmark.setPosition(BookTextView.getStartCursor().toString());
		bookmark.setNote(BookTextView.curenPageToString());
		bookmark.setIdBook(bookStruct.getId());
		bookmark.setTypenote(1);
		
		
		//Log.i("createBookmark", ""+ReaderActivity.bookContentResolver.addNote(bookmark));	
		ReaderActivity.bookContentResolver.addBookmark(bookmark);
	}

	//@Override	
	public boolean haveAnyBookmarks(){
		
		BookStruct bookStruct = ReaderActivity.bookContentResolver.getIdBookByFileName(ReaderActivity.instance.bookPath);		
		return (ReaderActivity.bookContentResolver.getBookmarks( bookStruct.getId()).size()!=0);
		
	}
	
	//@Override
	public void addNote() {
		
		NoteStruct note = new NoteStruct();
		BookStruct bookStruct = ReaderActivity.bookContentResolver.getIdBookByFileName(ReaderActivity.instance.bookPath);
		
		note.setTitle(bookStruct.getName());
		note.setIdBook(bookStruct.getId());
		note.setNote(BookTextView.curenPageToString());		
		note.setPosition(BookTextView.getStartCursor().toString());
		note.setTypenote(0);
				
		int id = ReaderActivity.bookContentResolver.addNote(note);
        
        Intent intent = new Intent(Intent.ACTION_VIEW);    
        intent.setClassName("ua.pocketbook.vinotes", "ua.pocketbook.vinotes.TextNote");
        intent.putExtra(BooksDatabase.Note._ID, id);
        
        ReaderActivity.instance.startActivity(intent);	
        //ReaderActivity.instance.finish();
		
	}

	//@Override
	public boolean zoomIn() {
		if( ReaderActivity.readerPreferences.getFontSize() < ReaderActivity.MAX_FONT_SIZE){
			ReaderActivity.readerPreferences.setFontSize(ReaderActivity.readerPreferences.getFontSize()+1);
			clearTextCaches();
			repaintView();
			ReaderActivity.instance.handler.sendEmptyMessage(ReaderActivity.MSG_PAGE_RECOUNT);
			return true;
		}
		return false;
	}

	//@Override
	public boolean zoomOut() {
		if( ReaderActivity.readerPreferences.getFontSize() > ReaderActivity.MIN_FONT_SIZE){
			ReaderActivity.readerPreferences.setFontSize(ReaderActivity.readerPreferences.getFontSize()-1);
			clearTextCaches();
			repaintView();
			ReaderActivity.instance.handler.sendEmptyMessage(ReaderActivity.MSG_PAGE_RECOUNT);
		}
		
		return false;
	}

	//@Override
	public boolean nextPage() {
		//BookTextView.textMaping.clear();
		return BookTextView.doScrollPage(true);
		
		
	}

	//@Override
	public boolean prevPage() {
		//BookTextView.textMaping.clear();
		return BookTextView.doScrollPage(false);
	}

	//@Override
	public int getPageCount() {
		// TODO Auto-generated method stub
		return 0;
	}

	//@Override
	public int getCurenPageNumber() {
		// TODO Auto-generated method stub
		return 0;
	}

	//@Override
	public boolean supportToScreenWidth() {		
		return false;
	}

	//@Override
	public void toScreenWidth() {				
	}

	//@Override
	public boolean supportNightReading() {
		return true;
	}

	//@Override
	public boolean isNightReading() {		
		return ReaderActivity.readerPreferences.isNightReading();
	}

	//@Override
	public void setNightReading(boolean value) {
		ReaderActivity.readerPreferences.setNightReading(value);	
		 	
    	setColorProfile();
    	clearTextCaches();
    	repaintView();
    	ReaderActivity.instance.handler.sendEmptyMessage(ReaderActivity.MSG_PAGE_RECOUNT);
	}

	//@Override
	public boolean supportContents() {
		return Model.TOCTree.hasChildren();
	}

	//@Override
	public void viewContents() {
		dialogManager.runActivity(ua.pocketbook.fb2viewer.android.fbreader.TOCActivity.class);		
	}

	//@Override
	public boolean supportDictionary() {
		return true;
	}

	//@Override
	public boolean supportSearch() {
		return true;
	}

	//@Override
	public void viewSearch() {
		dialogManager.startSearch(); 
		
	}

	//@Override
	public boolean supportTTS() {
		return true;
	}

	//@Override
	public boolean supportBookmarks() {
		return true;
	}

	//@Override
	public void viewBookmarks() {
		Intent  intent = new Intent(ReaderActivity.instance, FB2BookmarksActivity.class); 
		intent.putExtra("BOOK_ID", (long)ReaderActivity.bookContentResolver.getIdBookByFileName(ReaderActivity.instance.bookPath).getId());
		ReaderActivity.instance.startActivity(intent); 		
	}

	//@Override
	public boolean supportNotes() {
		return true;
	}

	//@Override
	public boolean supportAnimScrolling() {
		return true;
	}

	//@Override
	public boolean supportChangeFontSize() {
		return true;
	}
	
	//@Override
	public boolean readyGoToPage(){
		return !BookTextView.threadPageReCount.isRunning();
	}
	
	//@Override
	public boolean canBack(){
		return BookTextView.getBackCursor() != null;
	}
	
	//@Override
	public void goBack(){
		
	}
	
	//@Override
	public void prevWord2Dict(){
		Message msg = ReaderActivity.instance.handler.obtainMessage(ReaderActivity.MSG_SET_DIC_WORD);
		msg.obj = new String(BookTextView.textMaping.getPrev());
		ReaderActivity.instance.handler.sendMessage(msg);
	}
	
	//@Override
	public void nextWord2Dict(){
		Message msg = ReaderActivity.instance.handler.obtainMessage(ReaderActivity.MSG_SET_DIC_WORD);
		msg.obj = new String(BookTextView.textMaping.getNext());
		ReaderActivity.instance.handler.sendMessage(msg);
	}
	
	//@Override
	public boolean goToPage(int pageNumber){
		int countPage = BookTextView.pagesBefore+1+BookTextView.pagesAfter;
		 
		if (pageNumber < 1 || pageNumber > countPage)
        {
            Toast.makeText(ReaderActivity.instance, ReaderActivity.instance.getResources().getString(R.string.page_out_of_range) + countPage , 3000).show();
            return false;
        }
		
		BookTextView.pageShift = pageNumber - BookTextView.pagesBefore -1;
		BookTextView.gotoPosition((ZLTextPosition)BookTextView.pageMap.get((Integer)BookTextView.pageShift));
		repaintView();
		BookTextView.pagePanelUpdate();
		
		
    	
		return true;	       
	}
	
	
	//@Override
	public void setReflow(boolean r) {
		// TODO Auto-generated method stub
		
	}

	//@Override
	public boolean supportReflow() {
		// TODO Auto-generated method stub
		return false;
	}
	
	public boolean isReflow() {
		// TODO Auto-generated method stub
		return false;
	}

		
}
