#include "pbcommonbox.h"

PBCommonBox::PBCommonBox():PBPageBox()
{
}

PBCommonBox::~PBCommonBox()
{
	int i;
	for (i = 0; i < m_ItemsPerPage; i++)
	{
		delete m_pBaseItem[i];
		m_pBaseItem[i] = NULL;
	}
}

int PBCommonBox::OnCreate()
{
	SetBorderSize(1);
	m_CreatedItems = 0;
	m_FirstItem = 0;
	m_LastItem = 0;
	m_ItemCount = 0;
	m_ItemsPerPage = 0;
	m_ViewMode = PBVIEWMODE_LIST;
	CreateItems();
	m_CreatedItems = m_ItemsPerPage;
	m_SelectItem = -1;
	m_RowCount = -1;
	m_ColCount = -1;
	m_FreeSpaceV = 0;
	m_FreeSpaceH = 0;
	m_CircleVertical = false;
	return 0;
}

int PBCommonBox::MainHandler(int type, int par1, int par2)
{
	if (IsVisible())
	{
		if (GetVScroll() != NULL)
			return _ScrollMainHandler(type, par1, par2);
		if (GetTabPanel() != NULL)
			return _PageMainHandler(type, par1, par2);
	}

	return 0;
}

int PBCommonBox::OnCommand(int commandId, int par1, int par2)
{
	int i;
	PBWindow *item;
	PBPoint enter;
	PBScrollBar *scrollBar = static_cast<PBScrollBar *> ((void *) par2);
	if (m_ColCount == -1)
	{
		m_ColCount = 1;
	}

	switch(commandId)
	{
		case PBGRIDCONTAINER:
			item = (PBWindow *)par2;
			i = 0;
			while (i < m_LastItem - m_FirstItem)
			{
				if (m_pBaseItem[i]->IsSelect())
				{
					m_pBaseItem[i]->SetSelect(false);
				}
				if (m_pBaseItem[i] == item)
				{
					m_SelectItem = i;
					item->SetSelect(true);
				}
				i++;
			}
			if (m_pParent)
				m_pParent->OnCommand(m_CommandId, par1, par2);
			UpdateItems();
			break;

		case PBSCROLLBAR:
			if (m_FirstItem != scrollBar->value())
			{
				m_FirstItem = scrollBar->value();
				m_LastItem = m_FirstItem + m_ItemsPerPage;
				UpdateItems();
//				MarkAsChanged();
//				if (m_pParent)
//					m_pParent->OnCommand(commandId, par1, par2);
			}
			break;


		case PBENTERFOCUS:
			if (m_ItemCount > 0)
			{
				enter = ScreenToClient(PBPoint(par1, par2));
				i = 0;
				while (i < m_LastItem - m_FirstItem - 1 && m_pBaseItem[i]->IsVisible() && enter.GetY() >= m_pBaseItem[i]->GetBottom())
				{
					i++;
				}
				while (i < m_LastItem - m_FirstItem - 1 && m_pBaseItem[i]->IsVisible() && enter.GetX() >= m_pBaseItem[i]->GetRight() && (i % m_ColCount < m_ColCount - 1))
				{
					i++;
				}
				if (m_pBaseItem[i]->IsVisible())
				{
					m_SelectItem = i;
					m_pBaseItem[m_SelectItem]->SetSelect(true);
					m_pFocus = m_pBaseItem[m_SelectItem];
				}
				else
				{
					m_SelectItem = i - 1;
					m_pBaseItem[m_SelectItem]->SetSelect(true);
					m_pFocus = m_pBaseItem[m_SelectItem];
				}
			}
			else
			{
				m_SelectItem = 0;
				m_pFocus = m_pBaseItem[0];
			}
			break;

		case PBCHANGEVIEWMODE:
			ChangeViewMode(par1, true);
			break;

		default:
			PBPageBox::OnCommand(commandId, par1, par2);
//			if (m_pParent)
//				m_pParent->OnCommand(m_CommandId, 0, 0);
			break;
	}
	return 0;
}

int PBCommonBox::OnDraw(bool force)
{
	if (IsVisible() && (IsChanged() || force))
	{
		PBGraphics *graphics = GetGraphics();
		graphics->FillArea(GetLeft(), GetTop(), GetWidth(), GetHeight(), WHITE);
		graphics->DrawRect(GetLeft(), GetTop(), GetWidth(), GetHeight(), BLACK);
	}
	return 0;
}

