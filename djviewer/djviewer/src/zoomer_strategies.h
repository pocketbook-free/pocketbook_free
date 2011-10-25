/* 
 * File:   zoomer_strategies.h
 * Author: alexander
 *
 */

#ifndef _ZOOMER_STRATEGIES_H
#define	_ZOOMER_STRATEGIES_H

#include "zoomer.h"

#ifdef	__cplusplus
extern "C" {
#endif

// base class for zoom strategies
class ZoomStrategy
{
public:
    // get description of zoom strategy
    virtual void GetDescription(char* buffer, unsigned int size) const = 0;

    // calculate zoom
    virtual void CalculateZoom() = 0;

    // increase zoom
    virtual void ZoomIn() = 0;

    // decrease zoom
    virtual void ZoomOut() = 0;

    // check whether current zoom is allowed
    virtual void Validate() = 0;

    // get zoom parameters
    const ZoomerParameters& GetZoomParameters() const
    {
        return m_Parameters;
    }
protected:
	ZoomStrategy(const ZoomerParameters& parameters)
		:m_Parameters(parameters)
	{
	}

    ZoomerParameters m_Parameters;
};

// do zoom by whole pages
class PagesZoomStrategy : public ZoomStrategy
{
public:
    // constructor
    PagesZoomStrategy(const ZoomerParameters& parameters) : ZoomStrategy(parameters) {}

    // calculate zoom
    virtual void CalculateZoom()
    {
		if (m_Parameters.zoom != 33 && m_Parameters.zoom != 50)
        {
            m_Parameters.zoom = 50;
        }

        m_Parameters.offset = 0;
        m_Parameters.optimal_zoom = 0;
    }

    // increase zoom
    virtual void ZoomIn()
    {
        m_Parameters.zoom = 33;
    }

    // decrease zoom
    virtual void ZoomOut()
    {
        m_Parameters.zoom = 50;
    }

    // check zoom value
    virtual void Validate()
    {
        if (m_Parameters.zoom < 40) m_Parameters.zoom = 33;
        if (m_Parameters.zoom >= 40) m_Parameters.zoom = 50;
    }

    // get description of zoom strategy
    virtual void GetDescription(char* buffer, unsigned int size) const
    {
        int n;

        if (m_Parameters.orient == 0 || m_Parameters.orient == 3)
        {
            n = (m_Parameters.zoom == 33) ? 9 : 4;
        }
        else
        {
            n = (m_Parameters.zoom == 33) ? 6 : 2;
        }

        snprintf(buffer, size, "%s: %i%c %s", GetLangText(const_cast<char*>("@Preview")), n, 0x16, GetLangText(const_cast<char*>("@pages")));
    }
};

// calculate zoom to fit width of current page
class FitWidthStrategy : public ZoomStrategy
{
public:
    // constructor
	FitWidthStrategy(const ZoomerParameters& parameters)
		:ZoomStrategy(parameters)
	{
	}

    // calculate zoom
    virtual void CalculateZoom()
    {
		ddjvu_page_t *temp_page = NULL;
        ddjvu_rect_t rect = {0, 0, ScreenWidth(), ScreenHeight()};
        ddjvu_format_t* fmt = ddjvu_format_create(DDJVU_FORMAT_GREY8, 0, 0);
        ddjvu_format_set_row_order(fmt, 1);
        ddjvu_format_set_y_direction(fmt, 1);

        unsigned char* data = new unsigned char[ScreenWidth() * ScreenHeight()];

		if (m_Parameters.page == NULL)
		{
			if (!(temp_page = ddjvu_page_create_by_pageno(m_Parameters.doc, m_Parameters.cpage-1)))
			{
				fprintf(stderr, "Cannot access page %d.\n", m_Parameters.cpage);
				return;
			}
			else
			{
				m_Parameters.page = temp_page;
			}
		}

        if (ddjvu_page_render(m_Parameters.page, DDJVU_RENDER_COLOR, &rect, &rect, fmt, ScreenWidth(), reinterpret_cast<char*>(data)))
        {
            // calculate left offset
            int left = CalculateOffset(data, 0, ScreenHeight() / 8, ScreenWidth() / 2 - 1, 6*ScreenHeight() / 8, true);

            // calculate right offset
            int right = CalculateOffset(data, ScreenWidth() / 2, ScreenHeight() / 8, ScreenWidth() / 2 - 1, ScreenHeight() / 2, false);


            if (left == ScreenWidth() / 2 - 1 && right == ScreenWidth() / 2 - 1)
            {
                left = 0;
                right = 0;
            }

            // select smaller from two offsets and calculate zoom based on this value
            //m_Parameters.zoom = 100 * (0.5 * ScreenWidth() / (0.5 * ScreenWidth() - (left < right ? left : right)));

            // select zoom based on average of left and right offsets
            m_Parameters.zoom = 100 * (0.5 * ScreenWidth() / (0.5 * ScreenWidth() - (left + right) / 2));

            // limit maximum zoom
            if (m_Parameters.zoom >= 200)
            {
                m_Parameters.zoom = 199;
            }

            // caclulate additional offset
            m_Parameters.offset =  (left - right) / 2 * m_Parameters.zoom / 100;
            m_Parameters.optimal_zoom = 1;
        }
        else
        {
            m_Parameters.zoom = 100;
            m_Parameters.offset = 0;
            m_Parameters.optimal_zoom = 0;
        }

		if (temp_page != NULL)
			ddjvu_page_release(temp_page);

        delete[] data;
        ddjvu_format_release(fmt);
    }

    // increase zoom
    virtual void ZoomIn()
    {
    }

    // decrease zoom
    virtual void ZoomOut()
    {
    }

    // check zoom value
    virtual void Validate()
    {
    }

    // get description of zoom strategy
    virtual void GetDescription(char* buffer, unsigned int size) const
    {
        snprintf(buffer, size, "%s", GetLangText(const_cast<char*>("@Fit_width")));
    }
private:
    int CalculateOffset(unsigned char* data, int x, int y, int w, int h, bool startFromLeft)
    {
        int delta = startFromLeft ? 1 : -1;
        int offset = startFromLeft ? x : x + w;

        unsigned char* p = data + offset + ScreenWidth() * y;

        int number_of_lines = 1;

        for (int i = 0; i < w; ++i, p = data + delta * i + offset + ScreenWidth() * y)
        {
            int non_white_pixels = 0;
            int found = 0;
            for (int j = y; j < y + h; ++j, p += ScreenWidth())
            {
                if (*p < 200)
                {
                    if (++non_white_pixels > 5)
                    {
                        if (++number_of_lines > 3)
                        {
                            return (i >= 2 ? i - 2 : i) - (number_of_lines - 1);
                        }

                        found = 1;
                        break;
                    }
                }
            }

            if (found == 0) number_of_lines = 0;
        }

        return w;
    }

    ddjvu_document_t* m_Document;
    int               m_Page;
};

// allow user to choose appropriate zoom
class ManualZoomStrategy : public ZoomStrategy
{
public:
    // constructor
    ManualZoomStrategy(const ZoomerParameters& parameters) : ZoomStrategy(parameters) {}

    // calculate zoom
    virtual void CalculateZoom()
    {
    }

    // increase zoom
    virtual void ZoomIn()
    {
        m_Parameters.zoom += 5 - (m_Parameters.zoom % 5);
        if (m_Parameters.zoom > 195)
        {
            m_Parameters.zoom = 70;
        }

        m_Parameters.optimal_zoom = 0;
        m_Parameters.offset = 0;
    }

    // decrease zoom
    virtual void ZoomOut()
    {
        m_Parameters.zoom -= 5;

        if (m_Parameters.zoom % 5)
        {
            m_Parameters.zoom += 5 - (m_Parameters.zoom % 5);
        }

        if (m_Parameters.zoom < 70)
        {
            m_Parameters.zoom = 195;
        }

        m_Parameters.optimal_zoom = 0;
        m_Parameters.offset = 0;
    }

    // check zoom value
    virtual void Validate()
    {
        if (m_Parameters.zoom < 70) m_Parameters.zoom = 100;
        if (m_Parameters.zoom > 195) m_Parameters.zoom = 100;
    }

    // get description of zoom strategy
    virtual void GetDescription(char* buffer, unsigned int size) const
    {
        snprintf(buffer, size, "%s: %i%c", GetLangText(const_cast<char*>("@Normal_page")), m_Parameters.zoom, 0x16);
    }
};

// do zoom by columns
class ColumnsZoomStrategy : public ZoomStrategy
{
public:
    // constructor
    ColumnsZoomStrategy(const ZoomerParameters& parameters) : ZoomStrategy(parameters) {}

    // calculate zoom
    virtual void CalculateZoom()
    {
        if (m_Parameters.zoom < 200)
        {
            m_Parameters.zoom = 200;
        }
        
        m_Parameters.offset = 0;
        m_Parameters.optimal_zoom = 0;
    }

    // increase zoom
    virtual void ZoomIn()
    {
        if (m_Parameters.zoom < 200)
        {
            m_Parameters.zoom = 200;
        }
        else
        {
            m_Parameters.zoom += 100 - (m_Parameters.zoom % 100);
        }

        if (m_Parameters.zoom > 500)
        {
            m_Parameters.zoom = 200;
        }
    }

    // decrease zoom
    virtual void ZoomOut()
    {
        m_Parameters.zoom -= 100;

        if (m_Parameters.zoom % 100)
        {
            m_Parameters.zoom += 100 - (m_Parameters.zoom % 100);
        }

        if (m_Parameters.zoom < 200)
        {
            m_Parameters.zoom = 500;
        }
    }

    // check zoom value
    virtual void Validate()
    {
        if (m_Parameters.zoom < 200) m_Parameters.zoom = 200;
        if (m_Parameters.zoom > 500) m_Parameters.zoom = 500;
    }

    // get description of zoom strategy
    virtual void GetDescription(char* buffer, unsigned int size) const
    {
        int columns = m_Parameters.zoom / 100;
        snprintf(buffer, size, "%s: %i%c", GetLangText(const_cast<char*>("@Columns")), columns < 1 ? 1 : columns, 0x16);
    }
};


#ifdef	__cplusplus
}
#endif

#endif	/* _ZOOMER_STRATEGIES_H */

