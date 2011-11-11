#include "inkview.h"

static ibitmap *digits[10], *colon;
static ifont *font;
static int first = 1;
static int orient;

static void show_clock() {

	time_t t = time(NULL);
	struct tm *ctm = localtime(&t);
	int w;

	ClearScreen();
	int shiftx, shifty;

	shiftx = (ScreenWidth() - 800) / 2;
	shifty = (ScreenHeight() - 600) / 2;
	
	DrawBitmap(  8 + shiftx, 180 + shifty, digits[ctm->tm_hour/10]);
	DrawBitmap(190 + shiftx, 180 + shifty, digits[ctm->tm_hour%10]);
	DrawBitmap(372 + shiftx, 180 + shifty, colon);
	DrawBitmap(428 + shiftx, 180 + shifty, digits[ctm->tm_min/10]);
	DrawBitmap(610 + shiftx, 180 + shifty, digits[ctm->tm_min%10]);
	SetFont(font, BLACK);
	DrawTextRect(0 + shiftx, 525 + shifty, 800, 75, DateStr(t), ALIGN_CENTER | VALIGN_MIDDLE);

	first ? FullUpdate() : SoftUpdate();
	first = 0;
	w = 61-ctm->tm_sec;
	if (w < 15) w += 60;
	SetHardTimer("CLOCK", show_clock, w * 1000);

}

int clock_handler(int type, int par1, int par2)
{

	if (type==EVT_SHOW) show_clock();
	if (type==EVT_KEYPRESS)
	{
		SetOrientation(orient);
		CloseApp();
	}
	if (type==EVT_POINTERUP)
	{
		SetOrientation(orient);
		CloseApp();
	}
	if (type==EVT_ORIENTATION) {
		//SetOrientation(par1);
		if (ScreenWidth() > 900 || ScreenHeight() > 900)
		{
			SetOrientation(par1);
		}
		else
		{
			if (par1 == 0 || par1 == 2)
				SetOrientation(2);
			else
				SetOrientation(1);
		}
		show_clock();
	}
	return 0;

}

int main(int argc, char **argv)
{
	char buf[64];
	char font_name[128];
	int i;

	OpenScreen();
	SetAutoPowerOff(0);

	for (i=0; i<10; i++) {
		sprintf(buf, "clock_d%i", i);
		digits[i] = GetResource(buf, NULL);
		if (digits[i] == NULL) return 0;
	}
	colon = GetResource("clock_colon", NULL);
	sprintf(font_name, "#%s,36,0", DEFAULTFONTB);
	font = GetThemeFont("clock.font", font_name);

	orient = GetOrientation();

	// Set screen orientation: 0=portrait, 1=landscape 90, 2=landscape 270, 3=portrait 180
	if (ScreenWidth() > 900 || ScreenHeight() > 900)
	{
	}
	else
	{
		if (orient == 0 || orient == 2)
			SetOrientation(2);
		else
			SetOrientation(1);
	}
	InkViewMain(clock_handler);
	return 0;
}

