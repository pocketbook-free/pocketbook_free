#include "pbmainframe.h"
#include "main.h"

extern "C"
{
    extern const ibitmap search;
    extern const ibitmap contents;
    extern const ibitmap tts;
    extern const ibitmap note;
    extern const ibitmap bookmark;
    extern const ibitmap gotoPage;
    extern const ibitmap settings;
    extern const ibitmap dictionary;
    extern const ibitmap pbRotate;
    extern const ibitmap zoom;
    extern const ibitmap cancel;
    extern const ibitmap exitApp;
}

const int def_searchID          =   100;
const int def_bookmarkID        =   101;
const int def_contentsID        =   102;
const int def_noteID            =   103;
const int def_cancelID          =   104;
const int def_pageID            =   105;
const int def_zoomID		=   106;
const int def_dicID             =   107;
const int def_rotateID          =   108;
const int def_fontszID          =   109;
const int def_ExitID		=   123;
const int def_exitAppID         =   112;

PBMainFrame* PBMainFrame::s_pThis = NULL;

PBMainFrame::PBMainFrame()
{
    memset(m_kbdbuffer, 0, sizeof(m_kbdbuffer));

    m_KeyboardHandler.setKeyboardMapping(KEYMAPPING_PDF);
    m_PointerHandler.registrationRunner(this);
    m_KeyboardHandler.registrationRunner(this);
}

PBMainFrame::~PBMainFrame()
{
}

void PBMainFrame::Run()
{
    InkViewMain(main_handler);
}

PBMainFrame* PBMainFrame::GetThis()
{
    if (s_pThis == NULL)
    {
        s_pThis = new PBMainFrame;
    }
    return s_pThis;
}

int PBMainFrame::main_handler(int type, int par1, int par2)
{
    PBMainFrame* pThis = PBMainFrame::GetThis();
    pThis->MainHandler(type, par1, par2);

    return 0;
}

int PBMainFrame::MainHandler(int type, int par1, int par2)
{
	int res = 0;

	for (TWindowListIt it = m_Children.begin(); it != m_Children.end(); it++)
	{
		if ((*it)->IsVisible())
		{
			(*it)->MainHandler(type, par1, par2);
			res = 1;
			break;
		}
	}

	if (!res)
	{
		switch (type)
		{
		case EVT_INIT:
			onInit();
			break;
		case EVT_SHOW:
			onShow();
			break;
		case EVT_EXIT:
			onExit();
			break;
		case EVT_PREVPAGE:
			onPagePrev();
			break;
		case EVT_NEXTPAGE:
			onPageNext();
			break;
		case EVT_PANEL_TEXT:
			onPageSelect();
			break;
		case EVT_PANEL_PROGRESS:
			onContentsOpen();
			break;
		case EVT_OPENDIC:
			onDictionaryOpen();
			break;
		case EVT_ORIENTATION:
			changeOrientaition(par1);
			break;
		case EVT_SNAPSHOT:
			makeSnapShot();
			break;
		default:
			break;
		}
	}

	if (!res && m_PointerHandler.HandleEvent(type, par1, par2))
		res = 1;

	if (!res && m_KeyboardHandler.HandleEvent(type, par1, par2))
		res = 1;

	// draw visible controls (menu, dialogs, etc.)
	ReDraw();
	return res;
}

int PBMainFrame::onInit()
{
	int x = (ScreenWidth() - QUICK_MENU_SIZE)/2;
	int y = (ScreenHeight() - QUICK_MENU_SIZE)/2;
	m_menu.Create(this, x, y, QUICK_MENU_SIZE, QUICK_MENU_SIZE, 0, 0);

	m_menu.CreateButton(0, 0, def_searchID, &search, GetLangText("@Search"), false);
	m_menu.CreateButton(0, 1, def_contentsID, &contents, GetLangText("@Contents"));
	m_menu.CreateButton(0, 2, def_bookmarkID, &bookmark, GetLangText("@Bookmark"), false);
	m_menu.CreateButton(1, 0, def_noteID, &note, GetLangText("@Note"));
	m_menu.CreateButton(1, 1, def_ExitID, &cancel, GetLangText("@Cancel"));
	m_menu.CreateButton(1, 2, def_pageID, &gotoPage, GetLangText("@Goto_page"));
	m_menu.CreateButton(2, 0, def_zoomID, &zoom, GetLangText("@Zoom"));
	m_menu.CreateButton(2, 1, def_dicID, &dictionary, GetLangText("@Dictionary"));
	m_menu.CreateButton(2, 2, def_rotateID, &pbRotate, GetLangText("@Rotate"));

	SetJoystickKeyMapping(false);
	SetVisible(true);

    return 1;
}

