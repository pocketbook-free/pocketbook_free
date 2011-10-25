#include <stdio.h>
#include <stdlib.h>
#include <sys/statvfs.h>
#include <inkview.h>
#include <inkinternal.h>

#include "main.h"

#define MAXURLS 30
#define CACHEMINFREE 2000000
#define MAXMENUFAVORITES 100

#define WPA_BACK        1
#define WPA_STOP        2
#define WPA_RELOAD      3
#define WPA_FAVORITE    4
#define WPA_URL         5
#define WPA_SEARCH      6
#define WPA_SAVE        7
#define WPA_HIDE        8
#define WPA_URLHISTORY 15

int is_webpage = 0;
int webpanel = 0;
int downloading = 0;
int load_images = 1;

char *current_url = NULL;

char **dl_imageurl = NULL;
char **dl_imagepath = NULL;
int dl_imagecount = 0;
int dl_currentimg = 0;
int dl_updated = 0;
int dl_gotonext = 0;

static int sid = 0;
static int dl_progress = 0;
static int dl_percent = 0;
static int dl_counter = 0;
static int urlbuttonx = 0;
static int favbuttonx = 0;
static int webpanelheight = -1;
static int current_action = -1;
static char *savetext = NULL;
static ifont *url_font, *emptyurl_font;
static imenu *fav_menu = NULL;
static int fav_menu_items = 0;

static char dirbuf[MAXPATHLENGTH];
static char kbd_buffer[1024];
static char search_buffer[256];
static imenu urlhistory[MAXURLS+1];
static char current_file[64];
static char *dirslash;

static struct {
	int x;
	int y;
	int w;
	int h;
	const char *resource;
	const ibitmap *cbm;
	ibitmap *bm;
	ibitmap *bma;
	int action;
} buttons[] = {
	{ 0, 0, 0, 0, "w_back",    &def_w_back,    NULL, NULL, WPA_BACK },
	{ 0, 0, 0, 0, "w_stop",    &def_w_stop,    NULL, NULL, WPA_STOP },
	{ 0, 0, 0, 0, "w_reload",  &def_w_reload,  NULL, NULL, WPA_RELOAD },
	{ 0, 0, 0, 0, "w_fav",     &def_w_fav,     NULL, NULL, WPA_FAVORITE },
	{ 0, 0, 0, 0, "w_url",     &def_w_url,     NULL, NULL, WPA_URL },
	{ 0, 0, 0, 0, "w_search",  &def_w_search,  NULL, NULL, WPA_SEARCH },
	{ 0, 0, 0, 0, "w_save",    &def_w_save,    NULL, NULL, WPA_SAVE },
	{ 0, 0, 0, 0, "w_hide",    &def_w_hide,    NULL, NULL, WPA_HIDE },
	{ 0, 0, 0, 0, NULL,        NULL,           NULL, NULL, 0 },
};
int nbuttons = 0;

static struct {
	const char *type;
	const char *ext;
} mimetypes[] = {

	{ "text/plain",                    ".txt" },
	{ "text/html",                     ".html" },
	{ "text/x-server-parsed-html",     ".html" },
	{ "application/msword",            ".doc" },
	{ "image/gif",                     ".gif" },
	{ "image/bmp",                     ".bmp" },
	{ "image/x-windows-bmp",           ".bmp" },
	{ "image/tiff",                    ".tif" },
	{ "image/x-tiff",                  ".tif" },
	{ "application/pdf",               ".pdf" },
	{ "application/x-pdf",             ".pdf" },
	{ "application/rtf",               ".rtf" },
	{ "application/x-rtf",             ".rtf" },
	{ "text/richtext",                 ".rtf" },
	{ "application/x-compressed",      ".zip" },
	{ "application/x-zip-compressed",  ".zip" },
	{ "application/zip",               ".zip" },
	{ NULL, NULL }

};

