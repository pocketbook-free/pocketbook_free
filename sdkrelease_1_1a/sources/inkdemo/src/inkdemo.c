#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <dlfcn.h>
#include "inkview.h"

extern const ibitmap background, books, m3x3;

static char *sometext = "Portability Concerns\n\n"
"Of course, there are quite a few concerns that need to be thought about when considering bringing a program built for the desktop over to an embedded environment.\n"
"12345 67890"
;

static char kbuffer[36] = "Some text";

static tocentry contents[] = {

  {   1,   1,   1,  "Annotation" },
  {   1,   5,   5,  "Chapter 1" },
  {   2,  10,  10,  "Level 2 chapter 1" },
  {   2,  12,  12,  "Level 2 chapter 2" },
  {   2,  15,  15,  "Level 2 chapter 3" },
  {   3,  15,  15,  "Level 3 chapter 1 ejfhew wefdhwehwehwejh wehfdjkwehfkjwehkjf wenkjf nwekj" },
  {   3,  17,  17,  "Level 3 chapter 2 ejfhew wefdhwehwehwejh wehfdjkwehfkjwehkjf wenkjf nwekj" },
  {   3,  20,  20,  "Level 3 chapter 3 ejfhew wefdhwehwehwejh wehfdjkwehfkjwehkjf wenkjf nwekj" },
  {   1,  25,  25,  "Chapter 2" },
  {   2,  30,  30,  "C2 level 2 chapter 1" },
  {   3,  34,  34,  "C2 level 3 chapter 1" },
  {   4,  37,  37,  "C2 level 4 chapter 1" },
  {   4,  38,  38,  "C2 level 4 chapter 2" },
  {   3,  41,  41,  "C2 level 3 chapter 2" },
  {   3,  45,  45,  "C2 level 3 chapter 3" },
  {   2,  51,  51,  "index 1" },
  {   2,  55,  55,  "index 2" },
  {   2,  60,  60,  "index 3" },
  {   1,  102, 102,  "Chapter 3" },
  {   1,  105, 105,  "Chapter 4" },
  {   1,  110, 110,  "Chapter 5" },
  {   1,  115, 115,  "Chapter 6" },
  {   1,  130, 130,  "Chapter 7" },
  {   1,  150, 150,  "Chapter 8" },
  {   1,  200, 200,  "Chapter 9" },
  {   1,  250, 250,  "Chapter 10" },
  {   1,  300, 300,  "Chapter 11" },
  {   1,  350, 350,  "Chapter 12" },
  {   1,  400, 400,  "Chapter 13" },
  {   1,  410, 410,  "Chapter 14" },
  {   1,  420, 420,  "Chapter 15" },
  {   1,  430, 430,  "Chapter 16" },
  {   1,  440, 440,  "Chapter 17" },
  {   2,  445, 445,  "Ch 17 level 2 1" },
  {   2,  446, 446,  "Ch 17 level 2 2" },
  {   1,  450, 450,  "Chapter 18" },
  {   1,  460, 460,  "Chapter 19" },
  {   1,  470, 470,  "Chapter 20" },
  {   1,   70,  70,  "Conclusion" },
  {   1,   90,  90,  "End" },
  {   1,   95,  95,  "Whole end" },
  {   1,   12345,12345,  "Full end" },

};

static iconfig *testcfg = NULL;

static char *choice_variants[] = { "qqq", "www", "@Contents", "rrr", NULL };
static char *choice_variants2[] = { "q1", "q2", "q3", "q4", "w1", "w2", "w3", "w4", "e1", "e2", "e3", "e4", "r1", "r2", "r3", "r4", "t1", "t2", "t3", "t4", NULL };
static char *choice_variants3[] = { "q1", "q2", "q3", "q4", "w's", ":w1", ":w2", ":w3", ":w4", "e1", "e2", "e3", "e4", "r1", "r2", "r3", "r4", "t1", "t2", "t3", "t4", NULL };
static char *index_variants[] = { "value1", "value2", "value3", NULL };

