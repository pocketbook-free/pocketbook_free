#include "pbtoolbutton.h"

PBToolButton::PBToolButton()
	: PBWindow()
	,m_TextProperties(0)
	,m_pText(NULL)
	,m_pFontName(NULL)
	,m_FontSize(16)
	,m_TextColor(0)
	,m_pFont(NULL)
	,m_ImageAlignment(ALIGNMENT_LEFT)
	,m_pImage(NULL)
	,m_Command(0)
	,m_Pressed(false)
	,m_CanRelease(false)
{
}

PBToolButton::~PBToolButton()
{
	if (m_pFont)
		CloseFont(m_pFont);
	if (m_pText)
	{
		free(m_pText);
		m_pText = NULL;
	}
	if (m_pFontName)
	{
		free(m_pFontName);
		m_pFontName = NULL;
	}
	if (m_pImage)
	{
		free(m_pImage);
		m_pImage = NULL;
	}
}

int PBToolButton::OnCreate()
{
	SetBorderSize(3);
//	SetShadowSize(3);
	return 0;
}

int PBToolButton::MainHandler(int type, int par1, int par2)
{
	PBPoint clientPoint = m_pParent->ScreenToClient(PBPoint(par1, par2));

	if (IsVisible() && type == EVT_POINTERUP)
	{
		if (clientPoint.GetX() > GetLeft() && clientPoint.GetX() < (GetLeft() + GetWidth()) &&
		    clientPoint.GetY() > GetTop() && clientPoint.GetY() < (GetTop() + GetHeight()))
		{
			Run();
			return 1;
		}
	}
	if (IsVisible() && (type == EVT_KEYRELEASE || type == EVT_KEYREPEAT))
	{
		switch(par1)
		{
			case KEY_LEFT:
			case KEY_RIGHT:
			case KEY_UP:
			case KEY_DOWN:
				SetSelect(false);
				m_pParent->OnCommand(PBEXITFOCUS, par1, (int)this);
				break;

			case KEY_OK:
				Run();
				break;
		}
	}
	return 0;
}

int PBToolButton::OnCommand(int commandId, int par1, int par2)
{
	if (IsVisible() && commandId == PBENTERFOCUS)
	{
		SetSelect(true);
	}
	else
	{
		PBWindow::OnCommand(commandId, par1, par2);
	}
	return 0;
}

void PBToolButton::Run()
{
	if (!IsPressed() || m_CanRelease)
	{
		int pressed = IsPressed();
		if (m_pParent)
		{
			m_pParent->OnChildFocusChanged(this);
			m_pParent->OnCommand(PBREDRAW, 0, 0);
			GetGraphics()->ShowHourglassAt(GetLeft() + 20, GetTop() + GetHeight() / 2);
			m_pParent->OnCommand(PBTOOLBUTTON, m_Command ? m_Command : PBSTATECHANGED, (int) this);
			SetPressed(!pressed);
		}
	}
}

void PBToolButton::SetText(const char *text)
{
	if (text != NULL)
	{
		if (m_pText)
		{
			free(m_pText);
			m_pText = NULL;
		}
		m_pText = strdup(text);
		MarkAsChanged();
	}
}

void PBToolButton::SetImage(ibitmap *image)
{
	if (image != NULL)
	{
		if (m_pImage)
		{
			free(m_pImage);
			m_pImage = NULL;
		}
		m_pImage = image;
		MarkAsChanged();
	}
}

void PBToolButton::SetCommand(int command)
{
	m_Command = command;
}

void PBToolButton::SetPressed(bool pressed)
{
	if (pressed != m_Pressed)
	{
		m_Pressed = pressed;
		MarkAsChanged();
	}
}

bool PBToolButton::IsPressed() const
{
	return m_Pressed;
}

char *PBToolButton::GetText() const
{
	return m_pText;
}

void PBToolButton::SetFont(const char *fontname, int fontsize, int color)
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

void PBToolButton::SetTextProperties(int textproperties)
{
	m_TextProperties = textproperties;
	MarkAsChanged();
}

void PBToolButton::SetImageAlignment(int alignment)
{
	if (alignment >= 0 && alignment < 4 && alignment != m_ImageAlignment)
	{
		m_ImageAlignment = alignment;
		MarkAsChanged();
	}
}

int PBToolButton::OnDraw(bool force)
{
	if ( IsVisible())
	{
		PBGraphics *graphics = GetGraphics();
		if (IsChanged() || force)
		{
			graphics->FillArea(GetLeft(), GetTop(), GetWidth(), GetHeight(), WHITE);

			int ImageWidth;
			int ImageX = 0;
			int ImageY = 0;

			int textX = 0;
			int textY = 0;
			int textWidth = 0;
			int textHeight = 0;

			if (m_pImage != NULL)
			{
				ImageWidth = m_pImage->width;
				switch(m_ImageAlignment)
				{
					case ALIGNMENT_BOTTOM:
						ImageX = GetClientLeft() + (GetClientWidth() - ImageWidth) / 2;
						ImageY = GetClientTop() + GetClientHeight() - m_pImage->height;

						textX = GetClientLeft();
						textY = GetClientTop();
						textWidth = GetClientWidth();
						textHeight = GetClientHeight() - m_pImage->height;
						break;

					case ALIGNMENT_LEFT:
						ImageX = GetClientLeft();
						ImageY = GetClientTop() + (GetClientHeight() - m_pImage->height) / 2;

						textX = GetClientLeft() + (5 * ImageWidth / 4);
						textY = GetClientTop();
						textWidth = GetClientWidth() - (5 * ImageWidth / 4);
						textHeight = GetClientHeight();
						break;

					case ALIGNMENT_RIGHT:
						ImageX = GetClientLeft() + GetClientWidth() - ImageWidth;
						ImageY = GetClientTop() + (GetClientHeight() - m_pImage->height) / 2;

						textX = GetClientLeft();
						textY = GetClientTop();
						textWidth = GetClientWidth() - (5 * ImageWidth / 4);
						textHeight = GetClientHeight();
						break;

					case ALIGNMENT_TOP:
						ImageX = GetClientLeft() + (GetClientWidth() - ImageWidth) / 2;
						ImageY = GetClientTop();

						textX = GetClientLeft();
						textY = GetClientTop() + m_pImage->height;
						textWidth = GetClientWidth();
						textHeight = GetClientHeight() - m_pImage->height;
						break;
				}
				graphics->DrawBitmap(ImageX, ImageY, m_pImage);
			}
			else
			{
				textX = GetClientLeft();
				textY = GetClientTop();
				textWidth = GetClientWidth();
				textHeight = GetClientHeight();
			}

			if (m_pText)
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
				graphics->DrawTextRect(textX, textY, textWidth, textHeight, GetText(), m_TextProperties | (IsPressed() ? UNDERLINE : 0));
			}
		}
		if (ChangedSelection() || force)
		{
			if (IsSelect())
			{
				graphics->DrawFrame(GetLeft(), GetTop(), GetWidth(), GetHeight(), 10, GetBorderSize(), BLACK);
			}
			else
			{
				graphics->DrawFrame(GetLeft(), GetTop(), GetWidth(), GetHeight(), 10, GetBorderSize(), WHITE);
			}
		}
	}
	return 0;
}

void PBToolButton::RemoveFocus()
{
	SetSelect(false);
}

void PBToolButton::SetFocus()
{
	SetSelect(true);
}