void PBCommonBox::OnResize()
{
	if (GetVScroll() != NULL)
		return _ScrollOnResize();
	if (GetTabPanel() != NULL)
		return _PageOnResize();
}

void PBCommonBox::Refresh(int refresh)
{
	PBTabPanel *tabs = GetTabPanel();
	PBScrollBar *scroll = GetVScroll();
	int i;

	switch(refresh)
	{
		// Refresh for any mode
		case REFRESH_SELECT_FIRST:
			m_SelectItem = 0;
			if (m_ItemCount > 0)
			{
				i = 0;
				while (i < m_ItemsPerPage && !m_pBaseItem[i]->IsSelect())
				{
					i++;
				}
				if (i < m_ItemsPerPage)
				{
					m_pBaseItem[i]->SetSelect(false);
				}
			}
			m_pBaseItem[m_SelectItem]->SetSelect(true);
			m_pFocus = m_pBaseItem[m_SelectItem];
			break;

		case REFRESH_DISELECT_ITEM:
			m_SelectItem = -1;
			if (m_ItemCount > 0)
			{
				i = 0;
				while (i < m_ItemsPerPage)
				{
					m_pBaseItem[i]->SetSelect(false);
					i++;
				}
			}
			m_pFocus = NULL;
			break;

		// refresh for current mode
		default:
			if (scroll != NULL)
				_ScrollRefresh(refresh);
			if (tabs != NULL)
				_PageRefresh(refresh);
			break;

	}
}

void PBCommonBox::SetCommandId(int commandId)
{
	m_CommandId = commandId;
}

void PBCommonBox::UpdateScrollBar()
{
	PBScrollBar *scrollBar = GetVScroll();
	if (scrollBar)
	{
		scrollBar->setMinimum(0);
		scrollBar->setMaximum(m_ItemCount - m_ItemsPerPage);
		scrollBar->setPageStep(m_ItemsPerPage);
		scrollBar->setValue(m_FirstItem);
	}
	recalcClientRect();
}

void PBCommonBox::SetCircleVertical(bool value)
{
	m_CircleVertical = value;
}

void PBCommonBox::UpdateItems()
{
	int i;

	if (m_LastItem > m_ItemCount)
	{
		for (i = m_ItemCount - m_FirstItem; i < m_ItemsPerPage; i++)
		{
			if (m_pBaseItem[i]->IsVisible() == true)
			{
				//m_pBaseItem[i]->SetVisible(false);
				m_pBaseItem[i]->ChangeWindowStyle(0, PBWS_VISIBLE);
			}
		}
		m_LastItem = m_ItemCount;
	}

/*
	if (m_LastItem < m_FirstItem + m_ItemsPerPage && m_FirstItem + m_ItemsPerPage <= m_ItemCount)
	{
		m_LastItem = m_FirstItem + m_ItemsPerPage;
		for (i = m_ItemCount - m_FirstItem; i < m_LastItem - m_FirstItem; i++)
		{
			if (m_pBaseItem[i]->IsVisible() == false)
			{
				m_pBaseItem[i]->SetVisible(true);
			}
		}
	}
*/
	for (i = m_FirstItem; i < m_LastItem; i++)
	{
		if (m_pBaseItem[i - m_FirstItem]->IsVisible() == false)
		{
			//m_pBaseItem[i - m_FirstItem]->SetVisible(true);
			m_pBaseItem[i - m_FirstItem]->ChangeWindowStyle(PBWS_VISIBLE, 0);
		}
		m_pBaseItem[i - m_FirstItem]->Bind(m_pRecordset, i, -1);
	}

	for (i = m_LastItem - m_FirstItem; i < m_CreatedItems; i++)
	{
		if (m_pBaseItem[i]->IsVisible() == true)
		{
			m_pBaseItem[i]->ChangeWindowStyle(0, PBWS_VISIBLE);
		}
	}

	if (m_ItemCount == 0)
	{
		Control(false);
	}
	else
	{
		Control(true);
	}
}

void PBCommonBox::ChangeViewMode(int viewmode, bool redraw)
{
	if (m_ViewMode != viewmode)
	{
		m_ViewMode = viewmode;
		if (m_pRecordset != NULL)
		{
			char tmp[5];
			sprintf(tmp, "%d", m_ViewMode);
			m_pRecordset->SetParam("viewmode", tmp);
		}
		OnResizeItems(true);
		CreateItems();
		Refresh(REFRESH_CHANGE_ITEMS_PER_PAGE);
	}
	if (m_pParent != NULL && redraw)
		m_pParent->OnCommand(PBREDRAW, 0, 0);
}