static iconfigedit testce[] = {

  { "About device", NULL, CFG_INFO, "Name: PocketBook 310\nSerial: 123456789", NULL },
  { "Text edit", "param.textedit", CFG_TEXT, "qwerty", NULL },
  { "Choice", "param.choice", CFG_CHOICE, "eee", choice_variants },
  { "Many variants", "param.choice2", CFG_CHOICE, "qqq", choice_variants2 },
  { "Multi-level", "param.choice3", CFG_CHOICE, "cvb", choice_variants3 },
  { "Font", "param.font", CFG_FONT, "Arial,24", NULL },
  { "Font face", "param.fontface", CFG_FONTFACE, "Arial", NULL },
  { "Index", "param.index", CFG_INDEX, "2", index_variants },
  { "Time", "param.time", CFG_TIME, "1212396151", NULL },
  { NULL, NULL, 0, NULL, NULL}

};

int cindex=0;
int orient=0;
int timer_on=0;

int current_page = 5;
int bmklist[25] = { 1, 12, 25, 35, 79 };
long long poslist[25] = { 1, 12, 25, 35, 79 };
int bmkcount = 5;

ifont *arial8n, *arial12, *arialb12, *cour16, *cour24, *times20;

char **langs, **themes;

int main_handler(int type, int par1, int par2);
int screen2_handler(int type, int par1, int par2);


static const char *strings3x3[9] = {
  "Exit",
  "Open page",
  "Search...",
  "Goto",
  "Menu",
  "Quote",
  "Thumbnails",
  "Settings",
  "Something else"
};

static imenu menu1_lang[16];
static imenu menu1_theme[16];

static imenu menu1_notes[] = {
  { ITEM_ACTIVE, 119, "Create note", NULL },
  { ITEM_ACTIVE, 120, "View notes", NULL },
  { 0, 0, NULL, NULL }
};

static imenu menu1[] = {

  { ITEM_HEADER,   0, "Menu", NULL },
  { ITEM_ACTIVE, 101, "Screens", NULL },
  { ITEM_ACTIVE, 102, "Rotate", NULL },
  { ITEM_ACTIVE, 103, "TextRect", NULL },
  { ITEM_ACTIVE, 104, "Message", NULL },
  { ITEM_ACTIVE, 105, "Dialog", NULL },
  { ITEM_ACTIVE, 106, "Timer", NULL },
  { ITEM_ACTIVE, 107, "Menu3x3", NULL },
  { ITEM_ACTIVE, 108, "Hourglass", NULL },
  { ITEM_ACTIVE, 111, "Keyboard", NULL },
  { ITEM_ACTIVE, 112, "Configuration", NULL },
  { ITEM_ACTIVE, 113, "Page selector", NULL },
  { ITEM_ACTIVE, 116, "Bookmarks", NULL },
  { ITEM_ACTIVE, 117, "Contents", NULL },
  { ITEM_ACTIVE, 114, "Dither", NULL },
  { ITEM_ACTIVE, 115, "16-grays", NULL },
  { ITEM_SUBMENU, 0, "Notes", menu1_notes },
  { ITEM_SEPARATOR, 0, NULL, NULL },
  { ITEM_SUBMENU, 0, "Language", menu1_lang },
  { ITEM_SUBMENU, 0, "Theme", menu1_theme },
  { ITEM_SEPARATOR, 0, NULL, NULL },
  { ITEM_ACTIVE, 121, "Exit", NULL },
  { 0, 0, NULL, NULL }

};

static const unsigned char pic_example[] = {
  255,255,255,  0,  0,  0,  0,255,255,255,
  255,170,  0,255,255,255,255,  0,170,255,
  255,  0,255,255,255,255,255,255,  0,255,
    0,255,255,170,170,170,170,255,255,  0,
    0,255,255,170, 80, 80,170,255,255,  0,
    0,255,255,170, 80, 80,170,255,255,  0,
    0,255,255,170,170,170,170,255,255,  0,
  255,  0,255,255,255,255,255,255,  0,255,
  255,170,  0,255,255,255,255,  0,170,255,
  255,255,255,  0,  0,  0,  0,255,255,255,
};


