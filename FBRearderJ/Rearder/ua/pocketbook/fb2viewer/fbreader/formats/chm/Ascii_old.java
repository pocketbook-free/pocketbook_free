/*
 * Ascii.java
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
import java.math.*;

/**
 *
 * @author yufeng
 */
public class Ascii_old {
    private byte[] byteval;
    private int length;
    
    /** Creates a new instance of Ascii */
    public Ascii_old() {
    }
    public Ascii_old(String s) {
        string2Byte(s);
        
    }
    private void string2Byte(String s) {
        char[] c=s.toCharArray();
        this.length=c.length;
        for (int i=0;i<c.length;i++)
            this.byteval[i]=(byte)c[i];
    }
    public boolean isEqual(byte[] b) {
        if (b.length !=this.length)
            return false;
        for (int i=0;i<this.length;i++)
            if (b[i]!=this.byteval[i])
                return false;
        return true;
    }
   //this create  ascii byte array from string 
    public static byte[] string2ByteArray(String s) {
        char[] c=s.toCharArray();
        byte[] byteval=new byte[c.length];
        //this.length=c.length;
        for (int i=0;i<c.length;i++)
            byteval[i]=(byte)c[i];
        return byteval;
    }
    //this compare two byte arrays
    public static boolean isEqual(byte[] ori,byte[] dest) {
        if (ori.length !=dest.length)
            return false;
        for (int i=0;i<ori.length;i++)
            if (ori[i]!=dest[i])
                return false;
        return true;
    }
    //this trans little indian bytes to big indian bytes for creating int or long or big integer
    private static byte[] little2BigIndian(byte[] ori) {
         if (ori.length%2!=0)return null;//can't convert,it should be able to divide by 2,that is two bytes.
        byte [] dest=new byte[ori.length];
        for (int i=0;i<ori.length;i++)
            dest[ori.length-i-1]=ori[i+1-2*(i%2)];
        return dest;
    }
        private static byte[] big2LittleIndian(byte[] ori) {
            if (ori.length%2!=0)return null;
        byte [] dest=new byte[ori.length];
        for (int i=ori.length-1;i>=0;i--)
            dest[ori.length-i-1]=ori[i+1-2*(i%2)];
        return dest;
    }
        //this trans little indian bytes to int
    public static int littleIndian2Int(byte[] ori) {
        BigInteger bi=new BigInteger(little2BigIndian(ori));
        return bi.intValue();
    }
     //this trans little indian bytes to long
    public static long littleIndian2Long(byte[] ori) {
        BigInteger bi=new BigInteger(little2BigIndian(ori));
        return bi.longValue();
    }
     //this trans little indian bytes to big integer
        public static BigInteger littleIndian2BigInteger(byte[] ori) {
        return(new BigInteger(little2BigIndian(ori)));        
    }
         //this trans big integer to little indian bytes
        public static byte[] bigInteger2LittleBytes(BigInteger bi)
        {
            byte[] b=bi.toByteArray();
            return big2LittleIndian(b);            
        }
        //get encint integer in bytes,get no encint, 0,1,2,3
        public static BigInteger getEncint(byte[] b,int offset,int num)
        {
            BigInteger bi=BigInteger.ZERO;
            int i=offset;
             byte [] sb=new byte[1];
             if (num>0)
             {
             for(int j=0;j<num;j++)
             {
                 for(i=i;i<b.length;i++)
                 {
                 if(b[i]>=0)                 
                     break;
                 }
             }
             }
            while(i<b.length)
            {
                if(b[i]<0)//first bit is 1
                {                    
                    sb[0]=(byte)(b[i]&0x7f);
                    BigInteger tmp=new BigInteger(sb);
                    bi=bi.shiftLeft(7);
                    bi=bi.add(tmp);
                }
                else
                    break;
                i++;
            }
              sb[0]=(byte)(b[i]&0x7f);
                    BigInteger tmp=new BigInteger(sb);
                    bi=bi.shiftLeft(7);
                    bi=bi.add(tmp);
                    return bi;            
        }
        //trans ascii bytes to string
        public static String asciiBytes2String(byte[] b)
        {
            char[] tmp=new char[b.length];
            for (int i=0;i<b.length;i++)
                tmp[i]=(char)b[i];
            return String.valueOf(tmp);            
        }
        //trans string to ascii bytes
        public static byte[] string2AsciiBytes(String s)
        {
            char [] c=s.toCharArray();
            byte[] b=new byte[c.length];
            for (int i=0;i<c.length;i++)
                b[i]=(byte)c[i];
            return b;
        }
        
}
