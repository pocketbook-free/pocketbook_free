#include "pbmainframe.h"
#include "main.h"
#include <pbframework/pbframework.h>

#include "pbpagehistorynavigation.h"

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
	extern const ibitmap exitApp;
}
    extern bool doRestart;
    extern char ** argv_main;
    extern int argc_main;
#ifdef USESYNOPSIS

SynopsisContent *Content;
SynopsisNote *Note;
SynopsisBookmark *Bookmark;
TSynopsisWordList List;
SynopsisTOC m_TOC;

#endif

PBMainFrame* PBMainFrame::s_pThis = NULL;

PBMainFrame::PBMainFrame(char *filename)
#if defined(PLATFORM_NX)
	: m_OldMenuhandler("pdfviewer")
#endif
{
    s_pThis = this;
    m_fileName = filename;
    memset(m_kbdbuffer, 0, sizeof(m_kbdbuffer));

    m_KeyboardHandler.setKeyboardMapping(KEYMAPPING_PDF);
    m_PointerHandler.registrationRunner(this);
    m_KeyboardHandler.registrationRunner(this);

#if defined(PLATFORM_NX)
    m_OldMenuhandler.registrationRunner(this);
#endif
    m_pageHistory = new PBPageNavigatorMenu(this);
// crazy change keymaping // ->
    m_keyEvent = false;
// crazy change keymaping // <-
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
    return s_pThis;
}

int PBMainFrame::main_handler(int type, int par1, int par2)
{
        //if (type == EVT_KEYRELEASE && par1 == KEY_MENU)
        if (type == EVT_CONFIGCHANGED)
        {
            CloseApp();
            doRestart = true;
            return 0;
        }
	PBMainFrame* pThis = PBMainFrame::GetThis();
#ifdef SYNOPSISV2
	if (pThis->m_ToolBar.GetTool())
	{
		pThis->m_ToolBar.Note_Handler(type, par1, par2);
	}
	else
	{
		pThis->MainHandler(type, par1, par2);
	}
#else
	pThis->MainHandler(type, par1, par2);
#endif
	return 0;
}

int PBMainFrame::MainHandler(int type, int par1, int par2)
{
	int res = 0;

#if defined(PLATFORM_FC)
	// если открыть хоть один диалог, используем его хендлер и дальше событие не передаем
	for (TWindowListIt it = m_Children.begin(); it != m_Children.end(); it++)
	{
		if ((*it)->IsVisible())
		{
			(*it)->MainHandler(type, par1, par2);
			res = 1;
			break;
		}
	}
#endif

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
			main_prevpage();
			break;
		case EVT_NEXTPAGE:
			main_nextpage();
			break;
		case EVT_PANEL_TEXT:
			if (par1 < 4000)
			{
				onPageSelect();
			}
			else if (par1 > 6000)
			{
				onZoomerOpen();
			}
			PartialUpdateBW(0, 0, ScreenWidth(), ScreenHeight());
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

	//if (!res && pointer_handler(type, par1, par2))
	//	res = 1;

// crazy change keymaping // ->
	m_keyEvent = ISKEYEVENT(type);
// crazy change keymaping // <-

	if (!res && m_PointerHandler.HandleEvent(type, par1, par2))
		res = 1;

	if (!res && m_KeyboardHandler.HandleEvent(type, par1, par2))
		res = 1;

	ReDraw();
	return res;
}

