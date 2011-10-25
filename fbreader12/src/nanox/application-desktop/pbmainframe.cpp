#include "pbmainframe.h"
#include "main.h"
#include "pbnotes.h"
#include "inkview.h"
#include "inkinternal.h"
#include "pbpagehistorynavigation.h"
#include "inkview.h"
#include "wordnavigator.h"
#include <map>
#include <list>
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
    extern const ibitmap exitApp;
}

const int def_searchID          =   100;
const int def_bookmarkID        =   101;
const int def_contentsID        =   102;
const int def_noteID            =   103;
const int def_cancelID          =   104;
const int def_pageID            =   105;
const int def_zoomID            =   106;
const int def_dicID             =   107;
const int def_rotateID          =   108;
const int def_fontszID          =   109;
const int def_ttsID		=   110;
const int def_settingsID	=   111;
const int def_exitAppID         =   112;

const int def_ttsVolUpID	=   120;
const int def_ttsVolDownID	=   121;
const int def_ttsPauseID        =   122;
const int def_ttsExitID		=   123;

extern int Tool;
static CLines currentText;
   extern bool doRestart;
PBMainFrame* PBMainFrame::s_pThis = NULL;

PBMainFrame::PBMainFrame()
{
    m_menu = 0;    
    memset(m_kbdbuffer, 0, sizeof(m_kbdbuffer));

    m_PointerHandler.registrationRunner(this);
    m_KeyboardHandler.registrationRunner(this);
    if (!s_pThis)
    {
        s_pThis = this;
    }
    m_pageHistory = new PBPageNavigatorMenu(this);
}

PBMainFrame::~PBMainFrame()
{
    //delete m_menu;
}

void PBMainFrame::Run()
{    
    QuickMenuInit();
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
    if (type == EVT_CONFIGCHANGED)
    {
        CloseApp();
        doRestart = true;        
        return 0;
    }
    if (Tool)
    {
	pThis->NoteHandler(type, par1, par2);
    }
    else
    {
	pThis->MainHandler(type, par1, par2);
    }

    return 0;
}
void PBMainFrame::QuickMenuInit()
{
    if (m_menu)
        delete m_menu;
    m_menu = new PBQuickMenu;
}
bool PBMainFrame::is_menu_visible()
{
	PBMainFrame* pThis = PBMainFrame::GetThis();
	if (pThis)
                return pThis->m_menu->IsVisible();
	return false;
}
int PBMainFrame::MainHandler(int type, int par1, int par2)
{
	int res = 0; 

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

	if (!res && pointer_handler(type, par1, par2))
		res = 1;

	if (!res && m_PointerHandler.HandleEvent(type, par1, par2))
		res = 1;

	if (!res && m_KeyboardHandler.HandleEvent(type, par1, par2))
		res = 1;

	ReDraw();
	return res;
}
extern long long cpos; //from main.cpp
int PBMainFrame::onPageHistory()
{
    PBPagePosition * pagePosition = new PBPagePosition(Direction(position_to_page(cpos)));

        PBScreenBuffer * buf = new PBScreenBuffer();
            buf->FromScreen(true);
    pagePosition->SetScreenBuffer(buf);
    m_pageHistory->Show(pagePosition);

    return 1;
}

int PBMainFrame::NoteHandler(int type, int par1, int par2)
{
	int res = 0;

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

	if (!res && Note_Handler(type, par1, par2))
		res = 1;

	ReDraw();
	return res;
}

