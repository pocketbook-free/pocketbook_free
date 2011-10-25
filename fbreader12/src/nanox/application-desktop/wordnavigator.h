#pragma once
#include <list>
#include <inkview.h>
#include <vector>
/*!
  A struct containing ic_wlist structure
  contains info about line num of the word
*/
struct CMyWord// : public iv_wlist
{    
    CMyWord(iv_wlist * word, int n):
            m_word(word)
    {    
        m_n = n;
        m_line = 0;
    }
    ~CMyWord()
    {        
    }

    //! Set the line number pointer
    void SetLineNumPointer(int * p)
    {
        m_line = p;
    }
    iv_wlist * Get_iv_wlist()
    {
        return m_word;
    }
    void Set_iv_wlist(iv_wlist * w)
    {
        m_word = w;
    }

    /*!
      variable contains the number of the position in the word list
    */
    int m_n;
    int * m_line;
private:
    iv_wlist * m_word;
};

/*!
An extension of word vector wich also contains y--coordinate of the line
and its actual number in the text
*/
class CLine : public std::vector<CMyWord *>
{
    int m_line;
    int m_y;
public:
    //! constant wich determines in which ranges word corresponds to the current line
    static const int dyMax = 2;
    CLine(CMyWord * word);

    //! function which  check does the word corresponds to the current line
    /*!
            case 0: corresponds
            case -1: current line lies below then the word
            case 1: current line lies above then the word
    */
    int Corresponds(CMyWord * word);

    //! Iserts word to the current line
    void InsertHere(CMyWord * word);
    int GetLineNum() const;
    void SetLineNum(int n);
    CMyWord * GetNearestByX(int m_xPosition);
    void Print();
};
//! class for containing the page text and also contains information about the cursor
class CLines
{    
    int m_currentPosition;
    int m_xPosition;
    int m_currentLine;

    //! contains a pointer to static array after SetBasic call
    /*!
      we've got an error when we are trying to release memmory,
      so the calling function  suppose that the callig function will
      release memm itself
    */
    iv_wlist * m_toDel;
    std::vector<CMyWord *> m_basic;

    std::vector<CLine> m_lines;
    //! cursor move function
    CMyWord * MoveLeft();

    //! cursor move function
    CMyWord * MoveRight();

    //! cursor move function
    CMyWord * MoveUp();

    //! cursor move function
    CMyWord * MoveDown();
    //! find word by coordinate and set the current cursor on it
    CMyWord * FindByCoords(int x, int y);

    //! insert word to the text used by public SetBasic function
    void Insert(CMyWord * word);
public:
    CLines();
    CLines(iv_wlist * word, int count);

    //! clear data
    void ClearAll();

    //! set words from the page
    /*!
      we've got an error when we are trying to release memmory on word,
      so the calling function  suppose that the callig function will
      release memm itself
    */
    void SetBasic(iv_wlist * word, int count);

    //! function used by OpenControlledDictionaryView as a handler
    iv_wlist * WordFinder(int x, int y, int forward);
    void UpdateLinesNumbers();

};
