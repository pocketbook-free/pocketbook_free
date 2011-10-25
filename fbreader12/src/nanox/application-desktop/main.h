/*
 * Copyright (C) 2008 Most Publishing
 * Copyright (C) 2008 Dmitry Zakharov <dmitry-z@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef FB_MAIN_H
#define FB_MAIN_H

#include <synopsis/synopsis.h>

#define MAXNOTEPADS 15

#define MAXLINKS 100
#define DEFREADERFONT "default,24"

#define CONVTMP "/tmp/fbreader.temp"

typedef struct tdocstate_s {

	int magic;
	long long position;
	char preformatted;
	char  reserved11;
	short reserved2;
	short reserved3;
	short reserved4;
	char encoding[16];
	int nbmk;
	long long bmk[30];
	int text_direction;

} tdocstate;

extern char *OriginalName;
extern char *book_title;
extern char *book_author;

//$$$ extern char *encoding_override;
extern int break_override;
//$$$ extern int book_open_ok;

extern "C" const ibitmap searchbm;
extern "C" const ibitmap arrow_back;
extern "C" const ibitmap ci_font, ci_enc, ci_spacing, ci_margins, ci_textfmt, ci_hyphen, ci_about, ci_textdir, ci_font_size_step;
extern "C" const ibitmap ci_image;
extern "C" const ibitmap dl_icon;

extern "C" const ibitmap def_w_back, def_w_stop, def_w_reload, def_w_fav, def_w_url, def_w_search;
extern "C" const ibitmap def_w_save, def_w_hide;

extern int is_webpage;
extern int webpanel;
extern int downloading;
extern char *current_url;
extern int load_images;
extern int fontChangeStep;
typedef struct hlink_s {
        short x;
        short y;
        short w;
        short h;
        int kind;
        std::string type;
        char *href;
} hlink;

void prev_section();
void next_section();
void new_synopsis_note();
void new_synopsis_bookmark();
void OpenSynopsisToolBar();
void save_page_note();
void jump_pages(int n);
void stop_jump();
void select_page(int page);
int get_calc_progress();
void open_synopsis_contents();
void refresh_page(int update);
void repaint_all(int update_mode);
void back_link();
void open_links();
void open_config();
void show_about();
void search_enter(char *text);
void font_change(int d);
void show_hide_panel();
void calc_pages();
int one_page_back(int update_mode);
int one_page_forward(int update_mode);
int get_npage();
int get_cpage();
void make_update();
void main_show();
void save_screenshot();
int ornevt_handler(int n);
void rotate_handler(int n);
void save_state();
int set_page(int page);
int get_page_word_list(SynWordList **word_list, int *wlist_len);
int get_page_word_list(iv_wlist **word_list, int *wlist_len);
void show_hide_webpanel(int n);
void draw_webpanel(int update);
int webpanel_height();
void webpanel_update_orientation();
unsigned long cachename(char *s);
int is_weburl(char *url);
int is_cached_url(char *path);
void set_current_url(char *url);
int webpanel_handler(int type, int par1, int par2);
void draw_download_panel(char *text, int update);
void start_download();
void start_download_images();
void stop_download();
int pointer_handler(int type, int par1, int par2);

long long get_position();
void calc_timer();
long long pack_position(int para, int word, int letter);
void unpack_position(long long pos, int *para, int *word, int *letter);
long long page_to_position(int page);
int position_to_page(long long position);
int is_footnote_mode();
long long get_end_position();
void set_position(long long pos);
void restore_current_position();

void ChangeSettings(fb2_settings settings);

void PrepareSynTOC(int opentoc);

void read_styles();
void RestartApplication(bool saveState);
extern std::string stylesFileName;

#endif
