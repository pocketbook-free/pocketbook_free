#include "pbwindow.h"
#include "../controls/pbscrollbar.h"
#include "../controls/pbtabpanel.h"

PBRect PBWindow::m_sDirtyRect(0,0,0,0);
PBScreenUpdateType PBWindow::m_sScreenUpdateType = updateNone;
bool PBWindow::m_sRedrawRequired = true;

PBWindow::PBWindow()
	:m_pParent(NULL)
	,m_pGraphics(NULL)
	,m_pFont(NULL)
	,m_Style(-1)
	,m_ExStyle(0)
	,m_WindowRect(0,0,0,0)
	,m_ClientRect(0,0,0,0)
	,m_Background(WHITE)
	,m_BorderStyle(BS_NONE)
	,m_BorderSize(0)
	,m_ShadowSize(0)
	,m_pVScroll(NULL)
	,m_pHScroll(NULL)
	,m_pTabs(NULL)
	,m_pRecordset(NULL)
	,m_RecordsetRow(-1)
	,m_RecordsetCol(-1)
	,m_pFocus(NULL)
	,m_Changed(true)
	,m_Control(true)
{
};

PBWindow::~PBWindow()
{
	delete m_pGraphics;
}

int PBWindow::Create(PBWindow* pParent, int left, int top, int width, int height, int style, int exStyle)
{
	if (m_Style != -1)
		return 0; // already created

	m_pParent = pParent;
	m_WindowRect = PBRect(left, top, width, height);
	m_Style = style;
	m_ExStyle = exStyle;

    m_Background = WHITE;
    m_BorderStyle = BS_NONE;
    m_BorderSize = 0;
    m_ShadowSize = 0;

	// vertical scroll bar
	if ( m_Style & PBWS_VSCROLL)
	{
		m_pVScroll = new PBScrollBar;
		m_pVScroll->Create(this, 0, 100, 10, PB_VERTICAL);
	}

	// horizontal scroll bar
	if ( m_Style & PBWS_HSCROLL)
	{
		m_pHScroll = new PBScrollBar;
		m_pHScroll->Create(this, 0, 100, 10, PB_HORIZONTAL);
	}

	// vertical tab control
	if ( m_Style & PBWS_TABS)
	{
		m_pTabs = new PBTabPanel;
		m_pTabs->Create(this, 1, 1);
	}

	recalcClientRect();

	if (m_pParent)
		m_pParent->AddChild(this);

	return OnCreate();
}

void PBWindow::ChangeWindowStyle(int styleToSet, int styleToClear)
{
	if (styleToSet)
		m_Style |= styleToSet;

	if (styleToClear)
		m_Style &= ~styleToClear;

	MarkAsChanged();
}

void PBWindow::ChangeWindowStyleEx(int styleToSet, int styleToClear)
{
	if (styleToSet)
		m_ExStyle |= styleToSet;

	if (styleToClear)
		m_ExStyle &= ~styleToClear;

	MarkAsChanged();
}

PBGraphics* PBWindow::GetGraphics()
{
	if (m_pGraphics == NULL)
	{
		if (m_pParent)
			m_pGraphics = new PBGraphics(m_pParent);
		else
			m_pGraphics = new PBGraphics(this);
	}
	return m_pGraphics;
}

PBScrollBar *PBWindow::GetHScroll()
{
	return m_pHScroll;
}

PBScrollBar *PBWindow::GetVScroll()
{
	return m_pVScroll;
}

PBTabPanel *PBWindow::GetTabPanel()
{
	return m_pTabs;
}

void PBWindow::SetWindowPos(int left, int top, int width, int height)
{
	m_WindowRect = PBRect(left, top, width, height);
	recalcClientRect();
	OnResize();
}

void PBWindow::SetBackground(int color)
{
	m_Background = color;
}

void PBWindow::SetBorderStyle(int bstyle)
{
    if (bstyle < BS_NONE && bstyle > 2)
    {
        bstyle = BS_NONE;
    }

    m_BorderStyle = bstyle;

	recalcClientRect();
}

void PBWindow::SetBorderSize(int bsize)
{
    if (bsize < 0)
    {
        bsize = 0;
    }

    m_BorderSize = bsize;

	recalcClientRect();
}

