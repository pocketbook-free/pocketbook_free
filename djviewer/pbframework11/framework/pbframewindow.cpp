#include "pbframewindow.h"

PBFrameWindow::PBFrameWindow()
{
}

// virtual
int PBFrameWindow::OnDraw(bool force)
{
	// not implemented
	if (IsVisible() && (IsChanged() || force) )
	{
		// drawing code
		// ...
	}
	return 0;
}
