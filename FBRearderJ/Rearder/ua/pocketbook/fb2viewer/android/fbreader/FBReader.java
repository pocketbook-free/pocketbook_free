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

import android.app.SearchManager;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.RelativeLayout;
import ua.pocketbook.reader.R;
import ua.pocketbook.reader.ReaderControll;
import ua.pocketbook.fb2viewer.fbreader.fbreader.ActionCode;
import ua.pocketbook.fb2viewer.zlibrary.core.application.ZLApplication;
import ua.pocketbook.fb2viewer.zlibrary.ui.android.library.ZLAndroidActivity;
import ua.pocketbook.fb2viewer.zlibrary.ui.android.library.ZLAndroidApplication;

public final class FBReader extends ZLAndroidActivity implements ReaderControll{
	//private static final String BOOKFILE = "bookfile";

	public static FBReader Instance;
	
	private  ua.pocketbook.fb2viewer.fbreader.fbreader.FBReader fbreader;

//	private int myFullScreenFlag;
public static final String VIEW_FB2 = "ua.pocketbook.fb2viewer.android.fbreader.FBReader.VIEW_FB2"; 
	//private String defFileName = "";

	private static class TextSearchButtonPanel implements ZLApplication.ButtonPanel {
		boolean Visible;
		ControlPanel ControlPanel;

		public void hide() {
			Visible = false;
			if (ControlPanel != null) {
				ControlPanel.hide(false);
			}
		}

		public void updateStates() {
			if (ControlPanel != null) {
				ControlPanel.updateStates();
			}
		}
	}
	private static TextSearchButtonPanel myPanel;

	@Override
	public void onCreate(Bundle icicle) {

		
		super.onCreate(icicle);
		/*
		android.telephony.TelephonyManager tele =
			(android.telephony.TelephonyManager)getSystemService(TELEPHONY_SERVICE);
		System.err.println(tele.getNetworkOperator());
		*/
		
		
		/*if (icicle.get(BOOKFILE)!=null)
			defFileName =(String)icicle.get(BOOKFILE);*/
		
		
		
		
		Instance = this;
		final ZLAndroidApplication application = ZLAndroidApplication.Instance();
	//	myFullScreenFlag = 
		//	application.ShowStatusBarOption.getValue() ? 0 : WindowManager.LayoutParams.FLAG_FULLSCREEN;
		//getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, myFullScreenFlag);
		myPanel = null;
		if (myPanel == null) {
			myPanel = new TextSearchButtonPanel();
			ZLApplication.Instance().registerButtonPanel(myPanel);
		}
		
	//	String[] args = new String[]{"sdcard/Books/2.fb2"};
		
	//	org.geometerplus.fbreader.fbreader.FBReader fbr = new org.geometerplus.fbreader.fbreader.FBReader(args);
	//	fbr.initWindow();
		
         // createApplication("sdcard/Books/2.fb2");
		
	
	}

	@Override
	public void onStart() {
		super.onStart();
		final ZLAndroidApplication application = ZLAndroidApplication.Instance();

	/*	final int fullScreenFlag = 
			application.ShowStatusBarOption.getValue() ? 0 : WindowManager.LayoutParams.FLAG_FULLSCREEN;
		if (fullScreenFlag != myFullScreenFlag) {
			finish();
			startActivity(new Intent(this, this.getClass()));
		}*/

	/*	if (myPanel.ControlPanel == null) {
			myPanel.ControlPanel = new ControlPanel(this);

			myPanel.ControlPanel.addButton(ActionCode.FIND_PREVIOUS, false, R.drawable.text_search_previous);
			myPanel.ControlPanel.addButton(ActionCode.CLEAR_FIND_RESULTS, true, R.drawable.text_search_close);
			myPanel.ControlPanel.addButton(ActionCode.FIND_NEXT, false, R.drawable.text_search_next);
        
			RelativeLayout root = (RelativeLayout)findViewById(R.id.root_view);
			
		    RelativeLayout.LayoutParams p = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.WRAP_CONTENT,
                RelativeLayout.LayoutParams.WRAP_CONTENT);
            p.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
            p.addRule(RelativeLayout.CENTER_HORIZONTAL);
            root.addView(myPanel.ControlPanel, p);
            
            
		}*/
	}

//	private PowerManager.WakeLock myWakeLock;

	@Override
	public void onResume() {
		super.onResume();
		if (myPanel.ControlPanel != null) {
			myPanel.ControlPanel.setVisibility(myPanel.Visible ? View.VISIBLE : View.GONE);
		}
		/*if (ZLAndroidApplication.Instance().DontTurnScreenOffOption.getValue()) {
			myWakeLock =
				((PowerManager)getSystemService(POWER_SERVICE)).
					newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, "FBReader");
			myWakeLock.acquire();
		} else {
			myWakeLock = null;
		}*/
		
	}

	@Override
	public void onPause() {
		/*if (myWakeLock != null) {
			myWakeLock.release();
		}*/
		if (myPanel.ControlPanel != null) {
			myPanel.Visible = myPanel.ControlPanel.getVisibility() == View.VISIBLE;
		}
		super.onPause();
	}

	@Override
	public void onStop() {
		if (myPanel.ControlPanel != null) {
			myPanel.ControlPanel.hide(false);
			myPanel.ControlPanel = null;
		}
		fbreader.onWindowClosing();
		super.onStop();
	}