void PBWindow::SetShadowSize(int ssize)
{
    if (ssize < 0)
    {
        ssize = 0;
    }

    m_ShadowSize = ssize;

    recalcClientRect();
}

void PBWindow::SetVisible(bool visible)
{
	if (IsVisible() != visible)
	{
		if (visible)
			ChangeWindowStyle(PBWS_VISIBLE, 0);
		else
			ChangeWindowStyle(0, PBWS_VISIBLE);

//		m_Visible = visible;
//		MarkAsChanged();
	}
}

int PBWindow::GetCommandID() const
{
	return m_commandId;
}

void PBWindow::SetCommandID(int commandID)
{
	m_commandId = commandID;
}

const ifont* PBWindow::GetWindowFont()
{
	return m_pFont;
}

void PBWindow::SetWindowFont(const ifont* pFont)
{
	m_pFont = pFont;
}

const PBRect& PBWindow::GetWindowRect() const
{
	return m_WindowRect;
}

int PBWindow::GetLeft() const
{
	return m_WindowRect.GetLeft();
}

int PBWindow::GetTop() const
{
	return m_WindowRect.GetTop();
}

int PBWindow::GetWidth() const
{
	return m_WindowRect.GetWidth();
}

int PBWindow::GetHeight() const
{
	return m_WindowRect.GetHeight();
}

int PBWindow::GetRight() const
{
	return m_WindowRect.GetRight();
}

int PBWindow::GetBottom() const
{
	return m_WindowRect.GetBottom();
}

// возвращает прямоугольник окна в абсолютных координатах (относительно экрана)
PBRect PBWindow::GetWindowScreenRect()
{
	PBPoint offset;
	getClientToScreenOffset(&offset);

	//PBRect rect(offset.GetX(), offset.GetY(), GetWidth(), GetHeight());
	return PBRect(offset.GetX(), offset.GetY(), GetWidth(), GetHeight());
}

void PBWindow::getClientToScreenOffset(PBPoint *pOffset)
{
	PBPoint offset(GetLeft(), GetTop());
	*pOffset += offset;

	if (m_pParent)
		m_pParent->getClientToScreenOffset(pOffset);
}

PBPoint PBWindow::ScreenToClient(const PBPoint &point)
{
	PBPoint offset;
	getClientToScreenOffset(&offset);

	PBPoint client = point;
	client -= offset;
	return client;
}

PBPoint PBWindow::ClientToScreen(const PBPoint &point)
{
	PBPoint offset;
	getClientToScreenOffset(&offset);

	PBPoint screen = point;
	screen += offset;
	return screen;
}

PBRect PBWindow::ClientToScreen(const PBRect &rect)
{
	PBPoint offset;
	getClientToScreenOffset(&offset);

	PBRect screen = rect;
	screen += offset;
	return screen;
}

int PBWindow::GetClientLeft() const
{
	return m_ClientRect.GetLeft();
}

int PBWindow::GetClientTop() const
{
	return m_ClientRect.GetTop();
}

int PBWindow::GetClientWidth() const
{
	return m_ClientRect.GetWidth();
}

int PBWindow::GetClientHeight() const
{
	return m_ClientRect.GetHeight();
}

PBRect PBWindow::GetClientRect()
{
	return m_ClientRect;
}

void PBWindow::GetClientRect(PBRect *pRect)
{
	if (pRect)
		*pRect = m_ClientRect;
}

int PBWindow::GetBackground() const
{
	return m_Background;
}

int PBWindow::GetBorderSize() const
{
	return m_BorderSize;
}

int PBWindow::GetShadowSize() const
{
	return m_ShadowSize;
}