static void init_panel() {

	char buf[32];
	int i, x;

	if (webpanelheight > 0) return;
	x = 0;
	for (i=0; buttons[i].action!=0; i++) {
		if (buttons[i].bm != NULL && buttons[i].bm != buttons[i].cbm) free(buttons[i].bm);
		sprintf(buf, "%s", buttons[i].resource);
		buttons[i].bm = GetResource(buf, (ibitmap *) buttons[i].cbm);
		if (buttons[i].bma != NULL) free(buttons[i].bma);
		sprintf(buf, "%s_a", buttons[i].resource);
		buttons[i].bma = GetResource(buf, NULL);
		if (webpanelheight < buttons[i].bm->height) webpanelheight = buttons[i].bm->height;
		buttons[i].x = x;
		buttons[i].y = 0;
		buttons[i].w = buttons[i].bm->width;
		buttons[i].h = buttons[i].bm->height;
		if (buttons[i].action==WPA_URL) urlbuttonx = x;
		if (buttons[i].action==WPA_FAVORITE) favbuttonx = x;
		x += buttons[i].w;
	}
	nbuttons = i;
	url_font = OpenFont(DEFAULTFONT, 16, 1);
	emptyurl_font = OpenFont(DEFAULTFONTI, 16, 1);

}

void webpanel_update_orientation() {

	if (webpanelheight > 0) {
		webpanelheight = 0;
		init_panel();
	}

}

void draw_webpanel(int update) {	

	char *text;
	int i;
	init_panel();
	for (i=0; i<nbuttons; i++) {
		DrawBitmap(buttons[i].x, buttons[i].y, buttons[i].bm);
		if (buttons[i].action == WPA_URL) {
			text = current_url ? current_url : GetLangText("@enter_url");
			SetFont(current_url ? url_font : emptyurl_font, current_url ? BLACK : DGRAY);
			DrawTextRect(buttons[i].x+10, buttons[i].y+6, buttons[i].w-35, buttons[i].h-12, text, ALIGN_LEFT | VALIGN_MIDDLE | DOTS);
		}
	}
	if (update == 1) PartialUpdateBW(0, 0, ScreenWidth(), webpanelheight);
	if (update == 2) PartialUpdate(0, 0, ScreenWidth(), webpanelheight);

}

static void draw_button(int a) {

	int i;
	for (i=0; i<nbuttons; i++) {
		if (buttons[i].action == a && buttons[i].bma != NULL) {
			DrawBitmap(buttons[i].x, buttons[i].y, buttons[i].bma);
			PartialUpdateBW(buttons[i].x, buttons[i].y, buttons[i].w, buttons[i].h);
			break;
		}
	}

}

int webpanel_height() {

	init_panel();
	return webpanelheight;

}


int is_weburl(char *url) {

	if (strncasecmp(url, "http://", 7) == 0) return 1;
	if (strncasecmp(url, "ftp://", 6) == 0) return 1;
	if (strncasecmp(url, "https://", 8) == 0) return 1;
	return 0;

}

void set_current_url(char *url) {

	if (current_url) free(current_url);
	current_url = (url == NULL) ? NULL : strdup(url);

}

int is_cached_url(char *path) {

	char *buf = (char *) malloc(67000);
	FILE *f = fopen(WEBCACHEINDEX, "rb");
	int r = 0;
	while (f != NULL && fgets(buf, 67000, f) != NULL) {
		trim_right(buf);
		if (strncasecmp(buf, path, strlen(path)) == 0) {
			set_current_url(buf+strlen(path)+1);
			r = 1;
			break;
		}
	}
	if (f != NULL) fclose(f);
	free(buf);
	return r;

}

static int button_action(int x, int y) {

	int i;
	for (i=0; i<nbuttons; i++) {
		if (x < buttons[i].x) continue;
		if (y < buttons[i].y) continue;
		if (x > buttons[i].x+buttons[i].w) continue;
		if (y > buttons[i].y+buttons[i].h) continue;
		if (buttons[i].action == WPA_URL && x > buttons[i].x+buttons[i].w-30) {
			return WPA_URLHISTORY;
		}
		return buttons[i].action;
	}
	return 0;

}

