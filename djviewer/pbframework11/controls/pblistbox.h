#ifndef __PBLISTBOX_H__
#define __PBLISTBOX_H__

#include "../framework/pbwindow.h"
#include "pbscrollbar.h"

#define DRAW_BORDER 0x1l
#define PBLIST_BORDER 2

class PBListBox : public PBWindow
{
	public:
		PBListBox();
		~PBListBox();

		bool Create(PBWindow* pParent, int left, int top, int width, int height);

		int OnCommand(int commandId, int par1, int par2);
		int MainHandler(int type, int par1, int par2);

		int count() const;
		const char* text(int index) const;
		void changeString(const char* text, int index);
		void insertString(const char* text, int index = -1);
		void removeString(int index);
		int currentItem() const;
		const char* currentText() const;
		void setCurrentItem(int index);
		void centerCurrentItem();
		int itemHeight() const;
		void clear();

	protected:
	private:
		void DrawSelect(int index, int color);
		int OnDraw(bool force);
		int calcListCapacity();
		void updateScrollBar();

		int m_AutoScroll;
		int m_selected;
		int m_itemHeight;
		int m_listOffset;
		int m_properties;
		std::vector<std::string> m_stringList;
		ifont* m_font;
		PBScrollBar* m_pScrollBar;
};

#endif /* __PBLISTBOX_H__ */