void PBMainFrame::SetJoystickKeyMapping(bool mapping_allowed)
{
	m_KeyboardHandler.keyMappingJoystick(mapping_allowed);
}

void PBMainFrame::InvertRectange(int /*screen*/, std::vector<PBRect> &/*rectList*/)
{
}

bool PBMainFrame::GetText(int /*screen*/, std::vector<PBWordInfo> &/*dst*/)
{
	return false;
}

int PBMainFrame::onMenu()
{
    m_menu.Show((ScreenWidth() - QUICK_MENU_SIZE)/2, (ScreenHeight() - QUICK_MENU_SIZE)/2);
    return 0;
}

int PBMainFrame::onShow()
{
	main_show();
    return 0;
}

int PBMainFrame::onExit()
{
    fprintf(stderr, "EVT_EXIT\n");
	save_settings();
    return 0;
}

int PBMainFrame::OnCommand(int commandId, int par1, int par2)
{
    switch (commandId)
    {
	case def_searchID:
		onSearchStart();
		break;
	case def_contentsID:
		onContentsOpen();
		break;
	case def_noteID:
		onNoteNew();
		break;
	case def_bookmarkID:
		onBookMarkOpen();
		break;
	case def_pageID:
		onPageSelect();
		break;
	case def_zoomID:
		onZoomerOpen();
		break;
	case CMD_CHANGE_ZOOM:
		onChangeZoom(par1, par2);
		break;
	case def_dicID:
		onDictionaryOpen();
		break;
	case def_rotateID:
		onRotateOpen();
		break;
	case def_exitAppID:
		CloseApp();
		break;
	default:
		break;
    }

    return 0;
}


int PBMainFrame::OnDraw(bool force)
{
	// not implemented
	if (IsVisible() && (IsChanged() || force) )
	{
		// drawing code
		// ...
	}
	return 0;
}


int PBMainFrame::onPageSelect()
{
	open_pageselector();
    return 0;
}

int PBMainFrame::onContentsOpen()
{
#ifdef USESYNOPSIS
	open_synopsis_contents();
#else
	open_contents();
#endif
    return 0;
}

int PBMainFrame::onDictionaryOpen()
{
	open_dictionary();
    return 0;
}

int PBMainFrame::onKeyLeft()
{
	handle_navikey(KEY_LEFT);
	return 0;
}

int PBMainFrame::onKeyRight()
{
	handle_navikey(KEY_RIGHT);
	return 0;
}

int PBMainFrame::onKeyUp()
{
	handle_navikey(PB_KEY_UP);
	return 0;
}

int PBMainFrame::onKeyDown()
{
	handle_navikey(PB_KEY_DOWN);
	return 0;
}

int PBMainFrame::onPagePrev()
{
	handle_navikey(KEY_LEFT);
    return 0;
}

int PBMainFrame::onPageNext()
{
	handle_navikey(KEY_RIGHT);
    return 0;
}

int PBMainFrame::changeOrientaition(int n)
{
	return ornevt_handler(n);
}

void PBMainFrame::makeSnapShot()
{
}

int PBMainFrame::onPageJNext10()
{
    jump_pages(+10);
    stop_jump();
    return 0;
}

int PBMainFrame::onPageJNext10Hold()
{
    jump_pages(+10);
    return 0;
}

int PBMainFrame::onPageJNext10Up()
{
    stop_jump();
    return 0;
}

int PBMainFrame::onPageJPrev10()
{
    jump_pages(-10);
    stop_jump();
    return 0;
}

int PBMainFrame::onPageJPrev10Hold()
{
    jump_pages(-10);
    return 0;
}

int PBMainFrame::onPageJPrev10Up()
{
    stop_jump();
    return 0;
}