// пересчитывает размеры клиентской области в зависимости от: наличия рамки, скрола, заголовка, тени...
void PBWindow::recalcClientRect()
{
	m_ClientRect = m_WindowRect;

	m_ClientRect.SetX(m_ClientRect.GetX() + m_BorderSize);
	m_ClientRect.SetY(m_ClientRect.GetY() + m_BorderSize);
	m_ClientRect.SetWidth(m_ClientRect.GetWidth() - 2 * m_BorderSize - m_ShadowSize);
	m_ClientRect.SetHeight(m_ClientRect.GetHeight() - 2 * m_BorderSize - m_ShadowSize);

	PBScrollBar *scroll = GetVScroll();
	if (scroll && scroll->IsVisible())
	{
		m_ClientRect.SetWidth(m_ClientRect.GetWidth() - scroll->GetWidth());
	}

	scroll = GetHScroll();
	if (scroll && scroll->IsVisible())
	{
		m_ClientRect.SetHeight(m_ClientRect.GetHeight() - scroll->GetHeight());
	}

	PBTabPanel *tabs = GetTabPanel();

	if (tabs)
	{
		switch(tabs->GetAlignment())
		{
			case ALIGNMENT_BOTTOM:
				m_ClientRect.SetHeight(m_ClientRect.GetHeight() - tabs->GetHeight());
				break;

			case ALIGNMENT_LEFT:
				m_ClientRect.SetX(m_ClientRect.GetLeft() + tabs->GetWidth());
				m_ClientRect.SetWidth(m_ClientRect.GetWidth() - tabs->GetWidth());
				break;

			case ALIGNMENT_RIGHT:
				m_ClientRect.SetWidth(m_ClientRect.GetWidth() - tabs->GetWidth());
				break;

			case ALIGNMENT_TOP:
				m_ClientRect.SetY(m_ClientRect.GetTop() + tabs->GetHeight());
				m_ClientRect.SetHeight(m_ClientRect.GetHeight() - tabs->GetHeight());
				break;
		}
	}

	/*switch (m_BorderStyle)
    {
        case BS_NONE:
            if (client)
            {
                m_ClientLeft = m_Left;
                m_ClientTop = m_Top;
                m_ClientWidth = m_Width;
                m_ClientHeight = m_Height;
            }
            else
            {
                m_Left = m_ClientLeft;
                m_Top = m_ClientTop;
                m_Width = m_ClientWidth;
                m_Height = m_ClientHeight;
            }
            break;

        case BS_RECT:
            if (client)
            {
                m_ClientLeft = m_Left + m_BorderSize;
                m_ClientTop = m_Top + m_BorderSize;
                m_ClientWidth = m_Width - 2 * m_BorderSize;
                m_ClientHeight = m_Height - 2 * m_BorderSize;
            }
            else
            {
                m_Left = m_ClientLeft - m_BorderSize;
                m_Top = m_ClientTop - m_BorderSize;
                m_Width = m_ClientWidth + 2 * m_BorderSize;
                m_Height = m_ClientHeight + 2 * m_BorderSize;
            }
            break;

        case BS_SHADOW:
            if (client)
            {
                m_ClientLeft = m_Left + m_BorderSize;
                m_ClientTop = m_Top + m_BorderSize;
                m_ClientWidth = m_Width - 2 * m_BorderSize - m_ShadowSize;
                m_ClientHeight = m_Height - 2 * m_BorderSize - m_ShadowSize;
            }
            else
            {
                m_Left = m_ClientLeft - m_BorderSize;
                m_Top = m_ClientTop - m_BorderSize;
                m_Width = m_ClientWidth + 2 * m_BorderSize + m_ShadowSize;
                m_Height = m_ClientHeight + 2 * m_BorderSize + m_ShadowSize;
            }
            break;
    }

    if (m_ClientWidth < 0)
    {
        m_ClientWidth = 0;
    }

    if (m_ClientHeight < 0)
    {
        m_ClientHeight = 0;
    }
*/
}

bool PBWindow::IsVisible() const
{
	bool isParentVisible = true;
	if (m_pParent)
		isParentVisible = m_pParent->IsVisible();
	return (m_Style & PBWS_VISIBLE) && isParentVisible;
}

bool PBWindow::IsChanged() const
{
	return m_Changed;
}

bool PBWindow::IsFocusChanged() const
{
	return m_ChangedSelection;
}

bool PBWindow::IsFocused() const
{
	if (m_pParent)
	{
		if (m_pParent->m_pFocus != this)
			return false;
	}
	return true;
}

bool PBWindow::IsTransparent() const
{
	return m_Style & PBWS_TRANSPARENT;
}

bool PBWindow::IsTabstop() const
{
	return IsVisible() && (m_Style & PBWS_TABSTOP);
}

void PBWindow::SetSelect(bool select)
{
	if (((m_Style & PBWS_SELECT) > 0) != select)
	{
		if (select)
		{
			m_Style |= PBWS_SELECT;
		}
		else
		{
			m_Style &= (~PBWS_SELECT);
		}
		m_ChangedSelection = true;
	}
}

