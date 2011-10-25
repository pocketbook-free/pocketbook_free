package ua.pocketbook.fb2viewer.fbreader.formats.chm;


import android.util.Log;
import ua.pocketbook.fb2viewer.fbreader.bookmodel.BookModel;
import ua.pocketbook.fb2viewer.fbreader.formats.FormatPlugin;
import ua.pocketbook.fb2viewer.fbreader.library.Book;
import ua.pocketbook.fb2viewer.zlibrary.core.filesystem.ZLFile;

public class CHMPlugin extends FormatPlugin{

	@Override
	public boolean acceptsFile(ZLFile file) {
		return "chm".equals(file.getExtension().toLowerCase()) ;
		
	}

	@Override
	public boolean readMetaInfo(Book book) {
		// TODO Auto-generated method stub
		return true;
	}

	@Override
	public boolean readModel(BookModel model) {
		
		return new ChmReader().readBook(model);
		
		/*try {
			
		} catch (Exception e) {
			Log.e("CHM readModel Exception ", ""+e);
			return false;
		}*/
	}

}
