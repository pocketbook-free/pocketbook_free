/*
 * FileEntry.java
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
//import java.lang.*;
//import java.io.*;
import java.math.*;
//import java.util.*;

/**
 *
 * @author yufeng
*style of directory list entry in pmgl section
 *it will be used in many class, so declare it public 
 */
public class FileEntry
    {
        int nameLen;
        public String entryName;
        int compressType;
        BigInteger offset;
        BigInteger length;
        public FileEntry(String name)
        {
            entryName=name;
            nameLen=name.length();
        }
    }
