#include "pbpagehistorynavigation.h"
#include "main.h"
#include <typeinfo>
#include <inkview.h>
#include <pbframework/pbframework.h>

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
    int p = *cpage_main;

    page_selected_background(npage);

    PBScreenBuffer * buffer = new PBScreenBuffer();
    buffer->FromScreen(true);
        PBPagePosition * res = new PBPagePosition(Direction(npage));
        res->SetScreenBuffer(buffer);

    *cpage_main = p;
    page_selected_background(p);


    bufferSave->ToScreen(0, 0, ScreenWidth(), ScreenHeight());
    delete bufferSave;

    return res;
}

#include <iostream>
bool PBPagePosition::operator ==(const IHistoryPage & pos) const
{
    //std::string str = pos.ToString();

    if (GetKey() == (dynamic_cast<const PBPagePosition & >(pos)).GetKey())
        return true;
    else
        return false;
    return false;
}

void PBPagePosition::Init()
{
    m_buffer = 0;    
}

#include <stdio.h>

int PBPagePosition::OnDraw(bool force)
{
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
        std::stringstream sStr;
        sStr << GetKey();
        return sStr.str();
}

void PBPagePosition::Open()
{
    page_selected(dir.position);
}

void PBPagePosition::GoToPage(int page)
{
    page_selected(page);
}
