/* 
 * File:   zoomer.h
 * Author: alexander
 *
 */

#ifndef _ZOOMER_H
#define	_ZOOMER_H

typedef struct tagZoomerParameters
{
    int zoom;
    int offset;
    int optimal_zoom;
	int cpage;

    ddjvu_document_t* doc;
	ddjvu_page_t* page;
    int orient;
} ZoomerParameters;

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*CloseHandler)(ZoomerParameters* params);

// show new zoomer
void ShowZoomer(ZoomerParameters* params, CloseHandler closeHandler);

// calculate optimal zoom
void CalculateOptimalZoom(ddjvu_document_t* doc, ddjvu_page_t* page, int* zoom, int* offset);

#ifdef __cplusplus
}
#endif

#endif	/* _ZOOMER_H */

