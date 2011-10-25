#include "pbtouchzoomdlg.h"
#include <limits>
//#include "settings.h"

const int CMD_PREVIEW4	= ZoomTypePreview << 16 | 50;
const int CMD_PREVIEW9	= ZoomTypePreview << 16 | 33;
const int CMD_FITWIDTH	= ZoomTypeFitWidth << 16 | 100;
const int CMD_NORMAL75	= ZoomTypeNormal << 16 | 75;
const int CMD_NORMAL80	= ZoomTypeNormal << 16 | 80;
const int CMD_NORMAL85	= ZoomTypeNormal << 16 | 85;
const int CMD_NORMAL90	= ZoomTypeNormal << 16 | 90;
const int CMD_NORMAL95	= ZoomTypeNormal << 16 | 95;
const int CMD_NORMAL100	= ZoomTypeNormal << 16 | 100;
const int CMD_NORMAL105	= ZoomTypeNormal << 16 | 105;
const int CMD_NORMAL120	= ZoomTypeNormal << 16 | 120;
const int CMD_NORMAL130	= ZoomTypeNormal << 16 | 130;
const int CMD_NORMAL140	= ZoomTypeNormal << 16 | 140;
const int CMD_NORMAL150	= ZoomTypeNormal << 16 | 150;
const int CMD_NORMAL170	= ZoomTypeNormal << 16 | 170;
const int CMD_COLUMNS2	= ZoomTypeColumns << 16 | 200;
const int CMD_COLUMNS3	= ZoomTypeColumns << 16 | 300;
const int CMD_COLUMNS4	= ZoomTypeColumns << 16 | 400;
const int CMD_COLUMNS5	= ZoomTypeColumns << 16 | 500;
const int CMD_REFLOW150	= ZoomTypeReflow << 16 | 150;
const int CMD_REFLOW200	= ZoomTypeReflow << 16 | 200;
const int CMD_REFLOW300	= ZoomTypeReflow << 16 | 300;
const int CMD_REFLOW400	= ZoomTypeReflow << 16 | 400;
const int CMD_REFLOW500	= ZoomTypeReflow << 16 | 500;

#define BORDER_SIZE		2

#define LEFT_MARGIN		25
#define BTN_WIDTH		64
#define BTN_HEIGHT		70
#define BTN_HOR_MARGIN	5
#define BTN_VERT_MARGIN	10
#define ROW1_HEIGHT		10
#define ROW2_HEIGHT		90
#define ROW3_HEIGHT		170
#define ROW4_HEIGHT		250
#define ROW5_HEIGHT		370

extern "C" const ibitmap bmp_zoom_columns;
extern "C" const ibitmap bmp_zoom_fitwidth;
extern "C" const ibitmap bmp_zoom_normal;
extern "C" const ibitmap bmp_zoom_reflow;
extern "C" const ibitmap bmp_zoom_preview2;
extern "C" const ibitmap bmp_zoom_preview4;
extern "C" const ibitmap bmp_zoom_preview6;
extern "C" const ibitmap bmp_zoom_preview9;


PBTouchZoomDlg::PBTouchZoomDlg()
	:m_pImgSave(NULL)
{
}

int PBTouchZoomDlg::OnDraw(bool force)
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

void PBTouchZoomDlg::Command2Param(int commandId, TZoomType& zoom_type, int& zoom_param)
{
	zoom_type = (TZoomType)(commandId >> 16);
	zoom_param = commandId & 0x0000FFFF;
}

int PBTouchZoomDlg::Param2Command(TZoomType zoom_type, int zoom_param)
{
	return zoom_type << 16 | zoom_param;
}

