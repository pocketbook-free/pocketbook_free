#ifndef _PICSTORE_HXX_
#define _PICSTORE_HXX_


class picstore_type
{
private:
    enum {
        // color depth
        DEPTH = 2,
        // color mask (white color)
        MASK = ((1 << DEPTH) - 1),
        // prescribed transparent color
        TRANSPARENT = 0x02
    };

    // all bitmaps set
    map<int, const ibitmap *> m_bmp;
    // picture combining set (for transparency processing)
    vector<unsigned char> m_combine;
    // resulting picture storage
    vector<unsigned char> m_bitmap;

public:
    picstore_type(void)
    {
        // fill picture map
        extern const ibitmap qback;
        extern const ibitmap qexit;
        extern const ibitmap qwall;
        extern const ibitmap qstairs;
        extern const ibitmap qstone;
        extern const ibitmap qstoneanni;
        extern const ibitmap qvalveleft;
        extern const ibitmap qvalveright;
        extern const ibitmap qvalveup;
        extern const ibitmap qvalvedown;
        extern const ibitmap qcharalice;
        extern const ibitmap qcharbob;
        extern const ibitmap qchardead;
        extern const ibitmap qbridge;
        extern const ibitmap qbridgebroken;
        extern const ibitmap qteleport;
        extern const ibitmap qelevator;
        extern const ibitmap qswitchleft;
        extern const ibitmap qswitchright;
        extern const ibitmap qflash;
        extern const ibitmap qselector;

        struct { int num; const ibitmap *bmp; } array[] = {
            { game_type::QBACK, &qback },
            { game_type::QEXIT, &qexit },
            { game_type::QWALL, &qwall },
            { game_type::QSTAIRS, &qstairs },
            { game_type::QSTONE, &qstone },
            { game_type::QSTONEANNI, &qstoneanni },
            { game_type::QVALVELEFT, &qvalveleft },
            { game_type::QVALVERIGHT, &qvalveright },
            { game_type::QVALVEUP, &qvalveup },
            { game_type::QVALVEDOWN, &qvalvedown },
            { game_type::QCHARALICE, &qcharalice },
            { game_type::QCHARBOB, &qcharbob },
            { game_type::QCHARDEAD, &qchardead },
            { game_type::QBRIDGE, &qbridge },
            { game_type::QBRIDGEBROKEN, &qbridgebroken },
            { game_type::QTELEPORT, &qteleport },
            { game_type::QELEVATOR, &qelevator },
            { game_type::QSWITCHLEFT, &qswitchleft },
            { game_type::QSWITCHRIGHT, &qswitchright },
            { game_type::QFLASH, &qflash },
            { game_type::QSELECTOR, &qselector }
        };
        for (int i = 0; i < int(sizeof(array) / sizeof(array[0])); i++)
        {
            // all pictures must be the same size and color depth
            assert(array[i].bmp->depth == DEPTH);
            assert(m_bmp.empty() || (
                m_bmp.begin()->second->height == array[i].bmp->height &&
                m_bmp.begin()->second->width == array[i].bmp->width &&
                m_bmp.begin()->second->scanline == array[i].bmp->scanline));
            m_bmp[array[i].num] = array[i].bmp;
        };

        // prepare array for picture combining with transparency
        m_combine.assign(0x10000, 0xFF);
        for (int i = 0; i < int(m_combine.size()); i++)
        {
            // byte of lower picture
            int lo = (i & 0x000000FF);
            // byte of upper picture
            int hi = (i & 0x0000FF00) >> 8;
            // enumerate all pixels in byte
            for (int j = 0; j < 8 / DEPTH; j++)
            {
                // if pixel of upper pic is not transparent
                if ((hi & (MASK << DEPTH * j)) != (TRANSPARENT << DEPTH * j))
                    // overwrite corresponding lower pixel with it
                    lo = (lo & ~(MASK << DEPTH * j)) | (hi & (MASK << DEPTH * j));
            };
            // store for future
            m_combine[i] = lo;
        };
    }
    int width(void) const
    {
        return m_bmp.begin()->second->width;
    }
    int height(void) const
    {
        return m_bmp.begin()->second->height;
    }

    const ibitmap *mkpic(int content)
    {
        // number of bytes in picture header
        int headersize = offsetof(ibitmap, data);
        // number of bytes in picture data
        int datasize =
            m_bmp.begin()->second->scanline *
            m_bmp.begin()->second->height;
        // form data structure (white background)
        m_bitmap.assign(headersize + datasize, 0xFF);
        // copy header (all headers a equal)
        memcpy(&m_bitmap.front(), m_bmp.begin()->second, headersize);
        // enumerate all bits in content
        for (int n = 0; n < int(sizeof(content) * 8); n++)
        {
            // if bit is set
            if ((content & (1 << n)) != 0)
            {
                // put corresponding pic over previous
                for (int j = 0; j < datasize; j++)
                {
                    // compose index from bytes of upper and lower pics
                    int idx =
                        ((m_bmp[n]->data[j] & 0xFF) << 8) |
                        (m_bitmap[headersize + j] & 0xFF);
                    // store result of combining operation
                    m_bitmap[headersize + j] = m_combine[idx];
                };
            };
        };
        return reinterpret_cast<ibitmap *>(&m_bitmap.front());
    }
};


#endif // _PICSTORE_HXX_
