#ifndef __PBMAINFRAME_H__
#define __PBMAINFRAME_H__

#include "../../pbframework11/framework/pbwindow.h"
#include "../../pbframework11/framework/pbquickmenu.h"
#include "../../pbframework11/framework/pbpointerhandler.h"
#include "../../pbframework11/framework/pbkeyboardhandler.h"
#include "../../pbframework11/framework/pbireader.h"
#include "../../pbframework11/framework/pbutilities.h"

#include "pbtouchzoomdlg.h"

class PBMainFrame: public PBIEventHandler, public PBWindow, public PBIReader
{
    public:
        PBMainFrame();
        ~PBMainFrame();

        void Run();
        int onInit();

        // overload PBIReader
        void InvertRectange(int screen, std::vector<PBRect> &rectList);
        bool GetText(int screen, std::vector<PBWordInfo> &dst);
	void SetJoystickKeyMapping(bool mapping_allowed);

        // overloaded PBIEventHandler
        int onShow();
        int onExit();
		int onKeyLeft();
		int onKeyRight();
		int onKeyUp();
		int onKeyDown();
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
        int onMenu();
		int onZoomerOpen();
		int onChangeZoom(int zoom_type, int zoom_param);
        //
        int changeOrientaition(int n);
        void makeSnapShot();

        static PBMainFrame* GetThis();
        static PBMainFrame* s_pThis;

        int MainHandler(int type, int par1, int par2);
        static int main_handler(int type, int par1, int par2);

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

	PBQuickMenu m_menu;

    protected:

    private:
        std::vector<PBRect> m_prevRect;

        PBIReader m_reader;
        char m_kbdbuffer[32];
		PBTouchZoomDlg m_touchZoomDlg;
};

#endif /* __PBMAINFRAME_H__ */
