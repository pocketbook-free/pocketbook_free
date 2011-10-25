#ifndef __PBMAINFRAME_H__
#define __PBMAINFRAME_H__

#include <pbframework/pbframework.h>
#include "pbpagehistorynavigation.h"

class PBMainFrame: public PBIEventHandler, public PBWindow, public PBIReader
{
    public:
        PBMainFrame();
        ~PBMainFrame();

        void Run();
        int onInit();

        // overload PBIReader
        //int GetScreenCount();
        void InvertRectange(int screen, std::vector<PBRect> &rectList);
        bool GetText(int screen, std::vector<PBWordInfo> &dst);

        // overloaded PBIEventHandler
        int onShow();
        int onExit();
        int onPagePrev();
        int onPageNext();
        int onPageJNext10();
        int onPageJNext10Hold();
        int onPageJNext10Up();
        int onPageJPrev10();
        int onPageJPrev10Hold();
        int onPageJPrev10Up();
        int onBack();
        int onForward();
        int onPageSelect();
        int onContentsOpen();
        int onDictionaryOpen();
        int onPageFirst();
        int onPageLast();
        int onNextSelection();
        int onPrevSelection();
        int onBookMarkOpen();
        int onBookMarkNew();
        int onNoteNew();
        int onNoteOpen();
        int onNoteSave();
        int onLinkOpen();
        int onLinkBack();
        int onSearchStart();
        int onZoomIn();
        int onZoomOut();
        int onPanelHide();
        int onRotateOpen();
        int onMp3Open();
        int onMp3Pause();
        int onVolumeUp();
        int onVolumeDown();
        int onConfigOpen();
        int onAboutOpen();

        int onPageHistory();
		int onMenu();
		int onTouchClick(int x, int y);

        //
        int changeOrientaition(int n);
        void makeSnapShot();
        void QuickMenuInit();

        static PBMainFrame* GetThis();
        static PBMainFrame* s_pThis;

        int MainHandler(int type, int par1, int par2);
        static int main_handler(int type, int par1, int par2);
		static bool is_menu_visible();

	int NoteHandler(int type, int par1, int par2);

        virtual int OnChildFocusChanged(PBWindow* pFocused)
        {
            pFocused = NULL;
            return 0;
        }

	// virtuals
        int OnCommand(int commandId, int par1, int par2);
	int OnDraw(bool force);

        PBPointerHandler m_PointerHandler;
        PBKeyboardHandler m_KeyboardHandler;

        PBQuickMenu * m_menu;


        PBPageNavigatorMenu * m_pageHistory;

    protected:

    private:
        std::vector<PBRect> m_prevRect;

        PBIReader m_reader;
        char m_kbdbuffer[32];
};

#endif /* __PBMAINFRAME_H__ */