void msg(char *s) {

  FillArea(350, 770, 250, 20, WHITE);
  SetFont(arialb12, BLACK);
  DrawString(350, 770, s);
  PartialUpdate(350, 770, 250, 20);

}

void mainscreen_repaint() {

  char buf[64];
  ibitmap *b;
  int i;

  ClearScreen();

  SetFont(arialb12, BLACK);
  DrawString(5, 2, "InkView library demo, press OK to open menu");

  DrawBitmap(0, 20, &background);
  DrawBitmap(120, 30, &books);

  DrawLine(5, 500, 595, 500, BLACK);
  DrawLine(5, 502, 595, 502, DGRAY);
  DrawLine(5, 504, 595, 504, LGRAY);
  DrawLine(19, 516, 20, 517, BLACK);
  DrawLine(22, 516, 23, 516, BLACK);

  for (i=5; i<595; i+=3) DrawPixel(i, 507, BLACK);

  DrawRect(5, 510, 590, 10, BLACK);

  for (i=0; i<256; i++) FillArea(35+i*2, 524, 2, 12, i | (i << 8) | (i << 16));

  b = BitmapFromScreen(0, 520, 600, 20);
  DrawBitmap(0, 550, b);
  free(b);
  InvertArea(0, 550, 600, 20);
  DimArea(0, 575, 600, 10, BLACK);

  if (! orient) {
    Stretch(pic_example, IMAGE_GRAY8, 10, 10, 10, 10, 600,  6,  6, 0);
    Stretch(pic_example, IMAGE_GRAY8, 10, 10, 10, 20, 600, 10,  6, 0);
    Stretch(pic_example, IMAGE_GRAY8, 10, 10, 10, 35, 600, 30,  6, 0);
    Stretch(pic_example, IMAGE_GRAY8, 10, 10, 10, 10, 610,  6, 10, 0);
    Stretch(pic_example, IMAGE_GRAY8, 10, 10, 10, 20, 610, 10, 10, 0);
    Stretch(pic_example, IMAGE_GRAY8, 10, 10, 10, 35, 610, 30, 10, 0);
    Stretch(pic_example, IMAGE_GRAY8, 10, 10, 10, 10, 625,  6, 30, 0);
    Stretch(pic_example, IMAGE_GRAY8, 10, 10, 10, 20, 625, 10, 30, 0);
    Stretch(pic_example, IMAGE_GRAY8, 10, 10, 10, 35, 625, 30, 30, 0);
  }

  SetFont(arial8n, BLACK);
  DrawString(350, 600, "Arial 8 with no antialiasing");
  SetFont(arial12, BLACK);
  DrawString(350, 615, "Arial 12 regular");
  SetFont(arialb12, BLACK);
  DrawString(350, 630, "Arial 12 bold");
  SetFont(cour16, BLACK);
  DrawString(350, 645, "Courier 16");
  SetFont(cour24, BLACK);
  DrawString(350, 660, "Courier 24");
  DrawSymbol(500, 660, ARROW_LEFT);
  DrawSymbol(520, 660, ARROW_RIGHT);
  DrawSymbol(540, 660, ARROW_UP);
  DrawSymbol(560, 660, ARROW_DOWN);
  SetFont(times20, BLACK);
  DrawString(350, 680, "Times 20");
  DrawSymbol(450, 680, ARROW_LEFT);
  DrawSymbol(470, 680, ARROW_RIGHT);
  DrawSymbol(490, 680, ARROW_UP);
  DrawSymbol(510, 680, ARROW_DOWN);

  //DrawTextRect(25, 400, 510, 350, sometext, ALIGN_LEFT);

  FullUpdate();

}

