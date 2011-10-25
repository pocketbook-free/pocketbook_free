#include "pbscrollbar.h"

extern "C"
{
    extern const ibitmap scrollUp;
    extern const ibitmap scrollDown;
    extern const ibitmap scrollLeft;
    extern const ibitmap scrollRight;
}

PBScrollBar::PBScrollBar()
    : PBWindow()
{
    firstControl = NULL;
    secondControl = NULL;
    m_maximum = 0;
    m_minimum = 0;
    m_pageStep = 0;
    m_value = 0;
    m_orientation = PB_VERTICAL;
}

PBScrollBar::~PBScrollBar()
{
}

int PBScrollBar::OnDraw(bool force)
{
    int firstLength, secondLength;
    int sliderLength, sliderAreaLength, sliderPos, sliderWidth;
    
	if ( IsVisible() && (IsChanged() || force) && m_value >= m_minimum && m_value <= m_maximum)
    {

		PBGraphics *graphics = GetGraphics();
		if (m_orientation == PB_VERTICAL)
		{
			firstLength = firstControl->height;
			secondLength = secondControl->height;
			sliderAreaLength = GetHeight();
			sliderWidth = (firstControl->width > secondControl->width ? secondControl->width : firstControl->width) - 2;
		}
		else
		{
			firstLength = firstControl->width;
			secondLength = secondControl->width;
			sliderAreaLength = GetWidth();
			sliderWidth = (firstControl->height > secondControl->height ? secondControl->height : firstControl->height) - 2;
		}
        
		sliderAreaLength -= firstLength + secondLength;

		sliderLength = (m_pageStep * sliderAreaLength ) / (m_maximum - m_minimum + m_pageStep);

		// setup minimum slider Length
		if (sliderLength < 10) sliderLength = 10;

		// calc slider pos
		sliderPos = ((sliderAreaLength - sliderLength) * (m_value - m_minimum)) / (m_maximum - m_minimum);
        
		if (m_orientation == PB_VERTICAL)
		{
			graphics->FillArea(GetLeft(), GetTop() + firstLength, GetWidth(), sliderAreaLength, WHITE);


			graphics->DrawBitmapBW(GetLeft(), GetTop(), firstControl);
            
			graphics->DrawLine(GetLeft(), GetTop() + firstLength, GetLeft(), GetTop() + firstLength + sliderAreaLength, BLACK);
			graphics->DrawLine(GetLeft() + GetWidth(), GetTop() + firstLength, GetLeft() + GetWidth(), GetTop() + firstLength + sliderAreaLength, BLACK);
            
			//graphics->FillArea(GetLeft() + 1, GetTop() + firstLength + sliderPos, sliderWidth, sliderLength, BLACK);
			graphics->DimArea(GetLeft() + 1, GetTop() + firstLength + sliderPos, sliderWidth, sliderLength, BLACK);

			graphics->DrawBitmapBW(GetLeft(), GetTop() + firstLength + sliderAreaLength, secondControl);
		}
		else
		{
			graphics->FillArea(GetLeft() + firstLength, GetTop(), sliderAreaLength, GetHeight(), WHITE);

			graphics->DrawBitmapBW(GetLeft(), GetTop(), firstControl);
            
			graphics->DrawLine(GetLeft() + firstLength, GetTop(), GetLeft() + firstLength + sliderAreaLength, GetTop(), BLACK);
			graphics->DrawLine(GetLeft() + firstLength, GetTop() + GetHeight(), GetLeft() + firstLength + sliderAreaLength, GetTop() + GetHeight(), BLACK);
            
			//graphics->FillArea(GetLeft() + firstLength + sliderPos, GetTop() + 1, sliderLength, sliderWidth, BLACK);
			graphics->DimArea(GetLeft() + firstLength + sliderPos, GetTop() + 1, sliderLength, sliderWidth, BLACK);

			graphics->DrawBitmapBW(GetLeft() + firstLength + sliderAreaLength, GetTop(), secondControl);
		}

    }
    return 0;
}

