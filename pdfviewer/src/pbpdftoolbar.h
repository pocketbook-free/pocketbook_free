#ifndef PBPDFTOOLBAR_H
#define PBPDFTOOLBAR_H

#ifdef SYNOPSISV2

#include "pbnotes.h"
#include <synopsis/pbsynopsispen.h>
#include <synopsis/pbsynopsistoolbar.h>

class PBPDFToolBar : public PBSynopsisToolBar
{
public:
	PBPDFToolBar();
	~PBPDFToolBar();

	void Init(SynopsisTOC *toc);
	static PBPDFToolBar *GetThis();
	void OpenToolBar();

	static void ToolHandler(int button);
	int Note_Handler(int type, int par1, int par2);

	int GetTool();
	ibitmap *GetPagePreview();

	static PBPDFToolBar *m_pThis;
	PBPDFPen *m_pLocalPen;

private:
	virtual void PageBack();
	virtual void PageForward();
	virtual void OpenTOC();
	virtual void PauseCalcPage();
	virtual void ContinueCalcPage();
	virtual void RenderPage();
	virtual void CreateSnapshot(PBSynopsisSnapshot *snapshot);

	SynopsisTOC *m_pTOC;
	int m_Tool;
};

#endif

#endif // PBPDFTOOLBAR_H