void example_textrect() {

  ClearScreen();
  SetFont(arial12, BLACK);


  DrawRect(5, 5, 190, 190, LGRAY);
  DrawTextRect(5, 5, 190, 190, sometext, ALIGN_LEFT | VALIGN_TOP);
  DrawRect(200, 5, 190, 190, LGRAY);
  DrawTextRect(200, 5, 190, 190, sometext, ALIGN_CENTER | VALIGN_TOP);
  DrawRect(395, 5, 190, 190, LGRAY);
  DrawTextRect(395, 5, 190, 190, sometext, ALIGN_RIGHT | VALIGN_TOP);

  DrawRect(5, 200, 190, 190, LGRAY);
  DrawTextRect(5, 200, 190, 190, sometext, ALIGN_LEFT | VALIGN_MIDDLE);
  DrawRect(200, 200, 190, 190, LGRAY);
  DrawTextRect(200, 200, 190, 190, sometext, ALIGN_CENTER | VALIGN_MIDDLE);
  DrawRect(395, 200, 190, 190, LGRAY);
  DrawTextRect(395, 200, 190, 190, sometext, ALIGN_RIGHT | VALIGN_MIDDLE);

  DrawRect(5, 395, 190, 190, LGRAY);
  DrawTextRect(5, 395, 190, 190, sometext, ALIGN_LEFT | VALIGN_BOTTOM);
  DrawRect(200, 395, 190, 190, LGRAY);
  DrawTextRect(200, 395, 190, 190, sometext, ALIGN_FIT | VALIGN_BOTTOM);
  DrawRect(395, 395, 190, 190, LGRAY);
  DrawTextRect(395, 395, 190, 190, sometext, ALIGN_RIGHT | VALIGN_BOTTOM);

  DrawRect(5, 590, 190, 190, LGRAY);
  DrawTextRect(5, 590, 190, 190, sometext, ALIGN_LEFT | VALIGN_TOP | ROTATE);
  DrawRect(200, 590, 190, 190, LGRAY);
  DrawTextRect(200, 590, 190, 190, sometext, ALIGN_CENTER | VALIGN_MIDDLE | ROTATE);
  DrawRect(395, 590, 100, 100, LGRAY);
  DrawTextRect(395, 590, 100, 100, sometext, DOTS);

  FullUpdate();

}

void example_dither() {

  int x, y, c;

  ClearScreen();
  SetFont(arial12, BLACK);

  DrawTextRect( 20,  10, 256, 15, "No dithering", ALIGN_CENTER);
  DrawTextRect(300,  10, 256, 15, "Threshold (BW)", ALIGN_CENTER);
  DrawTextRect( 20, 300, 256, 15, "Pattern", ALIGN_CENTER);
  DrawTextRect(300, 300, 256, 15, "Diffusion", ALIGN_CENTER);
  for (y=0; y<256; y++) {
    for (x=0; x<256; x++) {
      c = (x+y)/2;
      DrawPixel(20+x, 40+y, c | (c<<8) | (c<<16));
      DrawPixel(300+x, 40+y, c | (c<<8) | (c<<16));
      DrawPixel(20+x, 320+y, c | (c<<8) | (c<<16));
      DrawPixel(300+x, 320+y, c | (c<<8) | (c<<16));
    }
  }
  DitherArea(300, 40, 256, 256, 2, DITHER_THRESHOLD);
  DitherArea(20, 320, 256, 256, 4, DITHER_PATTERN);
  DitherArea(300, 320, 256, 256, 4, DITHER_DIFFUSION);
  FullUpdate();

}

void dialog_handler(int button) {

  msg(button == 1 ? "Choosed: yes" : "Choosed: no");

}

void menu3x3_handler(int pos) {

  char buf[16];
  sprintf(buf, "Menu: %i", pos);
  msg(buf);

}

void page_selected(int page) {

  char buf[16];
  current_page = page;
  sprintf(buf, "Page: %i", page);
  msg(buf);

}

