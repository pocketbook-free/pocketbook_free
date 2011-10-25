#include "pbgridmenu.h"

PBGridMenu::PBGridMenu():PBGridContainer()
{
}

PBGridMenu::~PBGridMenu()
{
	while (!m_Items.empty())
	{
		delete m_Items.back();
		m_Items.pop_back();
	}
}

int PBGridMenu::Create(PBWindow *pParent, int left, int top, int width, int height, int style, int exStyle)
{
	PBGridContainer::Create(pParent, left, top, width, height, style, exStyle);
	SetBorderSize(6);
//	recalcClientRect();
	return 0;
}

int PBGridMenu::OnDraw(bool force)
{
	return force;
}

void PBGridMenu::OnPostDraw(bool force)
{
	if ( IsVisible() && (IsChanged() || force) )
	{
		PBGraphics *graphics = GetGraphics();
		graphics->DrawFrame(GetLeft(), GetTop(), GetWidth(), GetHeight(), 10, 3, BLACK);
	}
}

int PBGridMenu::MainHandler(int type, int par1, int par2)
{
	PBPoint clientPoint = m_pParent->ScreenToClient(PBPoint(par1, par2));

	if (IsVisible() && type == EVT_POINTERUP)
	{
		if (clientPoint.GetX() > GetLeft() && clientPoint.GetX() < (GetLeft() + GetWidth()) &&
		    clientPoint.GetY() > GetTop() && clientPoint.GetY() < (GetTop() + GetHeight()))
		{
			m_pParent->OnChildFocusChanged(this);
			PBWindow::MainHandler(type, par1, par2);
		}
	}
	if (IsVisible() && type == EVT_KEYPRESS)
	{
		m_pFocus->MainHandler(type, par1, par2);
	}
	return 0;
}

int PBGridMenu::OnCommand(int commandId, int par1, int par2)
{
	if (commandId == PBTOOLBUTTON)
	{
		if (par1 > 0)
		{
			PBToolButton *Btn = static_cast<PBToolButton *>((void *)par2);
			unsigned int i = 0;
			while (i < m_Items.size())
			{
				if (m_Items[i]->IsSelect())
				{
					m_Items[i]->SetSelect(false);
				}
				if (Btn == m_Items[i])
				{
					m_Items[i]->SetSelect(true);
				}
				i++;
			}
			if (m_pParent)
				m_pParent->OnCommand(PBGRIDMENU, par1, par2);
		}
	}

	return 0;
}

void PBGridMenu::SetGrid(int nrow, int ncol)
{
	PBToolButton *Btn;
	while (!m_Items.empty())
	{
		delete m_Items.back();
		m_Items.pop_back();
	}

	PBGridContainer::SetGrid(nrow, ncol);

	int i, j;
	for (i = 0; i < nrow; i++)
	{
		for (j = 0; j < ncol; j++)
		{
			PBRect cell = GetCell(i, j);
			Btn = new PBToolButton;
			Btn->Create(this, cell.GetLeft(), cell.GetTop(), cell.GetWidth(), cell.GetHeight());
			Btn->SetBorderSize(5);
			Btn->SetImageAlignment(ALIGNMENT_TOP);
			Btn->SetFont("LiberationSans-Bold", 14, BLACK);
			Btn->SetSelect(false);
			Btn->SetTextProperties(ALIGN_CENTER | VALIGN_MIDDLE | RTLAUTO | DOTS);
			//Btn->SetVisible(false);
			m_Items.push_back(Btn);
		}
	}
	m_Select = m_RowCount * m_ColCount / 2;
	m_Items[m_Select]->SetSelect(true);
	MarkAsChanged();
}

void PBGridMenu::SetButtonData(int index, ibitmap *bm, const char *text, int command)
{
	if (index >= 0 && index < m_RowCount * m_ColCount)
	{
		m_Items[index]->SetText(text);
		m_Items[index]->SetImage(bm);
		m_Items[index]->SetCommand(command);
		m_Items[index]->SetVisible(command > 0);
	}
	MarkAsChanged();
}
