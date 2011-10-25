#include "pbtabpanel.h"

PBTabPanel::PBTabPanel():
		PBWindow(),
		m_pActiveTab(NULL),
		m_pInactiveTab(NULL),
		m_Alignment(-1),
		m_TabsCount(0),
		m_TabsCurrent(0),
		m_pFont(NULL)
{
}

PBTabPanel::~PBTabPanel()
{
	if (m_pActiveTab)
	{
		free(m_pActiveTab);
		m_pActiveTab = NULL;
	}
	if (m_pInactiveTab)
	{
		free(m_pInactiveTab);
		m_pInactiveTab = NULL;
	}
	if (m_pFont)
	{
		CloseFont(m_pFont);
		m_pFont = NULL;
	}
}

void PBTabPanel::Create(PBWindow *pParent)
{
	PBWindow::Create(pParent, -1, -1, -1, -1);
	m_TabsCount = 1;
	m_TabsCurrent = 1;
	SetAlignment(ALIGNMENT_BOTTOM);
	MarkAsChanged();
}

void PBTabPanel::Create(PBWindow *pParent, int total)
{

	PBTabPanel::Create(pParent);

	if (total < 1)
	{
		total = 1;
	}
	m_TabsCount = total;

	RecountViewTabs();

}

void PBTabPanel::Create(PBWindow *pParent, int total, int current)
{

	PBTabPanel::Create(pParent, total);

	if (current < 1)
	{
		current = 1;
	}

	if (current > m_TabsCount)
	{
		current = m_TabsCount;
	}

	m_TabsCurrent = current;

	RecountViewTabs();

}

void PBTabPanel::Create(PBWindow *pParent, int total, int current, int alignment)
{

	PBTabPanel::Create(pParent, total, current);

	SetAlignment(alignment);

	RecountViewTabs();

}

void PBTabPanel::SetAlignment(int alignment)
{
	if (alignment < 0 || alignment > 3)
	{
		alignment = ALIGNMENT_BOTTOM;
	}

	if (m_Alignment != alignment)
	{
		m_Alignment = alignment;
		RecountNewPosition();
		MarkAsChanged();
	}
}

void PBTabPanel::SetBackground(int background)
{
	m_Background = background;
	MarkAsChanged();
}

void PBTabPanel::SetTabsCount(int tabscount)
{

	if (tabscount < 1)
	{
		tabscount = 1;
	}

	if (m_TabsCount != tabscount)
	{
		m_TabsCount = tabscount;
		RecountViewTabs();
		MarkAsChanged();
	}
}

void PBTabPanel::SetCurrent(int current)
{

	if (current < 1)
	{
		current = 1;
	}

	if (current > m_TabsCount)
	{
		current = m_TabsCount;
	}

	if (m_TabsCurrent != current)
	{
		m_TabsCurrent = current;
		RecountViewTabs();
		MarkAsChanged();
	}
}

void PBTabPanel::SetFont(ifont *font)
{
	m_pFont = font;
}

int PBTabPanel::GetWidth() const
{
	return PBWindow::GetWidth();
}

int PBTabPanel::GetHeight() const
{
	return PBWindow::GetHeight();
}

int PBTabPanel::GetAlignment() const
{
	return m_Alignment;
}

int PBTabPanel::GetBackground() const
{
	return m_Background;
}

int PBTabPanel::GetTabsCount() const
{
	return m_TabsCount;
}

int PBTabPanel::GetCurrent() const
{
	return m_TabsCurrent;
}