void timer_proc() {

  static int value=0;
  char buf[16];

  sprintf(buf, "Timer: %i", ++value);
  msg(buf);
  SetHardTimer("MYTIMER", timer_proc, 1000);

}

void keyboard_entry(char *s) {

  char buf[64];

  sprintf(buf, "Entered text: %s", s);
  msg(buf);

}

void config_ok() {

  SaveConfig(testcfg);

}

void print_bmk() {

  int i;

  for(i=0; i<bmkcount; i++) {
    printf("%i %lld\n", bmklist[i], poslist[i]);
  }
  printf("\n");

}

void bmk_added(int page, long long pos) {

  char buf[64];
  sprintf(buf, "Added bookmark: page %i, pos %lld", page, pos);
  msg(buf);
  print_bmk();
  mainscreen_repaint();

}

void bmk_removed(int page, long long pos) {

  char buf[64];
  sprintf(buf, "Removed bookmark: page %i, pos %lld", page, pos);
  msg(buf);
  print_bmk();
  mainscreen_repaint();

}

void bmk_selected(int page, long long pos) {

  char buf[64];
  sprintf(buf, "Selected bookmark: page %i, pos %lld", page, pos);
  msg(buf);

}

void bmk_handler(int action, int page, long long pos) {

  switch (action) {
    case BMK_ADDED: bmk_added(page, pos); break;
    case BMK_REMOVED: bmk_removed(page, pos); break;
    case BMK_SELECTED: bmk_selected(page, pos); break;
  }

}

void menu1_handler(int index) {

  int i;

  cindex = index;

  if (index > 400 && index <= 499) {
    LoadLanguage(langs[index-400]);
    mainscreen_repaint();
  }

  switch (index) {

    case 101:
      SetEventHandler(screen2_handler);
      break;

    case 102:
      orient++; if (orient > 2) orient = 0;
      SetOrientation(orient);
      mainscreen_repaint();
      break;

    case 103:
      example_textrect();
      break;

    case 104:
      Message(ICON_INFORMATION, "Message", "This is a message.\n"
        "It will disappear after 5 seconds, or press any key", 5000);
      break;

    case 105:
      Dialog(ICON_QUESTION, "Dialog", "This is a dialog.\n"
        "Do you like it?", "Yes", "No", dialog_handler);
      break;

    case 106:
      if (timer_on) {
        ClearTimer(timer_proc);
        timer_on = 0;
      } else {
        SetHardTimer("MYTIMER", timer_proc, 0);
        timer_on = 1;
      }
      break;

    case 107:
      OpenMenu3x3(&m3x3, strings3x3, menu3x3_handler);
      break;


    case 108:
      ShowHourglass();
      sleep(2);
      HideHourglass();
      break;

    case 111:
      OpenKeyboard("Keyboard test", kbuffer, 35, 0, keyboard_entry);
      break;

    case 112:
      OpenConfigEditor("Configuration", testcfg, testce, config_ok, NULL);
      break;

    case 113:
      OpenPageSelector(page_selected);
      break;

    case 114:
      example_dither();
      break;

    case 115:
      ClearScreen();
      for (i=0; i<256; i++) FillArea(10+i*2, 10, i*2, 25, i | (i<<8) | (i<<16));
      FullUpdate();
      FineUpdate();
      break;

    case 116:
      for (i=0; i<bmkcount; i++) poslist[i] = bmklist[i] + 1000000;
      OpenBookmarks(current_page, current_page+1000000, bmklist, poslist, &bmkcount, 15, bmk_handler);
      break;

    case 117:
      OpenContents(contents, 42, current_page, page_selected);
      break;

    case 119:
      CreateNote("/test", "test", 0);
      break;

    case 120:
      OpenNotesMenu("/test", "test", 0);
      break;

    case 121:
      CloseApp();
      break;

  }

}