int PBTouchZoomDlg::OnCreate()
{
	if (m_pFont == NULL)
	{
		PBGraphics *graphics = GetGraphics();
		m_pFont = graphics->OpenFont("LiberationSans-Bold", 16, 0);
	}

	SetBorderSize(BORDER_SIZE);

	m_pPreview4 = CreateButton(LEFT_MARGIN + 0*(BTN_WIDTH + BTN_HOR_MARGIN), ROW1_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_PREVIEW4, &bmp_zoom_preview4, "4");
	m_pPreview9 = CreateButton(LEFT_MARGIN + 1*(BTN_WIDTH + BTN_HOR_MARGIN), ROW1_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_PREVIEW9, &bmp_zoom_preview9, "9");

	CreateButton(LEFT_MARGIN + 3*(BTN_WIDTH + BTN_HOR_MARGIN), ROW1_HEIGHT, BTN_WIDTH*2, BTN_HEIGHT, CMD_FITWIDTH, &bmp_zoom_fitwidth, GetLangText("@Fit_width"));

	CreateButton(LEFT_MARGIN + 0*(BTN_WIDTH + BTN_HOR_MARGIN), ROW2_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_NORMAL75, &bmp_zoom_normal, "75%");
	CreateButton(LEFT_MARGIN + 1*(BTN_WIDTH + BTN_HOR_MARGIN), ROW2_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_NORMAL80, &bmp_zoom_normal, "80%");
	CreateButton(LEFT_MARGIN + 2*(BTN_WIDTH + BTN_HOR_MARGIN), ROW2_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_NORMAL85, &bmp_zoom_normal, "85%");
	CreateButton(LEFT_MARGIN + 3*(BTN_WIDTH + BTN_HOR_MARGIN), ROW2_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_NORMAL90, &bmp_zoom_normal, "90%");
	CreateButton(LEFT_MARGIN + 4*(BTN_WIDTH + BTN_HOR_MARGIN), ROW2_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_NORMAL95, &bmp_zoom_normal, "95%");
	CreateButton(LEFT_MARGIN + 5*(BTN_WIDTH + BTN_HOR_MARGIN), ROW2_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_NORMAL100, &bmp_zoom_normal, "100%");

	CreateButton(LEFT_MARGIN + 0*(BTN_WIDTH + BTN_HOR_MARGIN), ROW3_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_NORMAL105, &bmp_zoom_normal, "105%");
	CreateButton(LEFT_MARGIN + 1*(BTN_WIDTH + BTN_HOR_MARGIN), ROW3_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_NORMAL120, &bmp_zoom_normal, "120%");
	CreateButton(LEFT_MARGIN + 2*(BTN_WIDTH + BTN_HOR_MARGIN), ROW3_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_NORMAL130, &bmp_zoom_normal, "130%");
	CreateButton(LEFT_MARGIN + 3*(BTN_WIDTH + BTN_HOR_MARGIN), ROW3_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_NORMAL140, &bmp_zoom_normal, "140%");
	CreateButton(LEFT_MARGIN + 4*(BTN_WIDTH + BTN_HOR_MARGIN), ROW3_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_NORMAL150, &bmp_zoom_normal, "150%");
	CreateButton(LEFT_MARGIN + 5*(BTN_WIDTH + BTN_HOR_MARGIN), ROW3_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_NORMAL170, &bmp_zoom_normal, "170%");

	CreateButton(LEFT_MARGIN + 1*(BTN_WIDTH + BTN_HOR_MARGIN), ROW4_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_COLUMNS2, &bmp_zoom_columns, "200%");
	CreateButton(LEFT_MARGIN + 2*(BTN_WIDTH + BTN_HOR_MARGIN), ROW4_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_COLUMNS3, &bmp_zoom_columns, "300%");
	CreateButton(LEFT_MARGIN + 3*(BTN_WIDTH + BTN_HOR_MARGIN), ROW4_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_COLUMNS4, &bmp_zoom_columns, "400%");
	CreateButton(LEFT_MARGIN + 4*(BTN_WIDTH + BTN_HOR_MARGIN), ROW4_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_COLUMNS5, &bmp_zoom_columns, "500%");

//	CreateLabel(0, ROW5_HEIGHT - 20, WND_WIDTH, 20, GetLangText("@Reflow"));
//	CreateButton(LEFT_MARGIN + BTN_WIDTH/2 + 0*(BTN_WIDTH + BTN_HOR_MARGIN), ROW5_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_REFLOW150, &bmp_zoom_reflow, "150%");
//	CreateButton(LEFT_MARGIN + BTN_WIDTH/2 + 1*(BTN_WIDTH + BTN_HOR_MARGIN), ROW5_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_REFLOW200, &bmp_zoom_reflow, "200%");
//	CreateButton(LEFT_MARGIN + BTN_WIDTH/2 + 2*(BTN_WIDTH + BTN_HOR_MARGIN), ROW5_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_REFLOW300, &bmp_zoom_reflow, "300%");
//	CreateButton(LEFT_MARGIN + BTN_WIDTH/2 + 3*(BTN_WIDTH + BTN_HOR_MARGIN), ROW5_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_REFLOW400, &bmp_zoom_reflow, "400%");
//	CreateButton(LEFT_MARGIN + BTN_WIDTH/2 + 4*(BTN_WIDTH + BTN_HOR_MARGIN), ROW5_HEIGHT, BTN_WIDTH, BTN_HEIGHT, CMD_REFLOW500, &bmp_zoom_reflow, "500%");

	return 0;
}