int PBTabPanel::OnDraw(bool force)
{
	if (IsVisible() && (IsChanged() || force))
	{
		PBGraphics *graphics = GetGraphics();

		graphics->FillArea(GetLeft(), GetTop(), GetWidth(), GetHeight(), m_Background);
		switch(m_Alignment)
		{
			case ALIGNMENT_BOTTOM:
				graphics->DrawLine(GetLeft(), GetTop(), GetLeft() + GetWidth() - 1, GetTop(), BLACK);
				graphics->DrawLine(GetLeft(), GetTop() + 1, GetLeft() + GetWidth() - 1, GetTop() + 1, LGRAY);
				break;

			case ALIGNMENT_LEFT:
				graphics->DrawLine(GetLeft() + GetWidth() - 1, GetTop(), GetLeft() + GetWidth() - 1, GetTop() + GetHeight(), BLACK);
				graphics->DrawLine(GetLeft() + GetWidth() - 2, GetTop(), GetLeft() + GetWidth() - 2, GetTop() + GetHeight(), LGRAY);
				break;

			case ALIGNMENT_RIGHT:
				graphics->DrawLine(GetLeft(), GetTop(), GetLeft(), GetTop() + GetHeight(), BLACK);
				graphics->DrawLine(GetLeft() + 1, GetTop(), GetLeft() + 1, GetTop() + GetHeight(), LGRAY);
				break;

			case ALIGNMENT_TOP:
				graphics->DrawLine(GetLeft(), GetTop() + GetHeight() - 1, GetLeft() + GetWidth() - 1, GetTop() + GetHeight() - 1, BLACK);
				graphics->DrawLine(GetLeft(), GetTop() + GetHeight() - 2, GetLeft() + GetWidth() - 1, GetTop() + GetHeight() - 2, LGRAY);
				break;
		}


		int x = m_X, y = m_Y;
		int curx = x, cury = y;
		char tabn[5];
		int i;

		if (m_pFont == NULL)
		{
			m_pFont = graphics->OpenFont(DEFAULTFONTB, 16, 0);
		}
		if (m_pFont != NULL) graphics->SetFont(m_pFont, BLACK);

		if (m_LeftViewTab > 3)
		{
			sprintf(tabn, "1");
			DrawTab(x, y, tabn, false);
			x += m_ShiftX;
			y += m_ShiftY;
			sprintf(tabn, "...");
			DrawTab(x, y, tabn, false);
			x += m_ShiftX;
			y += m_ShiftY;
		}

		for (i = m_LeftViewTab; i <= m_RightViewTab; i++)
		{
			if (m_TabsCurrent == i)
			{
				curx = x;
				cury = y;
			}
			else
			{
				sprintf(tabn, "%d", i);
				DrawTab(x, y, tabn, false);
			}
			x += m_ShiftX;
			y += m_ShiftY;
		}

		if (m_RightViewTab + 1 < m_TabsCount)
		{
			sprintf(tabn, "...");
			DrawTab(x, y, tabn, false);
			x += m_ShiftX;
			y += m_ShiftY;
			sprintf(tabn, "%d", m_TabsCount);
			DrawTab(x, y, tabn, false);
			x += m_ShiftX;
			y += m_ShiftY;
		}

		sprintf(tabn, "%d", m_TabsCurrent);
		DrawTab(curx, cury, tabn, true);
	}

	return 0;
}

void PBTabPanel::UpdateSize()
{
	RecountNewPosition();
	MarkAsChanged();
}

int PBTabPanel::MainHandler(int type, int par1, int par2)
{
    PBPoint clientPoint = m_pParent->ScreenToClient(PBPoint(par1, par2));

    if (type == EVT_POINTERUP)
    {
	int tabclick = GetTabPointer(clientPoint.GetX(), clientPoint.GetY());

	if (tabclick > 0 && tabclick != m_TabsCurrent)
	{
		SetCurrent(tabclick);

		if (m_pParent)
		{
			m_pParent->OnCommand(PBTABPANEL, PBTABCHANGED, (int) this);
		}
		return 1;
	}
    }

    return 0;
}