int main_handler(int type, int par1, int par2) {

  int i;

fprintf(stderr, "[%i %i %i]\n", type, par1, par2);

  if (type == EVT_INIT) {

    // occurs once at startup, only in main handler

    testcfg = OpenConfig("/mnt/ext1/test.cfg", testce);

    arial8n = OpenFont("DroidSans", 8, 0);
    arial12 = OpenFont("DroidSans", 12, 1);
    arialb12 = OpenFont("DroidSans", 12, 1);
    cour16 = OpenFont("cour", 16, 1);
    cour24 = OpenFont("cour", 24, 1);
    times20 = OpenFont("times", 20, 1);

    themes = EnumThemes();
    for (i=0; i<15; i++) {
      if (themes[i] == NULL) break;
      menu1_theme[i].type = ITEM_ACTIVE;
      menu1_theme[i].index = 300 + i;
      menu1_theme[i].text = themes[i];
      menu1_theme[i].submenu = NULL;
    }
    menu1_theme[i].type = 0;

    langs = EnumLanguages();
    for (i=0; i<15; i++) {
      if (langs[i] == NULL) break;
      menu1_lang[i].type = ITEM_ACTIVE;
      menu1_lang[i].index = 400 + i;
      menu1_lang[i].text = langs[i];
      menu1_lang[i].submenu = NULL;
    }
    menu1_lang[i].type = 0;

  }

  if (type == EVT_SHOW) {

    // occurs when this event handler becomes active
    mainscreen_repaint();
  }

  if (type == EVT_KEYPRESS) {

    switch (par1) {

      case KEY_OK:
        OpenMenu(menu1, cindex, 20, 20, menu1_handler);
        break;

      case KEY_BACK:
        CloseApp();
        break;

      case KEY_LEFT:
        msg("KEY_LEFT");
        break;

      case KEY_RIGHT:
        msg("KEY_RIGHT");
        break;

      case KEY_UP:
        msg("KEY_UP");
        break;

      case KEY_DOWN:
        msg("KEY_DOWN");
        break;

      case KEY_MUSIC:
        msg("KEY_MUSIC");
        break;

      case KEY_MENU:
        msg("KEY_MENU");
        break;

      case KEY_DELETE:
        msg("KEY_DELETE");
        break;

    }
  }

  if (type == EVT_EXIT) {
    // occurs only in main handler when exiting or when SIGINT received.
    // save configuration here, if needed
  }

  return 0;

}

int screen2_handler(int type, int par1, int par2) {

  static int ctab = 1;
  ifont *f;
  int i;
  char buf[64];

  if (type == EVT_SHOW) {

    ClearScreen();
    SetFont(times20, BLACK);
    DrawTextRect(0, 100, 600, 300, "Screen 2\nPress OK to return", ALIGN_CENTER);
    FillArea(0, ScreenHeight()-24, ScreenWidth(), 24, DGRAY);
    DrawRect(0, ScreenHeight()-24, ScreenWidth(), 2, BLACK);
    DrawTabs(NULL, ctab, 25);

    for (i=9; i<21; i++) {
      f = OpenFont("LiberationSans", i, 0);
      SetFont(f, BLACK);
      sprintf(buf, "%i qwertyuiopasdfghjklzxcvbnm", i);
      DrawString(2, 100+i*24, buf);
      CloseFont(f);
      f = OpenFont("LiberationSans-Bold", i, 0);
      SetFont(f, BLACK);
      DrawString(305, 100+i*24, buf);
      CloseFont(f);
    }
    FullUpdate();

  }

  if (type == EVT_KEYPRESS) {

    switch (par1) {

      case KEY_LEFT:
        if (ctab > 0) ctab--;
        break;
      case KEY_RIGHT:
        if (ctab < 24) ctab++;
        break;
      default:
        SetEventHandler(main_handler);
        return 0;
    }

    DrawTabs(NULL, ctab, 25);
    FullUpdate();

  }

  return 0;

}



int main(int argc, char **argv)
{
  InkViewMain(main_handler);
  return 0;
}

