/*
 * RandomAccessChm.java
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
import java.math.*;

/**
 *
 * @author yufeng
 *
 *it provide the random method to read chm
 *now it has nothing to do but inherit from randomaccessfile
 */
public class RandomAccessChm extends RandomAccessFile {
    
    private static int remains=0;/** Creates a new instance of RandomAccessChm */
   private static long val=0;
    public RandomAccessChm(String filename,String mode) throws FileNotFoundException{
        super(filename,mode);
    }
   
    /*if file is too big,> 9223372036854775807
    * use this to location
     *seek(BigInteger loc)
     **/
   /* public void seek(BigInteger loc) throws IOException {
        long loci;
        if(loc.compareTo(new BigInteger("9223372036854775807"))==1)
            throw new IOException("can't locate the position");
        else
            loci =loc.longValue();
        try {
            this.seek(loci);
        } catch(IOException e) {
            throw e;
        }
    }
    ///////////////////////////////////////////////////////////////////////////////////////////
    /*
    //bitstream will have 8 bytes
    public long readBits(int i) throws IOException {
        try {
            //  int b=this.read();
            //   int c=this.read();
            if(remains==0){val=this.read()+(this.read()<<8);remains=remains+16;}
            // System.out.println(c);
            //   System.out.println(b);
            // System.out.println(val);
            if(remains<17){val=(val<<16)+this.read()+((this.read())<<8);remains=remains+16;}
            // System.out.println(val);
            if(remains<33){val=(val<<16)+this.read()+((this.read())<<8);remains=remains+16;}
            //System.out.println(val);
            if(remains<49){val=(val<<16)+this.read()+((this.read())<<8);remains=remains+16;}
            //System.out.println(val);
        } catch(IOException e) {
            throw e;
        }
        remains=remains-i;
        System.out.println("readbit");
        System.out.println(remains);
        System.out.println(val);
        long tmp=val>>>remains;
        val=val-(tmp<<remains);
        System.out.println(tmp);
        return tmp;
    }
    public long peekBit(int i) throws IOException {
        try {
            //  int b=this.read();
            //   int c=this.read();
            if(remains==0){val=this.read()+(this.read()<<8);remains=remains+16;}
            // System.out.println(c);
            //   System.out.println(b);
            // System.out.println(val);
            if(remains<17){val=(val<<16)+this.read()+((this.read())<<8);remains=remains+16;}
            // System.out.println(val);
            if(remains<33){val=(val<<16)+this.read()+((this.read())<<8);remains=remains+16;}
            //System.out.println(val);
            if(remains<49){val=(val<<16)+this.read()+((this.read())<<8);remains=remains+16;}
            //System.out.println(val);
        } catch(IOException e) {
            throw e;
        }
        // remains=remains-i;
        //System.out.println(remains);
        //System.out.println(val);
        long tmp=val>>>(remains-i);
        // val=val-(tmp<<remains);
       // System.out.println(tmp);
        return tmp;
    }
    public void removeBit(int i) throws IOException {
        try {
            //  int b=this.read();
            //   int c=this.read();
            if(remains==0){val=this.read()+(this.read()<<8);remains=remains+16;}
            // System.out.println(c);
            //   System.out.println(b);
            // System.out.println(val);
            if(remains<17){val=(val<<16)+this.read()+((this.read())<<8);remains=remains+16;}
            // System.out.println(val);
            if(remains<33){val=(val<<16)+this.read()+((this.read())<<8);remains=remains+16;}
            //System.out.println(val);
            if(remains<49){val=(val<<16)+this.read()+((this.read())<<8);remains=remains+16;}
            //System.out.println(val);
        } catch(IOException e) {
            throw e;
        }
        remains=remains-i;
       // System.out.println(remains);
       // System.out.println(val);
        long tmp=val>>>remains;
        val=val-(tmp<<remains);
        //System.out.println(tmp);
        //return tmp;
    }
    public boolean ready4Bit(BigInteger bi)  throws IOException {
        try {
            this.seek(bi);
        } catch(IOException e) {
            throw e;
        }
        remains=0;
        val=0;
        return true;
    }
     //this change 4 bytes or 8 bytes (into int or long), yet they are unsigned, so to long and biginteger
    //these two will transform little indian byte array to big indian number
    //so the last byte will be the significant
    //they are fit for unsigned int and unsigned long, just for keeping precision
    public long readUInt()  throws IOException {
        return readBytes2BigInteger(4).longValue();
    }
    //readChmInt()
    public  int readChmInt() throws IOException {
        return (int)(this.readUInt());
    }
    public BigInteger readULong() throws IOException {
        return readBytes2BigInteger(8);
    }
    private BigInteger readBytes2BigInteger(int i) throws IOException {
        // int i=8;
        byte[] b=new byte[i];
        try {
            this.read(b);
        } catch(IOException e) {
            throw e;
        }
        if(b==null) return BigInteger.ZERO;
        if(b.length<i)i=b.length;
        byte[] tmp=new byte[i];
        for(int j=i-1;j>=0;j--) {
            tmp[i-j-1]=b[j];
        }
        return new BigInteger(tmp);
    }
    //get encint integer in bytes,get no encint, 0,1,2,3
    //offset: from where to get bytes
    //num: get which encint,1st? 2nd? 3rd?
    public BigInteger readEncint() throws IOException {
        byte ob;
        BigInteger bi=BigInteger.ZERO;
        byte[] b=new byte[1];
        int i=0;
        try {
            while((ob=this.readByte())<0) {
                b[0]=(byte)((ob&0x7f));
                bi=bi.shiftLeft(7).add(new BigInteger(b));
            }
        } catch(IOException e) {
            throw e;
        }
        b[0]=(byte)((ob&0x7f));
        bi=bi.shiftLeft(7).add(new BigInteger(b));
        return bi;
    }
    //get and trans utf8 char to unicode char
    //modified utf8 was seemed to be used in chm,it meats the following constraints:
    //1.only 1 byte or 3 bytes are used
    //2.1 byte like 0*******,3 bytes like 1110**** 10****** 10******
    //so try to catch them
    public char readUtfChar() throws IOException {
        byte ob;
        int i=1;
        byte[] ba;
        try {
            ob=this.readByte();
            if(ob<0) {
                i=2;
                while((ob<<i)<0)i++;
            }
            ba=new byte[i];
            ba[0]=ob;
            int j=1;
            while(j<i) {
                ba[j]=this.readByte();
                j++;
            }
        } catch(IOException e) {
            throw e;
        }
        i=ba.length;
        //char tmp;
        if (i==1) return (char)ba[0];
        else {
            int n;
            n=(ba[0]<<(i+1))>>>(i+1);
            int j=1;
            while(j++<i)n=n<<2+ba[j]&0x3f;
            return (char)n;
        }
    }
    //get and trans utf8 string to unicode string
    public String readUtfString(int num)  throws IOException {
        char[] tmp=new char[num];
        try {
            for(int i=0;i<num;i++)
                tmp[i]=this.readUtfChar();
        } catch(IOException e) {
            throw e;
        }
        return (new String(tmp));
    }    
    */
    
}
