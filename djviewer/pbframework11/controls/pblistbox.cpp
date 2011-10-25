#include "pblistbox.h"

PBListBox::PBListBox()
	: PBWindow(),
	m_pScrollBar(NULL)
{
	//m_properties = 0;
	m_properties = DRAW_BORDER;
	m_AutoScroll = 1;
	m_selected = -1;
	m_listOffset = 0;
	m_itemHeight = 24;
	m_font = NULL;
}

PBListBox::~PBListBox()
{

}

int PBListBox::calcListCapacity()
{
    return ((GetClientHeight() - (PBLIST_BORDER << 1)) / m_itemHeight);
}

void PBListBox::updateScrollBar()
{
    int listCapacity;
    PBScrollBar *scrollBar = GetVScroll();
    if (scrollBar)
    {
		listCapacity = calcListCapacity();
		scrollBar->setMinimum(0);
		scrollBar->setMaximum(count() - listCapacity);
		scrollBar->setPageStep(listCapacity);
		scrollBar->setValue(m_listOffset);
    }
}

void PBListBox::DrawSelect(int index, int color)
{
	if (index >= m_listOffset && index < (m_listOffset + calcListCapacity()))
	{
		PBGraphics *graphics = GetGraphics();
		graphics->DrawSelection(GetClientLeft() + PBLIST_BORDER,
							GetClientTop() + PBLIST_BORDER + (index - m_listOffset) * m_itemHeight,
							GetClientWidth() - (PBLIST_BORDER << 1),
							m_itemHeight, color);
	}
}

int PBListBox::OnDraw(bool force)
{
    int elem_iter;
    int elem_render_count, elem_count;

	if ( IsVisible() && (IsChanged() || force) && m_font)
    {
		PBGraphics *graphics = GetGraphics();

		if (m_properties & DRAW_BORDER)
			graphics->DrawRect(GetLeft(), GetTop(), GetWidth(), GetHeight(), BLACK);

		graphics->FillArea(GetClientLeft() + PBLIST_BORDER, GetClientTop() + PBLIST_BORDER, GetClientWidth() - (PBLIST_BORDER << 1), GetClientHeight() - (PBLIST_BORDER << 1), WHITE);

		elem_count = count();
		elem_render_count = calcListCapacity();

		// corect elements for render
		if (elem_render_count > elem_count)
			elem_render_count = elem_count;

		if (elem_render_count)
		{
			if (m_AutoScroll)
			{
				if (elem_render_count < elem_count)
				{
					if (m_listOffset > m_selected) m_listOffset = m_selected;
					if (m_selected > (m_listOffset + elem_render_count)) m_listOffset = m_selected - elem_render_count;
				}
			}
			else m_AutoScroll = 1;

			updateScrollBar();

			if (m_selected != -1)
				DrawSelect(m_selected, BLACK);

			SetFont(m_font, BLACK);
			elem_iter = 0;
			while( elem_iter <  elem_render_count)
			{
				graphics->DrawTextRect(GetClientLeft() + (PBLIST_BORDER << 2),
									   GetClientTop() + PBLIST_BORDER + elem_iter * m_itemHeight,
									   GetClientWidth() - (PBLIST_BORDER << 3),
									   m_itemHeight,
									   text(elem_iter + m_listOffset), // m_stringList[elem_iter + m_listOffset].c_str(),
									   ALIGN_LEFT | VALIGN_MIDDLE | DOTS);
				elem_iter++;
			}
		}
    }
	return 0;//PBWindow::Draw();
}

int PBListBox::OnCommand(int commandId, int par1, int par2)
{
    if (commandId == PBSCROLLBAR && par1 == PBSTATECHANGED)
    {
		PBScrollBar *scrollBar = static_cast<PBScrollBar *> ((void *) par2);
		if (m_listOffset != scrollBar->value())
		{
			m_listOffset = scrollBar->value();
			m_AutoScroll = 0;
			MarkAsChanged();

			if (m_pParent)
				m_pParent->OnCommand(commandId, par1, par2);
		}
    }
    return 0;
}

