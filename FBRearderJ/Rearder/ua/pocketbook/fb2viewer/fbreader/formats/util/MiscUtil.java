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

package ua.pocketbook.fb2viewer.fbreader.formats.util;

import info.monitorenter.cpdetector.io.ASCIIDetector;
import info.monitorenter.cpdetector.io.ByteOrderMarkDetector;
import info.monitorenter.cpdetector.io.CodepageDetectorProxy;
import info.monitorenter.cpdetector.io.JChardetFacade;
import info.monitorenter.cpdetector.io.ParsingDetector;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.net.MalformedURLException;
import java.net.URL;
import java.nio.charset.Charset;


import android.util.Log;

import ua.pocketbook.fb2viewer.fbreader.Paths;
import ua.pocketbook.fb2viewer.zlibrary.core.filesystem.ZLFile;

public class MiscUtil {
	public static String htmlDirectoryPrefix(ZLFile file) {
		String shortName = file.getName(false);
		String path = file.getPath();
		int index = -1;
		if ((path.length() > shortName.length()) &&
				(path.charAt(path.length() - shortName.length() - 1) == ':')) {
			index = shortName.lastIndexOf('/');
		}
		return path.substring(0, path.length() - shortName.length() + index + 1);
	}

	public static String archiveEntryName(String fullPath) {
		final int index = fullPath.lastIndexOf(':');
		return (index >= 2) ? fullPath.substring(index + 1) : fullPath;
	}

	private static boolean isHexDigit(char ch) {
		return
			(ch >= '0' && ch <= '9') ||
			(ch >= 'a' && ch <= 'f') ||
			(ch >= 'A' && ch <= 'F');
	}

	public static String decodeHtmlReference(String name) {
		int index = 0;
		while (true) {
			index = name.indexOf('%', index);
			if (index == -1 || index >= name.length() - 2) {
				break;
			}
			if (isHexDigit(name.charAt(index + 1)) &&
				isHexDigit(name.charAt(index + 2))) {
				char c = 0;
				try {
					c = (char)Integer.decode("0x" + name.substring(index + 1, index + 3)).intValue();
				} catch (NumberFormatException e) {
				}
				name = name.substring(0, index) + c + name.substring(index + 3);
			}
			index = index + 1;
		}
		return name;
	}
	
	private static int MAX_LENGHT = 1024;
	public static String getEncodingByFile(URL url){
		
		//File src = new File(url.toString());
		File f = new File(Paths.cacheDirectory()+"/temp.enc");
		try {
			if(f.exists())
				f.delete();
			f.createNewFile();
			InputStream is = new FileInputStream(url.getPath());
			OutputStream osw = new FileOutputStream(f);
			byte[] buffer = new byte[MAX_LENGHT];
			
			int len = is.read(buffer);
			osw.write(buffer, 0 , len);
			is.close();
			osw.close();
			
		} catch (Exception e1) {
			// TODO Auto-generated catch block
			Log.e("temp.enc", ""+e1);
		}
		
		
		CodepageDetectorProxy detector = CodepageDetectorProxy.getInstance();
		
		// The first instance delegated to tries to detect the meta charset attribut in html pages.
	   // detector.add(new ParsingDetector(true)); // be verbose about parsing.
		
		// Add the implementations of info.monitorenter.cpdetector.io.ICodepageDetector: 
	    // This one is quick if we deal with unicode codepages: 
	 
	    
	    // This one does the tricks of exclusion and frequency detection, if first implementation is 
	    // unsuccessful:
	   detector.add(JChardetFacade.getInstance()); // Another singleton.
	   
	   detector.add(new ByteOrderMarkDetector()); 
	   // detector.add(ASCIIDetector.getInstance()); // Fallback, see javadoc.
	    java.nio.charset.Charset charset = null;
	    
	    try {
			charset = detector.detectCodepage(f.toURL());
		} catch (MalformedURLException e) {
			
			e.printStackTrace();
		} catch (IOException e) {
			
			e.printStackTrace();
		}
		if(charset != null && !charset.displayName().equals("void")){
			return charset.displayName();
		}else
			return "windows-1251";
		
	}
	
	public static String getEncodingByHtmlFile(URL url){
		
		
		if(url.getPath().toLowerCase().endsWith(".doc.html"))
			return "UTF8";
		//File src = new File(url.toString());
		File f = new File(Paths.cacheDirectory()+"/temp.enc");
		try {
			if(f.exists())
				f.delete();
			f.createNewFile();
			InputStream is = new FileInputStream(url.getPath());
			OutputStream osw = new FileOutputStream(f);
			byte[] buffer = new byte[MAX_LENGHT];
			
			int len = is.read(buffer);
			osw.write(buffer, 0 , len);
			is.close();
			osw.close();
			
		} catch (IOException e1) {
			// TODO Auto-generated catch block
			Log.e("temp.enc", ""+e1);
		}
		
		
		//CodepageDetectorProxy detector = CodepageDetectorProxy.getInstance();
		
		// The first instance delegated to tries to detect the meta charset attribut in html pages.
	   // detector.add(new ParsingDetector(true)); // be verbose about parsing.
		
		// Add the implementations of info.monitorenter.cpdetector.io.ICodepageDetector: 
	    // This one is quick if we deal with unicode codepages: 
	   // detector.add(new ByteOrderMarkDetector()); 
	    
	    // This one does the tricks of exclusion and frequency detection, if first implementation is 
	    // unsuccessful:
	    //detector.add(JChardetFacade.getInstance()); // Another singleton.
	    //detector.add(ASCIIDetector.getInstance()); // Fallback, see javadoc.
	    java.nio.charset.Charset charset = null;
	    
	    try {
	    	FileInputStream fis = new FileInputStream(f);
	    	ParsingDetector pd = new ParsingDetector();
			charset = pd.detectCodepage(fis, MAX_LENGHT);
		} catch (MalformedURLException e) {
			
			e.printStackTrace();
		} catch (IOException e) {
			
			e.printStackTrace();
		}
		if(charset != null && !charset.displayName().equals("void")){
			return charset.displayName();
		}else
			return "windows-1251";
		
	}
		
		
	/*	BufferedInputStream bis;
		CharsetMatch cm = null;
		try {
			bis = new BufferedInputStream(new FileInputStream(f));
			CharsetDetector cd = new CharsetDetector(); 
			cd.setText(bis); 
			cm = cd.detect();
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} 
		
		
		if (cm != null) {
            cm.getReader();
            return cm.getName();

        }else {
        	return "windows-1251";
        }
	}	*/
    	
	
}
