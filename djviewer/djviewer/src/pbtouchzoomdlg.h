#ifndef PBTOUCHZOOMDLG_H
#define PBTOUCHZOOMDLG_H

#include "../../pbframework11/framework/pbwindow.h"
#include "../../pbframework11/framework/pbbitmapbutton.h"
#include "../../pbframework11/controls/pblabel.h"
#include "settings.h"

class PBTouchZoomDlg : public PBWindow
{
public:
	static const int WND_WIDTH = 459;
	static const int WND_HEIGHT = 340;

	PBTouchZoomDlg();

	// virtual overloads
	int OnDraw(bool force);
	int OnCreate();

	void Show(int x, int y, TZoomType zoom_type, int zoom_param);
	void Hide();

	int MainHandler(int type, int par1, int par2);

	PBBitmapButton* CreateButton(int x, int y, int width, int height, int commandID, const ibitmap* pIcon, const char* label);
	PBLabel* CreateLabel(int x, int y, int width, int height, const char* label);

	PBWindow* FindNearestWindow(int x, int y, int direction);
	PBWindow* FindChildByCommandId(int commandId);

	static int GetNextZoomParam(TZoomType zoom_type, int zoom_param, bool decrement);

protected:
	ibitmap *m_pImgSave;
	PBBitmapButton* m_pPreview4;
	PBBitmapButton* m_pPreview9;

protected:
	PBWindow* MoveFocus(int direction);
	PBWindow* MoveFocusToNextTabstop();
	PBWindow* MoveFocusToPrevTabstop();

	void Command2Param(int commandId, TZoomType& zoom_type, int& zoom_param);
	int Param2Command(TZoomType zoom_type, int zoom_param);

	int OnCommand(int commandId, int par1, int par2);
};

#endif // PBTOUCHZOOMDLG_H
