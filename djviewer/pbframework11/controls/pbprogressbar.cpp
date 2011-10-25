#include "pbprogressbar.h"

PBProgressBar::PBProgressBar()
    : PBWindow()
{
    m_rectOffset = 2;
    m_properties = 0;
    m_value = 0;
}

PBProgressBar::~PBProgressBar()
{
}

int PBProgressBar::OnDraw(bool force)
{
    int drawBarWidth, drawBarHeight, leftSideWidth, rightSideWidth;

	if ( IsVisible() && (IsChanged() || force) )
    {
		PBGraphics *graphics = GetGraphics();
		graphics->DrawRect(GetLeft(), GetTop(), GetWidth(), GetHeight(), BLACK);

		if (m_value)
		{
			drawBarWidth = GetWidth() - (m_rectOffset << 1);
			drawBarHeight = GetHeight() - (m_rectOffset << 1);
			leftSideWidth = m_value ? (drawBarWidth * m_value) / 100 : 0;
			rightSideWidth = m_value != 100 ? (drawBarWidth - leftSideWidth) : 0;

			if (leftSideWidth)
				graphics->DimArea(GetLeft() + m_rectOffset, GetTop() + m_rectOffset, leftSideWidth, drawBarHeight, BLACK);

			if (rightSideWidth)
				graphics->FillArea(GetLeft() + m_rectOffset + leftSideWidth,  GetTop() + m_rectOffset, rightSideWidth, drawBarHeight, WHITE);
		}
    }
    return 0;
}

bool PBProgressBar::Create(PBWindow* pParent, int left, int top, int width, int height)
{
    PBWindow::Create(pParent, left, top, width, height);
    return true;
}

void PBProgressBar::reset()
{
    setValue(0);
}

void PBProgressBar::setValue(int value)
{
    if (value >= 0 && value <= 100 && m_value != value)
    {
        m_value = value;
		MarkAsChanged();
    }
}

int PBProgressBar::value() const
{
    return m_value;
}

