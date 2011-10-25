/*
 * File:   zoomer.cpp
 * Author: alexander
 *
 */

// system includes
#include <inkview.h>
#include <inkinternal.h>

#ifdef WIN32
#undef WIN32
#include "../libdjvu/ddjvuapi.h"
#define WIN32
#else
#include "../libdjvu/ddjvuapi.h"
#endif

// local includes
#include "zoomer_strategies.h"
#include "zoomer.h"

extern const ibitmap djvumode;

// zoomer control
class Zoomer
{
public:
    // create zoomer instance
    static Zoomer& CreateInstance(const ZoomerParameters& params, CloseHandler closeHandler);

    // show zoomer control
    void Show();
private:
    // available zoom strategies
    enum EZoomStrategy
    {
        ZOOM_STRATEGY_FIRST,
        ZOOM_STRATEGY_PAGES = ZOOM_STRATEGY_FIRST,
        ZOOM_STRATEGY_FIT_WIDTH,
        ZOOM_STRATEGY_MANUAL_ZOOM,
        ZOOM_STRATEGY_MANUAL_COLUMNS,
        ZOOM_STRATEGY_LAST = ZOOM_STRATEGY_MANUAL_COLUMNS
    };

    // constructor
    Zoomer(const ZoomerParameters& params, CloseHandler closeHandler);

    // destructor
    ~Zoomer();
 
    // handle input events from zoomer
    static int HandleEvents(int type, int par1, int par2);

    // handle input events from zoomer
    void HandleEvents(int key);

    // draw zoomer
    void Draw(int gu);

    // get bounding rectangle of window
    void GetWindowFrameRect(int& x, int& y, int& w, int& h);

    // get bounding rectangle of bitmap
    void GetBitmapFrameRect(int& x, int& y, int& w, int& h);

    // zoomer instance
    static Zoomer* m_Instance;

    // strategies
    ZoomStrategy* s_Strategies[ZOOM_STRATEGY_LAST + 1];

    // variables
    int m_Position;
    iv_handler m_PrevHandler;
    ibitmap* m_SavedArea;
    CloseHandler m_CloseHandler;

    // icon parameters
    static const int m_IconsNumber;
    static const int m_IconWidth;
    static const int m_IconHeight;
};

// initialization of static variables
Zoomer* Zoomer::m_Instance;
const int Zoomer::m_IconsNumber = 4;
const int Zoomer::m_IconWidth = djvumode.width / m_IconsNumber;
const int Zoomer::m_IconHeight = djvumode.height;

// constructor
Zoomer::Zoomer(const ZoomerParameters& params, CloseHandler closeHandler) : m_PrevHandler(0)
{
    // save area behind zoomer
    int xframe, yframe, wframe, hframe;
    GetWindowFrameRect(xframe, yframe, wframe, hframe);
    m_SavedArea = BitmapFromScreen(xframe, yframe, wframe, hframe);

    // memorize close handler
    m_CloseHandler = closeHandler;

    // create strategies
    s_Strategies[ZOOM_STRATEGY_PAGES] = new PagesZoomStrategy(params);
    s_Strategies[ZOOM_STRATEGY_FIT_WIDTH] = new FitWidthStrategy(params);
    s_Strategies[ZOOM_STRATEGY_MANUAL_ZOOM] = new ManualZoomStrategy(params);
    s_Strategies[ZOOM_STRATEGY_MANUAL_COLUMNS] = new ColumnsZoomStrategy(params);

    // select appropriate position
    if (params.optimal_zoom)
    {
        m_Position = ZOOM_STRATEGY_FIT_WIDTH;
    }
    else if (params.zoom <= 50)
    {
        m_Position = ZOOM_STRATEGY_PAGES;
    }
    else if (params.zoom >= 200)
    {
        m_Position = ZOOM_STRATEGY_MANUAL_COLUMNS;
    }
    else
    {
        m_Position = ZOOM_STRATEGY_MANUAL_ZOOM;
    }
}

// destructor
Zoomer::~Zoomer()
{
    // restore event handler
    if (m_PrevHandler != 0)
    {
        iv_seteventhandler(m_PrevHandler);
    }

    // restore area behind zoomer
    int xframe, yframe, wframe, hframe;
    GetWindowFrameRect(xframe, yframe, wframe, hframe);
    DrawBitmap(xframe, yframe, m_SavedArea);

    // deallocate memory which was allocated for saved area
    free(m_SavedArea);

    // update screen
    PartialUpdate(xframe, yframe, wframe, hframe);

    // notify that zoomer is about to close
    if (m_CloseHandler != 0)
    {
        ZoomerParameters zoomerParameters = s_Strategies[m_Position]->GetZoomParameters();
        m_CloseHandler(&zoomerParameters);
    }

    // delete strategies
    for (int i = ZOOM_STRATEGY_FIRST; i <= ZOOM_STRATEGY_LAST; ++i)
    {
        delete s_Strategies[i];
    }

    m_Instance = 0;
}

