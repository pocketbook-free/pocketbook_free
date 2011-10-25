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

package ua.pocketbook.fb2viewer.fbreader.formats.txt;



import ua.pocketbook.fb2viewer.fbreader.library.Book;


public class TxtMetaInfoReader {
	//private final Book myBook;

	public TxtMetaInfoReader(Book book) {
		
	//	myBook = book;
	/*	String encoding = "utf-8";
		CodepageDetectorProxy detector = CodepageDetectorProxy.getInstance();
		// Add the implementations of info.monitorenter.cpdetector.io.ICodepageDetector: 
	    // This one is quick if we deal with unicode codepages: 
	    detector.add(new ByteOrderMarkDetector()); 
	    // The first instance delegated to tries to detect the meta charset attribut in html pages.
	    detector.add(new ParsingDetector(true)); // be verbose about parsing.
	    // This one does the tricks of exclusion and frequency detection, if first implementation is 
	    // unsuccessful:
	   // detector.add(JChardetFacade.getInstance()); // Another singleton.
	    detector.add(ASCIIDetector.getInstance()); // Fallback, see javadoc.
	    java.nio.charset.Charset charset = null;
	    
	    try {
			charset = detector.detectCodepage(new URL("file://"+myBook.File.getPath()));
		} catch (MalformedURLException e) {
			
			e.printStackTrace();
		} catch (IOException e) {
			
			e.printStackTrace();
		}
		if(charset != null){
			encoding = charset.displayName();
		}		
		myBook.setEncoding(encoding);*/
		
		
	}



	public boolean readMetaInfo() {
		
		
		return true;
	}
	

	
}