int PBCommonBox::GetViewMode() const
{
	return m_ViewMode;
}

int PBCommonBox::_PageMainHandler(int type, int par1, int par2)
{
	int step = 0;
	PBWindow *item;

	if (m_ColCount == -1)
	{
		m_ColCount = 1;
	}

	if (IsVisible() && (type == EVT_KEYRELEASE || type == EVT_KEYREPEAT))
	{
		switch(par1)
		{
			case KEY_DOWN:
				if (!IsControl())
				{
						m_SelectItem = -1;
						m_pFocus = NULL;
						m_pParent->OnCommand(PBEXITFOCUS, par1, (int)this);
				}
				else
				{
					if (m_SelectItem != -1 && m_SelectItem + m_ColCount - 1 < m_ItemsPerPage - 1 &&
							m_SelectItem + m_ColCount - 1 < m_LastItem - m_FirstItem - 1)
					{
						m_pBaseItem[m_SelectItem]->SetSelect(false);
						m_SelectItem += m_ColCount;
						m_pBaseItem[m_SelectItem]->SetSelect(true);
						m_pFocus = m_pBaseItem[m_SelectItem];
					}
					else
					{
						if (m_CircleVertical)
						{
							m_pBaseItem[m_SelectItem]->SetSelect(false);
							m_SelectItem %= m_ColCount;
							m_pBaseItem[m_SelectItem]->SetSelect(true);
							m_pFocus = m_pBaseItem[m_SelectItem];
						}
						else
						{
							m_pBaseItem[m_SelectItem]->SetSelect(false);
							item = m_pBaseItem[m_SelectItem];
							m_SelectItem = -1;
							m_pFocus = NULL;
							m_pParent->OnCommand(PBEXITFOCUS, par1, (int)item);
						}
					}
				}
				break;

			case KEY_UP:
				if (!IsControl())
				{
						m_SelectItem = -1;
						m_pFocus = NULL;
						m_pParent->OnCommand(PBEXITFOCUS, par1, (int)this);
				}
				else
				{
					if (m_SelectItem - m_ColCount + 1 > 0 && m_SelectItem < m_ItemsPerPage &&
							m_SelectItem < m_LastItem - m_FirstItem)
					{
						m_pBaseItem[m_SelectItem]->SetSelect(false);
						m_SelectItem -= m_ColCount;
						m_pBaseItem[m_SelectItem]->SetSelect(true);
						m_pFocus = m_pBaseItem[m_SelectItem];
					}
					else
					{
						if (m_CircleVertical)
						{
							m_pBaseItem[m_SelectItem]->SetSelect(false);
							m_SelectItem = m_ItemsPerPage - (m_ColCount - m_SelectItem);
							int count = m_LastItem - m_FirstItem - 1;
							while (m_SelectItem > count)
							{
								m_SelectItem -= m_ColCount;
							}
	//						m_SelectItem = m_LastItem - m_FirstItem - 1;
							m_pBaseItem[m_SelectItem]->SetSelect(true);
							m_pFocus = m_pBaseItem[m_SelectItem];
						}
						else
						{
							if (m_SelectItem != -1 && m_SelectItem < m_ItemsPerPage)
								m_pBaseItem[m_SelectItem]->SetSelect(false);
							item = m_pBaseItem[m_SelectItem];
							m_SelectItem = -1;
							m_pFocus = NULL;
							m_pParent->OnCommand(PBEXITFOCUS, par1, (int)item);
						}
					}
				}
				break;

			case KEY_RIGHT:
			case KEY_NEXT:
				if (!IsControl())
				{
						m_SelectItem = -1;
						m_pFocus = NULL;
						m_pParent->OnCommand(PBEXITFOCUS, par1, (int)this);
				}
				else
				{
					if (m_SelectItem != -1 && m_SelectItem < m_ItemsPerPage &&
							m_SelectItem < m_LastItem - m_FirstItem)
					{
						if (m_SelectItem % m_ColCount == m_ColCount - 1)
						{
							if (m_CurrentPage < m_Pages)
							{
								//m_CurrentPage++;
								step = (type == EVT_KEYRELEASE ? (par2 > 0 ? 0 : 1) : 10);
								SetCurrentPage(m_CurrentPage + step);
								OnChangePage();
								m_pFocus = m_pBaseItem[m_SelectItem];
							}
							else
							{
								m_pBaseItem[m_SelectItem]->SetSelect(false);
								item = m_pBaseItem[m_SelectItem];
								m_SelectItem = -1;
								m_pFocus = NULL;
								m_pParent->OnCommand(PBEXITFOCUS, par1, (int)item);
							}
						}
						else
						{
							m_pBaseItem[m_SelectItem]->SetSelect(false);
							m_SelectItem++;
							m_pBaseItem[m_SelectItem]->SetSelect(true);
							m_pFocus = m_pBaseItem[m_SelectItem];
						}
					}
					else
					{
						if (m_SelectItem != -1 && m_SelectItem < m_ItemsPerPage)
							m_pBaseItem[m_SelectItem]->SetSelect(false);
						item = m_pBaseItem[m_SelectItem];
						m_SelectItem = -1;
						m_pFocus = NULL;
						m_pParent->OnCommand(PBEXITFOCUS, par1, (int)item);
					}
				}
				break;

			case KEY_LEFT:
			case KEY_PREV:
				if (!IsControl())
				{
						m_SelectItem = -1;
						m_pFocus = NULL;
						m_pParent->OnCommand(PBEXITFOCUS, par1, (int)this);
				}
				else
				{
					if (m_SelectItem != -1 && m_SelectItem < m_ItemsPerPage &&
							m_SelectItem < m_LastItem - m_FirstItem)
					{
						if (m_SelectItem % m_ColCount == 0)
						{
							if (m_CurrentPage > 1)
							{
								//m_CurrentPage--;
								step = (type == EVT_KEYRELEASE ? (par2 > 0 ? 0 : 1) : 10);
								SetCurrentPage(m_CurrentPage - step);
								OnChangePage();
								m_pFocus = m_pBaseItem[m_SelectItem];
							}
							else
							{
								m_pBaseItem[m_SelectItem]->SetSelect(false);
								item = m_pBaseItem[m_SelectItem];
								m_SelectItem = -1;
								m_pFocus = NULL;
								m_pParent->OnCommand(PBEXITFOCUS, par1, (int)item);
							}
						}
						else
						{
							m_pBaseItem[m_SelectItem]->SetSelect(false);
							m_SelectItem--;
							m_pBaseItem[m_SelectItem]->SetSelect(true);
							m_pFocus = m_pBaseItem[m_SelectItem];
						}
					}
					else
					{
						if (m_SelectItem != -1 && m_SelectItem < m_ItemsPerPage)
							m_pBaseItem[m_SelectItem]->SetSelect(false);
						item = m_pBaseItem[m_SelectItem];
						m_SelectItem = -1;
						m_pFocus = NULL;
						m_pParent->OnCommand(PBEXITFOCUS, par1, (int)item);
					}
				}
				break;

			case KEY_OK:
				if (m_ItemCount > 0)
				{
					if (type == EVT_KEYRELEASE)
					{
						m_pBaseItem[m_SelectItem]->Run();
					}
					else if (type == EVT_KEYREPEAT)
					{
						m_pBaseItem[m_SelectItem]->OpenMenu();
					}
				}
				break;

			case KEY_BACK:
				if (type == EVT_KEYRELEASE)
				{
					if (m_SelectItem != -1 && m_SelectItem < m_ItemsPerPage)
						m_pBaseItem[m_SelectItem]->SetSelect(false);
					item = m_pBaseItem[m_SelectItem];
					m_SelectItem = -1;
					m_pFocus = NULL;
					m_pParent->OnCommand(PBEXITFOCUS, KEY_LEFT, (int)item);
				}
				break;
			case KEY_MENU:
				m_pBaseItem[m_SelectItem]->OpenMenu();
				break;
		}
	}

	if (IsVisible() && ISPOINTEREVENT(type))
	{
		return PBWindow::MainHandler(type, par1, par2);
	}

	return 0;
}

