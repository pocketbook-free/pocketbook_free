#include "pbwordinfo.h"
#include <stdlib.h>
#include <string.h>

PBWordInfo::PBWordInfo(int screen, char* word, int x1, int y1, int x2, int y2)
{
    m_screen = screen;
    if (word)
    {
        m_wordlen = strlen(word);
        m_word = (char*) malloc(m_wordlen + 1);
        memcpy(m_word, word, m_wordlen + 1);
    }
    else
    {
        m_word = NULL;
        m_wordlen = 0;
    }

    if (x1 < x2)
    {
        m_x = x1;
        m_width = x2 - x1;
    }
    else
    {
        m_x = x2;
        m_width = x1 - x2;
    }
    if (y1 < y2)
    {
        m_y = y1;
        m_height = y2 - y1;
    }
    else
    {
        m_y = y2;
        m_height = y1 - y2;
    }
}

PBWordInfo::PBWordInfo(const PBWordInfo& src)
{
    m_wordlen = src.m_wordlen;
    if (m_wordlen)
    {
        m_word = (char*) malloc(m_wordlen + 1);
        memcpy(m_word, src.m_word, m_wordlen + 1);
    }
    else m_word = NULL;

    m_screen = src.m_screen;
    m_x = src.m_x;
    m_y = src.m_y;
    m_width = src.m_width;
    m_height = src.m_height;
}

PBWordInfo::~PBWordInfo()
{
    if (m_word)
        free(m_word);
}

int PBWordInfo::GetX()
{
    return m_x;
}

int PBWordInfo::GetY()
{
    return m_y;
}

int PBWordInfo::GetWidth()
{
    return m_width;
}

int PBWordInfo::GetHeight()
{
    return m_height;
}

void PBWordInfo::SetX(int value)
{
	m_x = value;
}

void PBWordInfo::SetY(int value)
{
	m_y = value;
}

void PBWordInfo::SetWidth(int value)
{
	m_width = value;
}

void PBWordInfo::SetHeight(int value)
{
	m_height = value;
}

int PBWordInfo::GetScreen()
{
    return m_screen;
}

bool PBWordInfo::isLastWord()
{
    bool result = false;
    char delimeters[] = {'.','!','?','\n','\r'};
    char word_char;
    int check_pos = 1;
    unsigned int del_iter;

    if (m_word)
    {
        if (m_wordlen >= 2 &&
            (m_word[m_wordlen - 1] == '\"' ||
			 m_word[m_wordlen - 1] == '\'' ||
			 m_word[m_wordlen - 1] == ' '))
            check_pos = 2;

        word_char = m_word[m_wordlen - check_pos];
        del_iter = 0;

        while (del_iter < sizeof(delimeters) &&
               word_char != delimeters[del_iter])
            del_iter++;

        if (del_iter != sizeof(delimeters))
            result = true;
    }

    return result;
}

char PBWordInfo::GetLastChar()
{
    char result = '\0';

    if (m_word)
        result = m_word[m_wordlen - 1];

    return result;
}

char* PBWordInfo::GetWord()
{
    return m_word;
}

int PBWordInfo::GetWordLen()
{
    return m_wordlen;
}