bool PBWindow::IsSelect() const
{
	return (m_Style & PBWS_SELECT);
}

bool PBWindow::ChangedSelection() const
{
	return (m_ChangedSelection || m_Changed);
}

void PBWindow::Control(bool control)
{
	m_Control = control;
}

bool PBWindow::IsControl() const
{
	return m_Control;
}

void PBWindow::RemoveFocus()
{
	if (m_pFocus)
	{
		m_pFocus->RemoveFocus();
		m_pFocus = NULL;
	}
	MarkAsFocusChanged();
}

void PBWindow::SetFocus()
{
	MarkAsFocusChanged();
}

int PBWindow::MainHandler(int type, int par1, int par2)
{
	int res = 0;

	std::vector<PBWindow*>::const_iterator it = m_Children.begin();
	while (it != m_Children.end() && !(res = (*it)->MainHandler(type, par1, par2)))
		it++;

	return res;
}

//virtual
int PBWindow::OnCommand(int commandId, int par1, int par2)
{
    if (m_pParent)
        m_pParent->OnCommand(commandId, par1, par2);
    return 0;
}


void PBWindow::AddChild(PBWindow* pChild)
{
    m_Children.push_back(pChild);
	//pChild->SetPosition(m_Children.size());
}

int PBWindow::OnChildFocusChanged(PBWindow* pFocused)
{
	if (m_pFocus != NULL)
	{
		m_pFocus->RemoveFocus();
	}
	else
	{
		if (m_pParent)
		{
			m_pParent->OnChildFocusChanged(this);
		}
	}

	m_pFocus = pFocused;
	m_pFocus->SetFocus();
//    if (m_pParent)
//	m_pParent->OnChildFocusChanged(this);
    return 0;
}

void PBWindow::OnMenu(int /*index*/)
{
	// nothing to do
}

void PBWindow::OpenMenu()
{
	// nothing to do
}
//
//int PBWindow::SetPosition(int position)
//{
//    m_position = position;
//    return 0;
//}

void PBWindow::Invalidate(PBRect screenRect, PBScreenUpdateType updateType)
{
//	fprintf(stderr, "Invalidate. ScreenRect: x=%d y=%d w=%d h=%d\n", screenRect.GetX(), screenRect.GetY(), screenRect.GetWidth(), screenRect.GetHeight());

	if (m_sDirtyRect.GetHeight() > 0)
		m_sDirtyRect += screenRect;
	else
		m_sDirtyRect = screenRect;

	if (m_sScreenUpdateType < updateType)
		m_sScreenUpdateType = updateType;

//	fprintf(stderr, "Invalidate. Result: x=%d y=%d w=%d h=%d\n", m_sDirtyRect.GetX(), m_sDirtyRect.GetY(), m_sDirtyRect.GetWidth(), m_sDirtyRect.GetHeight());
}

void PBWindow::DiscardInvalidation()
{
	// reset dirty rect
	m_sDirtyRect = PBRect(0,0,0,0);
	m_sScreenUpdateType = updateNone;
	m_sRedrawRequired = false;
}

// static признак - требуется обновление экрана или нет
bool PBWindow::IsDirty()
{
	return m_sDirtyRect.GetHeight() > 0 || m_sRedrawRequired;
}

int PBWindow::Draw()
{
	// draw ourserf. Force drawing if parent has been changed.
	bool force = false;
	if (m_pParent && m_pParent->IsChanged())
		force = true;
	OnDraw(force);
	if (force)
		m_Changed = true;

	// draw children
	for (TWindowListIt it = m_Children.begin(); it != m_Children.end(); it++)
		(*it)->Draw();

	OnPostDraw(force);
	// reset changed state
	m_Changed = false;
	m_ChangedSelection = false;
	return 0;
}

void PBWindow::OnResize()
{
}