int PBMainFrame::onInit()
{
#if defined(PLATFORM_FC)
	int x = (ScreenWidth() - QUICK_MENU_SIZE)/2;
	int y = (ScreenHeight() - QUICK_MENU_SIZE)/2;
	m_menu.Create(this, x, y, QUICK_MENU_SIZE, QUICK_MENU_SIZE, 0, 0);

	if (HWC_KEYBOARD_POCKET360) {
	  m_menu.CreateButton(0, 0, CMD_contentsID, &contents, GetLangText("@Contents"));
	  m_menu.CreateButton(0, 1, CMD_exitAppID, &exitApp, GetLangText("@Exit"));
	  m_menu.CreateButton(0, 2, CMD_searchID, &search, GetLangText("@Search"), false);
	  m_menu.CreateButton(1, 0, CMD_noteID, &note, GetLangText("@Note"));
	  m_menu.CreateButton(1, 1, CMD_bookmarkID, &bookmark, GetLangText("@Bookmark"), false);
	  m_menu.CreateButton(1, 2, CMD_pageID, &gotoPage, GetLangText("@Goto_page"));
	  m_menu.CreateButton(2, 0, CMD_ZOOM_DLG, &zoom, GetLangText("@Zoom"));
	  m_menu.CreateButton(2, 1, CMD_dicID, &dictionary, GetLangText("@Dictionary"));
	  m_menu.CreateButton(2, 2, CMD_rotateID, &pbRotate, GetLangText("@Rotate"));
	} else {
	m_menu.CreateButton(0, 0, CMD_searchID, &search, GetLangText("@Search"), false);
	m_menu.CreateButton(0, 1, CMD_contentsID, &contents, GetLangText("@Contents"));
	m_menu.CreateButton(0, 2, CMD_ttsID, &tts, GetLangText("@Tts"));
	m_menu.CreateButton(1, 0, CMD_noteID, &note, GetLangText("@Note"));
	m_menu.CreateButton(1, 1, CMD_bookmarkID, &bookmark, GetLangText("@Bookmark"), false);
	m_menu.CreateButton(1, 2, CMD_pageID, &gotoPage, GetLangText("@Goto_page"));
	m_menu.CreateButton(2, 0, CMD_ZOOM_DLG, &zoom, GetLangText("@Zoom"));
	m_menu.CreateButton(2, 1, CMD_dicID, &dictionary, GetLangText("@Dictionary"));
	m_menu.CreateButton(2, 2, CMD_rotateID, &pbRotate, GetLangText("@Rotate"));
	}
#endif

#ifdef SYNOPSISV2
	m_ToolBar.Init(&m_TOC);
#endif

	SetVisible(true);
	set_panel_height();
	return 1;
}

void PBMainFrame::InvertRectange(int screen, std::vector<PBRect> &rectList)
{
	if (screen >= 0 && screen < get_npage())
	{
		if (screen != (get_cpage() - 1))
		{
			//set_page(screen + 1);
			//make_update();
			page_selected(screen + 1);
		}
		else
		{
			MakeInvert(m_prevRect);
		}
		MakeInvert(rectList);
		m_prevRect = rectList;
	}
}

bool PBMainFrame::GetText(int screen, std::vector<PBWordInfo> &dst)
{
	bool restore_page = false;
	bool result = false;
	int iter;
	iv_wlist* wlist;
	int wlist_len;

	dst.clear();
	if (screen >= 0 && screen < get_npage())
	{
		if (get_page_word_list(&wlist, &wlist_len, screen + 1) &&
			wlist_len)
		{
			iter = 0;
			while (iter < wlist_len)
			{
				dst.push_back(PBWordInfo(screen, wlist[iter].word, wlist[iter].x1,  wlist[iter].y1,  wlist[iter].x2,  wlist[iter].y2));
				iter++;
			}
		}
		result = true;
	}
	return result;
}

int PBMainFrame::onMenu()
{
#if defined(PLATFORM_FC)
	m_menu.Show((ScreenWidth() - QUICK_MENU_SIZE)/2, (ScreenHeight() - QUICK_MENU_SIZE)/2);
#else
	m_OldMenuhandler.openMenu();
#endif
    return 0;
}

int PBMainFrame::onShow()
{
#ifdef SYNOPSISV2
	if (GetEventHandler() != PBMainFrame::main_handler && m_ToolBar.GetTool() == 0)
		return 0;
#endif       
	main_show();
	return 0;
}

int PBMainFrame::onExit()
{
    fprintf(stderr, "EVT_EXIT\n");
    save_settings();
    if (doRestart)
    {
        ShowHourglass();
        FullUpdate();
        switch (argc_main)
        {
        case 1:
            execl(argv_main[0], argv_main[0], 0, 0, 0);
            break;
        case 2:
            execl(argv_main[0], argv_main[0], argv_main[1], 0, 0);
            break;
        case 3:
            execl(argv_main[0], argv_main[0], argv_main[1], argv_main[2], 0);
            break;
        case 4:
            execl(argv_main[0], argv_main[0], argv_main[1], argv_main[2], argv_main[3]);
            break;
        }
    }

    CloseApp();
    return 0;
}

