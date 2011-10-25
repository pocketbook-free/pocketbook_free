#ifndef PBPAGEBOX_H
#define PBPAGEBOX_H

#include "../framework/pbwindow.h"
#include "pbtabpanel.h"

class PBPageBox : public PBWindow
{
public:
	PBPageBox();
	~PBPageBox();

	int OnCreate();

	void SetTabAlignment(int alignment);
	void SetPages(int npages);
	void SetCurrentPage(int page);

	int GetTabAlignment();
	int GetPages();
	int GetCurrentPage();

	void CreatePagesArea();				// Push or pop pages in vector of pages. Turn on page mode.
	void DeletePagesArea();
	PBWindow *GetPageArea(int index);		// Get pointer on page

	int MainHandler(int type, int par1, int par2);
	int OnCommand(int commandId, int par1, int par2);
	int OnDraw(bool force = false);

	virtual int OnChangePage();			// On change current page

protected:
	int m_Pages;					// Count of pages
	int m_CreatedPages;				// Last created pages (page mode)
	int m_CurrentPage;				// Current selected page
	int m_PrevPage;					// Prev selected page
	std::vector<PBWindow *> m_pPagesArea;		// Vector of pointers on pages
	bool m_HavePages;				// Page mode

};

#endif // PBPAGEBOX_H
