#include "pbpagehistorynavigation.h"
#include "main.h"
#include <typeinfo>
#include <inkview.h>
#include <ZLApplication.h>
extern ZLApplication *mainApplication;
extern int npages;
PBPagePosition::PBPagePosition(const Direction & direction):
        dir(direction)
{
    Init();
    dir = direction;
}

PBPagePosition::~PBPagePosition()
{
    delete m_buffer;
}
#include <iostream>
IHistoryPage *PBPagePosition::CreatePage(int npage)
{    
    PBScreenBuffer * bufferSave = new PBScreenBuffer();
    bufferSave->FromScreen(false);
    ///
    long long cposOld = get_position();
    set_page(npage);
    //setCurrent
    mainApplication->refreshWindow();
    repaint_all(-1);

    PBScreenBuffer * buffer = new PBScreenBuffer();
    buffer->FromScreen(true);
        PBPagePosition * res = new PBPagePosition(Direction(npage));
        res->SetScreenBuffer(buffer);
    set_position(cposOld);    
    mainApplication->refreshWindow();
    repaint_all(-1);
    ///
    bufferSave->ToScreen(0, 0, ScreenWidth(), ScreenHeight());
    delete bufferSave;

    return res;
}

#include <iostream>
bool PBPagePosition::operator ==(const IHistoryPage & pos) const
{
    if (dir.type == Direction::POSITION )
    {
        std::string str = pos.ToString();
        //std::cerr << pos.ToString();
        if (GetKey() == (dynamic_cast<const PBPagePosition & >(pos)).GetKey())
            return true;
        else
            return false;
    }else
    {
        //static int debugN = 0;
        //std::cerr<< "DebugPoint" <<debugN++ << std::endl;
        if (!strcmp(dynamic_cast<const PBPagePosition &>(pos).dir.link.href, dir.link.href))
        {
            return true;
        }

    }
    return false;
}

void PBPagePosition::Init()
{
    m_buffer = 0;    
}

#include <stdio.h>

int PBPagePosition::OnDraw(bool force)
{

//    if (IsVisible())
//    {
//            if (IsChanged() || force)
//            {
//                    PBGraphics *graphics = GetGraphics();
//
//                    if (IsFocused())
//                        graphics->DrawRectEx(GetLeft(), GetTop(), GetWidth(), GetHeight(), 1, 2, BLACK, WHITE);
//                    else
//                        graphics->DrawRectEx(GetLeft(), GetTop(), GetWidth(), GetHeight(), 1, 2, WHITE, WHITE);
//
//                    m_buffer->ToScreen(ClientToScreen(m_WindowRect).GetLeft()  + HMARGIN, ClientToScreen(m_WindowRect).GetTop() + VMARGIN,
//                                       ClientToScreen(m_WindowRect).GetWidth() - 2 * HMARGIN, ClientToScreen(m_WindowRect).GetHeight() - 2 * VMARGIN);
//
//
//                    std::string str = ToString();
//
//                    SetFont(OpenFont(DEFAULTFONTB, 20, 1), BLACK);
//
//                    //DrawString(ClientToScreen(m_rect).GetX() + 10, ClientToScreen(m_rect).GetY() + ClientToScreen(m_rect).GetHeight() - 25, str.c_str());
//                    DrawTextRect(ClientToScreen(m_WindowRect).GetX(), ClientToScreen(m_WindowRect).GetY() + ClientToScreen(m_WindowRect).GetHeight() - 25, ClientToScreen(m_WindowRect).GetWidth(), 25, str.c_str(), ALIGN_CENTER);
//
//
//            }
////            for (TWindowListIt it = m_Children.begin(); it != m_Children.end(); it++)
////                    (*it)->OnDraw(force);
//    }
    return 0;
}
void PBPagePosition::Show(const PBRect & rect)
{    
    m_buffer->ToScreen(rect.GetLeft(), rect.GetTop(),
                                          rect.GetWidth(), rect.GetHeight());    
}

void PBPagePosition::Free()
{
    delete m_buffer;
}
void PBPagePosition::SetScreenBuffer(PBScreenBuffer * buffer)
{
    if (m_buffer)
    {
        delete [] m_buffer;
        m_buffer = 0;
    }
    m_buffer = buffer;
}
#include <sstream>
std::string PBPagePosition::ToString() const //text info like page number or link
{
    if (dir.type != Direction::POSITION)
    {
        char *s = GetLangText("@Footnote");
        std::string str(s);
        delete [] s;        
        return str;
    }
    else //if (dir.type == Direction::POSITION)
    {        

        std::stringstream sStr;
        sStr << GetKey();
        return sStr.str();
    }
}
#include "ZLApplication.h"

void PBPagePosition::Open()
{
    if (dir.type == Direction::POSITION)
    {
        set_page(GetKey());

        mainApplication->refreshWindow();
        repaint_all(4);
    }
    else
    {
        assert(0);
    }

}
void PBPagePosition::GoToPage(int page)
{
    if (page > npages)
        page = npages;
    set_page(page);
    mainApplication->refreshWindow();
    repaint_all(4);
}
