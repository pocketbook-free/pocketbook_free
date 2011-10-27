#ifdef SYNOPSISV2

#include "main.h"
#include "pbmainframe.h"
#include "pbnotes.h"
#include "pbpdftoolbar.h"

PBPDFToolBar *PBPDFToolBar::m_pThis = NULL;

PBPDFToolBar::PBPDFToolBar()
	:PBSynopsisToolBar()
	,m_pLocalPen(NULL)
	,m_pTOC(NULL)
	,m_Tool(0)
{
	m_pThis = this;
}

PBPDFToolBar::~PBPDFToolBar()
{
}

void PBPDFToolBar::Init(SynopsisTOC *toc)
{
	m_pTOC = toc;
	this->AddPanel();
//	this->AddItem(STBI_MARKER, 0);
	this->AddItem(STBI_PEN, 0);
	this->AddItem(STBI_ERASER, 0);
//	this->AddItem(STBI_UNDO, 0);
//	this->AddItem(STBI_REDO, 0);
	this->AddItem(STBI_SEPARATOR, 0);
//	this->AddItem(STBI_NOTE, 0);
//	this->SetItemState(7, 0, 0, STBIS_ACTIVE);
	this->AddItem(STBI_IMAGE, 0);
//	this->SetItemState(4, 0, 0, STBIS_ACTIVE);
	this->AddItem(STBI_SEPARATOR, 0);
	this->AddItem(STBI_CONTENT, 0);

	this->AddPanel();
	this->AddItem(STBI_HELP, 1);
	this->AddItem(STBI_EXIT, 1);

	this->SetPanelPosition(0, 50);
	this->SetPanelPosition(1, ScreenWidth() - 50 - 50);
}

PBPDFToolBar *PBPDFToolBar::GetThis()
{
	return m_pThis;
}

void PBPDFToolBar::OpenToolBar()
{
	int zoom_type;
	int zoom_param;
	get_zoom_param(zoom_type, zoom_param);
	if (zoom_type == ZoomTypeFitWidth)
	{
		zoom_param = scale;
	}

	if (zoom_type == ZoomTypePreview || zoom_type == ZoomTypeReflow || zoom_type == ZoomTypeEpub)
	{
		Message(ICON_INFORMATION, "@Error", "Not support in this scale", 3000);
		return;
	}

	char startpos[30], endpos[30];
	PBSynopsisItem::PositionToChar((long long)cpage << 40, startpos);
	PBSynopsisItem::PositionToChar((long long)cpage << 40, endpos);

//	synopsis_matrix = m_pThis->GetZoomOut();

	stb_matrix matrix;

	matrix.a = (double)(zoom_param * ScreenWidth() / 600) / 100.;
	matrix.b = 0;
	matrix.c = 0;
	matrix.d = (double)(zoom_param * ScreenWidth() / 600) / 100.;
	matrix.e = -(offx - scrx);
	matrix.f = -(offy - scry);

	this->SetAdobeZoomIn(matrix);

	this->Open(m_pTOC, PBMainFrame::main_handler,
			BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight()),
			PBPDFToolBar::ToolHandler,
			(PBPDFPen *)m_pTOC->GetPen(startpos, endpos));
}

void PBPDFToolBar::ToolHandler(int button)
{
	if (m_pThis->m_Tool == button)
		return;

	if (m_pThis->m_Tool == STBI_PEN || m_pThis->m_Tool == STBI_ERASER)
	{
		m_pThis->m_pLocalPen->Close();
	}

	m_pThis->m_Tool = button;

	stb_matrix matrix, synopsis_matrix;
	stb_matrix navigationMatrix;
	int zoom_type;
	int zoom_param;
	ibitmap *bmp;
	int savescale;
	int saveoffx;
	int saveoffy;

	switch(button)
	{
		case STBI_MARKER:
		case STBI_NOTE:
		case STBI_IMAGE:
			break;

		case STBI_PEN:
		case STBI_ERASER:
			m_pThis->m_pLocalPen = (PBPDFPen *)m_pThis->GetPen();

			get_zoom_param(zoom_type, zoom_param);
			if (zoom_type == ZoomTypeFitWidth)
			{
				zoom_param = scale;
			}

			navigationMatrix.a = navigationMatrix.d = (double)(zoom_param * ScreenWidth() / 600) / 100.;
			navigationMatrix.e = navigationMatrix.f = navigationMatrix.b = navigationMatrix.c = 0;

			synopsis_matrix = m_pThis->GetZoomOut();

			matrix.a = synopsis_matrix.a / navigationMatrix.a;
			matrix.b = 0;
			matrix.c = 0;
			matrix.d = synopsis_matrix.d / navigationMatrix.d;
			matrix.e = (offx - scrx + synopsis_matrix.e) / navigationMatrix.a;
			matrix.f = (offy - scry + synopsis_matrix.f) / navigationMatrix.d;

			savescale = scale;
			saveoffx = offx;
			saveoffy = offy;
			scale = 100;
			offx = 0;
			offy = 0;
			out_page(-1);
			bmp = m_pThis->GetPagePreview();
			m_pThis->m_pLocalPen->Create((PBSynopsisTOC*)m_pThis->m_pTOC, bmp, synopsis_matrix, matrix);

			scale = savescale;
			offx = saveoffx;
			offy = saveoffy;
			out_page(0);

//			m_pThis->RenderPage();
			m_pThis->OutputPage(BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight()),
				2, 0, 0, ScreenWidth(), ScreenHeight());

//			navigationMatrix.a = navigationMatrix.d = (double)zoom_param / 100.;
//			navigationMatrix.e = navigationMatrix.f = navigationMatrix.b = navigationMatrix.c = 0;
//			synopsis_matrix = m_pThis->GetZoomOut();
//
//			matrix.a = synopsis_matrix.a / navigationMatrix.a;
//			matrix.b = 0;
//			matrix.c = 0;
//			matrix.d = synopsis_matrix.d / navigationMatrix.d;
//			matrix.e = (offx - scrx + synopsis_matrix.e) / navigationMatrix.a;
//			matrix.f = (offy - scry + synopsis_matrix.f) / navigationMatrix.d;
//			m_pThis->m_pLocalPen->Create((PBSynopsisTOC*)m_pThis->m_pTOC, bmp, synopsis_matrix, matrix);
			break;

		case STBI_EXIT:
			m_pThis->m_Tool = 0;
			SetEventHandler(PBMainFrame::main_handler);
			break;

		default:
			m_pThis->m_Tool = 0;
			break;
	}
}

