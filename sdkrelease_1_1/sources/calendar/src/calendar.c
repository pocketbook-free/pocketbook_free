#include "inkview.h"

#define MilleniumFirstDay 6

static char calendar[12][43];
static int months_by_x, months_by_y;
static int FirstDay;
static int year;
static char step=1;
static char MonthLong[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static char LongYear=0;
static char *MonthName[]={"@January", "@February", "@March", "@April", "@May", "@June", "@July", "@August", "@September", "@October", "@November", "@December"};
static char *WeekName[]={"@Mon", "@Tue", "@Wed", "@Thu", "@Fri", "@Sat", "@Sun"};
static int sx, sy, dx, dy, lh, aw;

static ifont  *cal_title_font, *cal_month_font, *cal_day_font, *cal_date_font, *cal_date2_font;

static time_t CurTime;
static struct tm *CurDate;

static struct {
	int tfontsize;
	int mfontsize;
	int dfontsize;
	int sx_p;
	int sy_p;
	int dx_p;
	int dy_p;
	int sx_l;
	int sy_l;
	int dx_l;
	int dy_l;
	int aw;
	int lh;
} params[] = {
	{ 26, 20, 16,   8,  80, 197, 180,   16, 72,  197, 175,   25, 20 },
	{ 36, 26, 17,  20, 120, 270, 260,   50, 100, 275, 250,   34, 26 },
}, *cpar;

static int HeaderWidth;
static int SymbolWidth;

void GenerateCalendar()
{
 int curmonth=0, curday=1, weekday;
 int i;
 weekday=FirstDay;
 if (year%4==0)
  	LongYear=1;
  	else
  	LongYear=0;
 if (LongYear && year%100==0 && year%400!=0)
  	LongYear=0;
 while (curmonth!=12)
 	{
 	 curday=1;
 	 while (curday%7!=weekday)
 	 	{
 	 	 calendar[curmonth][curday]=0;
 	 	 curday++;
 	 	}
 	 for (i=1; i<=MonthLong[curmonth]; i++)
 	 	{
 	 	 calendar[curmonth][curday]=i;
 	 	 curday++;
 	 	 weekday++;
 	 	 weekday%=7;
 	 	}
 	 if (curmonth==1 && LongYear)
 	 	{
	 	 calendar[curmonth][curday]=29;
 	 	 curday++;
 	 	 weekday++;
 	 	 weekday%=7;
 	 	}
 	 while (curday<43)
 	 	{
 	 	 calendar[curmonth][curday]=0;
 	 	 curday++;
 	 	}
 	 curmonth++;
 	}
}

void DrawCalendar()
{
 int i, j, k;
 char s[100];

	if (GetOrientation() == 0 || GetOrientation() == 3) {
		months_by_x = 3;
		months_by_y = 4;
		dx = cpar->dx_p;
		dy = cpar->dy_p;
		sx = cpar->sx_p;
		sy = cpar->sy_p;
		aw = cpar->aw;
		lh = cpar->lh;
	} else {
		months_by_x = 4;
		months_by_y = 3;
		dx = cpar->dx_l;
		dy = cpar->dy_l;
		sx = cpar->sx_l;
		sy = cpar->sy_l;
		aw = cpar->aw;
		lh = cpar->lh;
	}

 ClearScreen();
 //FillArea(0, 0, 600, 800, BLACK);
 //FillArea(10, 10, 580, 780, WHITE);
 SetFont(cal_title_font, BLACK);
 sprintf(s, "\x11    %d    \x12", year);
 DrawTextRect(0, 5, ScreenWidth(), 30, s, ALIGN_CENTER);
 HeaderWidth = StringWidth(s);
 SymbolWidth = CharWidth('A');

 for (i=0; i<months_by_y; i++)
 	for (j=0; j<months_by_x; j++)
 		{
		 int mn = i*months_by_x + j;
 		 SetFont(cal_month_font, BLACK);
 		 DrawTextRect(sx+dx*j, sy-(lh*3)/2+dy*i, dx, 1, GetLangText(MonthName[mn]), ALIGN_CENTER);
 		 SetFont(cal_day_font, BLACK);
 		 for (k=0; k<6; k++)
	 		DrawTextRect(sx+4+dx*j-(IsRTL() ? 10 : 0), sy+lh*k+dy*i, 50, 1, GetLangText(WeekName[k]), ALIGN_LEFT | RTLAUTO);
  		 SetFont(cal_day_font, DGRAY);
		 DrawTextRect(sx+4+dx*j-(IsRTL() ? 10 : 0), sy+lh*6+dy*i, 50, 1, GetLangText(WeekName[6]), ALIGN_LEFT | RTLAUTO);
	 	 for (k=1; k<43; k++)
	 	 	 if (calendar[mn][k]>0)
	 	 	 	{
	 	 		 if (k%7!=0) 
	 	 		 	SetFont(cal_date_font, BLACK);
	 	 		 	else
	 	 		 	SetFont(cal_date2_font, DGRAY);
 		 	 	 sprintf(s, "%d", calendar[mn][k]);
				 DrawTextRect(sx+(aw*8)/5+( (int) ((k-1)/7) )*aw+dx*j, sy+lh*((k-1)%7)+dy*i, aw, 1, s, ALIGN_CENTER);
				}
 		}
 if (year==CurDate->tm_year+1900)
 	{
 	 k=1;
 	 while (calendar[CurDate->tm_mon][k]!=CurDate->tm_mday) k++;
 	 InvertArea(sx+(aw*8)/5+1+( (int) ((k-1)/7) )*aw+dx*(CurDate->tm_mon%months_by_x), sy+lh*((k-1)%7)+dy*(CurDate->tm_mon/months_by_x), aw-2, lh-1);
 	 DrawRect(sx+(aw*8)/5+( (int) ((k-1)/7) )*aw+dx*(CurDate->tm_mon%months_by_x), sy+lh*((k-1)%7)+dy*(CurDate->tm_mon/months_by_x)+1, aw, lh-3, BLACK);
 	}
// SetFont(cour32, BLACK);
// sprintf(s, "%d %s %d", CurDate->tm_mday, GetLangText(MonthName[CurDate->tm_mon]), CurDate->tm_year+1900);
// DrawTextRect(50, 720, 500, 30, s, ALIGN_CENTER);
// DitherArea(0, 0, 600, 800, 4, 1);
 DrawPanel(NULL, CurrentDateStr(), NULL, -1);
 FullUpdate();
}

int calendar_handler(int type, int par1, int par2)
{
 if (type == EVT_SHOW) {
 	 GenerateCalendar();
 	 DrawCalendar();
 }
 if (type == EVT_ORIENTATION) {
	 SetOrientation(par1);
 	 GenerateCalendar();
 	 DrawCalendar();
 }
 if (type==EVT_KEYPRESS)
 	switch(par1)
 		{
 		 case KEY_LEFT:
		 case KEY_PREV:
		 case KEY_PREV2:
			if (year>1)
 		 		{
 		 		 year--;
		 	 	 if (year%4==0)
 	 			 	LongYear=1;
		 	 	 	else
 	 			 	LongYear=0;
		 	 	 if (LongYear && year%100==0 && year%400!=0)
 	 			 	LongYear=0;
 		 		 FirstDay=(FirstDay+371-365*(1-LongYear)-366*LongYear)%7;
			 	 GenerateCalendar();
			 	 DrawCalendar();
 		 		}
 		 	break;

 		 case KEY_RIGHT:
		 case KEY_NEXT:
		 case KEY_NEXT2:
			if (year<10000)
 		 		{
		 	 	 if (year%4==0)
 	 			 	LongYear=1;
		 	 	 	else
 	 			 	LongYear=0;
		 	 	 if (LongYear && year%100==0 && year%400!=0)
 	 			 	LongYear=0;
 		 		 FirstDay=(FirstDay+365*(1-LongYear)+366*LongYear)%7;
 		 		 year++;
			 	 GenerateCalendar();
			 	 DrawCalendar();
 		 		}
 		 	break;

		 case KEY_OK: 		 
 		 case KEY_BACK:
			CloseApp();
 		 	break;
 		}

#ifndef BASIC
  if (type == EVT_POINTERUP)
  {
	if (par1 > (ScreenWidth() - HeaderWidth) / 2 - 10 && par1 < (ScreenWidth() - HeaderWidth) / 2 + SymbolWidth + 10 &&
			par2 > 0 && par2 < 40)
		{
			calendar_handler(EVT_KEYPRESS, KEY_LEFT, 0);
		}
	if (par1 > (ScreenWidth() + HeaderWidth) / 2 - SymbolWidth - 10 && par1 < (ScreenWidth() + HeaderWidth) / 2 + 10 &&
			par2 > 0 && par2 < 40)
		{
			calendar_handler(EVT_KEYPRESS, KEY_RIGHT, 0);
		}
  }
#endif

  return 0;

}

int main(int argc, char **argv) {

	OpenScreen();
	// SetOrientation

	int sw = ScreenWidth();
	int sh = ScreenHeight();
	fprintf(stderr, "[%ix%i]\n", sw, sh);

	if (sw >= 1200 || sh >= 1200) {
		cpar = &params[1];
	} else {
		cpar = &params[0];
	}

	cal_title_font = OpenFont(DEFAULTFONTB, cpar->tfontsize, 1);
	cal_month_font = OpenFont(DEFAULTFONTB, cpar->mfontsize, 1);
	cal_day_font = OpenFont(DEFAULTFONTB, cpar->dfontsize - (IsRTL() ? 2 : 0), 1);
	cal_date_font = OpenFont(DEFAULTFONT, cpar->dfontsize, 1);
	cal_date2_font = OpenFont(DEFAULTFONTB, cpar->dfontsize, 1);

 	CurTime=time(NULL);
 	CurDate=localtime(&CurTime);
 	if (year>CurDate->tm_year+1900)
 		step=-step;
 	FirstDay=MilleniumFirstDay;
 	while (year!=CurDate->tm_year+1900)
 		{
 		 if (year%4==0)
 		 	LongYear=1;
 		 	else
 		 	LongYear=0;
 		 if (LongYear && year%100==0 && year%400!=0)
 		 	LongYear=0;
 		 FirstDay=(FirstDay+365*(1-LongYear)+366*LongYear)%7;
 		 year+=step;
 		}
	InkViewMain(calendar_handler);
	return 0;

}

