#include "pboldmenuhandler.h"

static const char *def_menutext[9] =
{ "@Goto_page", "@Exit", "@Search", "@Bookmarks", "@Menu", "@Rotate", "@Dictionary", "@Zoom", "@Contents" };

static const char *def_menuaction[9] =
{ "@KA_goto", "@KA_exit", "@KA_srch", "@KA_obmk", "@KA_none", "@KA_rtte", "@KA_dict", "@KA_zoom", "@KA_cnts" };

//static char *keyact0[32], *keyact1[32];

static const struct
{
		const char *action;
		void (*f1)();
		void (*f2)();
		void (*f3)();
} KA[] =
{
{ "@KA_goto", PBOldMenuHandler::open_pageselector, NULL, NULL },
{ "@KA_obmk", PBOldMenuHandler::open_bookmarks, NULL, NULL },
{ "@KA_nbmk", PBOldMenuHandler::open_bookmarks, NULL, NULL },
{ "@KA_olnk", PBOldMenuHandler::open_links, NULL, NULL },
{ "@KA_cnts", PBOldMenuHandler::open_contents, NULL, NULL },
{ "@KA_srch", PBOldMenuHandler::start_search, NULL, NULL },
{ "@KA_dict", PBOldMenuHandler::open_dictionary, NULL, NULL },
{ "@KA_zoom", PBOldMenuHandler::open_zoomer, NULL, NULL },
{ "@KA_rtte", PBOldMenuHandler::open_rotate, NULL, NULL },
{ "@KA_exit", PBOldMenuHandler::exit_reader, NULL, NULL },
{ "@KA_nnot", PBOldMenuHandler::open_notes, NULL, NULL },
{ "@KA_onot", PBOldMenuHandler::open_notes, NULL, NULL },
{ NULL, NULL, NULL, NULL }
};

PBOldMenuHandler* PBOldMenuHandler::s_pThis = NULL;

/*
PBOldMenuHandler::PBOldMenuHandler()
{
	s_pThis = this;
	m_pStarter = NULL;
	m_ReaderName = "pdfviewer";

	char buf[1024];
//	snprintf(buf, sizeof(buf), "viewermenu_%s", ReadString(GetGlobalConfig(), "language", "en"));
	snprintf(buf, sizeof(buf), "%s_menu", readername);
	m_pMenu3x3Bitmap = GetResource(buf, NULL);
//	if (m_pMenu3x3Bitmap == NULL) m_pMenu3x3Bitmap = GetResource("viewermenu_en", NULL);

	for (int i = 0; i < 9; ++i)
	{
		sprintf(buf, "qmenu.pdfviewer.%i.text", i);
		m_strings3x3[i] = GetThemeString(buf, (char*)def_menutext[i]);
	}
}
*/
PBOldMenuHandler::PBOldMenuHandler(const char *readername)
{
	s_pThis = this;
	m_pStarter = NULL;
	m_ReaderName = readername;

	char buf[1024];
//	snprintf(buf, sizeof(buf), "viewermenu_%s", ReadString(GetGlobalConfig(), "language", "en"));
	snprintf(buf, sizeof(buf), "%s_menu", readername);
	m_pMenu3x3Bitmap = GetResource(buf, NULL);
//	if (m_pMenu3x3Bitmap == NULL) m_pMenu3x3Bitmap = GetResource("viewermenu_en", NULL);

	for (int i = 0; i < 9; ++i)
	{
		sprintf(buf, "qmenu.%s.%i.text", m_ReaderName.c_str(), i);
		m_strings3x3[i] = GetThemeString(buf, (char*)def_menutext[i]);
	}
}

PBOldMenuHandler* PBOldMenuHandler::GetThis()
{
	return s_pThis;
}

int PBOldMenuHandler::registrationRunner(PBIEventHandler *pStarter)
{
	m_pStarter = pStarter;
	return 0;
}

void PBOldMenuHandler::openMenu()
{
	OpenMenu3x3(m_pMenu3x3Bitmap, (const char **)m_strings3x3, menu_handler);
}

void PBOldMenuHandler::menu_handler(int pos)
{
	if (pos >= 0)
	{
		char buf[32];
		sprintf(buf, "qmenu.%s.%i.action", s_pThis->m_ReaderName.c_str(), pos);

		char* act = GetThemeString(buf, (char*)def_menuaction[pos]);

		//fprintf(stderr, "munu handler. pos - %d, act - %s, def - %s", pos, act, (char*)def_menuaction[pos]);

		for (int i = 0; KA[i].action != NULL; i++)
		{
			if (strcmp(act, KA[i].action) == 0)
			{
				if (KA[i].f1 != NULL)
					(KA[i].f1)();
				if (KA[i].f3 != NULL)
					(KA[i].f3)();
				break;
			}
		}
	}
}

void PBOldMenuHandler::exit_reader()
{
	PBOldMenuHandler::GetThis()->m_pStarter->onExit();
	CloseApp();
}

void PBOldMenuHandler::open_zoomer()
{
	PBOldMenuHandler::GetThis()->m_pStarter->onZoomerOpen();
}

void PBOldMenuHandler::open_rotate()
{
	PBOldMenuHandler::GetThis()->m_pStarter->onRotateOpen();
}

void PBOldMenuHandler::open_pageselector()
{
	PBOldMenuHandler::GetThis()->m_pStarter->onPageSelect();
}

void PBOldMenuHandler::open_contents()
{
	PBOldMenuHandler::GetThis()->m_pStarter->onContentsOpen();
}

void PBOldMenuHandler::start_search()
{
	PBOldMenuHandler::GetThis()->m_pStarter->onSearchStart();
}

void PBOldMenuHandler::open_bookmarks()
{
	PBOldMenuHandler::GetThis()->m_pStarter->onBookMarkOpen();
}

void PBOldMenuHandler::open_notes()
{
	PBOldMenuHandler::GetThis()->m_pStarter->onNoteNew();
}

void PBOldMenuHandler::open_links()
{
	PBOldMenuHandler::GetThis()->m_pStarter->onLinkOpen();
}

void PBOldMenuHandler::open_dictionary()
{
	PBOldMenuHandler::GetThis()->m_pStarter->onDictionaryOpen();
}