PBBitmapButton* PBTouchZoomDlg::CreateButton(int x, int y, int width, int height, int commandID, const ibitmap* pIcon, const char* label)
{
	PBBitmapButton *pButton = new PBBitmapButton();
	pButton->Create(this, x, y, width, height, PBWS_VISIBLE | PBWS_TABSTOP, 0);
	pButton->SetCommandID(commandID);
	pButton->SetImage((ibitmap*)pIcon, aligmentCenter);
	pButton->SetText(label, ALIGN_CENTER | VALIGN_TOP | RTLAUTO);
	pButton->SetSelect(false);
	pButton->SetBorderSize(2);

	return pButton;
}

PBLabel* PBTouchZoomDlg::CreateLabel(int x, int y, int width, int height, const char* label)
{
	PBLabel *pLabel = new PBLabel();
	pLabel->Create(this, x, y, width, height, PBWS_VISIBLE | PBWS_TRANSPARENT, 0);
	pLabel->SetText(label, ALIGN_CENTER | VALIGN_TOP | RTLAUTO);
	pLabel->SetSelect(false);

	return pLabel;
}

void PBTouchZoomDlg::Show(int x, int y, TZoomType zoom_type, int zoom_param)
{
	SetWindowPos(x, y, GetWidth(), GetHeight());

	PBRect rcWindow = GetWindowScreenRect();

	if (m_pImgSave)
	{
		free(m_pImgSave);
		m_pImgSave = NULL;
	}

	m_pImgSave = BitmapFromScreen(rcWindow.GetX(), rcWindow.GetY(), rcWindow.GetWidth(), rcWindow.GetHeight());

	if (Settings::IsPortrait())
	{
		m_pPreview4->SetText("4", ALIGN_CENTER | VALIGN_TOP | RTLAUTO);
		m_pPreview4->SetImage(&bmp_zoom_preview4, aligmentCenter);

		m_pPreview9->SetText("9", ALIGN_CENTER | VALIGN_TOP | RTLAUTO);
		m_pPreview9->SetImage(&bmp_zoom_preview9, aligmentCenter);
	}
	else
	{
		m_pPreview4->SetText("2", ALIGN_CENTER | VALIGN_TOP | RTLAUTO);
		m_pPreview4->SetImage(&bmp_zoom_preview2, aligmentCenter);

		m_pPreview9->SetText("6", ALIGN_CENTER | VALIGN_TOP | RTLAUTO);
		m_pPreview9->SetImage(&bmp_zoom_preview6, aligmentCenter);
	}


	SetVisible(true);

	int commandId = Param2Command(zoom_type, zoom_param);

	PBWindow* pCurrent = FindChildByCommandId(commandId);
	assert(pCurrent);
	if (pCurrent)
		OnChildFocusChanged(pCurrent);
}

