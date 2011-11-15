#ifndef _VIEWPORT_HXX_
#define _VIEWPORT_HXX_


class viewport_type
{
public:
    // shift directions
    enum direction_type {
        LEFTUP,
        UP,
        RIGHTUP,
        LEFT,
        HERE,
        RIGHT,
        LEFTDOWN,
        DOWN,
        RIGHTDOWN
    };

private:
    // overcloak for window shifting
    enum { OVERCLOAK = 2};

    // game area cache
    game_type::area_type m_area;
    // window sizes
    int m_width, m_height;
    // window current position
    coord_type m_pos;

public:
    viewport_type(int areawidth, int areaheight, int winwidth, int winheight):
        m_area(areawidth, areaheight)
    {
        assert(areawidth > 0 && areaheight > 0);
        assert(winwidth > 0 && winheight > 0);
        resize(winwidth, winheight);
    }

    // changing window size
    void resize(int width, int height)
    {
        assert(width > 0 && height > 0);

        // calc new size
        width = min(m_area.width(), width);
        height = min(m_area.height(), height);

        // shift position
        m_pos.i += (m_height - height) / 2;
        m_pos.j += (m_width - width) / 2;

        // set new size
        m_width = width;
        m_height = height;

        // correct position
        shift(HERE);
    }

    // size of window
    int width(void) const
    {
        return m_width;
    }
    int height(void) const
    {
        return m_height;
    }

    // relative position and size of window
    void ratio(double &rx, double &ry, double &rw, double &rh) const
    {
        rx = 1.0 * m_pos.j / m_area.width();
        ry = 1.0 * m_pos.i / m_area.height();
        rw = 1.0 * m_width / m_area.width();
        rh = 1.0 * m_height / m_area.height();
    }

    // translate drawpack to window drawpack
    // (cache new cells, output cells in window)
    drawpack_type translate(const drawpack_type &src)
    {
        drawpack_type pack;

        // min and max coordinates in drawpack
        int mini = m_area.height(), maxi = 0;
        int minj = m_area.width(), maxj = 0;

        drawpack_type::const_iterator it;
        for (it = src.begin(); it != src.end(); it++)
        {
            assert(it->first.i >= 0 && it->first.i < m_area.height());
            assert(it->first.j >= 0 && it->first.j < m_area.width());

            // store cell new content
            m_area.cell(it->first) = it->second;
            // if cell coords hit in current window
            if (it->first.i >= m_pos.i && it->first.i < m_pos.i + m_height &&
                it->first.j >= m_pos.j && it->first.j < m_pos.j + m_width)
            {
                // add to pack with shifted coords
                pack.push_back(
                    make_pair(
                        coord_type(it->first.i - m_pos.i, it->first.j - m_pos.j),
                        it->second));
            };
            // store max and min cell coords (with overcloack correction)
            mini = min(mini, it->first.i - OVERCLOAK);
            maxi = max(maxi, it->first.i + OVERCLOAK);
            minj = min(minj, it->first.j - OVERCLOAK);
            maxj = max(maxj, it->first.j + OVERCLOAK);
        };

        // if coordinates is out of bounds, shift window
        if (mini < m_pos.i || maxi >= m_pos.i + m_height ||
            minj < m_pos.j || maxj >= m_pos.j + m_width)
        {
            coord_type oldpos = m_pos;
            if (mini < m_pos.i || maxi >= m_pos.i + m_height)
                m_pos.i = (mini + maxi - m_height) / 2;
            if (minj < m_pos.j || maxj >= m_pos.j + m_width)
                m_pos.j = (minj + maxj - m_width) / 2;
            // correct position
            shift(HERE);

            // if pos is changed, return full window content
            if (oldpos != m_pos)
                pack = content();
        };

        return pack;
    }

    // shift window in correspondent direction
    bool shift(direction_type direction)
    {
        coord_type oldpos = m_pos;

        // horizontal shift
        int hshift = 0;
        if (direction == LEFTUP || direction == LEFT || direction == LEFTDOWN)
            hshift = -1;
        if (direction == RIGHTUP || direction == RIGHT || direction == RIGHTDOWN)
            hshift = 1;
        // vertical shift
        int vshift = 0;
        if (direction == LEFTUP || direction == UP || direction == RIGHTUP)
            vshift = -1;
        if (direction == LEFTDOWN || direction == DOWN || direction == RIGHTDOWN)
            vshift = 1;

        // change positions
        m_pos.i += vshift * (m_height - OVERCLOAK);
        m_pos.j += hshift * (m_width - OVERCLOAK);

        // correct positions, if out of interval
        if (m_pos.i > m_area.height() - m_height)
            m_pos.i = m_area.height() - m_height;
        if (m_pos.j > m_area.width() - m_width)
            m_pos.j = m_area.width() - m_width;
        if (m_pos.i < 0)
            m_pos.i = 0;
        if (m_pos.j < 0)
            m_pos.j = 0;

        return oldpos != m_pos;
    }

    bool shift_rel(double sx, double sy)
    {
        coord_type oldpos = m_pos;
        // relative shift
        m_pos.j -= int(m_width * sx + 0.5);
        m_pos.i -= int(m_height * sy + 0.5);
        // correct position
        shift(HERE);
        return oldpos != m_pos;
    }

    bool shift_abs(double cx, double cy)
    {
        coord_type oldpos = m_pos;
        // absolute shift
        m_pos.j = int(m_area.width() * cx + 0.5) - m_width / 2;
        m_pos.i = int(m_area.height() * cy + 0.5) - m_height / 2;
        // correct position
        shift(HERE);
        return oldpos != m_pos;
    }

    // full window content
    drawpack_type content(void) const
    {
        drawpack_type pack;
        // for every cell in window
        for (int i = 0; i < m_height; i++)
        {
            for (int j = 0; j < m_width; j++)
            {
                // add current content to pack
                pack.push_back(
                    make_pair(
                        coord_type(i, j),
                        m_area.cell(coord_type(m_pos.i + i, m_pos.j + j))));
            };
        };
        return pack;
    }
};


#endif // _VIEWPORT_HXX_