int PBMainFrame::onInit()
{
        m_Children.clear();
        AddChild(m_pageHistory);
	int x = (ScreenWidth() - QUICK_MENU_SIZE)/2;
	int y = (ScreenHeight() - QUICK_MENU_SIZE)/2;
        m_menu->Create(this, x, y, QUICK_MENU_SIZE, QUICK_MENU_SIZE, 0, 0);

	if (HWC_KEYBOARD_POCKET360) {
          m_menu->CreateButton(0, 0, def_contentsID, &contents, GetLangText("@Contents"));
          m_menu->CreateButton(0, 1, def_exitAppID, &exitApp, GetLangText("@Exit"));
          m_menu->CreateButton(0, 2, def_searchID, &search, GetLangText("@Search"), false);
          m_menu->CreateButton(1, 0, def_noteID, &note, GetLangText("@Note"));
          m_menu->CreateButton(1, 1, def_bookmarkID, &bookmark, GetLangText("@Bookmark"), false);
          m_menu->CreateButton(1, 2, def_pageID, &gotoPage, GetLangText("@Goto_page"));
          m_menu->CreateButton(2, 0, def_settingsID, &settings, GetLangText("@Settings"), false);
          m_menu->CreateButton(2, 1, def_dicID, &dictionary, GetLangText("@Dictionary"));
          m_menu->CreateButton(2, 2, def_rotateID, &pbRotate, GetLangText("@Rotate"));
	} else {
          m_menu->CreateButton(0, 0, def_searchID, &search, GetLangText("@Search"), false);
          m_menu->CreateButton(0, 1, def_contentsID, &contents, GetLangText("@Contents"));
          m_menu->CreateButton(0, 2, def_ttsID, &tts, GetLangText("@Tts"));
          m_menu->CreateButton(1, 0, def_noteID, &note, GetLangText("@Note"), false);
          m_menu->CreateButton(1, 1, def_bookmarkID, &bookmark, GetLangText("@Bookmark"), false);
          m_menu->CreateButton(1, 2, def_pageID, &gotoPage, GetLangText("@Goto_page"));
          m_menu->CreateButton(2, 0, def_settingsID, &settings, GetLangText("@Settings"), false);
          m_menu->CreateButton(2, 1, def_dicID, &dictionary, GetLangText("@Dictionary"));
          m_menu->CreateButton(2, 2, def_rotateID, &pbRotate, GetLangText("@Rotate"));
	}

	SetVisible(true);
    return 1;
}

