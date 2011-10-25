#ifndef PBOLDMENUHANDLER_H
#define PBOLDMENUHANDLER_H

#include "inkview.h"
#include "inkinternal.h"
//#include "settings.h"
#include "pbieventhandler.h"

class PBOldMenuHandler
{
public:
//    PBOldMenuHandler();
    PBOldMenuHandler(const char *readername);

	void openMenu();
	static PBOldMenuHandler* GetThis();
	int registrationRunner(PBIEventHandler *pStarter);

protected:
	static void menu_handler(int pos);

public:
	static void exit_reader();
	static void open_zoomer();
	static void open_rotate();
	static void open_pageselector();
	static void open_contents();
	static void start_search();
	static void open_bookmarks();
	static void open_notes();
	static void open_links();
	static void open_dictionary();


protected:
	PBIEventHandler *m_pStarter;
	static PBOldMenuHandler *s_pThis;
	ibitmap *m_pMenu3x3Bitmap;
	char *m_strings3x3[9];
	std::string m_ReaderName;
};

#endif // PBOLDMENUHANDLER_H