int PBCommonBox::_ScrollMainHandler(int type, int par1, int par2)
{
	int step;
	PBWindow *item;
	if (type == EVT_KEYRELEASE || type == EVT_KEYREPEAT)
	{
		switch(par1)
		{
			case KEY_DOWN:
				m_SelectItem++;
				if (m_SelectItem >= m_ItemsPerPage || m_SelectItem >= m_ItemCount)
				{
					if (m_SelectItem >= m_ItemsPerPage && m_SelectItem + m_FirstItem < m_ItemCount)
					{
						if (m_LastItem + m_ItemsPerPage - 3 > m_ItemCount)
						{
							step = m_ItemCount - m_LastItem - 1;
						}
						else
						{
							step = m_ItemsPerPage - 4;
						}
						m_LastItem += step;
						m_FirstItem += step;
						m_pBaseItem[m_SelectItem - 1]->SetSelect(false);
						m_SelectItem -= (step);
					}
					if (m_SelectItem + m_FirstItem >= m_ItemCount)
					{
						m_SelectItem--;
						m_pBaseItem[m_SelectItem]->SetSelect(false);
						item = m_pBaseItem[m_SelectItem];
						m_SelectItem = -1;
						m_pFocus = NULL;
//						m_pParent->OnCommand(PBEXITFOCUS, par1, (int)m_pBaseItem[m_SelectItem]);
						m_pParent->OnCommand(PBEXITFOCUS, par1, (int)item);
					}
					else
					{
						m_SelectItem--;
						m_pFocus = m_pBaseItem[m_SelectItem];
						m_pBaseItem[m_SelectItem]->SetSelect(true);
						m_FirstItem++;
						m_LastItem++;
						UpdateScrollBar();
						UpdateItems();
					}
				}
				else
				{
					m_pBaseItem[m_SelectItem - 1]->SetSelect(false);
					m_pFocus = m_pBaseItem[m_SelectItem];
					m_pBaseItem[m_SelectItem]->SetSelect(true);
				}
				break;

			case KEY_UP:
				m_SelectItem--;
				if (m_SelectItem < 0)
				{
					if (m_FirstItem > 0)
					{
						if (m_FirstItem - m_ItemsPerPage + 3 < 0)
						{
							step = m_FirstItem - 1;
						}
						else
						{
							step = m_ItemsPerPage - 4;
						}

						m_LastItem -= step;
						m_FirstItem -= step;
						m_pBaseItem[m_SelectItem + 1]->SetSelect(false);
						m_SelectItem += (step);
					}
					if (m_FirstItem > 0)
					{
						m_SelectItem++;
						m_pFocus = m_pBaseItem[m_SelectItem];
						m_pBaseItem[m_SelectItem]->SetSelect(true);
						m_FirstItem--;
						m_LastItem--;
						UpdateScrollBar();
						UpdateItems();
					}
					else
					{
						m_SelectItem++;
						m_pBaseItem[m_SelectItem]->SetSelect(false);
						item = m_pBaseItem[m_SelectItem];
						m_SelectItem = -1;
						m_pFocus = NULL;
//						m_pParent->OnCommand(PBEXITFOCUS, par1, (int)m_pBaseItem[m_SelectItem]);
						m_pParent->OnCommand(PBEXITFOCUS, par1, (int)item);
					}
				}
				else
				{
					m_pBaseItem[m_SelectItem + 1]->SetSelect(false);
					m_pFocus = m_pBaseItem[m_SelectItem];
					m_pBaseItem[m_SelectItem]->SetSelect(true);
				}
				break;

			case KEY_RIGHT:
			case KEY_LEFT:
			case KEY_PREV:
			case KEY_NEXT:
				m_pBaseItem[m_SelectItem]->SetSelect(false);
				item = m_pBaseItem[m_SelectItem];
				m_SelectItem = -1;
				m_pFocus = NULL;
				m_pParent->OnCommand(PBEXITFOCUS, par1, (int)item);
				break;

			case KEY_OK:
				if (m_ItemCount > 0)
				{
					if (type == EVT_KEYRELEASE)
					{
						m_pBaseItem[m_SelectItem]->Run();
						UpdateItems();
					}
					else if (type == EVT_KEYREPEAT)
					{
						m_pBaseItem[m_SelectItem]->OpenMenu();
					}
				}
				break;
			case KEY_MENU:
				if (m_ItemCount > 0)
				{
					m_pBaseItem[m_SelectItem]->OpenMenu();
				}
				break;
			case KEY_BACK:
				if (type == EVT_KEYRELEASE)
				{
					if (m_SelectItem != -1 && m_SelectItem < m_ItemsPerPage)
						m_pBaseItem[m_SelectItem]->SetSelect(false);
					item = m_pBaseItem[m_SelectItem];
					m_SelectItem = -1;
					m_pFocus = NULL;
					m_pParent->OnCommand(PBEXITFOCUS, KEY_LEFT, (int)item);
				}
				break;
		}
	}

	if (ISPOINTEREVENT(type))
	{
		return PBWindow::MainHandler(type, par1, par2);
	}
	return 1;

}

