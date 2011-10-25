#ifndef PBGRIDMENU_H
#define PBGRIDMENU_H

#include "../framework/pbgridcontainer.h"
#include "pbtoolbutton.h"

class PBGridMenu : protected PBGridContainer
{
public:
	PBGridMenu();
	~PBGridMenu();

	int Create(PBWindow* pParent, int left, int top, int width, int height, int style = 0, int exStyle = 0);

	int OnDraw(bool force);
	void OnPostDraw(bool force);
	int MainHandler(int type, int par1, int par2);
	int OnCommand(int commandId, int par1, int par2);
	void SetGrid(int nrow, int ncol);
	void SetButtonData(int index, ibitmap *bm, const char *text, int command);

protected:
	std::vector<PBToolButton *> m_Items;
	int m_Select;
};

#endif // PBGRIDMENU_H
