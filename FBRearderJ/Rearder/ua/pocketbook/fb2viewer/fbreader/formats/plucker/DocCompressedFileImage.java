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

package ua.pocketbook.fb2viewer.fbreader.formats.plucker;

import java.io.IOException;
import java.io.InputStream;


import ua.pocketbook.fb2viewer.fbreader.formats.pdb.DocDecompressor;
import ua.pocketbook.fb2viewer.zlibrary.core.filesystem.ZLFile;
import ua.pocketbook.fb2viewer.zlibrary.core.image.ZLSingleImage;
import ua.pocketbook.fb2viewer.zlibrary.core.library.ZLibrary;

public class DocCompressedFileImage extends ZLSingleImage {
	private final ZLFile myFile;
	private final int myOffset;
	private final int myCompressedSize;
	
	public DocCompressedFileImage(String mimeType, final ZLFile file, final int offset, final int compressedSize) {
		super(mimeType);
		myFile = file;
		myOffset = offset;
		myCompressedSize = compressedSize;
	}

	public byte[] byteData() {
		try {
			final InputStream stream = myFile.getInputStream();
			if (stream == null) {
				return new byte[0];
			}

			stream.skip(myOffset);
			byte [] targetBuffer = new byte[65535];
			final int size = DocDecompressor.decompress(stream, targetBuffer, myCompressedSize);
			if (size > 0 && size != 65535) {
				byte [] buffer = new byte[size];
				System.arraycopy(targetBuffer, 0, buffer, 0, size);
				return buffer;
			}
			return targetBuffer;
		} catch (IOException e) {}
		
		return new byte[0];
	}
}