void PBCommonBox::_PageRefresh(int refresh)
{
	PBTabPanel *tabs = GetTabPanel();
	int i;
	int SelectItem;

	switch(refresh)
	{
		case REFRESH_UPDATE_ITEMS:
			UpdateItems();
			MarkAsChanged();
			break;

		case REFRESH_CHANGE_COUNT:
			if (m_ItemCount != m_pRecordset->RowCount())
			{
				m_ItemCount = m_pRecordset->RowCount();
				SetPages((m_ItemCount - 1) / m_ItemsPerPage + 1);
			}
			_PageRefresh(REFRESH_CHANGE_PAGE);
			break;

		case REFRESH_MOVE_AT_BEGIN:
			SetCurrentPage(1);
			_PageRefresh(REFRESH_CHANGE_COUNT);
			break;

		case REFRESH_CHANGE_PAGE:
			i = 0;
			m_SelectItem = -1;
			while (i < m_ItemsPerPage && m_pBaseItem[i] != m_pFocus)
			{
				i++;
			}
			if (i < m_ItemsPerPage)
			{
				m_SelectItem = i;
			}
			m_FirstItem = (tabs->GetCurrent() - 1) * m_ItemsPerPage;
			m_LastItem = tabs->GetCurrent() * m_ItemsPerPage;
			_PageRefresh(REFRESH_UPDATE_ITEMS);
			if (m_SelectItem > -1 && m_LastItem - m_FirstItem > 0 && m_LastItem - m_FirstItem <= m_SelectItem)
			{
				m_pBaseItem[m_SelectItem]->SetSelect(false);
				m_SelectItem = m_LastItem - m_FirstItem - 1;
				m_pBaseItem[m_SelectItem]->SetSelect(true);
				m_pFocus = m_pBaseItem[m_SelectItem];
			}
			break;

		case REFRESH_CHANGE_ITEMS_PER_PAGE:
			if (m_SelectItem > -1)
			{
				SelectItem = m_FirstItem + m_SelectItem;
				m_pBaseItem[m_SelectItem]->SetSelect(false);
			}
			else
			{
				SelectItem = m_FirstItem;
			}
			SetPages((m_ItemCount - 1) / m_ItemsPerPage + 1);
			tabs->SetCurrent(SelectItem / m_ItemsPerPage + 1);
			m_CurrentPage = tabs->GetCurrent();
			m_FirstItem = (tabs->GetCurrent() - 1) * m_ItemsPerPage;
			m_LastItem = tabs->GetCurrent() * m_ItemsPerPage;
			_PageRefresh(REFRESH_UPDATE_ITEMS);
			if (m_SelectItem > -1)
			{
				m_SelectItem = SelectItem - m_FirstItem;
				m_pBaseItem[m_SelectItem]->SetSelect(true);
				m_pFocus = m_pBaseItem[m_SelectItem];
			}
			MarkAsChanged();
			break;
	}
}

