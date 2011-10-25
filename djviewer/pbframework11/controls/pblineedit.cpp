#include "pblineedit.h"

PBLineEdit* PBLineEdit::s_pThis = NULL;

PBLineEdit::PBLineEdit()
	:PBWindow()
{
}

PBLineEdit::~PBLineEdit()
{
}

int PBLineEdit::OnDraw(bool force)
{
	PBGraphics *graphics;

	if (IsVisible() && (IsChanged() || force) && (graphics = GetGraphics()) != NULL)
	{
		graphics->FillArea(GetClientLeft() + PBEDIT_BORDER,
				   GetClientTop() + PBEDIT_BORDER,
				   GetClientWidth() - (PBEDIT_BORDER << 1),
				   GetClientHeight() - (PBEDIT_BORDER << 1),
				   WHITE);

		if (m_text[0])
		{
			SetFont(m_font, BLACK);
			graphics->DrawTextRect(GetClientLeft() + PBEDIT_BORDER,
					       GetClientTop() + PBEDIT_BORDER,
					       GetClientWidth() - (PBEDIT_BORDER << 1),
					       GetClientHeight() - (PBEDIT_BORDER << 1),
					       m_text,
					       ALIGN_LEFT | VALIGN_MIDDLE | DOTS);
		}

		graphics->DrawRect(GetLeft(), GetTop(), GetWidth(), GetHeight(), BLACK);
	}
	return 0;
}

bool PBLineEdit::Create(PBWindow* pParent, int left, int top, int width, int height, PBENTERSTYPE style)
{
	char defaultFont[32];
	PBWindow::Create(pParent, left, top, width, height);

	style = PBES_TEXT;
	// font prepare
	snprintf(defaultFont, sizeof(defaultFont), "#LiberationSans,%u,0", (height << 2) / 5);
	m_font = GetThemeFont("dictionary.font.normal", defaultFont);

	m_enterstyle = PBES_TEXT;

	// clearing m_text
	memset(m_text, 0, sizeof(m_text));
	return true;
}

void PBLineEdit::enttext_handler(char *text)
{
	PBLineEdit* pThis = GetThis();

	if (pThis)
		pThis->EntText_Handler(text);

	return;
}

int PBLineEdit::MainHandler(int type, int par1, int par2)
{
	int fcX1, fcX2, fcY1, fcY2;
	int flags;
	int result = 0;

	if (IsVisible() && type == EVT_POINTERUP)
	{
	    PBPoint clientPoint =  m_pParent->ScreenToClient(PBPoint(par1, par2));

	    fcX1 = GetClientLeft() + PBEDIT_BORDER;
	    fcX2 = fcX1 + GetClientWidth() - (PBEDIT_BORDER << 1);
	    fcY1 = GetClientTop() + PBEDIT_BORDER;
	    fcY2 = fcY1 + GetClientHeight() - (PBEDIT_BORDER << 1);

	    if (clientPoint.GetX() > fcX1 && clientPoint.GetX() < fcX2 &&
		clientPoint.GetY() > fcY1 && clientPoint.GetY() < fcY2)
	    {
		    switch(m_enterstyle)
		    {
		    case PBES_PHONE:
			    flags = KBD_PHONE;
			    break;
		    case PBES_NUMERIC:
			    flags = KBD_NUMERIC;
			    break;
		    case PBES_IPADDR:
			    flags = KBD_IPADDR;
			    break;
		    case PBES_URL:
			    flags = KBD_URL;
			    break;
		    case PBES_DATETIME:
			    flags = KBD_DATETIME;
			    break;
		    case PBES_TEXT:
		    default:
			    flags = KBD_ENTEXT;
			    break;
		    }

		    s_pThis = this;
		    m_pParent->OnChildFocusChanged(this);

		    OpenKeyboard("VK", m_text, PBEDIT_MAXENTER, flags, enttext_handler);

		    result = 1;
	    }
	    else result = PBWindow::MainHandler(type, par1, par2);

	}
	return result;
}

PBLineEdit* PBLineEdit::GetThis()
{
    return s_pThis;
}

void PBLineEdit::EntText_Handler(char *text)
{
	setText(text);

	if (m_pParent)
	    m_pParent->OnCommand(PBLINEEDIT, PBSTATECHANGED, (int) this);

	return;
}

void PBLineEdit::setText(const char *text)
{
	int textLen;
	if (text && (textLen = strlen(text)))
	{
		if (textLen > PBEDIT_MAXENTER) textLen = PBEDIT_MAXENTER;
		memcpy(m_text, text, textLen);
		m_text[textLen] = '\0';
		MarkAsChanged();
	}
	return;
}

const char *PBLineEdit::text() const
{
	return m_text;
}