void PBTouchZoomDlg::Hide()
{
	ChangeWindowStyle(0, PBWS_VISIBLE);

	if (m_pImgSave)
	{
		PBRect rcWindow = GetWindowScreenRect();

		DrawBitmap(rcWindow.GetX(), rcWindow.GetY(), m_pImgSave);
		PartialUpdate(rcWindow.GetX(), rcWindow.GetY(), m_pImgSave->width, m_pImgSave->height);

		free(m_pImgSave);
		m_pImgSave = NULL;
	}
}

// получает команды от кнопок в меню.
// нужно закрыть меню, а команду передать в MainFrame
int PBTouchZoomDlg::OnCommand(int commandId, int /*par1*/, int /*par2*/)
{
	Hide();

	TZoomType zoom_type;
	int zoom_param;
	Command2Param(commandId, zoom_type, zoom_param);

	return m_pParent->OnCommand(CMD_CHANGE_ZOOM, zoom_type, zoom_param);
}

int PBTouchZoomDlg::MainHandler(int type, int par1, int par2)
{
	int res = 0;

	if ((res = PBWindow::MainHandler(type, par1, par2)))
		return res; // one of children processed this message

	if(type == EVT_POINTERUP)
	{
		if (GetWindowScreenRect().isInRectange(par1, par2) == false)
		{
			// out of window - closing menu
			Hide();
			return 1;
		}
	}
	else if (type == EVT_KEYPRESS)
	{
		if (par1 == PB_KEY_UP || par1 == PB_KEY_DOWN || par1 == KEY_LEFT || par1 == KEY_RIGHT)
		{
			PBWindow* pNewFocus = MoveFocus(par1);
			if (pNewFocus)
				OnChildFocusChanged(pNewFocus);
		}
		else if (par1 == KEY_BACK)
		{
			Hide();
			return 1;	// exit immediately
		}
	}

	return res;
}

PBWindow* PBTouchZoomDlg::MoveFocus(int direction)
{
	if (m_pFocus == NULL)
	{
		m_pFocus = m_Children.at(0);
		return MoveFocusToNextTabstop();
	}

	if (direction == KEY_LEFT)
	{
		return MoveFocusToPrevTabstop();
	}
	else if (direction == KEY_RIGHT)
	{
		return MoveFocusToNextTabstop();
	}
	else
	{
		const PBRect& rc = m_pFocus->GetWindowRect();
		return FindNearestWindow(rc.GetCenterX(), rc.GetCenterY(), direction);
	}

	return NULL;
}

PBWindow* PBTouchZoomDlg::MoveFocusToNextTabstop()
{
	TWindowListIt it = find(m_Children.begin(), m_Children.end(), m_pFocus);

	if (it != m_Children.end())
		it++;

	if (it == m_Children.end())
		it = m_Children.begin();

	while ((*it)->IsTabstop() == false)
	{
		it++;
		if (it == m_Children.end())
			it = m_Children.begin();
	}

	return *it;
}

PBWindow* PBTouchZoomDlg::MoveFocusToPrevTabstop()
{
	TWindowListRIt it = find(m_Children.rbegin(), m_Children.rend(), m_pFocus);

	if (it != m_Children.rend())
		it++;

	if (it == m_Children.rend())
		it = m_Children.rbegin();

	while ((*it)->IsTabstop() == false)
	{
		it++;
		if (it == m_Children.rend())
			it = m_Children.rbegin();
	}

	return *it;
}

