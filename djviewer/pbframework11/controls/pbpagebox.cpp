#include "pbpagebox.h"
#include "../framework/pbframewindow.h"

PBPageBox::PBPageBox():
	PBWindow(),
	m_Pages(1),
	m_CreatedPages(0),
	m_CurrentPage(1),
	m_PrevPage(-1),
	m_HavePages(false)
{
}

PBPageBox::~PBPageBox()
{
	DeletePagesArea();
}

int PBPageBox::OnCreate()
{
	PBTabPanel *tabs = GetTabPanel();
	m_Pages = tabs->GetTabsCount();
	m_CurrentPage = tabs->GetCurrent();
	tabs->SetBackground(DGRAY);
	m_PrevPage = -1;
	m_HavePages = false;
	m_CreatedPages = 0;
	return 0;
}

void PBPageBox::SetTabAlignment(int alignment)
{
	if (GetTabPanel()->GetAlignment() != alignment)
	{
		GetTabPanel()->SetAlignment(alignment);
		recalcClientRect();
		MarkAsChanged();
	}
}

void PBPageBox::SetPages(int npages)
{
	if (m_Pages != npages && npages > 0)
	{
		GetTabPanel()->SetTabsCount(npages);
		if (m_HavePages)
		{
			m_CreatedPages = m_Pages;
		}
		m_Pages = npages;
		if (m_CurrentPage > m_Pages)
		{
			SetCurrentPage(m_Pages);
		}
		if (m_HavePages)
		{
			CreatePagesArea();
		}
		MarkAsChanged();
	}
}

void PBPageBox::SetCurrentPage(int page)
{
	if (page > m_Pages)
		page = m_Pages;
	if (page < 1)
		page = 1;
	if (GetTabPanel()->GetCurrent() != page && (page > 0 && page < m_Pages + 1))
	{
		GetTabPanel()->SetCurrent(page);
		OnChangePage();
		m_CurrentPage = page;
		MarkAsChanged();
	}
}

int PBPageBox::GetTabAlignment()
{
	return GetTabPanel()->GetAlignment();
}

int PBPageBox::GetPages()
{
	return GetTabPanel()->GetTabsCount();
}

int PBPageBox::GetCurrentPage()
{
	return GetTabPanel()->GetCurrent();
}

void PBPageBox::CreatePagesArea()
{
	PBWindow *page;
	int i;

	m_HavePages = true;

	if (m_CreatedPages < m_Pages)
	{
		for (i = m_CreatedPages; i < m_Pages; i++)
		{
			page = new PBFrameWindow;
			page->Create(this, 0, 0, GetClientWidth(), GetClientHeight(), 0, 0);
//			page->SetVisible(false);
			m_pPagesArea.push_back(page);
		}
		m_CreatedPages = m_Pages;
	}

	if (m_CreatedPages > m_Pages)
	{
		for (i = m_CreatedPages; i > m_Pages; i--)
		{
			delete m_pPagesArea.back();
			m_pPagesArea.pop_back();
		}
		m_CreatedPages = m_Pages;
	}
}

void PBPageBox::DeletePagesArea()
{
	while (!m_pPagesArea.empty())
	{
		delete m_pPagesArea.back();
		m_pPagesArea.pop_back();
	}
	m_CreatedPages = 0;
}

PBWindow *PBPageBox::GetPageArea(int index)
{
	if (index >= 0 && index < m_Pages)
	{
		return m_pPagesArea[index];
	}
	return NULL;
}

int PBPageBox::MainHandler(int type, int par1, int par2)
{
	PBPoint clientPoint = m_pParent->ScreenToClient(PBPoint(par1, par2));

	if (IsVisible() && (type == EVT_POINTERDOWN || type == EVT_POINTERUP || type == EVT_POINTERLONG))
	{
		if (clientPoint.GetX() > GetLeft() && clientPoint.GetX() < (GetLeft() + GetWidth()) &&
		    clientPoint.GetY() > GetTop() && clientPoint.GetY() < (GetTop() + GetHeight()))
		{
			PBWindow::MainHandler(type, par1, par2);
			return 1;
		}
	}
	if (type == EVT_COMMAND)
	{
		return PBWindow::MainHandler(type, par1, par2);
	}
	return 0;
}

int PBPageBox::OnCommand(int commandId, int par1, int par2)
{
	if (commandId == PBTABPANEL && par1 == PBTABCHANGED)
	{
		m_CurrentPage = GetTabPanel()->GetCurrent();
		OnChangePage();
		MarkAsChanged();
	}

	if (m_pParent)
		m_pParent->OnCommand(commandId, par1, par2);
	return 0;
}

int PBPageBox::OnDraw(bool force)
{
	if ( IsVisible() && (IsChanged() || force) )
	{
		PBGraphics *graphics = GetGraphics();
		graphics->DrawRect(GetLeft(), GetTop(), GetWidth(), GetHeight(), BLACK);
	}
	return 0;
}

int PBPageBox::OnChangePage()
{
	PBTabPanel *tabs = GetTabPanel();

	if (tabs != NULL)
	{
		PBWindow *page;

		m_PrevPage = m_CurrentPage;
		m_CurrentPage = tabs->GetCurrent();

		if (m_HavePages)
		{
			GetGraphics()->FillArea(GetClientLeft(), GetClientTop(), GetClientWidth(), GetClientHeight(), WHITE);
			if (m_PrevPage > 0 && m_PrevPage <= m_Pages)
			{
				page = m_pPagesArea[m_PrevPage - 1];
				//page->SetVisible(false);
				page->ChangeWindowStyle(0, PBWS_VISIBLE);
			}

			if (m_CurrentPage > 0 && m_CurrentPage <= m_Pages)
			{
				page = m_pPagesArea[m_CurrentPage - 1];
				//page->SetVisible(true);
				page->ChangeWindowStyle(PBWS_VISIBLE, 0);
			}
		}
		MarkAsChanged();
	}
	return 0;
}