// create zoomer instance
Zoomer& Zoomer::CreateInstance(const ZoomerParameters& params, CloseHandler closeHandler)
{
    if (m_Instance == 0)
    {
        m_Instance = new Zoomer(params, closeHandler);
    }

    return *m_Instance;
}

// show zoomer
void Zoomer::Show()
{
    // memorize old event handler and set new one
    m_PrevHandler = iv_seteventhandler(HandleEvents);

    // draw zoomer
    Draw(1);
}

// handle input events from zoomer
int Zoomer::HandleEvents(int type, int par1, int par2)
{
    if (type == EVT_KEYPRESS || type == EVT_KEYREPEAT)
    {
        m_Instance->HandleEvents(par1);
    }

    return 0;
}

// handle input events from zoomer
void Zoomer::HandleEvents(int key)
{
    switch (key)
    {
        // move to left icon
        case KEY_LEFT:
        {
            if (--m_Position < ZOOM_STRATEGY_FIRST)
            {
                m_Position = ZOOM_STRATEGY_LAST;
            }

            s_Strategies[m_Position]->Validate();
            Draw(0);

            break;
        }
        // move to right icon
        case KEY_RIGHT:
        {
            if (++m_Position > ZOOM_STRATEGY_LAST)
            {
                m_Position = ZOOM_STRATEGY_FIRST;
            }

            s_Strategies[m_Position]->Validate();
            Draw(0);

            break;
        }
        // decrease current value:
        case KEY_DOWN:
        {
            s_Strategies[m_Position]->ZoomOut();
            Draw(0);
            break;
        }
        // increase current value:
        case KEY_UP:
        {
            s_Strategies[m_Position]->ZoomIn();
            Draw(0);
            break;
        }
        // close zoomer
        case KEY_OK:
        {
            s_Strategies[m_Position]->CalculateZoom();
            delete m_Instance;
            break;
        }
        // close zoomer
        case KEY_BACK:
        case KEY_PREV:
        {
            delete m_Instance;
            break;
        }
    }
}

// draw zoomer
void Zoomer::Draw(int gu)
{
    // draw frame
    int xframe, yframe, wframe, hframe;
    GetWindowFrameRect(xframe, yframe, wframe, hframe);
    iv_windowframe(xframe, yframe, wframe, hframe, BLACK, WHITE, const_cast<char*>("@Zoom"), 0);

    // draw bitmap
    int xbitmap, ybitmap, wbitmap, hbitmap;
    GetBitmapFrameRect(xbitmap, ybitmap, wbitmap, hbitmap);
    DrawBitmap(xbitmap, ybitmap, &djvumode);

    // draw description
    char buffer[128] = {0};
    s_Strategies[m_Position]->GetDescription(buffer, sizeof(buffer));
    SetFont(menu_n_font, 0);
    DrawTextRect(xframe, yframe + hframe - 1.25 * menu_n_font->height, wframe, 1, buffer, ALIGN_CENTER);

    // current selection
    InvertAreaBW(xbitmap + m_IconWidth * m_Position, ybitmap, m_IconWidth, m_IconHeight);

	// update screen
	if (gu) {
		//PartialUpdate(xframe, yframe, wframe, hframe);
	} else {
		PartialUpdateBW(xframe, yframe, wframe, hframe);
	}
}

// get bounding rectangle of window
void Zoomer::GetWindowFrameRect(int& x, int& y, int& w, int& h)
{
    x = (ScreenWidth() - djvumode.width) / 2;
    y = (ScreenHeight() - djvumode.height) / 2;
    w = djvumode.width + 2 * m_IconWidth / 3;
    h = djvumode.height + 1.5 * menu_n_font->height + 2 * m_IconHeight / 3;
}

// get bounding rectangle of bitmap
void Zoomer::GetBitmapFrameRect(int& x, int& y, int& w, int& h)
{
    GetWindowFrameRect(x, y, w, h);

    x += m_IconWidth / 3;
    y += m_IconHeight / 3 + header_font->height;
    w = djvumode.width;
    h = djvumode.height;
}

// create and show zoomer
void ShowZoomer(ZoomerParameters* params, CloseHandler closeHandler)
{
    Zoomer::CreateInstance(*params, closeHandler).Show();
}

// calculate optimal width
void CalculateOptimalZoom(ddjvu_document_t* doc, ddjvu_page_t* page, int* zoom, int* offset)
{
    ZoomerParameters params = {0};
    params.doc = doc;
    params.page = page;

    FitWidthStrategy fit(params);
    fit.CalculateZoom();

    *zoom = fit.GetZoomParameters().zoom;
    *offset = fit.GetZoomParameters().offset;
}
