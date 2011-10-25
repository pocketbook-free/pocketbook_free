package ua.pocketbook.fb2viewer.fbreader.fbreader;


import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;



public class ShowBookShelf extends FBAction {

	ShowBookShelf(FBReader fbreader) {		
		super(fbreader);		
	}

	@Override
	protected void run() {
       Intent intent = new Intent();
        intent.setClassName("ua.pocketbook.bookshelf", "ua.pocketbook.bookshelf.BookshelfScreen");
        
        PendingIntent pendingIntent = PendingIntent.getActivity(ua.pocketbook.fb2viewer.android.fbreader.FBReader.Instance, 0, intent, 0);
        try{
        pendingIntent.send();
        }catch(Exception e){}
		
	}
	
	
	

}