void PBCommonBox::_ScrollRefresh(int refresh)
{
	int i;
	int step;

	switch(refresh)
	{
		case REFRESH_UPDATE_ITEMS:
			UpdateItems();
			UpdateScrollBar();
			MarkAsChanged();
			break;

		case REFRESH_CHANGE_ITEMS_PER_PAGE:	// need to upgrade as other action(optimize)
		case REFRESH_CHANGE_COUNT:
			if (m_ItemCount != m_pRecordset->RowCount())
			{
				m_ItemCount = m_pRecordset->RowCount();
			}
			if (m_SelectItem >= m_ItemsPerPage && m_ItemsPerPage < m_ItemCount)
			{
				m_pBaseItem[m_SelectItem]->SetSelect(false);
				m_FirstItem += m_SelectItem - m_ItemsPerPage + 1;
				m_SelectItem = m_ItemsPerPage - 1;
				m_LastItem = m_FirstItem + m_SelectItem;
			}
			if (m_LastItem <= m_FirstItem + m_ItemsPerPage)
			{
				if (m_FirstItem + m_ItemsPerPage <= m_ItemCount)
				{
					m_LastItem = m_FirstItem + m_ItemsPerPage;
				}
				else
				{
					if (m_LastItem > m_ItemCount)
					{
						step = m_LastItem - m_ItemCount;
						if (m_FirstItem - step < 0)
							step = m_FirstItem;
						m_FirstItem -= step;
						if (m_pFocus != NULL)
						{
							m_pBaseItem[m_SelectItem]->SetSelect(false);
							m_SelectItem += step;
							m_pBaseItem[m_SelectItem]->SetSelect(true);
						}
					}
					else
					{
						m_LastItem = m_ItemCount;
					}
				}
			}
			if (m_ItemCount == 0)
			{
				m_SelectItem = -1;
			}
			UpdateItems();
			if (m_ItemCount > 0)
			{
				i = 0;
				while (i < m_ItemsPerPage && !m_pBaseItem[i]->IsSelect())
				{
					i++;
				}
				if (i < m_ItemsPerPage)
				{
					m_pBaseItem[i]->SetSelect(false);
				}
				if (m_pFocus != NULL)
				{
					if (m_SelectItem == -1)
					{
						m_SelectItem = 0;
					}
					if (m_SelectItem >= m_LastItem - m_FirstItem)
					{
						m_SelectItem = m_LastItem - m_FirstItem - 1;
					}
					m_pBaseItem[m_SelectItem]->SetSelect(true);
					m_pFocus = m_pBaseItem[m_SelectItem];
				}
			}
			UpdateScrollBar();
			MarkAsChanged();
			break;

		case REFRESH_MOVE_AT_BEGIN:
			if (m_ItemCount != m_pRecordset->RowCount())
			{
				m_ItemCount = m_pRecordset->RowCount();
			}
			m_FirstItem = 0;
			m_LastItem = m_ItemsPerPage;
			if (m_ItemCount == 0)
			{
				m_SelectItem = -1;
			}
			UpdateItems();
			if (m_ItemCount > 0)
			{
				i = 0;
				while (i < m_ItemsPerPage && !m_pBaseItem[i]->IsSelect())
				{
					i++;
				}
				if (i < m_ItemsPerPage)
				{
					m_pBaseItem[i]->SetSelect(false);
				}
				if (m_SelectItem == -1)
				{
					m_SelectItem = 0;
				}
				if (m_SelectItem >= m_LastItem - m_FirstItem)
				{
					m_SelectItem = m_LastItem - m_FirstItem - 1;
				}
			}
			UpdateScrollBar();
			MarkAsChanged();
			break;
	}
}

