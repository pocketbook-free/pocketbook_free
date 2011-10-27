#ifndef MAIN_H
#define MAIN_H

#include <inkview.h>
#ifdef USESYNOPSIS

#include <synopsis/synopsis.h>
#include "pbnotes.h"


extern SynopsisContent *Content;
extern SynopsisNote *Note;
extern SynopsisBookmark *Bookmark;
extern TSynopsisWordList List;
extern SynopsisTOC m_TOC;
void PrepareActiveContent(int opentoc);
void synopsis_toc_handler(char *cposition);
void new_synopsis_bookmark();
void new_synopsis_note();
#endif
extern int * cpage_main;

int main_handler(int type, int par1, int par2);

int get_page_word_list(iv_wlist **word_list, int *wlist_len, int page);
void turn_page(int n);
void rotate_handler(int n);
void show_hide_panel();
void search_enter(char* text);
void page_selected(int page);
void page_selected_background(int page);
void set_panel_height();
int pointer_handler(int type, int par1, int par2);
void main_nextpage();
void main_prevpage();
int get_cpage();
int get_npage();
int isReflowMode();
void jump_pages(int n);
void save_screenshot();
void save_settings();
void main_show();
void new_bookmark();
void open_bookmarks();
void new_note();
void save_page_note();
void open_contents();
void zoom_in();
void zoom_out();
void pdf_mode();
void apply_scale_factor(int zoom_type, int new_scale);
int get_zoom_param(int& zoom_type, int& zoom_param);

#endif // MAIN_H