	void showTextSearchControls(boolean show) {
		
		Log.i("showTextSearchControls", "show = "+show+" myPanel.ControlPanel "+myPanel.ControlPanel);
		
		if (myPanel.ControlPanel == null) {
			myPanel.ControlPanel = new ControlPanel(this);

			myPanel.ControlPanel.addButton(ActionCode.FIND_PREVIOUS, false, R.drawable.text_search_previous);
			myPanel.ControlPanel.addButton(ActionCode.CLEAR_FIND_RESULTS, true, R.drawable.text_search_close);
			myPanel.ControlPanel.addButton(ActionCode.FIND_NEXT, false, R.drawable.text_search_next);
        
			RelativeLayout root = (RelativeLayout)findViewById(R.id.root_view);
			
		    RelativeLayout.LayoutParams p = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.WRAP_CONTENT,
                RelativeLayout.LayoutParams.WRAP_CONTENT);
            p.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
            p.addRule(RelativeLayout.CENTER_HORIZONTAL);
            root.addView(myPanel.ControlPanel, p);
            
            
		}
		
		
		if (myPanel.ControlPanel != null) {
			if (show) {
				myPanel.ControlPanel.show(true);
			} else {
				myPanel.ControlPanel.hide(false);
				
			}
		}
	}

	protected ZLApplication createApplication(String fileName) {
		//new SQLiteBooksDatabase();
		
		//String path  = fileName.substring("file://".length());
		String[] args = (fileName != null) ? new String[] { fileName } : new String[0];
		
		fbreader = new ua.pocketbook.fb2viewer.fbreader.fbreader.FBReader(args);
		return fbreader;
	}

	@Override
	public boolean onSearchRequested() {
		final ua.pocketbook.fb2viewer.fbreader.fbreader.FBReader fbreader =
			(ua.pocketbook.fb2viewer.fbreader.fbreader.FBReader)ZLApplication.Instance();
		
		Log.i("FBReader", "onSearchRequested(), myPanel.ControlPanel = "+myPanel.ControlPanel);
		
		if (myPanel.ControlPanel != null) {
			final boolean visible = myPanel.ControlPanel.getVisibility() == View.VISIBLE;
			myPanel.ControlPanel.hide(false);
			SearchManager manager = (SearchManager)getSystemService(SEARCH_SERVICE);
			manager.setOnCancelListener(new SearchManager.OnCancelListener() {
				public void onCancel() {
					if ((myPanel.ControlPanel != null) && visible) {
						myPanel.ControlPanel.show(false);	
						
						
					}					
					
				}
			});
		}
		
		
		
		startSearch(fbreader.TextSearchPatternOption.getValue(), true, null, false);
		return true;
	}

	//@Override
	public void addNote() {
		fbreader.addNote();
		
	}

	//@Override
	public boolean canBack() {
		return fbreader.canBack();
	}

	//@Override
	public void createBookmark() {
		fbreader.createBookmark();
		
	}

	//@Override
	public int getCurenPageNumber() {
		return fbreader.getCurenPageNumber();
	}

	//@Override
	public int getPageCount() {
		return fbreader.getPageCount();
	}

	//@Override
	public void goBack() {
		fbreader.goBack();
		
	}

	//@Override
	public boolean goToPage(int pageNumber) {
		return fbreader.goToPage(pageNumber);
	}

	//@Override
	public boolean haveAnyBookmarks() {
		return fbreader.haveAnyBookmarks();
	}

	//@Override
	public boolean isNightReading() {
		return fbreader.isNightReading();
	}

	//@Override
	public boolean nextPage() {
		return fbreader.nextPage();
	}

	//@Override
	public void nextWord2Dict() {
		fbreader.nextWord2Dict(); 
		
	}

	//@Override
	public boolean prevPage() {		
		return fbreader.prevPage();
	}

	//@Override
	public void prevWord2Dict() {
		fbreader.prevWord2Dict();
		
	}

	//@Override
	public boolean readyGoToPage() {		
		return fbreader.readyGoToPage();
	}

	//@Override
	public void setNightReading(boolean value) {
		fbreader.setNightReading(value);
		
	}

	//@Override
	public boolean supportAnimScrolling() {
		return fbreader.supportAnimScrolling();
	}

	//@Override
	public boolean supportBookmarks() {
		return fbreader.supportBookmarks();
	}

	//@Override
	public boolean supportChangeFontSize() {
		return fbreader.supportChangeFontSize();
	}

	//@Override
	public boolean supportContents() {
		return fbreader.supportContents();
	}

	//@Override
	public boolean supportDictionary() {
		return fbreader.supportDictionary();
	}

	//@Override
	public boolean supportNightReading() {
		return fbreader.supportNightReading();
	}

	//@Override
	public boolean supportNotes() {
		return fbreader.supportNotes();
	}

	//@Override
	public boolean supportSearch() {
		return fbreader.supportSearch();
	}

	//@Override
	public boolean supportTTS() {
		return fbreader.supportTTS();
	}

	//@Override
	public boolean supportToScreenWidth() {
		return fbreader.supportToScreenWidth();
	}

	//@Override
	public void toScreenWidth() {
		fbreader.toScreenWidth();
		
	}

	//@Override
	public void viewBookmarks() {
		fbreader.viewBookmarks();		
	}

	//@Override
	public void viewContents() {
		fbreader.viewContents();
		
	}

	//@Override
	public void viewSearch() {
		fbreader.viewSearch();		
	}

	//@Override
	public boolean zoomIn() {
		return fbreader.zoomIn();
	}

	//@Override
	public boolean zoomOut() {
		return fbreader.zoomOut();
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
