#include "pbpoint.h"
#include "math.h"
#include <cmath>

PBPoint::PBPoint()
	:m_x(0)
	,m_y(0)
{
}

PBPoint::PBPoint(int x, int y)
	:m_x(x)
	,m_y(y)
{
}

PBPoint PBPoint::operator+ (PBPoint& r)
{
	PBPoint res;
	res.m_x = m_x + r.m_x;
	res.m_y = m_y + r.m_y;
	return res;
}

void PBPoint::operator= (const PBPoint& r)
{
	m_x = r.m_x;
	m_y = r.m_y;
}

void PBPoint::operator+= (PBPoint& r)
{
	m_x += r.m_x;
	m_y += r.m_y;
}

void PBPoint::operator-= (PBPoint& r)
{
	m_x -= r.m_x;
	m_y -= r.m_y;
}

int PBPoint::GetX() const
{
	return m_x;
}

int PBPoint::GetY() const
{
	return m_y;
}

double PBPoint::Distance(const PBPoint& p)
{
	int dx = m_x - p.m_x;
	int dy = m_y - p.m_y;
	return sqrt( dx*dx + dy*dy );
}
