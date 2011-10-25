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


import java.io.*;
import java.net.MalformedURLException;
import java.net.URL;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.*;

import android.util.Log;

import ua.pocketbook.fb2viewer.fbreader.bookmodel.BookModel;
import ua.pocketbook.fb2viewer.fbreader.bookmodel.BookReader;
import ua.pocketbook.fb2viewer.fbreader.bookmodel.FBTextKind;
import ua.pocketbook.fb2viewer.fbreader.formats.util.MiscUtil;
import ua.pocketbook.fb2viewer.zlibrary.text.model.ZLTextParagraph;


public class TxtReader extends BookReader  {
	

	//protected final CharsetDecoder myAttributeDecoder;

	
	
	public TxtReader(BookModel model) throws UnsupportedEncodingException {
		super(model);
		try {	
			//String encoding = model.Book.getEncoding();
			//myAttributeDecoder = createDecoder();
			setByteDecoder(createDecoder());
		} catch (UnsupportedCharsetException e) {
			throw new UnsupportedEncodingException(e.getMessage());
		}
	}

	protected final CharsetDecoder createDecoder() throws UnsupportedEncodingException {
		
		
		String encoding = "utf-8";
		
		try {
			String e = MiscUtil.getEncodingByFile(new URL("file://"+Model.Book.File.getPath()));
			if (e != null) encoding = e;
		} catch (MalformedURLException e) {	}
		
		Model.Book.setEncoding(encoding);
		Log.i("Encoding************", ""+encoding);		
		
		return Charset.forName(Model.Book.getEncoding()).newDecoder();
	}

	public boolean readBook() throws IOException {
		
		setMainTextModel();
		pushKind(FBTextKind.REGULAR);
		beginParagraph(ZLTextParagraph.Kind.TEXT_PARAGRAPH);
				
		byte[] data = new byte[(int) Model.Book.File.getPhysicalFile().size()];
		
		Model.Book.File.getInputStream().read(data);
		CharBuffer cb = CharBuffer.allocate(data.length);
		ByteBuffer bb = ByteBuffer.wrap(data);
		myByteDecoder.decode(bb, cb, false);
		
		char[] cd = cb.array();
		
		int i =0;
		int begin = 0;
		while(i < cd.length){
			
			if(cd[i] == '\n'){
				addData(cd, begin, i-begin, false);
				begin = i;
				endParagraph();
				beginParagraph(ZLTextParagraph.Kind.TEXT_PARAGRAPH);
			} 				
			i++;	
			
		}		
		
		addData(cd, begin, i-begin, false);
		endParagraph();
		unsetCurrentTextModel();
		
		return true;
	}
	
	

	
	

	
	
	

	
}