int PBMainFrame::OnCommand(int commandId, int par1, int par2)
{
    switch (commandId)
    {
	case CMD_searchID:
		onSearchStart();
		break;
	case CMD_contentsID:
		onContentsOpen();
		break;
	case CMD_ttsID:
#ifdef TTS
		if (isReflowMode()) Message(ICON_WARNING, "TTS", "Please, turn off reflow mode and run tts engine again.", 2000);
			else TTSProcessor::Init(*this, get_cpage() - 1);
#endif
		break;
	case CMD_noteID:
		onNoteNew();
		break;
	case CMD_bookmarkID:
		onBookMarkNew();
		break;
	case CMD_pageID:
		onPageSelect();
		break;
	case CMD_ZOOM_DLG:
		onZoomerOpen();
		break;
	case CMD_CHANGE_ZOOM:
		onChangeZoom(par1, par2);
		break;
	case CMD_dicID:
		onDictionaryOpen();
		break;
	case CMD_rotateID:
		onRotateOpen();
		break;
	case CMD_showID:
		onShow();
		break;
	case CMD_exitAppID:
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
        //OpenPageSelector(page_selected);
    PBPagePosition * pagePosition = new PBPagePosition(Direction(*cpage_main));

    PBScreenBuffer * buf = new PBScreenBuffer();
    buf->FromScreen(true);
    pagePosition->SetScreenBuffer(buf);
    m_pageHistory->FullShow(pagePosition);
    return 0;
}

int PBMainFrame::onContentsOpen()
{
#ifdef USESYNOPSIS
	PrepareActiveContent(1);
	m_TOC.SetBackTOCHandler(synopsis_toc_handler);
	SynopsisContent Location(0, (long long)cpage << 40, NULL);
	m_TOC.Open(&Location);
#else
	open_contents();
#endif
    return 0;
}

int PBMainFrame::onDictionaryOpen()
{
    iv_wlist* word_list;
    int word_list_len;

	if (get_page_word_list(&word_list, &word_list_len, -1))
        OpenDictionaryView(word_list, NULL);

    return 0;
}

int PBMainFrame::onPagePrev()
{
	//turn_page(-1);
	main_prevpage();
	return 0;
}

int PBMainFrame::onPageNext()
{
	//turn_page(1);
	main_nextpage();
	return 0;
}

int PBMainFrame::changeOrientaition(int n)
{
	rotate_handler(n);
	return 0;
}

void PBMainFrame::makeSnapShot()
{
    save_screenshot();
	return;
}

int PBMainFrame::onPageJNext10()
{
	jump_pages(+10);
	out_page(1);
    return 0;
}

int PBMainFrame::onPageJNext10Hold()
{
    jump_pages(+10);
    return 0;
}

int PBMainFrame::onPageJNext10Up()
{
	out_page(1);
    return 0;
}

int PBMainFrame::onPageJPrev10()
{
    jump_pages(-10);
	out_page(1);
    return 0;
}

int PBMainFrame::onPageJPrev10Hold()
{
    jump_pages(-10);
    return 0;
}

int PBMainFrame::onPageJPrev10Up()
{
	out_page(1);
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
	page_selected(1);
    return 0;
}

int PBMainFrame::onPageLast()
{
	page_selected(get_npage());
    return 0;
}

int PBMainFrame::onBookMarkOpen()
{
	open_bookmarks();
	return 0;
}

int PBMainFrame::onPageHistory()
{
    PBPagePosition * pagePosition = new PBPagePosition(Direction(*cpage_main));

        PBScreenBuffer * buf = new PBScreenBuffer();
            buf->FromScreen(true);
    pagePosition->SetScreenBuffer(buf);
    m_pageHistory->Show(pagePosition);

    return 1;
}
int PBMainFrame::onBookMarkNew()
{
#if defined(PLATFORM_FC)
	int x = (ScreenWidth() - QUICK_MENU_SIZE)/2;
	int y = (ScreenHeight() - QUICK_MENU_SIZE)/2;
	PartialUpdate(x, y, QUICK_MENU_SIZE, QUICK_MENU_SIZE);
#endif
#ifdef USESYNOPSIS
	new_synopsis_bookmark();
#else
	new_bookmark();
#endif
    return 0;
}

int PBMainFrame::onNoteNew()
{
#ifdef USESYNOPSIS
	#ifdef SYNOPSISV2
		m_ToolBar.OpenToolBar();
	#else
		new_synopsis_note();
	#endif
#else
	new_note();
#endif
    return 0;
}

int PBMainFrame::onNoteOpen()
{
	OpenNotepad(NULL);
    return 0;
}

int PBMainFrame::onNoteSave()
{
    save_page_note();
    return 0;
}

int PBMainFrame::onSearchStart()
{
#if defined(PLATFORM_FC)
	int x = (ScreenWidth() - QUICK_MENU_SIZE)/2;
	int y = (ScreenHeight() - QUICK_MENU_SIZE)/2;
	PartialUpdate(x, y, QUICK_MENU_SIZE, QUICK_MENU_SIZE);
#endif
	OpenKeyboard(GetLangText("@Search"), m_kbdbuffer, 30, 0, search_enter);
    return 0;
}

int PBMainFrame::onZoomerOpen()
{
	if (QueryTouchpanel())
	{
		#if defined(PLATFORM_NX)
		TouchZoom::open_zoomer();
		#else
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
		#endif
	}
	else
	{
//		OldZoom::open_zoomer((int)presenter->GetZoomType(), presenter->GetZoomParam());
		open_mini_zoomer();
		//open_new_zoomer();
	}

	return 0;
}

int PBMainFrame::onChangeZoom(int zoom_type, int zoom_param)
{
	apply_scale_factor(zoom_type, zoom_param);
	//PbDisplay::GetDisplay()->SetZoomParameters((TZoomType)zoom_type, zoom_param);
	return 0;
}

int PBMainFrame::onZoomIn()
{
// crazy change keymaping // ->
    if (m_keyEvent)
	onKeyUp();
    else
	zoom_in();
// crazy change keymaping // <-
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
    OpenRotateBox(rotate_handler);
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

static std::string translate(const std::string &word)
{
       const std::string key = "@" + word;
       const std::string trans = GetLangText(key.c_str());
       return (trans == key) ? word : trans;
}

static int HumanSize(char *buf, unsigned long long bsize, unsigned long long nblocks)
{
	unsigned long long value = bsize * nblocks;
	int n;

	if (value < 1000LL) {
		n = snprintf(buf, 32, "%i %s", (int)value, GetLangText("@bytes"));
	} else if (value < 100000LL) {
		value = (value * 10LL) / 1024LL;
		n = snprintf(buf, 32, "%i.%i %s", (int)(value/10LL), (int)(value%10LL), GetLangText("@kb"));
	} else if (value < 1000000LL) {
		n = snprintf(buf, 32, "%i %s", (int)(value/1024LL), GetLangText("@kb"));
	} else if (value < 100000000LL) {
		value = (value * 10LL) / (1024LL * 1024LL);
		n = snprintf(buf, 32, "%i.%i %s", (int)(value/10LL), (int)(value%10LL), GetLangText("@mb"));
	} else {
		n = snprintf(buf, 32, "%i %s", (int)(value/(1024LL * 1024LL)), GetLangText("@mb"));
	}
	return n;
}

static int HumanDate(char *buf, time_t t)
{
	time_t tt = t;
	struct tm *ctm = localtime(&tt);

	return sprintf(buf, "%s  %02i:%02i", DateStr(t), ctm->tm_hour, ctm->tm_min);
}

void PBMainFrame::ShowFileInfo(const char *path)
{

	//PBFileManager::FM_ShowItemInfo
    //fprintf(stderr, "ShowFileInfo: <%s>\n", path);
	char buf[2048], sbuf[128], *p, *pp;
	int i, n, size;

	bookinfo *info = GetBookInfo(path);
	if (info == NULL) {
		Message(ICON_ERROR, "@Error", "@Cant_get_info", 2000);
		return;
	}
	size = sizeof(buf)-1;
	n = 0;

	p = strdup(path);
	pp = strrchr(p, '/');
	n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@Filename"), pp ? pp+1 : p);

	if (pp != NULL)
		*pp = '\0';

	n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@FileLocation"), p + strlen(FLASHDIR)); //TODO: check if it must be under "if"

	free(p);

	if (info->typedesc && info->typedesc[0]) {
	  n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@Type"), GetLangText(info->typedesc));
	}

	if (info->title && info->title[0]) {
	  n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@Title"), info->title);
	}

	if (info->author && info->author[0]) {
	  n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@Author"), info->author);
	}

	if (info->series && info->series[0]) {
	  n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@Series"), info->series);
	}

	for (i=0; i<10; i++) {
	  if (info->genre[i] == NULL) break;
	  if (info->genre[i][0]) {
		  n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@Genre"), translate(info->genre[i]).c_str());
	  }
	}

	HumanSize(sbuf, 1, info->size);
	n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@Size"), sbuf);

	HumanDate(sbuf, info->ctime);
	n += snprintf(buf+n, size-n, "%s:  %s\n", GetLangText("@Written"), sbuf);

	Message(ICON_INFORMATION, "@Info", buf, 30000);
}

int PBMainFrame::onAboutOpen()
{
    const char *path = m_fileName.c_str();
    ShowFileInfo(path);
    return 1;
}

int PBMainFrame::onPdfMode()
{
	pdf_mode();
	return 0;
}

int PBMainFrame::onTouchClick(int x, int y)
{
	// overloaded to avoid warnings
	return 0;
}

// crazy change keymaping // ->

int PBMainFrame::onLinkOpen()
{
	return onKeyDown();
}

int PBMainFrame::onKeyUp()
{
	find_off(-1);
	out_page(1);
	return 1;
}

int PBMainFrame::onKeyDown()
{
	find_off(+1);
	out_page(1);
	return 1;
}

// crazy change keymaping // <-

#ifdef SYNOPSISV2

void PBMainFrame::DrawPen()
{
	int zoom_type;
	int zoom_param;
	get_zoom_param(zoom_type, zoom_param);

	if (zoom_type == ZoomTypeEpub || zoom_type == ZoomTypeReflow)
		return;

	if (zoom_type == ZoomTypeFitWidth)
	{
		zoom_param = scale;
	}

	bool preview = (zoom_type == ZoomTypePreview);

	int nx = 1, ny = 1, boxx, boxy, boxw, boxh;

	if (preview)
	{
		if (Settings::IsPortrait() && zoom_param == 50)
		{
			nx = ny = 2;
		}
		else if (Settings::IsPortrait() && zoom_param == 33)
		{
			nx = ny = 3;
		}
		else if (!Settings::IsPortrait() && zoom_param == 50)
		{
			nx = 2;
			ny = 1;
		}
		else if (!Settings::IsPortrait() && zoom_param == 33)
		{
			nx = 3;
			ny = 2;
		}

		boxw = ScreenWidth() / nx;
		boxh = (ScreenHeight() - 30) / ny;
	}

	int page_num = cpage;
	int page_count = npages;

	double viewportWidth;
	double viewportHeight;
	double viewportX = 0;
	double viewportY = 0;

	for (int yy = 0; yy < ny; yy++)
	{
		for (int xx = 0; xx < nx; xx++)
		{
			if (page_num > page_count)
			{
				return;
			}

			if (preview)
			{
				boxx = xx * boxw;
				boxy = yy * boxh;

				viewportWidth = boxw - 10;
				viewportHeight = boxh - 10;
				viewportX = boxx+5;
				viewportY = boxy+5;
			}

			PBSynopsisItem *item = m_TOC.GetHeaderM(SYNOPSIS_PEN);
			PBSynopsisPen *Pen;

			while (item != NULL)
			{
				Pen = (PBSynopsisPen *)item;

				char startpos[30], endpos[30];
				PBSynopsisItem::PositionToChar((long long)page_num << 40, startpos);
				PBSynopsisItem::PositionToChar((long long)page_num << 40, endpos);

				if (Pen->Equal(startpos, endpos))
				{
					if (m_ToolBar.GetTool() == 0)
					{
						double shiftx = 0, shifty = 0;
						if (preview)
						{
							shiftx = 5;
							shifty = (scale == 33 && !is_portrait()) ? 15 : 5;
							offx = 0;
							offy = 0;
							scrx = 0;
							scry = 0;
						}

						stb_matrix navigationMatrix;
						navigationMatrix.a = navigationMatrix.d = (double)(zoom_param * ScreenWidth() / 600) / 100.;
						navigationMatrix.e = navigationMatrix.f = navigationMatrix.b = navigationMatrix.c = 0;

						stb_matrix matrix;

						matrix.a = navigationMatrix.a;
						matrix.b = 0;
						matrix.c = 0;
						matrix.d = navigationMatrix.d;
						matrix.e = -offx + viewportX + scrx - shiftx;
						matrix.f = -offy + viewportY + scry - shifty;

						Pen->OutputSVG(matrix);
					}
				}
				else
				{
//					dp::ref<dpdoc::Location> loc = Host::GetHost()->getDocument()->getLocationFromBookmark(Pen->GetPosition());
//					if (end->compare(loc) < 0)
//					{
//						break;
//					}
//					else if (zoom_type == ZoomTypeEpub || zoom_type == ZoomTypeReflow)
//					{
//						if (start->compare(loc) <= 0 && end->compare(loc) >= 0)
//						{
//							AdobePtr<dpdoc::RangeInfo> spWordPosInfo;
//							spWordPosInfo = Host::GetHost()->getRenderer()->getRangeInfo(loc, end);
//
//							double xmin, ymin, xmax, ymax;
//							PBGetPositionRect(spWordPosInfo, &xmin, &ymin, &xmax, &ymax);
//							DrawBitmap(ScreenWidth() - pencil_icon.width - 5, (ymin + ymax - pencil_icon.height) / 2, &pencil_icon);
//						}
//					}
				}
				item = item->GetBelowM(SYNOPSIS_PEN);
			}
			page_num++;
		}
	}

}

#endif