void PBCommonBox::OnResizeItems(bool /*ChangeMode*/)
{
	int i;
	PBScrollBar *scrollBar = GetVScroll();

	for (i = 0; i < m_ItemsPerPage; i++)
	{
		m_pBaseItem[i]->SetWindowPos(GetClientLeft() - GetLeft(), i * m_pBaseItem[i]->GetHeight() + m_FreeSpaceV, GetWidth() - (scrollBar == NULL ? 2 * GetBorderSize() : scrollBar->GetWidth() + GetBorderSize()), m_pBaseItem[i]->GetHeight());
	}
}

void PBCommonBox::_PageOnResize()
{
	CreateItems();
	m_LastItem = m_FirstItem + m_ItemsPerPage;
	if (m_CreatedItems < m_ItemsPerPage)
	{
		m_CreatedItems = m_ItemsPerPage;
	}
	OnResizeItems(false);
	PBTabPanel *tabs = GetTabPanel();
	tabs->UpdateSize();
	Refresh(REFRESH_CHANGE_ITEMS_PER_PAGE);
}

void PBCommonBox::_ScrollOnResize()
{
	CreateItems();
	m_LastItem = m_FirstItem + m_ItemsPerPage;
	if (m_CreatedItems < m_ItemsPerPage)
	{
		m_CreatedItems = m_ItemsPerPage;
	}
	OnResizeItems(false);
	PBScrollBar *scrollBar = GetVScroll();
	scrollBar->SetWindowPos(GetWidth() - scrollBar->GetWidth() - 1, 0, scrollBar->GetWidth(), GetHeight());
	Refresh(REFRESH_CHANGE_ITEMS_PER_PAGE);
}
