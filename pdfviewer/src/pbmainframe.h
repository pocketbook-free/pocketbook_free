#ifndef __PBMAINFRAME_H__
#define __PBMAINFRAME_H__

#include "pdfviewer.h"
#include "pbtouchzoomdlg.h"
#include "TouchZoom.h"

#ifdef SYNOPSISV2
#include "pbpdftoolbar.h"
#endif

class PBPageNavigatorMenu;
class PBMainFrame: public PBIEventHandler, public PBWindow, public PBIReader
{
	public:
		PBMainFrame(char *filename = "");
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
		int onBookMarkOpen();
		int onBookMarkNew();
		int onNoteNew();
		int onNoteOpen();
		int onNoteSave();
		int onSearchStart();
		int onZoomerOpen();
		int onChangeZoom(int zoom_type, int zoom_param);
		int onZoomIn();
		int onZoomOut();
		int onPanelHide();
		int onRotateOpen();
		int onMp3Open();
		int onMp3Pause();
		int onVolumeUp();
		int onVolumeDown();
		int onMenu();
		void ShowFileInfo(const char *path);
		int onAboutOpen();
		int onPdfMode();
		int onTouchClick(int x, int y);
                int onPageHistory();
		int onLinkOpen();
		int onKeyUp();
		int onKeyDown();

		int changeOrientaition(int n);
		void makeSnapShot();

		static PBMainFrame* GetThis();
		static PBMainFrame* s_pThis;

		int MainHandler(int type, int par1, int par2);
		static int main_handler(int type, int par1, int par2);

		// virtuals
		virtual int OnChildFocusChanged(PBWindow* pFocused)
		{
                    pFocused = NULL;
                    return 0;
		}
		virtual int OnCommand(int commandId, int par1, int par2);

		int OnDraw(bool force);

		PBPointerHandler m_PointerHandler;
		PBKeyboardHandler m_KeyboardHandler;

            protected:

#if defined(PLATFORM_FC)

		PBQuickMenu m_menu;
#endif
#if defined(PLATFORM_NX)
		PBOldMenuHandler m_OldMenuhandler;
#endif

#ifdef SYNOPSISV2
                PBPDFToolBar m_ToolBar;
            public:
                void DrawPen();
#endif
            private:
                PBPageNavigatorMenu * m_pageHistory;
                std::vector<PBRect> m_prevRect;

                PBIReader m_reader;
                char m_kbdbuffer[32];
                PBTouchZoomDlg m_touchZoomDlg;
		std::string m_fileName;
		bool m_keyEvent;
	    };

#endif /* __PBMAINFRAME_H__ */