//TODO move this method to MainFrame
void PBWindow::ReDraw()
{
	// draw all windows
	Draw();

	// do screen update
	if (m_sScreenUpdateType == updateRectBW)
	{
		PartialUpdateBW(m_sDirtyRect.GetX(), m_sDirtyRect.GetY(), m_sDirtyRect.GetWidth(), m_sDirtyRect.GetHeight());
		//fprintf(stderr, "PartialUpdate (BW)\n");
	}
	else if (m_sScreenUpdateType == updateRect)
	{
		PartialUpdate(m_sDirtyRect.GetX(), m_sDirtyRect.GetY(), m_sDirtyRect.GetWidth(), m_sDirtyRect.GetHeight());
		//fprintf(stderr, "PartialUpdate (gray)\n");
	}
	else if (m_sScreenUpdateType == updateFullScreen)
	{
		FullUpdate();
		//fprintf(stderr, "FullUpdate\n");
	}

//	// temp rect for debug
//	PBRect rect = m_sDirtyRect;

	// reset dirty rect
	DiscardInvalidation();

//	// for debug
//	InvertAreaBW(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
//	PartialUpdateBW(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
//	InvertAreaBW(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
//	PartialUpdateBW(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());

}

void PBWindow::Bind(PBRecordset* pRecordset, int row, int col)
{
	m_pRecordset = pRecordset;
	m_RecordsetRow = row;
	m_RecordsetCol = col;
}

PBWindow *PBWindow::GetParent()
{
	return m_pParent;
}

void PBWindow::Run()
{
	// virtual
}

// mark this window as chandged (need redraw). Also marks static flag m_sRedrawRequired
void PBWindow::MarkAsChanged()
{
	m_Changed = true;
	m_sRedrawRequired = true;
}

void PBWindow::MarkAsFocusChanged()
{
	m_ChangedSelection = true;
	m_sRedrawRequired = true;
}

int PBWindow::Distance(PBPoint p0, PBPoint p1, PBPoint p2)
{
	PBPoint v(p2.GetX() - p1.GetX(), p2.GetY() - p1.GetY());
	PBPoint w(p0.GetX() - p1.GetX(), p0.GetY() - p1.GetY());
	PBPoint u(p0.GetX() - p2.GetX(), p0.GetY() - p2.GetY());

	int c1, c2;
	if ((c1 = v.GetX() * w.GetX() + v.GetY() * w.GetY()) <= 0)
	{
		return w.GetX() * w.GetX() + w.GetY() * w.GetY();
	}
	if ((c2 = v.GetX() * v.GetX() + v.GetY() * v.GetY()) <= c1)
	{
		return u.GetX() * u.GetX() + u.GetY() * u.GetY();
	}
	PBPoint t(p0.GetX() - p1.GetX() - v.GetX() * c1 / c2, p0.GetY() - p1.GetY() - v.GetY() * c1 / c2);
	return t.GetX() * t.GetX() + t.GetY() * t.GetY();
}