// ближайший в заданном направлении
// x,y - center of current window
PBWindow* PBTouchZoomDlg::FindNearestWindow(int x, int y, int direction)
{
	PBPoint point(x,y);
	double min_distance = std::numeric_limits <double>::max();
	TWindowListIt res = m_Children.end();

	for (TWindowListIt it = m_Children.begin(); it != m_Children.end(); it++)
	{
		if ( *it == m_pFocus || (*it)->IsTabstop() == false )
			continue;

		const PBRect& other = (*it)->GetWindowRect();

		switch (direction)
		{
		case KEY_LEFT:
			if (other.GetX() > x || other.GetY() > y)
				continue;
			break;
		case KEY_RIGHT:
			if (other.GetX() < x || other.GetY() < y)
				continue;
			break;
		case PB_KEY_UP:
			if (other.GetBottom() > y)
				continue;
			break;
		case PB_KEY_DOWN:
			if (other.GetTop() < y)
				continue;
			break;
		}

		double distance = other.GetCenter().Distance(point);

		if (distance < min_distance)
		{
			min_distance = distance;
			res = it;
		}
	}

	if (res != m_Children.end())
		return *res;

	return NULL;
}

PBWindow* PBTouchZoomDlg::FindChildByCommandId(int commandId)
{
	for (TWindowListIt it = m_Children.begin(); it != m_Children.end(); it++)
	{
		if ( (*it)->GetCommandID() == commandId)
			return *it;
	}
	return NULL;
}

int IncrementCyclically(std::vector<int>& v, int current)
{
	std::vector<int>::iterator it = find(v.begin(), v.end(), current);
	if (it == v.end())
		it = v.begin();
	else
		it++;

	if (it == v.end())
		it = v.begin();

	return *it;
}

int DecrementCyclically(std::vector<int>& v, int current)
{
	std::vector<int>::reverse_iterator it = find(v.rbegin(), v.rend(), current);
	if (it == v.rend())
		it = v.rbegin();
	else
		it++;

	if (it == v.rend())
		it = v.rbegin();

	return *it;
}

int PBTouchZoomDlg::GetNextZoomParam(TZoomType zoom_type, int zoom_param, bool decrement)
{
	const int SC_PREVIEW[] = { 33, 50 };
	const int SC_NORMAL[] =	{ 75, 80, 85, 90, 95, 100, 105, 110, 120, 130, 140, 150, 170 };
	const int SC_COLUMNS[] = { 200, 300, 400, 500 };
	const int SC_REFLOW[] =	{ 150, 200, 300, 400, 500 };
	const int SC_EPUB[] = { 6, 9, 12, 14, 16, 21, 28 };

	if (zoom_type == ZoomTypeNormal)
	{
		std::vector<int> v(SC_NORMAL, SC_NORMAL + sizeof(SC_NORMAL)/sizeof(*SC_NORMAL));
		return decrement ? DecrementCyclically(v, zoom_param) : IncrementCyclically(v, zoom_param);
	}
	else if (zoom_type == ZoomTypePreview)
	{
		std::vector<int> v(SC_PREVIEW, SC_PREVIEW + sizeof(SC_PREVIEW)/sizeof(*SC_PREVIEW) );
		return decrement ? DecrementCyclically(v, zoom_param) : IncrementCyclically(v, zoom_param);
	}
	else if (zoom_type == ZoomTypeColumns)
	{
		std::vector<int> v(SC_COLUMNS, SC_COLUMNS + sizeof(SC_COLUMNS)/sizeof(*SC_COLUMNS) );
		return decrement ? DecrementCyclically(v, zoom_param) : IncrementCyclically(v, zoom_param);
	}
	else if (zoom_type == ZoomTypeReflow)
	{
		std::vector<int> v(SC_REFLOW, SC_REFLOW + sizeof(SC_REFLOW)/sizeof(*SC_REFLOW) );
		return decrement ? DecrementCyclically(v, zoom_param) : IncrementCyclically(v, zoom_param);
	}
	else if (zoom_type == ZoomTypeEpub)
	{
		std::vector<int> v(SC_EPUB, SC_EPUB + sizeof(SC_EPUB)/sizeof(*SC_EPUB) );
		return decrement ? DecrementCyclically(v, zoom_param) : IncrementCyclically(v, zoom_param);
	}

	assert(0);
	return zoom_param;
}