void PBTabPanel::RecountNewPosition()
{
	if (GetLeft() >= 0 && GetTop() >= 0 && GetWidth() > 0 && GetHeight() > 0)
	{
		MarkAsChanged();
	}

	if (m_pActiveTab != NULL)
	{
		free(m_pActiveTab);
		m_pActiveTab = NULL;
	}
	if (m_pInactiveTab != NULL)
	{
		free(m_pInactiveTab);
		m_pInactiveTab = NULL;
	}

	m_pActiveTab = GetResource("tab_active", NULL);
	m_pInactiveTab = GetResource("tab_inactive", NULL);

	if (m_pActiveTab == NULL || m_pInactiveTab == NULL) {
		m_pActiveTab = NewBitmap(68, 26);
		m_pInactiveTab = NewBitmap(68, 26);
		memset(m_pActiveTab->data, 0xff, m_pActiveTab->height * m_pActiveTab->scanline);
		memset(m_pInactiveTab->data, 0xaa, m_pInactiveTab->height * m_pInactiveTab->scanline);
	}

	switch(m_Alignment)
	{
		case ALIGNMENT_BOTTOM:
			GetGraphics()->RotateBitmap(&m_pActiveTab, ROTATE0);
			GetGraphics()->RotateBitmap(&m_pInactiveTab, ROTATE0);

			SetWindowPos(0, m_pParent->GetHeight() - m_pActiveTab->height, m_pParent->GetWidth(), m_pActiveTab->height);
//			SetLeft(0);
//			SetTop(m_pParent->GetHeight() - m_pActiveTab->height);
//			SetWidth(m_pParent->GetWidth());
//			SetHeight(m_pActiveTab->height);

			m_Length = m_pParent->GetWidth();
			m_X = GetLeft() + HIDESPACE;
			m_Y = GetTop() - 2;
			m_ShiftX = m_pActiveTab->width - HIDESPACE;
			m_ShiftY = 0;
			break;

		case ALIGNMENT_LEFT:
			GetGraphics()->RotateBitmap(&m_pActiveTab, ROTATE90);
			GetGraphics()->RotateBitmap(&m_pInactiveTab, ROTATE90);

			SetWindowPos(0, 0, m_pActiveTab->width, m_pParent->GetHeight());
//			SetLeft(0);
//			SetTop(0);
//			SetWidth(m_pActiveTab->width);
//			SetHeight(m_pParent->GetHeight());

			m_Length = m_pParent->GetHeight();
			m_X = GetLeft() + 2;
			m_Y = GetTop() + HIDESPACE;
			m_ShiftX = 0;
			m_ShiftY = m_pActiveTab->height - HIDESPACE;
			break;

		case ALIGNMENT_RIGHT:
			GetGraphics()->RotateBitmap(&m_pActiveTab, ROTATE270);
			GetGraphics()->RotateBitmap(&m_pInactiveTab, ROTATE270);

			SetWindowPos(m_pParent->GetWidth() - m_pActiveTab->width, 0, m_pActiveTab->width, m_pParent->GetHeight());
//			SetLeft(m_pParent->GetWidth() - m_pActiveTab->width);
//			SetTop(0);
//			SetWidth(m_pActiveTab->width);
//			SetHeight(m_pParent->GetHeight());

			m_Length = m_pParent->GetHeight();
			m_X = GetLeft() - 2;
			m_Y = GetTop() + HIDESPACE;
			m_ShiftX = 0;
			m_ShiftY = m_pActiveTab->height - HIDESPACE;
			break;

		case ALIGNMENT_TOP:
			GetGraphics()->RotateBitmap(&m_pActiveTab, ROTATE180);
			GetGraphics()->RotateBitmap(&m_pInactiveTab, ROTATE180);

			SetWindowPos(0, 0, m_pParent->GetWidth(), m_pActiveTab->height);
//			SetLeft(0);
//			SetTop(0);
//			SetWidth(m_pParent->GetWidth());
//			SetHeight(m_pActiveTab->height);

			m_Length = m_pParent->GetWidth();
			m_X = GetLeft() + HIDESPACE;
			m_Y = GetTop() + 2;
			m_ShiftX = m_pActiveTab->width - HIDESPACE;
			m_ShiftY = 0;
			break;
	}

	RecountViewTabs();

	MarkAsChanged();

}

