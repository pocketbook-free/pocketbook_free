#include "pbbitmapbutton.h"

PBBitmapButton::PBBitmapButton()
	:m_TextColor(BLACK)
{
}

const char* PBBitmapButton::GetText() const
{
	return  m_text.c_str();
}

void PBBitmapButton::SetImage(const ibitmap* pImage, PBAlignment alignment)
{
	m_pImage = pImage;
	m_ImageAlignment = alignment;
}

void PBBitmapButton::SetText(const char* text, int textProperties)
{
	if (text)
	{
		m_text = text;
		m_TextProperties = textProperties;
	}
}

int PBBitmapButton::OnCreate()
{
	if (m_pFont == NULL && m_pParent != NULL)
	{
		m_pFont = m_pParent->GetWindowFont();
	}
	return 0;
}

bool PBBitmapButton::IsPressed()
{
	return m_ExStyle & PBWS_BTN_PRESSED;
}

int PBBitmapButton::OnDraw(bool force)
{
	if (IsVisible())
	{
		if (IsChanged() || force)
		{
			PBGraphics *graphics = GetGraphics();
			graphics->FillArea(GetLeft(), GetTop(), GetWidth(), GetHeight(), WHITE);

			if (m_pFont)
				graphics->SetFont(m_pFont, m_TextColor);

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
				case aligmentBottom:
					ImageX = GetClientLeft() + (GetClientWidth() - ImageWidth) / 2;
					ImageY = GetClientTop() + GetClientHeight() - m_pImage->height;

					textX = GetClientLeft();
					textY = GetClientTop();
					textWidth = GetClientWidth();
					textHeight = GetClientHeight() - m_pImage->height;
					break;

				case aligmentLeft:
					ImageX = GetClientLeft();
					ImageY = GetClientTop() + (GetClientHeight() - m_pImage->height) / 2;

					textX = GetClientLeft() + (5 * ImageWidth / 4);
					textY = GetClientTop();
					textWidth = GetClientWidth() - (5 * ImageWidth / 4);
					textHeight = GetClientHeight();
					break;

				case aligmentRigth:
					ImageX = GetClientLeft() + GetClientWidth() - ImageWidth;
					ImageY = GetClientTop() + (GetClientHeight() - m_pImage->height) / 2;

					textX = GetClientLeft();
					textY = GetClientTop();
					textWidth = GetClientWidth() - (5 * ImageWidth / 4);
					textHeight = GetClientHeight();
					break;

				case aligmentTop:
					ImageX = GetClientLeft() + (GetClientWidth() - ImageWidth) / 2;
					ImageY = GetClientTop();

					textX = GetClientLeft();
					textY = GetClientTop() + m_pImage->height;
					textWidth = GetClientWidth();
					textHeight = GetClientHeight() - m_pImage->height;
					break;
				case aligmentCenter:
					int text_height = TextRectHeight(GetClientWidth(), GetText(), m_TextProperties);

					ImageX = GetClientLeft() + (GetClientWidth() - ImageWidth) / 2;
					ImageY = GetClientTop() +(GetClientHeight() - m_pImage->height - text_height) / 2;

					textX = GetClientLeft();
					textY = GetClientTop() +(GetClientHeight() - m_pImage->height - text_height)/2 + m_pImage->height;
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

			if (m_text.size() > 0)
			{
				graphics->DrawTextRect(textX, textY, textWidth, textHeight, GetText(), m_TextProperties);
			}

			// selection border
			int borderColor = IsFocused() ? BLACK : WHITE;
			graphics->DrawRectEx(GetLeft(), GetTop(), GetWidth(), GetHeight(), 10, GetBorderSize(), borderColor, -1);

			// invert button if it is pressed
			if (IsPressed())
				graphics->InvertRectEx(GetLeft(), GetTop(), GetWidth(), GetHeight(), 10);
		}
		else if (IsFocusChanged())
		{
			PBGraphics *graphics = GetGraphics();
			// selection border
			int borderColor = IsFocused() ? BLACK : WHITE;
			graphics->DrawRectEx(GetLeft(), GetTop(), GetWidth(), GetHeight(), 10, GetBorderSize(), borderColor, -1);
		}
	}

	return 0;
}

int PBBitmapButton::MainHandler(int type, int par1, int par2)
{
	if (IsVisible() == false)
		return 0;

	if (type == EVT_KEYPRESS && par1 == KEY_OK && IsFocused())
	{
		ChangeWindowStyleEx(PBWS_BTN_PRESSED, 0);
		return 1;
	}
	else if (type == EVT_KEYRELEASE && par1 == KEY_OK && IsFocused() && (m_ExStyle & PBWS_BTN_PRESSED) )
	{
		ChangeWindowStyleEx(0, PBWS_BTN_PRESSED);
		m_pParent->OnCommand(m_commandId, par1, par2);
		return 1; // processed
	}
	else if (type == EVT_POINTERDOWN)
	{
		PBPoint clientPoint = m_pParent->ScreenToClient(PBPoint(par1, par2));

		if (clientPoint.GetX() > GetLeft() && clientPoint.GetX() < (GetLeft() + GetWidth()) &&
			clientPoint.GetY() > GetTop() && clientPoint.GetY() < (GetTop() + GetHeight()))
		{
			ChangeWindowStyleEx(PBWS_BTN_PRESSED, 0);
			m_pParent->OnChildFocusChanged(this);
			return 1;
		}
	}
	else if (type == EVT_POINTERUP && IsPressed())
	{
		ChangeWindowStyleEx(0, PBWS_BTN_PRESSED);

		PBPoint clientPoint = m_pParent->ScreenToClient(PBPoint(par1, par2));

//		fprintf(stderr, "click in button - %d, clientX - %d, clientY - %d, btnX - %d, btnY - %d\n",
//				m_commandId, clientPoint.GetX(), clientPoint.GetY(), GetLeft(), GetTop());

		if (clientPoint.GetX() > GetLeft() && clientPoint.GetX() < (GetLeft() + GetWidth()) &&
			clientPoint.GetY() > GetTop() && clientPoint.GetY() < (GetTop() + GetHeight()))
		{
			m_pParent->OnCommand(m_commandId, par1, par2);
			return 1; // processed
		}
	}

	return 0;
}
