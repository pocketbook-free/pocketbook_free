#ifndef __PBCHECKBOX_H__
#define __PBCHECKBOX_H__

#include "../framework/pbwindow.h"

class PBCheckBox : public PBWindow
{
public:
        PBCheckBox();
        ~PBCheckBox();

        int MainHandler(int type, int par1, int par2);
        
        void setCheckState(bool state);
        void setText(const char *text);
        bool isChecked() const;
        char *text() const;

	void SetFont(const char *fontname, int fontsize, int color);
	void SetProperties(int properties);
	void SetCheckRight(bool right);

protected:

private:
	int OnDraw(bool force);
        void freeData();

	int m_Properties;
	char *m_Text;
	bool m_Checked;

	char *m_pFontName;
	int m_FontSize;
	int m_TextColor;
	ifont *m_pFont;
	bool m_CheckRight;
};

#endif /* __PBCHECKBOX_H__ */