//PBWindow *PBWindow::MoveFocus(int par1, PBWindow *focus, bool round)
//{
//	int d;
//	int min = -1;
//	int enterX = 0, enterY = 0;
//	PBRect AbsoluteFocusRect = focus->GetParent()->ClientToScreen(PBRect(focus->GetLeft(), focus->GetTop(), focus->GetWidth(), focus->GetHeight()));
//	PBWindow *item = NULL;
//	switch(par1)
//	{
//		case KEY_UP:
//			enterX = AbsoluteFocusRect.GetLeft();
//			enterY = AbsoluteFocusRect.GetTop();
//			m_itSearchChild = m_Children.begin();
//			while (m_itSearchChild != m_Children.end())
//			{
//				if ((*m_itSearchChild) != m_pFocus && (*m_itSearchChild)->IsVisible() && (*m_itSearchChild)->IsControl() && (*m_itSearchChild)->IsControl() && (*m_itSearchChild)->IsControl())
//				{
//					PBRect AbsoluteSearchRect = (*m_itSearchChild)->GetParent()->ClientToScreen(PBRect((*m_itSearchChild)->GetLeft(), (*m_itSearchChild)->GetTop(), (*m_itSearchChild)->GetWidth(), (*m_itSearchChild)->GetHeight()));
//					if (AbsoluteSearchRect.GetBottom() <= enterY &&
//						(
//							(AbsoluteSearchRect.GetLeft() >= AbsoluteFocusRect.GetLeft() &&
//							AbsoluteSearchRect.GetLeft() < AbsoluteFocusRect.GetRight())
//							||
//							(AbsoluteSearchRect.GetLeft() <= AbsoluteFocusRect.GetLeft() &&
//							AbsoluteSearchRect.GetRight() > AbsoluteFocusRect.GetLeft())
//						))
//					{
//						d = Distance(PBPoint(enterX, enterY),
//									PBPoint(AbsoluteSearchRect.GetLeft(), AbsoluteSearchRect.GetBottom()),
//									PBPoint(AbsoluteSearchRect.GetRight(), AbsoluteSearchRect.GetBottom()));
//						if (d < min || min == -1)
//						{
//							min = d;
//							item = (*m_itSearchChild);
//						}
//					}
//				}
//				++m_itSearchChild;
//			}
//			if (item == NULL && round)
//			{
//				enterY = ScreenHeight();
//				m_itSearchChild = m_Children.begin();
//				while (m_itSearchChild != m_Children.end())
//				{
//					if ((*m_itSearchChild) != m_pFocus && (*m_itSearchChild)->IsVisible() && (*m_itSearchChild)->IsControl())
//					{
//						PBRect AbsoluteSearchRect = (*m_itSearchChild)->GetParent()->ClientToScreen(PBRect((*m_itSearchChild)->GetLeft(), (*m_itSearchChild)->GetTop(), (*m_itSearchChild)->GetWidth(), (*m_itSearchChild)->GetHeight()));
//						if (AbsoluteSearchRect.GetBottom() <= enterY &&
//							(
//								(AbsoluteSearchRect.GetLeft() >= AbsoluteFocusRect.GetLeft() &&
//								AbsoluteSearchRect.GetLeft() < AbsoluteFocusRect.GetRight())
//								||
//								(AbsoluteSearchRect.GetLeft() <= AbsoluteFocusRect.GetLeft() &&
//								AbsoluteSearchRect.GetRight() > AbsoluteFocusRect.GetLeft())
//							))
//						{
//							d = Distance(PBPoint(enterX, enterY),
//										PBPoint(AbsoluteSearchRect.GetLeft(), AbsoluteSearchRect.GetBottom()),
//										PBPoint(AbsoluteSearchRect.GetRight(), AbsoluteSearchRect.GetBottom()));
//							if (d < min || min == -1)
//							{
//								min = d;
//								item = (*m_itSearchChild);
//							}
//						}
//					}
//					m_itSearchChild++;
//				}
//			}
//			if (item == NULL && round)
//			{
//				item = m_pFocus;
//			}
//			break;
//
//		case KEY_DOWN:
//			enterX = AbsoluteFocusRect.GetLeft();
//			enterY = AbsoluteFocusRect.GetBottom();
//			m_itSearchChild = m_Children.begin();
//			while (m_itSearchChild != m_Children.end())
//			{
//				if ((*m_itSearchChild) != m_pFocus && (*m_itSearchChild)->IsVisible() && (*m_itSearchChild)->IsControl())
//				{
//					PBRect AbsoluteSearchRect = (*m_itSearchChild)->GetParent()->ClientToScreen(PBRect((*m_itSearchChild)->GetLeft(), (*m_itSearchChild)->GetTop(), (*m_itSearchChild)->GetWidth(), (*m_itSearchChild)->GetHeight()));
//					if (AbsoluteSearchRect.GetTop() >= enterY &&
//						(
//							(AbsoluteSearchRect.GetLeft() >= AbsoluteFocusRect.GetLeft() &&
//							AbsoluteSearchRect.GetLeft() < AbsoluteFocusRect.GetRight())
//							||
//							(AbsoluteSearchRect.GetLeft() <= AbsoluteFocusRect.GetLeft() &&
//							AbsoluteSearchRect.GetRight() > AbsoluteFocusRect.GetLeft())
//						))
//					{
//						d = Distance(PBPoint(enterX, enterY),
//									PBPoint(AbsoluteSearchRect.GetLeft(), AbsoluteSearchRect.GetTop()),
//									PBPoint(AbsoluteSearchRect.GetRight(), AbsoluteSearchRect.GetTop()));
//						if (d < min || min == -1)
//						{
//							min = d;
//							item = (*m_itSearchChild);
//						}
//					}
//				}
//				m_itSearchChild++;
//			}
//			if (item == NULL && round)
//			{
//				enterY = 0;
//				m_itSearchChild = m_Children.begin();
//				while (m_itSearchChild != m_Children.end())
//				{
//					if ((*m_itSearchChild) != m_pFocus && (*m_itSearchChild)->IsVisible() && (*m_itSearchChild)->IsControl())
//					{
//						PBRect AbsoluteSearchRect = (*m_itSearchChild)->GetParent()->ClientToScreen(PBRect((*m_itSearchChild)->GetLeft(), (*m_itSearchChild)->GetTop(), (*m_itSearchChild)->GetWidth(), (*m_itSearchChild)->GetHeight()));
//						if (AbsoluteSearchRect.GetTop() >= enterY &&
//							(
//								(AbsoluteSearchRect.GetLeft() >= AbsoluteFocusRect.GetLeft() &&
//								AbsoluteSearchRect.GetLeft() < AbsoluteFocusRect.GetRight())
//								||
//								(AbsoluteSearchRect.GetLeft() <= AbsoluteFocusRect.GetLeft() &&
//								AbsoluteSearchRect.GetRight() > AbsoluteFocusRect.GetLeft())
//							))
//						{
//							d = Distance(PBPoint(enterX, enterY),
//										PBPoint(AbsoluteSearchRect.GetLeft(), AbsoluteSearchRect.GetTop()),
//										PBPoint(AbsoluteSearchRect.GetRight(), AbsoluteSearchRect.GetTop()));
//							if (d < min || min == -1)
//							{
//								min = d;
//								item = (*m_itSearchChild);
//							}
//						}
//					}
//					m_itSearchChild++;
//				}
//			}
//			if (item == NULL && round)
//			{
//				item = m_pFocus;
//			}
//			break;
//
//		case KEY_LEFT:
//		case KEY_PREV:
//			enterX = AbsoluteFocusRect.GetLeft();
//			enterY = AbsoluteFocusRect.GetTop();
//			m_itSearchChild = m_Children.begin();
//			while (m_itSearchChild != m_Children.end())
//			{
//				if ((*m_itSearchChild) != m_pFocus && (*m_itSearchChild)->IsVisible() && (*m_itSearchChild)->IsControl())
//				{
//					PBRect AbsoluteSearchRect = (*m_itSearchChild)->GetParent()->ClientToScreen(PBRect((*m_itSearchChild)->GetLeft(), (*m_itSearchChild)->GetTop(), (*m_itSearchChild)->GetWidth(), (*m_itSearchChild)->GetHeight()));
//					if (AbsoluteSearchRect.GetLeft() <= enterX &&
//						(
//							(AbsoluteSearchRect.GetTop() >= AbsoluteFocusRect.GetTop() &&
//							AbsoluteSearchRect.GetTop() < AbsoluteFocusRect.GetBottom())
//							||
//							(AbsoluteSearchRect.GetTop() <= AbsoluteFocusRect.GetTop() &&
//							AbsoluteSearchRect.GetBottom() > AbsoluteFocusRect.GetTop())
//						))
//					{
//						d = Distance(PBPoint(enterX, enterY),
//									PBPoint(AbsoluteSearchRect.GetRight(), AbsoluteSearchRect.GetTop()),
//									PBPoint(AbsoluteSearchRect.GetRight(), AbsoluteSearchRect.GetBottom()));
//						if (d < min || min == -1)
//						{
//							min = d;
//							item = (*m_itSearchChild);
//						}
//					}
//				}
//				++m_itSearchChild;
//			}
//			if (item == NULL && round)
//			{
//				enterX = ScreenWidth();
//				m_itSearchChild = m_Children.begin();
//				while (m_itSearchChild != m_Children.end())
//				{
//					if ((*m_itSearchChild) != m_pFocus && (*m_itSearchChild)->IsVisible() && (*m_itSearchChild)->IsControl())
//					{
//						PBRect AbsoluteSearchRect = (*m_itSearchChild)->GetParent()->ClientToScreen(PBRect((*m_itSearchChild)->GetLeft(), (*m_itSearchChild)->GetTop(), (*m_itSearchChild)->GetWidth(), (*m_itSearchChild)->GetHeight()));
//						if (AbsoluteSearchRect.GetLeft() <= enterX &&
//							(
//								(AbsoluteSearchRect.GetTop() >= AbsoluteFocusRect.GetTop() &&
//								AbsoluteSearchRect.GetTop() < AbsoluteFocusRect.GetBottom())
//								||
//								(AbsoluteSearchRect.GetTop() <= AbsoluteFocusRect.GetTop() &&
//								AbsoluteSearchRect.GetBottom() > AbsoluteFocusRect.GetTop())
//							))
//						{
//							d = Distance(PBPoint(enterX, enterY),
//										PBPoint(AbsoluteSearchRect.GetRight(), AbsoluteSearchRect.GetTop()),
//										PBPoint(AbsoluteSearchRect.GetRight(), AbsoluteSearchRect.GetBottom()));
//							if (d < min || min == -1)
//							{
//								min = d;
//								item = (*m_itSearchChild);
//							}
//						}
//					}
//					m_itSearchChild++;
//				}
//			}
//			if (item == NULL && round)
//			{
//				item = m_pFocus;
//			}
//			break;
//
//		case KEY_RIGHT:
//		case KEY_NEXT:
//			enterX = AbsoluteFocusRect.GetRight();
//			enterY = AbsoluteFocusRect.GetTop();
//			m_itSearchChild = m_Children.begin();
//			while (m_itSearchChild != m_Children.end())
//			{
//				if ((*m_itSearchChild) != m_pFocus && (*m_itSearchChild)->IsVisible() && (*m_itSearchChild)->IsControl())
//				{
//					PBRect AbsoluteSearchRect = (*m_itSearchChild)->GetParent()->ClientToScreen(PBRect((*m_itSearchChild)->GetLeft(), (*m_itSearchChild)->GetTop(), (*m_itSearchChild)->GetWidth(), (*m_itSearchChild)->GetHeight()));
//					if (AbsoluteSearchRect.GetLeft() >= enterX &&
//						(
//							(AbsoluteSearchRect.GetTop() >= AbsoluteFocusRect.GetTop() &&
//							AbsoluteSearchRect.GetTop() < AbsoluteFocusRect.GetBottom())
//							||
//							(AbsoluteSearchRect.GetTop() <= AbsoluteFocusRect.GetTop() &&
//							AbsoluteSearchRect.GetBottom() > AbsoluteFocusRect.GetTop())
//						))
//					{
//						d = Distance(PBPoint(enterX, enterY),
//									PBPoint(AbsoluteSearchRect.GetLeft(), AbsoluteSearchRect.GetTop()),
//									PBPoint(AbsoluteSearchRect.GetLeft(), AbsoluteSearchRect.GetBottom()));
//						if (d < min || min == -1)
//						{
//							min = d;
//							item = (*m_itSearchChild);
//						}
//					}
//				}
//				++m_itSearchChild;
//			}
//			if (item == NULL && round)
//			{
//				enterX = 0;
//				m_itSearchChild = m_Children.begin();
//				while (m_itSearchChild != m_Children.end())
//				{
//					if ((*m_itSearchChild) != m_pFocus && (*m_itSearchChild)->IsVisible() && (*m_itSearchChild)->IsControl())
//					{
//						PBRect AbsoluteSearchRect = (*m_itSearchChild)->GetParent()->ClientToScreen(PBRect((*m_itSearchChild)->GetLeft(), (*m_itSearchChild)->GetTop(), (*m_itSearchChild)->GetWidth(), (*m_itSearchChild)->GetHeight()));
//						if (AbsoluteSearchRect.GetLeft() >= enterX &&
//							(
//								(AbsoluteSearchRect.GetTop() >= AbsoluteFocusRect.GetTop() &&
//								AbsoluteSearchRect.GetTop() < AbsoluteFocusRect.GetBottom())
//								||
//								(AbsoluteSearchRect.GetTop() <= AbsoluteFocusRect.GetTop() &&
//								AbsoluteSearchRect.GetBottom() > AbsoluteFocusRect.GetTop())
//							))
//						{
//							d = Distance(PBPoint(enterX, enterY),
//										PBPoint(AbsoluteSearchRect.GetLeft(), AbsoluteSearchRect.GetTop()),
//										PBPoint(AbsoluteSearchRect.GetLeft(), AbsoluteSearchRect.GetBottom()));
//							if (d < min || min == -1)
//							{
//								min = d;
//								item = (*m_itSearchChild);
//							}
//						}
//					}
//					m_itSearchChild++;
//				}
//			}
//			if (item == NULL && round)
//			{
//				item = m_pFocus;
//			}
//			break;
//
//	}
//
//	if (item != NULL)
//	{
//		item->OnCommand(PBENTERFOCUS, enterX, enterY);
//	}
//	return item;
//}
