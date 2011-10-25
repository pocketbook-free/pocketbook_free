#include "pblabel.h"

PBLabel::PBLabel()
	: PBWindow() // call PBWindow constructor
	,m_Properties(0)
	,m_TextColor(BLACK)
	,m_pFont(NULL)
{
}

PBLabel::~PBLabel()
{
//	if (m_pFont)
//		CloseFont(m_pFont);
}

int PBLabel::OnCreate()
{
	if (m_pFont == NULL && m_pParent != NULL)
	{
		m_pFont = m_pParent->GetWindowFont();
	}
	return 1;
}

void PBLabel::SetText(const char *text, int properties)
{
	if (text != NULL)
	{
		m_text = text;
		MarkAsChanged();
	}

	if (properties >= 0)
	{
		m_Properties = properties;
		MarkAsChanged();
	}
}

const char* PBLabel::GetText() const
{
//
//	char *result;
//	if (m_pRecordset != NULL)
//	{
//		m_pRecordset->Move(m_RecordsetRow);
//		result = (char *)m_pRecordset->GetText(m_RecordsetCol);
//	}
//	else
//	{
//		result = m_text;
//	}
	return m_text;
}

void PBLabel::SetFont(const char *fontname, int fontsize, int color)
{
	if (fontname != NULL && *fontname != '\0')
	{
//		if (m_pFontName != NULL)
//		{
//			free(m_pFontName);
//			m_pFontName = NULL;
//		}
//		m_pFontName = fontname;
	}
	m_TextColor = color;
	MarkAsChanged();
}

void PBLabel::SetProperties(int properties)
{
	m_Properties = properties;
	MarkAsChanged();
}

int PBLabel::OnDraw(bool force)
{
	PBGraphics *graphics = GetGraphics();

	if ( IsVisible() && (IsChanged() || force) )
	{
		if (GetText())
		{
			graphics->SetFont(m_pFont, m_TextColor);
			if ( !IsTransparent() )
				graphics->FillArea(GetLeft(), GetTop(), GetWidth(), GetHeight(), m_Background);
			graphics->DrawTextRect(GetLeft(), GetTop(), GetWidth(), GetHeight(), GetText(), m_Properties);
		}
	}
	return 0;
}

