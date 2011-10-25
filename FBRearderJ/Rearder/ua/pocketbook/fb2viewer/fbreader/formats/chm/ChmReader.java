/*
 * ChmReader.java
 ***************************************************************************************
 * Author: Feng Yu. <yfbio@hotmail.com>
 *org.yufeng.jchmlib 
 *version: 1.0
 ****************************************************************************************
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
**********************************************************************************************/

package ua.pocketbook.fb2viewer.fbreader.formats.chm;
import java.io.*;
import java.util.*;

import android.util.Log;

import ua.pocketbook.fb2viewer.fbreader.bookmodel.BookModel;
import ua.pocketbook.fb2viewer.zlibrary.text.model.ZLTextParagraph;


/**
 *
 * @author yufeng
 */
public class ChmReader {
    
    /** Creates a new instance of ChmReader */
    public ChmReader() {
    }
   
     public static void showOne(String chmfile,String htmlf) {
        ChmManager cm=new ChmManager(chmfile);//"micro.fictions.chm");
        showOne(cm, htmlf);
    }
     public static void showOne(ChmManager cm,String htmlf) {
        //ChmManager cm=new ChmManager(chmfile);//"micro.fictions.chm");
         byte[][]tmp=cm.retrieveObject(htmlf);
        showFile(tmp);
    }
     private static void showFile(byte[][] tmp) {
         String[] s=new String[tmp.length];
         try
         {
        for(int i=0;i<tmp.length;i++)
        {
         s[i]=new String(tmp[i],"utf8");//"ISO8859-1");//gbk
         System.out.println(s[i]);
        }
         }
         catch(UnsupportedEncodingException uee)
         {
             
         }
       /* FileOutputStream fops=null;//,fops1=null;
        try {
            //fops=new OutputStreamWriter(System.out);
                    //((savef);//"a/mini.GIF"); //cann't use path like /lay/layout.html
            fops=new FileOutputStream("content.html");
            // DataPutStream dps=new DataPutStream(fops);
            //dps.
            for(int i=0;i<tmp.length;i++)
                //System.out.p
               fops.write(tmp[i]);
            // fops1.write(tmp[1]);
            fops.close();
            //  fops1.close();
        } catch(IOException e) {
            
        }
        FileInputStream fis=null;
        try {
            //fops=new OutputStreamWriter(System.out);
                    //((savef);//"a/mini.GIF"); //cann't use path like /lay/layout.html
            fis=new FileInputStream("content.html");
            // DataPutStream dps=new DataPutStream(fops);
            //dps.
            fis.r
            for(int i=0;i<tmp.length;i++)
                //System.out.p
               fops.write(tmp[i]);
            // fops1.write(tmp[1]);
            fops.close();
            //  fops1.close();
        } catch(IOException e) {
            
        }*/
        
    }
    public static void extractOne(ChmManager cm,FileEntry fe,String savef) {
        byte[][]tmp=cm.retrieveObject(fe);//"/mini.GIF");///java/awt/CardLayout.html");///java/awt/AWTEvent.html");///javalogo52x88.gif");////test.html");//"/test.html");//"/����.html");//content.html");//
        writeFile(tmp,savef);
    }
    public static void extractOne(String chmfile,String htmlf,String savef) {
        ChmManager cm=new ChmManager(chmfile);//"micro.fictions.chm");
        extractOne(cm, htmlf, savef);
    }
    public static void extractOne(ChmManager cm,String htmlf,String savef) {
        byte[][]tmp=cm.retrieveObject(htmlf);//"/mini.GIF");///java/awt/CardLayout.html");///java/awt/AWTEvent.html");///javalogo52x88.gif");////test.html");//"/test.html");//"/����.html");//content.html");//
        writeFile(tmp,savef);
        // if(cm.open("utest.chm"))
        
        //cm.lzx.decompress();
        //cm.readBlock();
        //System.out.println(cm.read("/content.html"));
        
    }
    private static void writeFile(byte[][] tmp,String savef) {
        FileOutputStream fops=null,fops1=null;
        try {
            fops=new FileOutputStream(savef);//"a/mini.GIF"); //cann't use path like /lay/layout.html
            // fops1=new FileOutputStream("content1.html");
            // DataPutStream dps=new DataPutStream(fops);
            //dps.
            for(int i=0;i<tmp.length;i++)
                fops.write(tmp[i]);
            // fops1.write(tmp[1]);
            fops.close();
            //  fops1.close();
        } catch(IOException e) {
            
        }
    }
    
    
    public static void mysnail(String fpath) {
        //System.out.println("........mysnail");
        File tmpf=new File(fpath);
        MyFileFilter mff=new MyFileFilter("chm");
        if (tmpf.isDirectory()) {
            try {
                //System.out.println("-- Begin --\n");
                String[] c=tmpf.list();
                if ((c!=null) && (c.length!=0)) {
                    for (int i=0;i<c.length;i++) {
                        String tmp_path=fpath+"/"+c[i];
                        File tf=new File(tmp_path);
                        if (tf.isDirectory()) {
                            mysnail(tmp_path);
                        }
                    }
                    String[] li=tmpf.list(mff);
                    ChmManager cm=null;
                    if(li!=null) {
                        FileEntry fe;
                        File f;
                        for(int j=0;j<li.length;j++) {
                            System.out.print("............................");
                            System.out.println(li[j]);
                            f=new File(li[j]);
                            if(f.exists())System.out.println("i'm here");
                            try {
                                cm=new ChmManager(li[j]);
                            } catch(Exception e) {
                            }
                            
                            
                            if(cm==null) System.out.println("can't open"); //hmm,it seemed that files that can't be opened is because they don't exist.
                            else {
                                ArrayList<FileEntry> fes=cm.enumerateFiles();
                                System.out.println("ok");
                                for(int k=0;k<fes.size();k++) {
                                    fe=(fes.get(k));
                                    System.out.println(fe.entryName);
                                }
                            }
                            //   continue;
                            
                        }
                    }
                }
                //System.out.println("\n-- End --");
            } catch (Exception e) {
                //System.out.println("-- ERROR --");
                //e.printStackTrace();
            }
        }
    }
     public static void extractInSitu(String chmfile,String savep) {
     ChmManager cm=new ChmManager(chmfile);
     }
    public static void extractInSitu(ChmManager cm,String savep) {
        ArrayList<FileEntry> fes=cm.enumerateFiles();
        FileEntry fe;
        String name;
        File f;
        System.out.println("ok");
        for(int k=0;k<fes.size();k++) {
            fe=(fes.get(k));
            name=fe.entryName;
            if(name.lastIndexOf('/')==name.length()-1)//dir
            {
                f=new File(savep.concat(name));
                f.mkdirs();
            } else
                extractOne(cm,fe,savep.concat(name));
            System.out.println(fe.entryName);
            
        }
    }
    
