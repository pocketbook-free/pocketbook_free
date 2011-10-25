#ifndef PBUTILITIES_H
#define PBUTILITIES_H

#include "pbrect.h"
#include "inkview.h"
#include <sys/timeb.h>
#include <stdint.h>

static void MakeInvert(std::vector<PBRect> &rectList)
{
	if (!rectList.empty())
	{
		std::vector<PBRect>::iterator iter = rectList.begin();
		while (iter != rectList.end())
		{
			InvertArea((*iter).GetX(), (*iter).GetY(), (*iter).GetWidth(), (*iter).GetHeight());
			++iter;
		}
		PBRect updateRect = PBRect::GetUpdateRectange(rectList);
		PartialUpdate(updateRect.GetX(), updateRect.GetY(), updateRect.GetWidth(), updateRect.GetHeight());
	}
	return;
}

static void MakeInvert(PBRect &rect)
{
	InvertArea(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
	PartialUpdate(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
}

static double getCurrentTime()
{
	struct timeb timeVal;
	ftime(&timeVal);
	return timeVal.time*1000 + timeVal.millitm;
}

static double s_time = 0;
static void _start_time_trace()
{
	s_time = getCurrentTime();
}

static void _trace_time(const char * message)
{
	double diff = getCurrentTime() - s_time;
	fprintf( stderr, "----%s >>> time %4.0fms\n", message, diff);
	s_time = getCurrentTime();
}

static int isRegistered()
{
	return iv_ipc_cmd(MSG_REG_CHECK, 0) == MSG_OK ? 1 : 0;
}

#endif // PBUTILITIES_H
