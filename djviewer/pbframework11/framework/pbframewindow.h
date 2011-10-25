#ifndef PBFRAMEWINDOW_H
#define PBFRAMEWINDOW_H
#include "pbwindow.h"

class PBFrameWindow : public PBWindow
{
public:
    PBFrameWindow();

	int OnDraw(bool force);
};

#endif // PBFRAMEWINDOW_H