int PBScrollBar::MainHandler(int type, int par1, int par2)
{
    int fcX1, fcX2, fcY1, fcY2;
    int result = 0;

    PBPoint clientPoint = ScreenToClient(PBPoint(par1, par2));

	if (IsVisible() && firstControl && secondControl && m_pParent && type == EVT_POINTERUP)
    {
	    fcX1 = 0;
		fcX2 = m_orientation == PB_VERTICAL ? GetWidth() : firstControl->width;
	    fcY1 = 0;
		fcY2 = m_orientation == PB_VERTICAL ? firstControl->height : GetHeight();
	    // check if first control pressed
	    if (clientPoint.GetX() >= fcX1 && clientPoint.GetX() <= fcX2 &&
			clientPoint.GetY() >= fcY1 && clientPoint.GetY() <= fcY2)
	    {
			if (m_value <= m_minimum)
			{
				setValue(m_minimum);
			}
			else
			{
				setValue(m_value - 1);
			}
			result = 1;
	    }

	    // check if second control pressed
		fcX1 = m_orientation == PB_VERTICAL ? 0 : GetWidth() - secondControl->width;
		fcX2 = GetWidth();
		fcY1 = m_orientation == PB_VERTICAL ? GetHeight() - secondControl->height : 0;
		fcY2 = GetHeight();

	    if (clientPoint.GetX() >= fcX1 && clientPoint.GetX() <= fcX2 &&
			clientPoint.GetY() >= fcY1 && clientPoint.GetY() <= fcY2)
	    {
			if (m_value >= m_maximum)
			{
				setValue(m_maximum);
			}
			else
			{
				setValue(m_value + 1);
			}
			result = 1;
	    }

	    // check slider click
	    fcX1 = m_orientation == PB_VERTICAL ? 0 : firstControl->width;
		fcX2 = m_orientation == PB_VERTICAL ? GetWidth() : GetWidth() - secondControl->width;
	    fcY1 = m_orientation == PB_VERTICAL ? firstControl->height : 0;
		fcY2 = m_orientation == PB_VERTICAL ? GetHeight() - secondControl->height : GetHeight();

	    if (clientPoint.GetX() >= fcX1 && clientPoint.GetX() <= fcX2 &&
			clientPoint.GetY() >= fcY1 && clientPoint.GetY() <= fcY2)
	    {
			setValue(m_orientation == PB_VERTICAL ?
					 ((clientPoint.GetY() - fcY1) * (m_maximum - m_minimum + 1) / (fcY2 - fcY1) + m_minimum):
					 ((clientPoint.GetX() - fcX1) * (m_maximum - m_minimum + 1) / (fcX2 - fcX1) + m_minimum));
			result = 1;
	    }

	    if (result)
	    {
			if (m_pParent)
				m_pParent->OnCommand(PBSCROLLBAR, PBSTATECHANGED, (int) this);

			//m_pParent->OnChildFocusChanged(this);

			// if(m_isActive)
			//m_pParent->OnCommand(m_commandId, par1, par2);
	    }
    }

    return result;
}

bool PBScrollBar::Create(PBWindow* pParent, int minimum, int maximum, int pageStep, PBORIENTATION orientation)
{
    int scrollWidth;
    bool result = false;
    if (pParent != NULL)
    {
		m_orientation = orientation;

		if (m_orientation == PB_VERTICAL)
		{
			firstControl = (ibitmap*) &scrollUp;
			secondControl = (ibitmap*) &scrollDown;
			scrollWidth = (firstControl->width > secondControl->width ? firstControl->width : secondControl->width) - 1;

			PBWindow::Create(pParent,
							 pParent->GetWidth() - scrollWidth - 1,
							 0,
							 scrollWidth,
							 pParent->GetHeight());
		}
		else
		{
			firstControl = (ibitmap*) &scrollLeft;
			secondControl = (ibitmap*) &scrollRight;
			scrollWidth = (firstControl->height > secondControl->height ? firstControl->height : secondControl->height) - 1;

			PBWindow::Create(pParent,
							 0,
							 pParent->GetHeight() - scrollWidth - 1,
							 pParent->GetWidth(),
							 scrollWidth);
		}

		m_minimum = minimum;
		m_maximum = maximum;
		m_pageStep = pageStep;
		MarkAsChanged();
		result = true;
    }
    return result;
}

void PBScrollBar::updateVisibleState()
{
    if (m_maximum > m_minimum &&
		(m_maximum - m_minimum))
    {
		ChangeWindowStyle(PBWS_VISIBLE, 0);
		//SetVisible(true);
    }
    else
    {
		ChangeWindowStyle(0, PBWS_VISIBLE);
		//SetVisible(false);
    }
}

void PBScrollBar::setMaximum(int value)
{
    if (m_maximum != value)
    {
		m_maximum = value;
		MarkAsChanged();
		updateVisibleState();
    }
}

void PBScrollBar::setMinimum(int value)
{
    if (m_minimum != value)
    {
        m_minimum = value;
		MarkAsChanged();
		updateVisibleState();
    }
}

void PBScrollBar::setPageStep(int value)
{
    if (value < (m_maximum - m_minimum) &&
		m_pageStep != value)
    {
        m_pageStep = value;
		MarkAsChanged();
		updateVisibleState();
    }
}

void PBScrollBar::setValue(int value)
{
    if (m_value != value)
    {
        m_value = value;
		MarkAsChanged();
    }
}

int PBScrollBar::maximum() const
{
    return m_maximum;
}

int PBScrollBar::minimum() const
{
    return m_minimum;
}

PBORIENTATION PBScrollBar::orientation() const
{
    return m_orientation;
}

int PBScrollBar::pageStep() const
{
    return m_pageStep;
}

int PBScrollBar::value() const
{
    return m_value;
}