int PBListBox::MainHandler(int type, int par1, int par2)
{
    int fcX1, fcX2, fcY1, fcY2;
	int clickedElem;
    int result = 0;

	if (IsVisible() && type == EVT_POINTERUP)
    {
		PBPoint clientPoint =  m_pParent->ScreenToClient(PBPoint(par1, par2));

		fcX1 = GetClientLeft() + PBLIST_BORDER;
		fcX2 = fcX1 + GetClientWidth() - (PBLIST_BORDER << 1);
		fcY1 = GetClientTop() + PBLIST_BORDER;
		fcY2 = fcY1 + GetClientHeight() - (PBLIST_BORDER << 1);

		if (clientPoint.GetX() > fcX1 && clientPoint.GetX() < fcX2 &&
			clientPoint.GetY() > fcY1 && clientPoint.GetY() < fcY2 &&
			(clickedElem = m_listOffset + (clientPoint.GetY() - fcY1) / m_itemHeight) < count())
		{
			if (m_selected != -1)
				DrawSelect(m_selected, WHITE);

			m_selected = clickedElem;
			DrawSelect(m_selected, BLACK);

			if (m_pParent)
				m_pParent->OnCommand(PBLISTBOX, PBSTATECHANGED, (int) this);

			result = 1;
		}
		else result = PBWindow::MainHandler(type, par1, par2);
    }

    return result;
}

bool PBListBox::Create(PBWindow* pParent, int left, int top, int width, int height)
{
	char defaultFont[32];
	PBWindow::Create(pParent, left, top, width, height, PBWS_VSCROLL);
	snprintf(defaultFont, sizeof(defaultFont), "#LiberationSans,%u,0", (m_itemHeight << 2) / 5);
	m_font = GetThemeFont("dictionary.font.normal", defaultFont);
	updateScrollBar();
	return true;
}

int PBListBox::count() const
{
	return m_pRecordset ? m_pRecordset->RowCount() : m_stringList.size();
}

const char* PBListBox::text(int index) const
{
	if (m_pRecordset && m_RecordsetCol != -1)
	{
		m_pRecordset->Move(index);
		return m_pRecordset->GetText(m_RecordsetCol);
	}
	else
	{
		return (index >= 0 && index < count()) ? m_stringList.at(index).c_str() : NULL;
	}
}

void PBListBox::changeString(const char* text, int index)
{
	if (text &&
	    index >= 0 && index < count())
	{
		m_stringList.at(index).assign(text);
		MarkAsChanged();
	}
}

void PBListBox::insertString(const char* text, int index)
{
	// Simply add the new string if index is out of range
	if (text)
	{
		if (index >= 0 && index < count())
		{
			m_stringList.insert(m_stringList.begin() + index, text);
			if (index < m_selected)
				m_selected++;
		}
		else
		{
			m_stringList.push_back(text);
		}
		MarkAsChanged();
	}
}

void PBListBox::removeString(int index)
{
	if (index >= 0 && index < count())
	{
		m_stringList.erase(m_stringList.begin() + index);
		if (index < m_selected)
			m_selected--;
		MarkAsChanged();
	}
}

int PBListBox::currentItem() const
{
	return m_selected;
}

const char* PBListBox::currentText() const
{
	return (m_selected >= 0 && m_selected < count()) ? m_stringList.at(m_selected).c_str() : NULL;
}

void PBListBox::setCurrentItem(int index)
{
	if (index >= 0 && index < count())
	{
		m_selected = index;
		MarkAsChanged();
	}
}

void PBListBox::centerCurrentItem()
{
    int listCapacity = calcListCapacity();
    int centerOffset = listCapacity >> 1;

    if (m_selected >= centerOffset &&  m_selected <= (count() - centerOffset) && count() > listCapacity)
    {
		m_listOffset = m_selected - centerOffset;
		MarkAsChanged();
    }
}

int PBListBox::itemHeight() const
{
	return m_itemHeight;
}

void PBListBox::clear()
{
	m_stringList.clear();
	m_selected = -1;
	m_listOffset = 0;
	MarkAsChanged();
}
