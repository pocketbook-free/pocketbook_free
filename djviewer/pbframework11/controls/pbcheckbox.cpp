#include "pbcheckbox.h"

extern "C"
{
	extern const ibitmap checkOn;
	extern const ibitmap checkOff;
}

PBCheckBox::PBCheckBox()
        : PBWindow()
	,m_Properties(0)
	,m_Text(NULL)
	,m_Checked(false)
	,m_pFontName(NULL)
	,m_FontSize(16)
	,m_TextColor(0)
	,m_pFont(NULL)
	,m_CheckRight(false)
{
}

PBCheckBox::~PBCheckBox()
{
	if (m_pFont)
		CloseFont(m_pFont);
	if (m_pFontName)
	{
		free(m_pFontName);
		m_pFontName = NULL;
	}
	freeData();
}

void PBCheckBox::freeData()
{
	if (m_Text)
	{
		free(m_Text);
		m_Text = NULL;
	}
}

int PBCheckBox::OnDraw(bool force)
{
	if ( IsVisible() )
	{
		if (IsChanged() || force)
		{
			PBGraphics *graphics = GetGraphics();

			ibitmap *checkPic = (ibitmap *) (m_Checked ? &checkOn : &checkOff);

			int checkWidth = checkPic->width;
			int checkX = m_CheckRight ? GetWidth() - checkWidth : 0;
			int checkY = (GetHeight() - checkPic->height ) / 2;
			if (checkY < 0)
				checkY = 0;

			int textOffset = checkWidth + (checkWidth >> 2);
			int textX = m_CheckRight ? GetLeft() : GetLeft() + textOffset;
			int textWidth = GetWidth() - textOffset;

			if (m_Text)
			{
				if (m_pFont == NULL)
				{
					if (m_pFontName != NULL)
					{
						if (m_FontSize == -1)
						{
							m_pFont = graphics->OpenFont(m_pFontName, (GetHeight() << 2) / 5, 0);
						}
						else
						{
							m_pFont = graphics->OpenFont(m_pFontName, m_FontSize, 0);
						}
					}
					else
					{
						m_pFont = graphics->OpenFont(DEFAULTFONT, (GetHeight() << 2) / 5, 0);
					}
				}
				graphics->SetFont(m_pFont, m_TextColor);
				graphics->DrawTextRect(textX, GetTop(), textWidth, GetHeight(), text(), m_Properties);

				if (m_Properties & ALIGN_CENTER)
				{
					int strWidth = StringWidth(text());
					checkX = ((GetWidth() - checkWidth - checkWidth/2) - strWidth)/2;
				}

				graphics->DrawBitmap(GetLeft() + checkX, GetTop() + checkY, checkPic);
			}
		}
		else if (IsFocusChanged())
		{
			PBGraphics *graphics = GetGraphics();
			// selection border
			int borderColor = IsFocused() ? BLACK : WHITE;
			graphics->DrawRectEx(GetLeft(), GetTop(), GetWidth(), GetHeight(), 10, GetBorderSize(), borderColor, -1);
		}

		// CHECK_DRAW_BORDER need for debug
//		if (m_properties & DRAW_BORDER)
//			graphics->DrawRect(GetLeft(), GetTop(), GetWidth(), GetHeight(), BLACK);
	}

	return 0;
}

// virtual
int PBCheckBox::MainHandler(int type, int par1, int par2)
{
	PBPoint clientPoint = m_pParent->ScreenToClient(PBPoint(par1, par2));

	if ( IsVisible() )
	{
		if (type == EVT_POINTERUP)
		{
			if (clientPoint.GetX() > GetLeft() && clientPoint.GetX() < (GetLeft() + GetWidth()) &&
				clientPoint.GetY() > GetTop() && clientPoint.GetY() < (GetTop() + GetHeight()))
			{
				setCheckState(!m_Checked);

				if (m_pParent)
					m_pParent->OnCommand(PBCHECKBOX, PBSTATECHANGED, (int) this);

				m_pParent->OnChildFocusChanged(this);

				// if(m_isActive)
				//m_pParent->OnCommand(m_commandId, par1, par2);
				return 1;
			}
		}
		else if (IsFocused() && type == EVT_KEYPRESS)
		{
			if (par1 == KEY_OK)
			{
				setCheckState(!isChecked());

				if (m_pParent)
					m_pParent->OnCommand(PBCHECKBOX, PBSTATECHANGED, (int) this);

				return 1;
			}
		}
	}
	return 0;
}

void PBCheckBox::setCheckState(bool state)
{
	if (m_Checked != state)
	{
		m_Checked = state;
		MarkAsChanged();
	}
}

void PBCheckBox::setText(const char *text)
{
	if (text != NULL)
	{
		freeData();
		m_Text = strdup(text);
		MarkAsChanged();
	}
}

bool PBCheckBox::isChecked() const
{
	return m_Checked;
}

char *PBCheckBox::text() const
{
	return m_Text;
}

void PBCheckBox::SetFont(const char *fontname, int fontsize, int color)
{
	if (fontname != NULL && *fontname != '\0')
	{
		if (m_pFontName != NULL)
		{
			free(m_pFontName);
			m_pFontName = NULL;
		}
		m_pFontName = strdup(fontname);
	}
	m_FontSize = fontsize;
	m_TextColor = color;
	MarkAsChanged();
}

void PBCheckBox::SetProperties(int properties)
{
	m_Properties = properties;
	MarkAsChanged();
}

void PBCheckBox::SetCheckRight(bool right)
{
	if (right != m_CheckRight)
	{
		m_CheckRight = right;
		MarkAsChanged();
	}
}