    public boolean readBook(BookModel model) {
		
    	ChmHtmlReader chr = null;
    	ArrayList<FileEntry> fes = null;
    	FileEntry fe;
        String name;
        ChmManager cm = null;
        
    	try {
    		
    		Log.i("CHM model", "model = "+model);
    	
    	String path = model.Book.File.getPath();
    	Log.i("CHM path", "path = "+path);
    	
    	cm = new ChmManager(path);
    	
    	Log.i("ChmManager", ""+ cm);
    	
    	fes =cm.enumerateFiles();
       
        
		
			model.Book.setEncoding(cm.encoding);
			
			Log.i("CHM ENCODING" , ""+ cm.encoding);
			
			chr = new ChmHtmlReader(model);
		} catch (Exception e1) {
			
			Log.e("CHM readBook Exception",""+e1);
		}
		
		chr.setMainTextModel();
        
        for(int k=0;k<fes.size();k++) {
            fe=fes.get(k);
            name=fe.entryName;
            if(name.lastIndexOf('/')==name.length()-1)//dir
            {
              //  f=new File(savep.concat(name));
              //  f.mkdirs();
            } else
            if(name.toLowerCase().endsWith("html")||name.toLowerCase().endsWith("htm")){           	 
            	 
            	 ByteArrayOutputStream baops=null;
                 try {
                	 byte[][]tmp=cm.retrieveObject(fe);
                	 baops=new ByteArrayOutputStream();
                	 
                     for(int i=0;i<tmp.length;i++)
                    	 baops.write(tmp[i]);
                    
                     baops.close();
                     
                     byte[] ba = baops.toByteArray();
                     
                     Log.i(name, ba.toString());
                     
                    // chr.endParagraph();
                     chr.beginParagraph( ZLTextParagraph.Kind.END_OF_SECTION_PARAGRAPH);
                     chr.clearConrol();
                     
                     chr.readBook(new ByteArrayInputStream(ba));
                     
                     
                     
                 } catch(Exception e) {
                	 
                	 Log.e("CHM PLUGIN EXCEPTION",""+e);
                     
                 }
            }            
            
           
        }
        
        chr.unsetCurrentTextModel();
        
        Log.e("CHM PLUGIN ","ALL OK");
		return true;
	}
    
    
    
    public static void extractAll(String fpath,String savep) {
        //System.out.println("........mysnail");
        File tmpf=new File(fpath);
        MyFileFilter mff=new MyFileFilter("chm");
        if (tmpf.isDirectory()) {
            try {
                //System.out.println("-- Begin --\n");
                String[] c=tmpf.list();
                if ((c!=null) && (c.length!=0)) {
                    for (int i=0;i<c.length;i++) {
                        String tmp_path=fpath+"/"+c[i];
                        File tf=new File(tmp_path);
                        if (tf.isDirectory()) {
                            extractAll(tmp_path, savep+"/"+c[i]);
                        }
                    }
                    File[] li=tmpf.listFiles(mff);
                    ChmManager cm=null;
                    if(li!=null) {
                        FileEntry fe;
                        String f;
                        for(int j=0;j<li.length;j++) {
                            System.out.print("............................");
                            System.out.println(li[j]);
                            f=li[j].getAbsolutePath();
                            if(li[j].exists())System.out.println("i'm here");
                            try {
                                cm=new ChmManager(f);
                            } catch(Exception e) {
                            }
                            if(cm==null) System.out.println("can't open"); //hmm,it seemed that files that can't be opened is because they don't exist.
                            else {
                                extractInSitu(cm,savep);
                            }
                        }
                        //   continue;
                        
                    }
                }
                //System.out.println("\n-- End --");
            } catch (Exception e) {
                //System.out.println("-- ERROR --");
                //e.printStackTrace();
            }
        }
        
        
        
        
    }
	
}
class MyFileFilter implements FilenameFilter//it's an interface
{
    // File f=new File("lay");
    String ext;
    public MyFileFilter(String fileext) {
        ext="."+fileext;
    }
    public boolean accept(File dir, String name)  //��INTERFACE�﷽��ʵ�廯ʱ���÷���ӦΪpublic ���Ե�
    {
        return  name.toLowerCase().endsWith(ext)&dir.canRead();
        // if(dir.getParent()="D:\\lay"&&name.indexOf(name)>-1)return true;
        // else return false;
    }
    
    
}
