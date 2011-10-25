#pragma once
#include <pbframework/pbframework.h>
#include "main.h"
#include <iostream>
typedef long long TPosition;
struct Direction
{
    enum TYPE {LINK, POSITION, NOTSET};
    TYPE type;
    TPosition position;
    hlink link;// TODO
    void SetPage(int n) {
        position = page_to_position(n);
        //std::cerr << "Page number " << n << " is " << position <<std::endl;
    }
    Direction & operator = (const Direction & dir)
    {
        type = dir.type;
        position =  dir.position;
        if (type == LINK)
        {
            link = dir.link;
            link.href = new char[strlen(dir.link.href) + 1];
            strcpy(link.href, dir.link.href);
        }
        return *this;
    }
    ~Direction()
    {
        if (type == LINK)
            delete [] link.href;
    }    
    Direction(int n):
            type(POSITION)
    {
        SetPage(n);
    }
    Direction(const Direction & dir)
    {
        *this = dir;
    }
};
#include <iostream>

class PBPagePosition : public IHistoryPage
{ 
    static const float HMARGIN = 4;
    static const float VMARGIN = 4;
    long long position;
    Direction dir;
    void Init();
    PBScreenBuffer *m_buffer;
    //returns true when number is updated
    //returns false when number has not changed
    bool UpdatePageNumber();
public:    


    PBPagePosition(const Direction & direction);

    virtual ~PBPagePosition();

    void SetScreenBuffer(PBScreenBuffer * buffer);    

    int OnDraw(bool force);

    int GetKey() const
    {
        int x = position_to_page(dir.position);
        //std::cerr << x << "\n";
        return x;
    }

    virtual void Show(const PBRect & rect);

    virtual void Free();

    virtual  IHistoryPage *CreatePage(int npage);    

    virtual void GoToPage()
    {
        Open();
    }
    virtual void GoToPage(int page);

    //override
    virtual bool operator ==(const IHistoryPage & pos) const;

    //override
    virtual std::string ToString() const; //text info like page number or link

    //override
    virtual void Open();
};
