#include "wordnavigator.h"
#include "inkview.h"
#include <list>
#include <iostream>
#include <pbframework/pbframework.h>
using namespace std;

int CLine::Corresponds(CMyWord * word)
{
    if (abs(m_y - word->Get_iv_wlist()->y2) <= dyMax)
    {
        return 0;
    }
    else if (m_y - word->Get_iv_wlist()->y2 < 0) return 1;
    else if (m_y - word->Get_iv_wlist()->y2 > 0) return -1;
}
CLine::CLine(CMyWord * word)
{
    InsertHere(word);
}
void CLine::InsertHere(CMyWord * word)
{
    iterator it, itPrev;
    static int a = 0;    
    word->SetLineNumPointer(&m_line);
    if (empty())
    {
        m_y = word->Get_iv_wlist()->y2;
    }
    push_back(word);

}
void CLine::Print()
{
    iterator it;
    for (it = begin(); it != end(); it++ )
    {
        std::cerr << "\nWord: " << (*it)->Get_iv_wlist()->word;
    }
}
CMyWord * CLine::GetNearestByX(int xPosition)
{
    CMyWord * ret = 0;
    for (int i = 0; i < size(); i++)
    {
        CMyWord * w = (*this)[i];
        if (w->Get_iv_wlist()->x2 > xPosition)
        {
            ret = (*this)[i];
            break;
        }
    }
    return ret ? ret : back();

}
int CLine::GetLineNum() const
{
    return m_line;
}
void CLine::SetLineNum(int n)
{
    m_line = n;
    iterator it;
    for (it = begin(); it != end(); it++ )
    {
        (*it)->m_line = &m_line;
    }
}
void CLines::Insert(CMyWord * word)
{
    std::vector<CLine>::iterator it, itPrev;
    int res;
    int a = 0;
    word->m_line = &(m_currentLine);
    int l = 0, r = m_lines.size() - 1, i;
    if (r >= 1)
    {
        a++;        
    }
    if (m_lines.size())
    {
        for (l = 0, r = m_lines.size() - 1; r - l > 1 ;)
        {
            i = (l + r) / 2;
            res = m_lines[i].Corresponds(word);
            switch (res)
            {
            case 0: // equal
                m_lines[i].InsertHere(word);
                return;  //!!!!!!!!!!!!!!!!!!!!!!!!!!
            case -1: // less
                /*if (it == m_lines.begin())
                {
                    CLine t(word);
                    m_lines.push_back(t);
                }
                else
                {
                    CLine t(word);
                    m_lines.insert(itPrev, t);
                }*/
                r = i;
                continue;
            case 1:
                l = i;
                continue;
            }
        }
    }
    out:
    if (m_lines.empty())
    {
        CLine w(word);
        m_lines.push_back(w);
    }
    else
    {
        res = m_lines[l].Corresponds(word);
        if (res == -1)
        {
            CLine line(word);
            m_lines.insert(m_lines.begin() + l, line);
        }
        else if (res == 0)
        {
            m_lines[l].InsertHere(word);
        }
        else if (res == 1 && l != r)
        {
            res = m_lines[r].Corresponds(word);
            if (res == -1)
            {
                CLine line(word);
                m_lines.insert(m_lines.begin() + r, line);
            }
            else if (res == 0)
            {
                m_lines[r].InsertHere(word);
            } else if (res == 1)
            {
                CLine line(word);
                if (r == m_lines.size())
                {
                    m_lines.push_back(line);
                }
                else
                {
                    m_lines.insert(m_lines.begin() + r + 1, line);
                }
            }
        }
        else if (res == 1 && l == r)
        {
            CLine line(word);
            if (r == m_lines.size())
            {
                m_lines.push_back(line);
            }
            else
            {
                m_lines.insert(m_lines.begin() + r + 1, line);
            }
        }

    }
}
iv_wlist * CLines::WordFinder(int x, int y, int forward)
{
    CMyWord * w;
    switch (forward)
    {
        case 1:
            w = MoveRight();
            break;
        case -1:
            w = MoveLeft();
            break;
        case -2:
            w = MoveUp();
            break;
        case 2:
            w = MoveDown();
            break;
        case 0:
            w = FindByCoords(x, y);
            break;
    };
    if (!w)
    {
        return 0;
    }
    iv_wlist * iw = w->Get_iv_wlist();
    int s = strlen(iw->word);
    if (iw->word[s - 1] == '-')
    {
        char * newWord, *nextWord;

        if (m_currentPosition < (m_basic.size() - 1))
        {
            nextWord = m_basic[m_currentPosition + 1]->Get_iv_wlist()->word;
            newWord = (char *)malloc(s + 1 + strlen(nextWord) + 1);
            strcpy(newWord, iw->word);
            newWord[s - 1] = 0;
            strcat(newWord, nextWord);
            free(iw->word);
            iw->word = newWord;
        }
    }
    m_currentLine = *(w->m_line);
    return w->Get_iv_wlist();
}
void CLines::SetBasic(iv_wlist * word, int count)
{
    ClearAll();
    m_toDel = word;
    for (int i = 0; i < count; i++)
    {        
        //CMyWord * temp= new CMyWord(&(word[i]), i);
        CMyWord * temp = new CMyWord(&(word[i]), i);
        this->m_basic.push_back(temp);
        Insert(temp);
    }
    UpdateLinesNumbers();
}