int PBPDFToolBar::Note_Handler(int type, int par1, int par2)
{
	int replay = 0;

	if (type == EVT_POINTERDOWN)
	{
//		if (OpenPen(par1, par2))
//		{
//			return 1;
//		}
	}

	if (m_Tool == STBI_PEN || m_Tool == STBI_ERASER)
	{
		replay = (m_pLocalPen->PointerHandler(type, par1, par2) == 0);
		if (m_Tool == STBI_ERASER && !replay && type == EVT_POINTERUP)
		{
			this->RenderPage();
			this->OutputPage(BitmapFromScreen(0, 0, ScreenWidth(), ScreenHeight()-PanelHeight()),
						2, 0, 0, ScreenWidth(), ScreenHeight()-PanelHeight());
		}
	}

//	if (m_Tool == STBI_MARKER)
//	{
//		this->MarkerHandler(type, par1 + m_pDisplay->GetScrollX(), par2 + m_pDisplay->GetScrollY());
//	}

//	if (replay)
//	{
//		savetoc = 0;
//		comments_to_delete = 0;
//		m_vDeleteComments.clear();
//		m_pLocalPen->Replay(PBAdobeToolBar::ReplayHandler);
//		int size = m_vDeleteComments.size();
//		if (size > 0)
//		{
//			char tmp[200];
//			if (size == 1)
//			{
//				sprintf(tmp, "%s", GetLangText("@DeleteItem"));
//			}
//			else
//			{
//				sprintf(tmp, "%s: %d?", GetLangText("@DeleteComments"), size);
//			}
//			Dialog(ICON_QUESTION, GetLangText("@Delete"), tmp, "@Yes", "@No", DeleteComments);
//		}
//		else
//		{
//			m_pDisplay->RestoreCurrentLocation(false);
//			m_pPresenter->RefreshPage(-1);
//			m_pDisplay->Render(false, 2);
//			OpenSynopsisToolBar();
//			if (savetoc)
//				m_pTOC->SaveTOC();
//		}
//	}

	return 1;
}

int PBPDFToolBar::GetTool()
{
	return m_Tool;
}

ibitmap *PBPDFToolBar::GetPagePreview()
{
	return BitmapFromScreen(scrx, scry, ScreenWidth() - 2 * scrx, ScreenHeight()-PanelHeight());
}

void PBPDFToolBar::PageBack()
{
	// ok
	if (cpage > 0)
	{
		PBMainFrame* pMainFrame = PBMainFrame::GetThis();
		pMainFrame->onPagePrev();
		OpenToolBar();
	}
}

void PBPDFToolBar::PageForward()
{
	// ok
	if (cpage < npages)
	{
		PBMainFrame* pMainFrame = PBMainFrame::GetThis();
		pMainFrame->onPageNext();
		OpenToolBar();
	}
}

void PBPDFToolBar::OpenTOC()
{
	// ok
	PrepareActiveContent(1);
	m_pTOC->SetBackTOCHandler(synopsis_toc_handler);
	SynopsisContent Location(0, (long long)cpage << 40, NULL);
	m_pTOC->Open(&Location);
}

void PBPDFToolBar::PauseCalcPage()
{
}

void PBPDFToolBar::ContinueCalcPage()
{
}

void PBPDFToolBar::RenderPage()
{
	// bad
//	repaint_all(-2);
	// ok?
	PBMainFrame* pMainFrame = PBMainFrame::GetThis();
	pMainFrame->onShow();
}

void PBPDFToolBar::CreateSnapshot(PBSynopsisSnapshot *snapshot)
{
	// ok
	snapshot->Create((PBSynopsisTOC *)m_pTOC, (long long)cpage << 40);
}

#endif