void PBTabPanel::RecountViewTabs()
{
	m_ViewTabs = (m_Length - 2 * HIDESPACE) / ((m_Alignment == ALIGNMENT_BOTTOM || m_Alignment == ALIGNMENT_TOP ? m_pActiveTab->width : m_pActiveTab->height) - HIDESPACE);

	if (m_ViewTabs >= m_TabsCount)
	{
		m_LeftViewTab = 1;
		m_RightViewTab = m_TabsCount;
	}
	else
	{
		if (m_ViewTabs / 2 >= m_TabsCurrent)
		{
			m_LeftViewTab = 1;
			m_RightViewTab = m_ViewTabs - 2;
		}
		else
		{
			if (m_ViewTabs / 2 + m_TabsCurrent >= m_TabsCount)
			{
				m_LeftViewTab = m_TabsCount - m_ViewTabs + 3;
				m_RightViewTab = m_TabsCount;
			}
			else
			{
				m_LeftViewTab = m_TabsCurrent - m_ViewTabs / 2 + 2 + (m_ViewTabs + 1) % 2;
				m_RightViewTab = m_ViewTabs / 2 + m_TabsCurrent - 2;
			}
		}
	}

	if (m_LeftViewTab < 4)
	{
		m_LeftViewTab = 1;
	}
}

void PBTabPanel::DrawTab(int x, int y, const char *text, bool active)
{
	PBGraphics *graphics = GetGraphics();
	ibitmap *bm;
	if (active)
	{
		bm = m_pActiveTab;
	}
	else
	{
		bm = m_pInactiveTab;
	}

	graphics->DrawBitmap(x, y, bm);
	graphics->DrawTextRect(x, y, m_pActiveTab->width, m_pActiveTab->height, text,
			ALIGN_CENTER | VALIGN_MIDDLE | (m_Alignment == ALIGNMENT_LEFT || m_Alignment == ALIGNMENT_RIGHT ? ROTATE : 0));
	graphics->DrawTextRect(x, y, m_pInactiveTab->width, m_pInactiveTab->height, text,
			ALIGN_CENTER | VALIGN_MIDDLE | (m_Alignment == ALIGNMENT_LEFT || m_Alignment == ALIGNMENT_RIGHT ? ROTATE : 0));

}

bool PBTabPanel::ClickTab(int x, int y, int tabx, int taby)
{
	switch(m_Alignment)
	{
		case ALIGNMENT_BOTTOM:
		case ALIGNMENT_TOP:
			if (tabx < x && tabx + m_ShiftX > x && taby < y && taby + GetHeight() > y)
			{
				return true;
			}
			break;

		case ALIGNMENT_LEFT:
		case ALIGNMENT_RIGHT:
			if (tabx < x && tabx + GetWidth() > x && taby < y && taby + m_ShiftY > y)
			{
				return true;
			}
			break;
	}
	return false;
}

int PBTabPanel::GetTabPointer(int x, int y)
{

	int tabx = m_X, taby = m_Y;
	int i;

	if (m_LeftViewTab > 3)
	{
		if (ClickTab(x, y, tabx, taby))
		{
			return 1;
		}
		tabx += m_ShiftX;
		taby += m_ShiftY;
		if (ClickTab(x, y, tabx, taby))
		{
			return m_LeftViewTab - 1;
		}
		tabx += m_ShiftX;
		taby += m_ShiftY;
	}

	for (i = m_LeftViewTab; i <= m_RightViewTab; i++)
	{
		if (ClickTab(x, y, tabx, taby))
		{
			return i;
		}
		tabx += m_ShiftX;
		taby += m_ShiftY;
	}

	if (m_RightViewTab + 1 < m_TabsCount)
	{
		if (ClickTab(x, y, tabx, taby))
		{
			return m_RightViewTab + 1;
		}
		tabx += m_ShiftX;
		taby += m_ShiftY;
		if (ClickTab(x, y, tabx, taby))
		{
			return m_TabsCount;
		}
	}

	return 0;

}
