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
 *this deal with some differents between windows and java 
 */
public class Ascii {
     /*just reverse bytes order,
     **/
  /*  public static byte[] reverseBytesOrder(byte[] b)
    {
        if(b==null)return null;
        byte[] tmp=new byte[b.length];
        for(int i=0;i<b.length;i++)
            tmp[b.length-i-1]=b[i];
        return tmp;
    }
    
    /*this create  ascii byte array from string, 
     *these chars are Ascii, but in java string
     *they have two bytes each
     *so, use each byte to represent one char
      **/
   /* public static byte[] string2ByteArray(String s) {
        char[] c=s.toCharArray();
        byte[] byteval=new byte[c.length];
        //this.length=c.length;
        for (int i=0;i<c.length;i++)
            byteval[i]=(byte)c[i];
        return byteval;
    }
     /*this compare two byte arrays,
      *return true if they are really same
      **/
  /*  public static boolean isEqual(byte[] ori,byte[] dest) {
        if (ori.length !=dest.length)
            return false;
        for (int i=0;i<ori.length;i++)
            if (ori[i]!=dest[i])
                return false;
        return true;
    }
 /////////////////////////////////////////////////////////////////   
    /** Creates a new instance of Ascii */
  /*  public Ascii() {
    }
   
     
    //rearrange bytes order
    public static byte[] badc2Abcd(byte[] b)
    {
        byte[]tmp=null;
        if (b.length%2==1)return tmp;
        tmp=new byte[b.length];
        for(int i=0;i<b.length;i++)
            tmp[i+1-(i%2)*2]=b[i];
        return tmp;
    }
   
    //this change 4 bytes or 8 bytes (into int or long), yet they are unsigned, so to long and biginteger
    //these two will transform little indian byte array to big indian number
    //so the last byte will be the significant
    //they are fit for unsigned int and unsigned long, just for keeping precision
    public static long fourBytes2Long(byte[] b)
    {
        if(b==null)return 0;
        int i=4;
        long tmp=0;
      if(b.length<i)i=b.length;
        for(int j=i-1;j>=0;j--)
        {
            tmp=tmp+(long)(b[j]<<(j*8));
        }
        return tmp;       
    }
        public static BigInteger eightBytes2BigInteger(byte[] b)
    {            
        if(b==null) return BigInteger.ZERO;
        int i=8;        
      if(b.length<i)i=b.length;
        byte[] tmp=new byte[i];
        for(int j=i-1;j>=0;j--)
        {
            tmp[i-j-1]=b[j];
        }
        return new BigInteger(tmp);         
    }
        //get encint integer in bytes,get no encint, 0,1,2,3
        //offset: from where to get bytes
        //num: get which encint,1st? 2nd? 3rd?
        public static BigInteger transEncint2BigInteger(byte[] b,int offset,int num)
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
        //get first encint from bytes
        public static BigInteger transEncint2BigInteger(byte[] b)
        {
            return transEncint2BigInteger(b, 0, 1);       
        }
        //trans utf8 to unicode
        //modified utf8 was seemed to be used in chm,it meats the following constraints:
        //1.only 1 byte or 3 bytes are used
        //2.1 byte like 0*******,3 bytes like 1110**** 10****** 10******
        //so try to catch them
        public static String transUtf82Unicode(byte[] b)
        {    String tmp=null;
             return tmp;
        }
        //this trans big integer to little indian bytes
        public static byte[] bigInteger2LittleBytes(BigInteger bi)
        {
            byte[] b=bi.toByteArray();
            return big2LittleIndian(b);            
        }
        private static byte[] big2LittleIndian(byte[] ori) {
            if (ori.length%2!=0)return null;
            byte [] dest=new byte[ori.length];
            for (int i=ori.length-1;i>=0;i--)
                dest[ori.length-i-1]=ori[i+1-2*(i%2)];
            return dest;
        }
   **/
        
}
