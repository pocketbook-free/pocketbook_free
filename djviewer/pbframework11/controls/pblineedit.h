#ifndef __PBLINEEDIT_H__
#define __PBLINEEDIT_H__

#include "../framework/pbwindow.h"

#define PBEDIT_MAXENTER 255
#define PBEDIT_BORDER 2

typedef enum
{
    PBES_TEXT,
    PBES_PHONE,
    PBES_NUMERIC,
    PBES_IPADDR,
    PBES_URL,
    PBES_DATETIME
} PBENTERSTYPE;

class PBLineEdit : public PBWindow
{
public:
	PBLineEdit();
	~PBLineEdit();

	bool Create(PBWindow* pParent, int left, int top, int width, int height, PBENTERSTYPE style);


	void setText(const char *text);
	const char *text() const;

	int MainHandler(int type, int par1, int par2);
	static PBLineEdit *GetThis();
	static PBLineEdit *s_pThis;

	void EntText_Handler(char *text);
	static void enttext_handler(char *text);

protected:
private:
	int OnDraw(bool force);

	PBENTERSTYPE m_enterstyle;
	char m_text[PBEDIT_MAXENTER + 1];
	ifont *m_font;
};

#endif /* __PBLINEEDIT_H__ */
