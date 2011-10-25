#ifndef SETTINGS_H
#define SETTINGS_H


#include "inkview.h"
#include <string>
#include <time.h>
#include <assert.h>

#include "../../pbframework11/framework/pbstring.h"
#include "../../pbframework11/framework/pbsingleton.h"
#include "../../pbframework11/framework/pbptr.h"
#include "../../pbframework11/framework/pbobject.h"
#include "../../pbframework11/framework/pbrect.h"
#include "../../pbframework11/framework/pbutilities.h"
#include "../../pbframework11/framework/pbpath.h"

#define PB_KEY_UP		0x11
#define PB_KEY_DOWN		0x12
//#define PB_KEY_LEFT		0x13
//#define PB_KEY_RIGHT	0x14

#undef KEY_UP
#undef KEY_DOWN
//#undef KEY_LEFT
//#undef KEY_RIGHT

static const char* APPTITLE = "DjVu Viewer";

// command id's
const int CMD_searchID          =   100;
const int CMD_bookmarkID        =   101;
const int CMD_contentsID        =   102;
const int CMD_noteID            =   103;
const int CMD_cancelID          =   104;
const int CMD_pageID            =   105;
const int CMD_dicID             =   107;
const int CMD_rotateID          =   108;
const int CMD_fontszID          =   109;
const int CMD_ttsID				=   110;
const int CMD_ZOOM_DLG			=   111;
const int CMD_showID			=	112;
const int CMD_CHANGE_ZOOM		=   113;

enum TZoomType
{
	ZoomTypePreview = 0, 	// preview
	ZoomTypeFitWidth = 1, 	// fit width
	ZoomTypeNormal = 2, 	// normal
	ZoomTypeColumns = 3, 	// columns
	ZoomTypeReflow = 4, 	// reflow
	ZoomTypeEpub = 5,		// EPUB
	ZoomTypeFit = 6, 		// fit page
	ZoomTypeEmpty = 999
};

class Settings
{
public:
	static const double extraMarginVert = 0;
	static const double extraMarginHor = 0;
	static const double dpi = 166.0;
	static const bool verbose = false;
	static const bool monochrome = true;
	static const bool alpha = false;

	static bool registered;
	static iconfig *gcfg;
	static int invertupdate;
	static int upd_count;

	static void ReadConfig()
	{
		//registered = isRegistered();
		registered = true;

		if (gcfg == NULL)
		{
			gcfg = GetGlobalConfig();
		}

		if (gcfg)
		{
			invertupdate = ReadInt(gcfg, "invertupdate", 1);
		}
	}

	static double GetViewportWidth()
	{
		return ScreenWidth() - 2*Settings::extraMarginHor;
	};

	static double GetViewportHeight()
	{
		return ScreenHeight() - 2*Settings::extraMarginVert - PanelHeight();
	};

	static bool IsPortrait()
	{
		int orientation = GetOrientation();
		return orientation == ROTATE0 || orientation == ROTATE180;
	}

	static void PrintWithTimestamp(const char* message)
	{
		time_t now;
		time(&now);
		fprintf(stderr, "%s - %s\n", asctime(localtime(&now)), message);
	}

	// invertupdate:
	// 0 - newer
	// 1 - always
	// 5, 10 ... - every N pages
	static int GetNextUpdateType()
	{
		int res = 1; // full update

		if (invertupdate == 0 || ++upd_count < invertupdate)
			res = 0;

		if (upd_count >= invertupdate)
			upd_count = 0;

		return res;
	}
};


#endif // SETTINGS_H
