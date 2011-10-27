#pragma once
#include <pbframework/pbframework.h>

#include "main.h"
#include <iostream>
typedef int TPosition;
struct Direction
{   
    TPosition position;    
    void SetPage(int n) {
        position = n;
        //std::cerr << "Page number " << n << " is " << position <<std::endl;
    }
    Direction & operator = (const Direction & dir)
    {        
        position =  dir.position;        
        return *this;
    }
    ~Direction()
    {        
    }    
    Direction(int n)
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
    bool UpdatePageNumber();
public:    


    PBPagePosition(const Direction & direction);

    virtual ~PBPagePosition();

    void SetScreenBuffer(PBScreenBuffer * buffer);    

    int OnDraw(bool force);

    int GetKey() const
    {
        int x = dir.position;
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
