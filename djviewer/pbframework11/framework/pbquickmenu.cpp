#include "pbquickmenu.h"

const int VERT_MARGIN  = 10;
const int HOR_MARGIN  = 10;
const int BTN_WIDTH = 94;
const int BTN_HEIGHT = 94;

extern "C" {
	extern const ibitmap selected;
	extern const ibitmap selectedPress;
	extern const ibitmap SelectEmpty;
}

PBQuickMenu::PBQuickMenu()
	:m_pImgSave(NULL)
	,m_row(0)
	,m_col(0)
{
	memset(m_buttons, 0, sizeof(m_buttons));
}

PBQuickMenu::~PBQuickMenu()
{
	if (m_pImgSave)
	{
		free(m_pImgSave);
		m_pImgSave = NULL;
	}

	for (int row = 0; row<3; row++)
		for (int col = 0; col<3; col++)
			delete m_buttons[row][col];
}

int PBQuickMenu::OnCreate()
{
	if (m_pFont == NULL)
	{
		iconfig* gcfg = GetGlobalConfig();

		PBGraphics *graphics = GetGraphics();
		m_pFont = graphics->OpenFont(ReadString(gcfg, "font", DEFAULTFONT), 12, 0);
	}

	SetBorderSize(2);
	return 0;
}

int PBQuickMenu::OnDraw(bool force)
{
	if (IsVisible())
	{
		if (IsChanged() || force)
		{
			PBGraphics *graphics = GetGraphics();
			graphics->DrawRectEx(GetLeft(), GetTop(), GetWidth(), GetHeight(), 10, GetBorderSize(), BLACK, WHITE);
		}
	}
	return 0;
}

// получает команды от кнопок в меню.
// нужно закрыть меню, а команду передать в MainFrame
int PBQuickMenu::OnCommand(int commandId, int par1, int par2)
{
	Hide();
	if (m_commandUpdateMap[commandId] == false)
		DiscardInvalidation();
	return m_pParent->OnCommand(commandId, par1, par2);
}

// если updateAfterClose == false при закрытии диалога экран восстановливается, но не обновляется.
// Это полезно, если команда вызывает полное обновление экрана и прятать меню не нужно.
PBBitmapButton* PBQuickMenu::CreateButton(int row, int col, int commandID, const ibitmap* pIcon, const char* label, bool updateAfterClose)
{
	assert(row >= 0 && row < 3);
	assert(col >= 0 && col < 3);
	if (row < 0 || row >= 3 || col < 0 || col >= 3)
		return NULL;

	int x = VERT_MARGIN + col * (VERT_MARGIN + BTN_WIDTH);
	int y = HOR_MARGIN + row * (HOR_MARGIN + BTN_HEIGHT);

	PBBitmapButton *pButton = new PBBitmapButton();
	pButton->Create(this, x, y, BTN_WIDTH, BTN_HEIGHT, PBWS_VISIBLE, 0);
	pButton->SetCommandID(commandID);
	pButton->SetImage((ibitmap*)pIcon, aligmentCenter);
	pButton->SetText(label, ALIGN_CENTER | VALIGN_TOP | RTLAUTO);
	pButton->SetSelect(false);
	pButton->SetBorderSize(2);

	if (row == 1 && col == 1)
		pButton->SetFocus();

	m_buttons[row][col] = pButton;

	m_commandUpdateMap[commandID] = updateAfterClose;
	return pButton;
}

void PBQuickMenu::Show(int x, int y)
{
	SetWindowPos(x, y, GetWidth(), GetHeight());

	PBRect rcWindow = GetWindowScreenRect();

	if (m_pImgSave)
	{
		free(m_pImgSave);
		m_pImgSave = NULL;
	}

	m_pImgSave = BitmapFromScreen(rcWindow.GetX(), rcWindow.GetY(), rcWindow.GetWidth(), rcWindow.GetHeight());


	m_pFocus = NULL;
	m_row = 1;
	m_col = 1;
	if (m_buttons[m_row][m_col] != NULL)
	{
		OnChildFocusChanged(m_buttons[m_row][m_col]);
	}

	ChangeWindowStyle(PBWS_VISIBLE, 0);
}

void PBQuickMenu::Hide()
{
	ChangeWindowStyle(0, PBWS_VISIBLE);

	if (m_pImgSave)
	{
		PBRect rcWindow = GetWindowScreenRect();
		DrawBitmap(rcWindow.GetX(), rcWindow.GetY(), m_pImgSave);

		Invalidate(rcWindow, updateRect);

		free(m_pImgSave);
		m_pImgSave = NULL;
	}
}

int PBQuickMenu::MainHandler(int type, int par1, int par2)
{
	int res = 0;

	//fprintf(stderr, "PBQuickMenu::MainHandler type - %d, int par1 - %d, int par2 - %d\n", type, par1, par2);
	if ((res = PBWindow::MainHandler(type, par1, par2)))
		return res;

	if(type == EVT_POINTERUP)
	{
		PBRect rcWindow = GetWindowScreenRect();

		if (par1 < rcWindow.GetLeft() || par1 > rcWindow.GetRight() ||
			par2 < rcWindow.GetTop() || par2 > rcWindow.GetBottom())
		{
			// out of window - closing menu
			Hide();
			return 1;
		}
	}
	else if (type == EVT_KEYPRESS &&
			 (par1 == KEY_UP || par1 == KEY_DOWN || par1 == KEY_LEFT || par1 == KEY_RIGHT))
	{
		switch(par1)
		{
		case KEY_UP:
			DecrementRow(true, true);
			break;
		case KEY_DOWN:
			IncrementRow(true, true);
			break;
		case KEY_LEFT:
			DecrementCol(true, true);
			break;
		case KEY_RIGHT:
			IncrementCol(true, true);
			break;
//		case KEY_BACK:
//		case KEY_PREV:
//			Hide();
//			return 1;	// exit immediately
//			break;
		}
		OnChildFocusChanged(m_buttons[m_row][m_col]);
	}
	else if (type == EVT_KEYRELEASE && (par1 == KEY_BACK || par1 == KEY_PREV) )
	{
		Hide();
		return 1;	// exit immediately
	}

	return res;
}

void PBQuickMenu::IncrementCol(bool checkOther, bool checkEmpty)
{
	m_col++;
	if (m_col > 2)
	{
		m_col = 0;
		if (checkOther)
			IncrementRow(false, false);
	}

	if (checkEmpty && m_buttons[m_row][m_col] == NULL)
		IncrementCol(checkOther, true);
}

void PBQuickMenu::DecrementCol(bool checkOther, bool checkEmpty)
{
	m_col--;
	if (m_col < 0)
	{
		m_col = 2;
		if (checkOther)
			DecrementRow(false, false);
	}

	if (checkEmpty && m_buttons[m_row][m_col] == NULL)
		DecrementCol(checkOther, true);
}

void PBQuickMenu::IncrementRow(bool checkOther, bool checkEmpty)
{
	m_row++;
	if (m_row > 2)
	{
		m_row = 0;
		if (checkOther)
			IncrementCol(false, false);
	}

	if (checkEmpty && m_buttons[m_row][m_col] == NULL)
		IncrementRow(checkOther, true);
}

void PBQuickMenu::DecrementRow(bool checkOther, bool checkEmpty)
{
	m_row--;
	if (m_row < 0)
	{
		m_row = 2;
		if (checkOther)
			DecrementCol(false, false);
	}

	if (checkEmpty && m_buttons[m_row][m_col] == NULL)
		DecrementRow(checkOther, true);
}