int PBMainFrame::onBack()
{
    onPagePrev();
    return 0;
}

int PBMainFrame::onForward()
{
    onPageNext();
    return 0;
}

int PBMainFrame::onPageFirst()
{
	turn_page(1);
    return 0;
}

int PBMainFrame::onPageLast()
{
	turn_page(999999);
    return 0;
}

int PBMainFrame::onNextSelection()
{
    return 0;
}

int PBMainFrame::onPrevSelection()
{
    return 0;
}

int PBMainFrame::onBookMarkOpen()
{
#ifdef USESYNOPSIS
	//new_synopsis_bookmark();
	onBookMarkNew();
#else
	//open_bookmarks();
	open_bookmarks();
#endif
    return 0;
}

int PBMainFrame::onBookMarkNew()
{
#ifdef USESYNOPSIS
	int x = (ScreenWidth() - QUICK_MENU_SIZE)/2;
	int y = (ScreenHeight() - QUICK_MENU_SIZE)/2;
	PartialUpdate(x, y, QUICK_MENU_SIZE, QUICK_MENU_SIZE);
	new_synopsis_bookmark();
#else
	//new_bookmark();
	new_bookmark();
#endif
	return 0;
}

int PBMainFrame::onNoteNew()
{
#ifdef USESYNOPSIS
	new_synopsis_note();
#else
	new_note();
#endif
    return 0;
}

int PBMainFrame::onNoteOpen()
{
#ifdef USESYNOPSIS
	new_synopsis_note();
#else
	open_notes();
#endif
    return 0;
}

int PBMainFrame::onNoteSave()
{
    save_page_note();
    return 0;
}

int PBMainFrame::onLinkOpen()
{
    return 0;
}

int PBMainFrame::onLinkBack()
{
    return 0;
}

int PBMainFrame::onSearchStart()
{
	int x = (ScreenWidth() - QUICK_MENU_SIZE)/2;
	int y = (ScreenHeight() - QUICK_MENU_SIZE)/2;
	PartialUpdate(x, y, QUICK_MENU_SIZE, QUICK_MENU_SIZE);
	start_search();
    return 0;
}

int PBMainFrame::onZoomIn()
{
	zoom_in();
    return 0;
}

int PBMainFrame::onZoomOut()
{
	zoom_out();
    return 0;
}

int PBMainFrame::onPanelHide()
{
    show_hide_panel();
    return 0;
}

int PBMainFrame::onRotateOpen()
{
	open_rotate();
    return 0;
}

int PBMainFrame::onMp3Open()
{
    OpenPlayer();
    return 0;
}

int PBMainFrame::onMp3Pause()
{
    TogglePlaying();
    return 0;
}

int PBMainFrame::onVolumeUp()
{
    int r = GetVolume();
    SetVolume(r + 3);
    return 0;
}

int PBMainFrame::onVolumeDown()
{
    int r = GetVolume();
    SetVolume(r - 3);
    return 0;
}

int PBMainFrame::onConfigOpen()
{
    return 0;
}

int PBMainFrame::onAboutOpen()
{
    return 0;
}

int PBMainFrame::onZoomerOpen()
{
	if (QueryTouchpanel())
	{
		int zoom_type;
		int zoom_param;
		get_zoom_param(zoom_type, zoom_param);


		m_touchZoomDlg.Create(this,
							  (ScreenWidth() - PBTouchZoomDlg::WND_WIDTH)/2,
							  (ScreenHeight() - PBTouchZoomDlg::WND_HEIGHT)/2,
							  PBTouchZoomDlg::WND_WIDTH,
							  PBTouchZoomDlg::WND_HEIGHT,
							  PBWS_VISIBLE, 0);

		m_touchZoomDlg.Show((ScreenWidth() - PBTouchZoomDlg::WND_WIDTH)/2,
							(ScreenHeight() - PBTouchZoomDlg::WND_HEIGHT)/2,
							(TZoomType)zoom_type,
							zoom_param);
	}
	else
	{
		open_new_zoomer();
	}

	return 0;
}

int PBMainFrame::onChangeZoom(int zoom_type, int zoom_param)
{
	apply_scale_factor(zoom_type, zoom_param);
	return 0;
}