void PBMainFrame::InvertRectange(int screen, std::vector<PBRect> &rectList)
{
    if (screen >= 0 && screen < get_npage())
    {
        if (screen != (get_cpage() - 1))
        {
            set_page(screen + 1);
            make_update();
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
    int current_page, iter;
	SynWordList* wlist;
    int wlist_len;

    dst.clear();
    if (screen >= 0 && screen < get_npage())
    {
        current_page = get_cpage();

        if (screen != (current_page - 1))
        {
            set_page(screen + 1);
            restore_page = true;
        }

		if (get_page_word_list(&wlist, &wlist_len) && wlist && wlist_len)
        {
            iter = 0;
            while (iter < wlist_len)
            {
				if (wlist[iter].type != -1)
				{
					dst.push_back(PBWordInfo(screen, wlist[iter].word, wlist[iter].x1,  wlist[iter].y1,  wlist[iter].x2,  wlist[iter].y2));
				}
				else dst.push_back(PBWordInfo(screen, "", wlist[iter].x1,  wlist[iter].y1,  wlist[iter].x2,  wlist[iter].y2));
                iter++;
            }
        }
        if (restore_page)
            set_page(current_page);

        result = true;
    }
    return result;
}

int PBMainFrame::onMenu()
{
        m_menu->Show((ScreenWidth() - QUICK_MENU_SIZE)/2, (ScreenHeight() - QUICK_MENU_SIZE)/2);
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
    save_state();
    if (doRestart)
    {
        RestartApplication(false);
    }
    return 0;
}

int PBMainFrame::OnCommand(int commandId, int /*par1*/, int /*par2*/)
{
    switch (commandId)
    {
        case def_searchID:
            onSearchStart();
            break;
        case def_contentsID:
            onContentsOpen();
            break;
        case def_ttsID:
	    if (!get_calc_progress())
            	TTSProcessor::Init(*this, get_cpage() - 1);
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
        case def_settingsID:
            onConfigOpen();
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

//myPage from main.cpp where virtual function Open is overloaded

int PBMainFrame::onPageSelect()
{
    //OpenPageSelector(select_page);

    if (!get_calc_progress()){

        PBPagePosition * pagePosition = new PBPagePosition(Direction(position_to_page(cpos)));

        PBScreenBuffer * buf = new PBScreenBuffer();
        buf->FromScreen(true);
        pagePosition->SetScreenBuffer(buf);
        m_pageHistory->FullShow(pagePosition);
    }
    return 0;
}
int PBMainFrame::onContentsOpen()
{
    open_synopsis_contents();
    return 0;
}
iv_wlist* dictionaryWordHandler(int x, int y, int forward)
{
   iv_wlist *other, *w = currentText.WordFinder(x, y, forward);
   if (w)
   {
       other = (iv_wlist *)malloc(sizeof(iv_wlist));
       *other = *w;
   }
   else
   {
       other = w;
   }
   return other;
}
int PBMainFrame::onDictionaryOpen()
{
    iv_wlist* word_list;
    int word_list_len;    

    //std::cerr << "\n\nWords.......\n\n";
    if (get_page_word_list(&word_list, &word_list_len))
    {        
//        for (int i = 0; i < word_list_len; i++)
//        {
//            std::cerr << "Word: " << word_list[i].word;
//            std::cerr << " x1: " << word_list[i].x1 << " y1: " << word_list[i].y2;
//            std::cerr << " x2: " << word_list[i].x1 << " y2: " << word_list[i].y2;
//            std::cerr << std::endl;
//        }
        currentText.SetBasic(word_list, word_list_len);
        iv_wlist* word = (iv_wlist* )malloc(2 * sizeof(iv_wlist));
        memset(word, 0, 2 * sizeof(iv_wlist));
        memcpy(word, word_list, sizeof(iv_wlist));
        OpenControlledDictionaryView(dictionaryWordHandler, word, NULL);
    }

    return 0;
}

int PBMainFrame::onPagePrev()
{
    one_page_back(1);
    return 0;
}

int PBMainFrame::onPageNext()
{
    one_page_forward(1);    
    return 0;
}

int PBMainFrame::changeOrientaition(int n)
{
    return ornevt_handler(n);
}

void PBMainFrame::makeSnapShot()
{
    save_screenshot();
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
    select_page(1);
    return 0;
}

int PBMainFrame::onPageLast()
{
    select_page(999999);
    return 0;
}

int PBMainFrame::onNextSelection()
{
    next_section();
    return 0;
}

int PBMainFrame::onPrevSelection()
{
    prev_section();
    return 0;
}

int PBMainFrame::onBookMarkOpen()
{
	return onBookMarkNew();
}

int PBMainFrame::onBookMarkNew()
{
	int x = (ScreenWidth() - QUICK_MENU_SIZE)/2;
	int y = (ScreenHeight() - QUICK_MENU_SIZE)/2;
	PartialUpdate(x, y, QUICK_MENU_SIZE, QUICK_MENU_SIZE);
    new_synopsis_bookmark();
    return 0;
}

int PBMainFrame::onNoteNew()
{
	if (QueryTouchpanel())
	{
		OpenSynopsisToolBar();
	}
	else
	{
		new_synopsis_note();
	}
    return 0;
}

int PBMainFrame::onNoteOpen()
{
	if (QueryTouchpanel())
	{
		OpenSynopsisToolBar();
	}
	else
	{
		new_synopsis_note();
	}
    return 0;
}

int PBMainFrame::onNoteSave()
{
    save_page_note();
    return 0;
}

int PBMainFrame::onLinkOpen()
{
    open_links();
    return 0;
}

int PBMainFrame::onLinkBack()
{
    back_link();
    return 0;
}

int PBMainFrame::onSearchStart()
{
	int x = (ScreenWidth() - QUICK_MENU_SIZE)/2;
	int y = (ScreenHeight() - QUICK_MENU_SIZE)/2;
	PartialUpdate(x, y, QUICK_MENU_SIZE, QUICK_MENU_SIZE);
    OpenKeyboard("@Search", m_kbdbuffer, 30, 0, search_enter);
    return 0;
}

int PBMainFrame::onZoomIn()
{
    font_change(fontChangeStep);
    return 0;
}

int PBMainFrame::onZoomOut()
{
    font_change(-1 * fontChangeStep);
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
    ttsengine_setvolume(100 * GetVolume() / MAXVOL);
    return 0;
}

int PBMainFrame::onVolumeDown()
{
    int r = GetVolume();
    SetVolume(r - 3);
    ttsengine_setvolume(100 * GetVolume() / MAXVOL);
    return 0;
}

int PBMainFrame::onConfigOpen()
{
    open_config();
    return 0;
}

int PBMainFrame::onAboutOpen()
{
    show_about();
    return 0;
}

int PBMainFrame::onTouchClick(int /*x*/, int /*y*/)
{
	// overloaded to avoid warnings
	return 0;
}

