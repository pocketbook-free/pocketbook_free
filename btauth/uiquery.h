#ifndef UIQUERY_H
#define UIQUERY_H

int uiquery_textentry(char *title, char *text, int maxlen, int flags);
int uiquery_progress(int icon, char *title, char *text, int percent);
int uiquery_nprogress(int icon, char *title, char *text, int percent);
void uiquery_update(int seq, char *text, int percent);
int uiquery_status(int seq, char *buf, int size);
void uiquery_dismiss(int seq);

#endif
