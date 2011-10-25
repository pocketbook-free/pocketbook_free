#include "pbrect.h"
#include <assert.h>

PBRect::PBRect(int x, int y, int width, int height)
{
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
}

PBRect::~PBRect()
{
}

void PBRect::operator+= (const PBPoint &r)
{
	m_x += r.GetX();
	m_y += r.GetY();
}

// объединяет прямоугольники. Результат - прямоугольник описывающий оба прямоугольника
void PBRect::operator+= (const PBRect &r)
{
	int right = GetRight() > r.GetRight() ? GetRight() : r.GetRight();
	int bottom = GetBottom() > r.GetBottom() ? GetBottom() : r.GetBottom();
	m_x = m_x < r.m_x ? m_x : r.m_x;
	m_y = m_y < r.m_y ? m_y : r.m_y;
	SetRight(right);
	SetBottom(bottom);
}

int PBRect::GetX() const
{
    return m_x;
}

int PBRect::GetY() const
{
    return m_y;
}

int PBRect::GetLeft() const
{
	return m_x;
}

int PBRect::GetTop() const
{
	return m_y;
}

int PBRect::GetWidth() const
{
    return m_width;
}

int PBRect::GetHeight() const
{
    return m_height;
}

int PBRect::GetRight() const
{
//	assert(m_width > 0);
	return m_x + m_width - 1;
}

int PBRect::GetBottom() const
{
//	assert(m_height > 0);
	return m_y + m_height - 1;
}

int PBRect::GetCenterX() const
{
	return m_x + m_width/2;
}

int PBRect::GetCenterY() const
{
	return m_y + m_height/2;
}

int PBRect::GetSquare() const
{
	return GetHeight() * GetWidth();
}

PBPoint PBRect::GetCenter() const
{
	return PBPoint( m_x + m_width/2, m_y + m_height/2 );
}

void PBRect::SetX(int value)
{
    m_x = value;
}

void PBRect::SetY(int value)
{
    m_y = value;
}

void PBRect::SetWidth(int value)
{
    m_width = value;
}

void PBRect::SetHeight(int value)
{
    m_height = value;
}

void PBRect::SetRight(int value)
{
	m_width = value - m_x + 1;
}

void PBRect::SetBottom(int value)
{
	m_height = value - m_y + 1;
}

bool PBRect::isValid()
{
	return m_width > 0 && m_height > 0;
}

bool PBRect::isInRectange(int x, int y)
{
    bool result = false;

    if (x >= m_x &&
		x < (m_x + m_width) &&
        y >= m_y &&
		y < (m_y + m_height))
        result = true;

    return result;
}

bool PBRect::AddRectHorizontal(int x, int y, int width, int height)
{
    bool result = false;

    if (y == m_y &&
        height == m_height)
    {
		// if more from left (RTL support)
		if (x < m_x)
		{
			m_width += m_x - x;
			m_x = x;
		}
		// if more from right
        if ((x + width) > (m_x + m_width))
            m_width = x + width - m_x;
        result = true;
    }

    return result;
}

PBRect PBRect::GetUpdateRectange(std::vector<PBRect> &src)
{
    int x = 0, y = 0, width = 0, height = 0;
    int x1 = 0, y1 = 0, width1 = 0, height1 = 0;

    if (!src.empty())
    {
        std::vector<PBRect>::iterator iter = src.begin();
        x = (*iter).GetX();
        y = (*iter).GetY();
        width = (*iter).GetWidth();
        height = (*iter).GetHeight();
        // inc because first element was already obtained
        ++iter;

        while (iter != src.end())
        {
            x1 = (*iter).GetX();
            y1 = (*iter).GetY();
            width1 = (*iter).GetWidth();
            height1 = (*iter).GetHeight();

            if (x1 < x)
            {
                width += x - x1;
                x = x1;
            }

            if (y1 < y)
            {
                height += y - y1;
                y = y1;
            }

            if ((x1 + width1) > (x + width))
                width = x1 + width1 - x;

            if ((y1 + height1) > (y + height))
                height = y1 + height1 - y;

            ++iter;
        }
    }

    return PBRect(x, y, width, height);
}
