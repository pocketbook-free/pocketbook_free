#ifndef MAIN_H
#define MAIN_H

#ifdef WIN32
#undef WIN32
#include "../libdjvu/ddjvuapi.h"
#define WIN32
#else
#include "../libdjvu/ddjvuapi.h"
#endif

#define MAXNOTEPADS 15

typedef struct tdocstate_s {

	int magic;
	int page;
	int offx;
	int offy;
	int scale;
	int orient;
	int reserved2;
	int nbmk;
	int bmk[30];

} tdocstate;

void out_page(int full);
void draw_page_image();
int main_handler(int type, int par1, int par2);
void menu_handler(int pos);


extern int scale;
extern int offx, offy, oldoffy;
extern ddjvu_page_t* last_decoded_page;




int get_npage();
int get_cpage();
void main_show();
void save_settings();
void open_contents();
void open_quickmenu();
void prev_page();
void next_page();
void jump_pages(int n);
void jump_pr10();
void jump_nx10();
void stop_jump();
void open_pageselector();
void first_page();
void last_page();
void new_note();
void save_page_note();
void open_notes();
void open_dictionary();
void zoom_in();
void zoom_out();
void open_rotate();
int ornevt_handler(int n);
void main_menu();
void exit_reader();
void open_mp3();
void mp3_pause();
void volume_up();
void volume_down();
void turn_page(int n);
void open_bookmarks();
void new_bookmark();
void start_search();
void show_hide_panel();
void open_new_zoomer();
void handle_navikey(int key);
void open_synopsis_contents();
void new_synopsis_bookmark();
void new_synopsis_note();
void apply_scale_factor(int zoom_type, int new_scale);
int get_zoom_param(int& zoom_type, int& zoom_param);

#endif // MAIN_H