unsigned long cachename(char *s) {

	unsigned long r = 0;
	char *p = strchr(s, '#');
	if (p == NULL) p = s + strlen(s);
	while (s < p) r = *(s++) + (r << 6) - r;
	return r;

}

static int get_cache_item(char *url, char *buf) {

	DIR *dir;
	struct dirent *dp;
	char tname[10];
	int ok=0;

	unsigned long h = cachename(url);
	sprintf(tname, "%08x", (unsigned int)h);

	dir = iv_opendir(WEBCACHE);
	if (dir == NULL) return 0;
	while ((dp = iv_readdir(dir)) != NULL) {
		if (dp->d_name[0] == '.') continue;
		if (strlen(dp->d_name) > 15) continue;
		if (strncmp(dp->d_name, tname, 8) == 0) {
			sprintf(buf, "%s/%s", WEBCACHE, dp->d_name);
			ok=1;
			break;
		}
	}
	iv_closedir(dir);
	return ok;

}

static void cache_cleanup() {

	char buf[64], cfile[64];
	DIR *dir;
	struct dirent *dp;
	struct statvfs vst;
	struct stat st;
	time_t mint;
	unsigned long avail;

	if (statvfs(WEBCACHE, &vst) < 0) return;
	avail = vst.f_bfree * vst.f_frsize;

	mint = time(NULL);
	cfile[0] = 0;

#ifndef EMULATOR
	if (avail < CACHEMINFREE) {
#else
	static int ctr=0;
	if (++ctr >= 20) {
		ctr = 0;
#endif

		dir = iv_opendir(WEBCACHE);
		if (dir == NULL) return;
		while ((dp = iv_readdir(dir)) != NULL) {
			if (dp->d_name[0] == '.') continue;
			sprintf(buf, "%s/%s", WEBCACHE, dp->d_name);
			if (stat(buf, &st) < 0)  continue;
			if (! S_ISREG(st.st_mode)) continue;
			if (strcasecmp(buf+strlen(buf)-6, ".image") == 0) {
				st.st_atime -= 120;
			}
			if (st.st_atime < mint) {
				mint = st.st_atime;
				strcpy(cfile, buf);
			}
		}
		iv_closedir(dir);

		if (cfile[0] != 0) {
			fprintf(stderr, "cache cleanup: %s\n", cfile);
			unlink(cfile);
		}

	}		

}

static void url_entered(char *text) {

	char buf[1024];
	FILE *f;

	if (text == NULL) return;
	if (! is_weburl(text)) {
		sprintf(buf, "http://%s", text);
		set_current_url(buf);
	} else {
		set_current_url(text);
	}
	f = iv_fopen(URLHISTORY, "a");
	if (f != NULL) {
		fprintf(f, "%s\n", current_url);
		iv_fclose(f);
		sync();
	}
	start_download();

}

static void search_entered(char *text) {

	char buf[1024], *p, *pp;

	if (text == NULL) return;
	strcpy(buf, "http://www.google.com/search?q=");
	p = text;
	pp = buf + strlen(buf);
	while (*p) {
		if (*p == ' ') {
			*(pp++) = '+';
		} else if (isalnum(*p) || *p=='-' || *p=='_' || *p=='.') {
			*(pp++) = *p;
		} else {
			pp += sprintf(pp, "%%%02X", (*p & 0xff));
		}
		p++;
	}
	*pp = 0;
	set_current_url(buf);
	start_download();

}

static void savename_entered(char *name) {

	if (name != NULL) {
		if (! copy_file(OriginalName, dirbuf)) {
			Message(ICON_ERROR, "@Error", "@Cant_move", 5000);
		}
	}
	if (dirslash) *dirslash = 0;

}

static void savedir_selected(char *path) {

	char *s;

	if (path == NULL) return;
	dirslash = path + strlen(path);
	strcpy(dirslash, "/");
	s = strrchr(OriginalName, '/');
	if (s) strcpy(dirslash+1, s+1);
	OpenKeyboard("@SaveAs", dirslash+1, 60, 0, savename_entered);

}

static void button_goback() {
	back_link();
}

static void button_stop() {
	stop_download();
}

static void button_reload() {
	if (current_url == NULL) return;
	if (get_cache_item(current_url, current_file)) {
		unlink(current_file);
	}
	start_download();
}

static void favorites_handler(int idx) {

	char buf[160], *p;
	FILE *f;
	int i;

	if (idx >= 0 && idx < 1000) {
		ShowHourglass();
		OpenBook((char *)(fav_menu[idx].submenu), NULL, 0);
	}

	if (idx == 1000) {
		if (current_url == NULL || current_url[0] == 0)  return;
		if (book_title == NULL || book_title[0] == 0) {
			snprintf(buf, sizeof(buf)-6, "%s/%s", FAVORITES, current_url);
		} else {
			snprintf(buf, sizeof(buf)-6, "%s/%s", FAVORITES, book_title);
		}
		for (p=buf+strlen(FAVORITES)+1; *p!=0; p++) {
			if ((! isalnum(*p)) && *p!='_' && *p!='-' && *p!='.' && *p!=' ' && (*p&0x80)==0) *p = '_';
		}
		strcat(buf, ".wlnk");
		f = fopen(buf, "w");
		if (f != NULL) {
			fwrite(current_url, 1, strlen(current_url), f);
			fclose(f);
		}
	}

	for (i=0; i<fav_menu_items; i++) {
		if (fav_menu[i].index >= 1000) continue;
		free(fav_menu[i].text);
		free(fav_menu[i].submenu);
	}
	free(fav_menu);
	fav_menu = NULL;

}

static void button_fav() {

	char buf[MAXPATHLENGTH];
	DIR *dir;
	struct dirent *dp;
	struct stat st;
	bookinfo *bi;
	int n, firstitem;

	fav_menu = (imenu *) malloc((MAXMENUFAVORITES+1) * sizeof(imenu));

	n=0;
	fav_menu[n].type = ITEM_ACTIVE;
	fav_menu[n].index = 1000;
	fav_menu[n].text = "@Add_to_fav";
	fav_menu[n].submenu = NULL;
	n++;

	firstitem = 1;
	dir = iv_opendir(FAVORITES);
	if (dir == NULL) return;
	while ((dp = iv_readdir(dir)) != NULL) {
		if (dp->d_name[0] == '.') continue;
		snprintf(buf, sizeof(buf), "%s/%s", FAVORITES, dp->d_name);
		if (iv_stat(buf , &st) < 0) continue;
		if (S_ISDIR(st.st_mode)) continue;
		bi = GetBookInfo(buf);
		if (! bi->title) continue;
		if (firstitem) {
			fav_menu[n].type = ITEM_SEPARATOR;
			fav_menu[n].index = 9999;
			fav_menu[n].text = NULL;
			fav_menu[n].submenu = NULL;
			n++;
			firstitem = 0;
		}
		fav_menu[n].type = ITEM_ACTIVE;
		fav_menu[n].index = n;
		fav_menu[n].text = strdup(bi->title);
		fav_menu[n].submenu = (imenu *) strdup(buf);
		n++;
		if (n >= MAXMENUFAVORITES) break;
	}
	fav_menu[n].type = 0;
	fav_menu[n].index = 0;
	fav_menu[n].text = NULL;
	fav_menu[n].submenu = NULL;
	fav_menu_items = n;
	iv_closedir(dir);

	OpenMenu(fav_menu, 1, favbuttonx, webpanelheight, favorites_handler);

}

static void button_url() {
	if (kbd_buffer[0] == 0) strcpy(kbd_buffer, "http://");
	OpenKeyboard("@enter_url", kbd_buffer, 1000, KBD_URL | KBD_NOSELECT, url_entered);
}

static void button_search() {
	OpenKeyboard("@Search", search_buffer, 32, 0, search_entered);
}

static void button_save() {
	OpenDirectorySelector("@Save", dirbuf, MAXPATHLENGTH-120, savedir_selected);
}

static void button_hide() {
	show_hide_webpanel(0);
}

static void urlhistory_handler(int idx) {

	if (idx >= 1000) {
		set_current_url(urlhistory[idx-1000].text);
		strcpy(kbd_buffer, current_url);
		start_download();
	}

}

int webpanel_handler(int type, int par1, int par2) {

	char buf[1024], *p;
	int a, i, j, n, uhsize;
	FILE *f;
	char **tlist = NULL;

	if (type == EVT_POINTERDOWN) {
		a = current_action = button_action(par1, par2);
		draw_button(a);
		if (a == WPA_URLHISTORY) {
			for (i=0; i<MAXURLS; i++) {
				if (urlhistory[i].text) free(urlhistory[i].text);
				urlhistory[i].text = NULL;
			}
			n = 0;
			f = fopen(URLHISTORY, "r");
			if (f == NULL) {
				f = fopen(URLHISTORY, "w");
				if (f != NULL) {
					fprintf(f, "%s\n", "http://www.pocketbook.com.ua/");
					fprintf(f, "%s\n", "http://www.bookland.net.ua/");
					fprintf(f, "%s\n", "http://www.the-ebook.org/");
					fclose(f);
				}
				f = fopen(URLHISTORY, "r");
			}
			while (f != NULL && fgets(buf, sizeof(buf)-1, f) !=NULL) {
				p = buf+strlen(buf);
				while (p>buf && *(p-1)>=0 && *(p-1)<=0x20) *(--p)=0;
				p = buf;
				while (*p>0 && *p<=0x20) p++;
				if (strlen(p) < 5) continue;
				tlist = (char **) realloc(tlist, (n+2) * sizeof(char *));
				tlist[n++] = strdup(p);
			}
			if (f != NULL) fclose(f);
			uhsize = 0;
			for (i=n-1; i>=0; i--) {
				for (j=0; j<uhsize; j++) {
					if (strcmp(urlhistory[j].text, tlist[i]) == 0) break;
				}
				if (j != uhsize) continue;
				urlhistory[uhsize].type = ITEM_ACTIVE;
				urlhistory[uhsize].index = uhsize+1000;
				urlhistory[uhsize].text = tlist[i];
				tlist[i] = NULL;
				uhsize++;
				if (uhsize >= MAXURLS) break;
			}
			urlhistory[uhsize].type = 0;
			urlhistory[uhsize].index = 0;
			urlhistory[uhsize].text = NULL;

			if (tlist != NULL) {
				for (i=0; i<n; i++) {
					if (tlist[i] != NULL) free(tlist[i]);
				}
				free(tlist);
			}
			OpenMenu(urlhistory, 0, urlbuttonx, webpanelheight, urlhistory_handler);
		}
	}

	if (type == EVT_POINTERUP) {
		a = button_action(par1, par2);
		if (a != current_action) a = 0;
		draw_webpanel(1);
		switch (a) {
		  case WPA_BACK: button_goback(); break;
		  case WPA_STOP: button_stop(); break;
		  case WPA_RELOAD: button_reload(); break;
		  case WPA_FAVORITE: button_fav(); break;
		  case WPA_URL: button_url(); break;
		  case WPA_SEARCH: button_search(); break;
		  case WPA_SAVE: button_save(); break;
		  case WPA_HIDE: button_hide(); break;
		}
	}
	return 1;

}

static char *hashpart(char *s) {

	char *r = strchr(s, '#');
	return (r == NULL) ? NULL : r+1;

}

static void clear_imagelist() {

	int i;
	for (i=0; i<dl_imagecount; i++) {
		free(dl_imageurl[i]);
		free(dl_imagepath[i]);
	}
	dl_imagecount = 0;

}

static void download_timer() {

	iv_sessioninfo *si;
	int newpercent, i, r;
	char *tmp, *p, *ctype;
	const char *ext;
	FILE *f;

	if (! downloading) return;

	r = GetSessionStatus(sid);
	if (r < 0) {
		NetErrorMessage(r);
		stop_download();
		return;
	}

	if (r == 0) {
		tmp = strdup(current_file);
		ctype = GetHeader(sid, "Content-Type");
		fprintf(stderr, "Content-type: %s\n", ctype);
		p = strchr(ctype, ';');
		if (p) *p = 0;
		ext = NULL;
		if (ctype != NULL) {
			for (i=0; mimetypes[i].type!=NULL; i++) {
				if (strcasecmp(mimetypes[i].type, ctype) == 0) {
					ext = mimetypes[i].ext;
					break;
				}
			}
		}
		if (ext == NULL) {
			// TODO - unknown content-types (analyze file header)
			ext = ".html";
		}
		strcat(current_file, ext);
		rename(tmp, current_file);
		free(tmp);
		f = fopen(WEBCACHEINDEX, "ab");
		if (f != NULL) {
			fprintf(f, "%s %s\n", current_file, current_url);
			fclose(f);
		}
		ShowHourglass();
		OpenBook(current_file, hashpart(current_url), OB_WITHRETURN);
		stop_download();
		return;
	}

	cache_cleanup();

	if (++dl_counter >= 4) {
		si = GetSessionInfo(sid);
		dl_progress = si->progress;
		newpercent = (si->progress * 100) / si->length;
		fprintf(stderr, "%i/%i %i%%\n", si->progress, si->length, newpercent);
		if (newpercent > 100) newpercent = 0;
		if (newpercent != dl_percent) {
			dl_percent = newpercent;
			draw_download_panel(NULL, 1);
			dl_counter = 0;
		}
	}

	SetHardTimer("download", download_timer, 500);

}

void start_download() {

	unsigned long h;
	int r;

	if (downloading) stop_download();

	fprintf(stderr, "url: %s\n", current_url);

	if (get_cache_item(current_url, current_file)) {
		fprintf(stderr, "found in cache: %s\n", current_file);
		ShowHourglass();
		OpenBook(current_file, hashpart(current_url), OB_WITHRETURN);
		stop_download();
		return;
	}

	draw_webpanel(2);

	if ((r = NetConnect(NULL)) != NET_OK) {
		NetErrorMessage(r);
		return;
	}

	if (sid == 0) sid = NewSession();

	mkdir(WEBCACHE, 0777);
	h = cachename(current_url);
	sprintf(current_file, "%s/%08x", WEBCACHE, (unsigned int) h);

	clear_imagelist();

	r = DownloadTo(sid, current_url, NULL, current_file, 60000);
	if (r != NET_OK) {
		NetErrorMessage(r);
		unlink(current_file);
		return;
	}

	downloading = 1;
	dl_counter = 0;
	dl_percent = 0;
	dl_progress = 0;
	draw_download_panel(NULL, 1);
	SetHardTimer("download", download_timer, 500);

}

static void clarify_image_urls() {

	char *s, *news, *pp;
	int i, len;

	for (i=0; i<dl_imagecount; i++) {

		s = dl_imageurl[i];
		while (*s == ' ' || *s == '\"' || *s == '\'' || *s == '\\') s++;
		trim_right(s);
		pp = s + strlen(s);
		while (pp > s && (*(pp-1) == '\"' || *(pp-1) == '\'' || *(pp-1) == '\\')) *(--pp) = 0;
		if (*s == 0) continue;
		for (pp=s; *pp; pp++) if (*pp == '\"') *pp = '&';

		if (strncasecmp(s, "http:", 5) == 0 ||
		    strncasecmp(s, "ftp:", 4) == 0 ||
		    strncasecmp(s, "https:", 6) == 0
		) {
			news = strdup(s);
		} else if (s[0] == '/') {
			pp = strchr(current_url, ':');
			if (pp) pp = strchr(pp+4, '/');
			if (! pp) pp = current_url+strlen(current_url);
			len = pp - current_url;
			news = (char *) malloc(len+strlen(s)+4);
			strncpy(news, current_url, len);
			strcpy(news+len, s);
		} else {
			pp = strrchr(current_url, '/');
			if (pp == NULL || strchr(current_url, '/')+1 >= pp) pp = current_url+strlen(current_url);
			len = pp - current_url;
			news = (char *) malloc(len+strlen(s)+4);
			strncpy(news, current_url, len);
			news[len] = '/';
			strcpy(news+len+1, s);
		}
		free(dl_imageurl[i]);
		dl_imageurl[i] = news;
	}

}

void dl_image_timer() {

	struct stat st;
	char *url;
	int i, r;
	int interval = 100;

	if (dl_gotonext) {
		while (1) {
			i = ++dl_currentimg;
			if (i >= dl_imagecount) {
				fprintf(stderr, "IMG complete (%i)\n", dl_updated);
				downloading = 0;
				if (dl_updated) {
                                        calc_pages();
					refresh_page(0);
				} else {
					repaint_all(2);
				}
				return;
			}
			if (stat(dl_imagepath[i], &st) == 0) {
				fprintf(stderr, "IMG + %s\n", dl_imageurl[i]);
			} else {
				url = dl_imageurl[i];
				fprintf(stderr, "IMG * %s\n", url);
				r = DownloadTo(sid, url, NULL, dl_imagepath[i], 20000);
				if (r != NET_OK) fprintf(stderr, "^^^ %i\n", r);
				if (r != NET_OK) interval = 20;
				if (r == NET_OK) dl_gotonext = 0;
				break;
			}
		}
	} else {
		r = GetSessionStatus(sid);
		if (r <= 0) {
			fprintf(stderr, "~~~ %i\n", r);
			if (r == 0) dl_updated = 1;
			dl_gotonext = 1;
		}
		cache_cleanup();
	}

	if (++dl_counter >= 24) {
		dl_counter = 0;
		draw_download_panel(NULL, 1);
	}
	SetHardTimer("dl_image", dl_image_timer, interval);

}

void start_download_images() {

	if (load_images == 0) return;
	if (dl_imagecount == 0) return;
	clarify_image_urls();
	fprintf(stderr, "download images\n");
	if (sid == 0) sid = NewSession();
	dl_currentimg = -1;
	dl_updated = 0;
	dl_gotonext = 1;
	downloading = 2;
	draw_download_panel(NULL, 1);
	SetHardTimer("dl_image", dl_image_timer, 100);
	
}

void stop_download() {

	if (! downloading) return;
	AbortTransfer(sid);
	unlink(current_file);
	ClearTimer(download_timer);
	ClearTimer(dl_image_timer);
	if (downloading == 2) {
                calc_pages();
	}
	downloading = 0;
	repaint_all(2);

}

void draw_download_panel(char *text, int update) {

	char buf[64];
	int pc;

	if (text != NULL) {
		if (savetext) free(savetext);
		savetext = strdup(text);
	}
	if (downloading == 1) {
		sprintf(buf, "%s - %i bytes", "Downloading", dl_progress);
		pc = dl_percent;
	} else {
		sprintf(buf, "%s", "Downloading images");
		pc = (dl_currentimg * 100) / dl_imagecount;
	}
	DrawPanel((ibitmap *) &dl_icon, savetext, buf, pc);
	if (update) PartialUpdate(0, ScreenHeight()-PanelHeight(), ScreenWidth(), PanelHeight());

}