CLines::CLines()
{
    m_toDel = 0;
    m_currentPosition = 0;    
}
CLines::CLines(iv_wlist *word, int count)
{
    m_toDel = 0;
    m_currentPosition = 0;
    SetBasic(word, count);
}
void CLines::UpdateLinesNumbers()
{
    std::vector<CLine>::iterator it;
    int i = 0;
    for (it = m_lines.begin(); it != m_lines.end(); it++)
    {
        (*it).SetLineNum(i++);
    }
    m_currentLine = *(m_basic[m_currentPosition]->m_line);
}
CMyWord * CLines::MoveLeft()
{    
    CMyWord * w;
    if (m_currentPosition > 0)
        --m_currentPosition;
    else
    {
        m_currentPosition = m_basic.size() - 1;
    }
    w = m_basic[m_currentPosition];
    m_xPosition = (w->Get_iv_wlist()->x1 + w->Get_iv_wlist()->x2) / 2;
    return w;

}
CMyWord * CLines::MoveRight()
{
    CMyWord * w;
    if (m_currentPosition < m_basic.size() - 1)
        ++m_currentPosition;
    else
    {
        m_currentPosition = 0;
    }
    w = m_basic[m_currentPosition];
    m_xPosition = (w->Get_iv_wlist()->x1 + w->Get_iv_wlist()->x2) / 2;
    return w;
}
CMyWord * CLines::MoveUp()
{
    if (m_currentLine > 0)
    {
        --m_currentLine;
    }
    else
    {
        m_currentLine = m_lines.size() - 1;
    }
    CMyWord * w = m_lines[m_currentLine].GetNearestByX(m_xPosition);
    m_currentPosition = w->m_n;
    return w;
}
CMyWord * CLines::MoveDown()
{
    if (m_currentLine < (m_lines.size() - 1))
    {
        ++m_currentLine;
    }
    else
    {
        m_currentLine = 0;
    }
    CMyWord * w = m_lines[m_currentLine].GetNearestByX(m_xPosition);
    m_currentPosition = w->m_n;
    return w;
}
CMyWord * CLines::FindByCoords(int x, int y)
{
    CMyWord * ret = 0;
    for (int i = 0; i < m_basic.size(); i++)
    {
        iv_wlist * w = m_basic[i]->Get_iv_wlist();
        if (PBRect(w->x1, w->y1, w->x2 - w->x1, w->y2 - w->y1).isInRectange(x, y))
        {
            ret = m_basic[i];
            break;
        }
    }
    if (ret)
    {
        m_xPosition = (ret->Get_iv_wlist()->x1 + ret->Get_iv_wlist()->x2) / 2;
        m_currentPosition = ret->m_n;
        m_currentLine = *(m_basic[m_currentPosition]->m_line);
        return ret;
    }
    else
    {
        return 0;
    }
}
void CLines::ClearAll()
{
    if (m_toDel)
    {
        //free(m_toDel);
        m_toDel = 0;
    }
    m_currentPosition = 0;
    m_currentLine = 0;
    m_xPosition = 0;
    for (int i = 0; i < m_basic.size(); i++)
    {
        delete m_basic[i];
    }
    this->m_lines.clear();
    m_basic.clear();
}
