#ifndef PBCOMMONBOX_H
#define PBCOMMONBOX_H

#define PBCOMMON_MAXFILECOUNT 100	// max visible items
#define PBCOMMON_ITEMHEIGHT 50		// default height of item

#define REFRESH_SELECT_FIRST 1			// Set focus to first item in visible items
#define REFRESH_DISELECT_ITEM 2			// Diselect focused item
#define REFRESH_UPDATE_ITEMS 4			// Update items (Bind(), Visible())
#define REFRESH_CHANGE_COUNT 8			// If count of items in list is changed
#define REFRESH_MOVE_AT_BEGIN 16		// Set focus to start of list
#define REFRESH_CHANGE_PAGE 32			// Change view page with updating items
#define REFRESH_CHANGE_ITEMS_PER_PAGE 64	// Change numbers of items on page

#include "pbpagebox.h"
#include "pbscrollbar.h"

class PBCommonBox : public PBPageBox
{
public:
	PBCommonBox();
	~PBCommonBox();

	int OnCreate();
	int MainHandler(int type, int par1, int par2);
	int OnCommand(int commandId, int par1, int par2);
//	int OnChangePage();
	int OnDraw(bool force);
	void Refresh(int refresh);		// refresh view list from recordset
	void SetCommandId(int commandId);	// set commandId for parent OnCommand function
	void SetCircleVertical(bool value);	// set m_CircleVertical
	void UpdateScrollBar();			// Updating position of scrollbar
	void UpdateItems();			// Update items from recordset
	virtual void ChangeViewMode(int viewmode, bool redraw);		// for change view mode, prefered to calling OnResizeItems(),
									// CreateItems(), Refresh(REFRESH_CHANGE_ITEMS_PER_PAGE).
	int GetViewMode() const;		// return m_ViewMode

protected:
	bool m_CircleVertical;	// flag for non-exiting cursor from box on vertical moving (cursor goes by cicle)
	int m_ViewMode;		// View mode
	int m_ItemCount;	// Count item
	int m_RowCount;		// Count of rows in grid mode
	int m_ColCount;		// Count of columns in grid mode
	int m_ItemsPerPage;	// Max visible items on page(screen)
	int m_FreeSpaceV;	// space from top and bottom for centering
	int m_FreeSpaceH;	// space from left and right for centering
	int m_SelectItem;	// select item form visible items
	int m_FirstItem;	// number of first visible item in list
	int m_LastItem;		// number of last visible item in list
	int m_CommandId;	// commandId for parent OnCommand function
	int m_CreatedItems;	// how many items was created maximum
	PBWindow *m_pBaseItem[PBCOMMON_MAXFILECOUNT];	// base items for inside work this ancestor

	virtual void CreateItems() = 0;			// function for creating items
		// !!!	DON'T FORGET SET POINTERS FROM ITEMS ON BASE ITEMS !!!

	void OnResize();
	virtual void OnResizeItems(bool ChangeMode);		// Resizing items and if nessecary changes items view mode
	int _PageMainHandler(int type, int par1, int par2);	// handler for page mode
	int _ScrollMainHandler(int type, int par1, int par2);	// handler for scroll mode
	void _PageRefresh(int refresh);		// refresh view list from recordset for page mode
	void _ScrollRefresh(int refresh);	// refresh view list from recordset for scroll mode
	void _PageOnResize();			// resize list for page mode
	void _ScrollOnResize();			// resize list for scroll mode
};

#endif // PBCOMMONBOX_H

/*
// don't delete, this is a example for create items
void PBCommonBox::CreateItems()
{
	int i;

	m_ItemsPerPage = GetClientHeight() / PBCOMMON_ITEMHEIGHT;

	m_FreeSpace = (GetClientHeight() - m_ItemsPerPage * PBCOMMON_ITEMHEIGHT) / 2;
	if (m_FreeSpace == 0)
		m_FreeSpace = 1;

	for (i = m_CreatedItems; i < m_ItemsPerPage; i++)
	{
//		m_pBaseItem[i] = new PBWindow;

//		m_pBaseItem[i]->Create(this, GetClientLeft() - GetLeft(), i * PBCOMMON_ITEMHEIGHT + m_FreeSpace, GetClientWidth(), PBCOMMON_ITEMHEIGHT);
//				or
//		m_pBaseItem[i]->Create(this, GetClientLeft() - GetLeft(), i * PBCOMMON_ITEMHEIGHT + m_FreeSpace, GetClientWidth(), PBCOMMON_ITEMHEIGHT, 0, m_ViewMode);

//		m_pBaseItem[i]->Bind(m_pRecordset, i, -1);
	}

}
*/
