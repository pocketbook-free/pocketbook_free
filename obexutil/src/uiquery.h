#ifndef UIQUERY_H
#define UIQUERY_H

#define UIQ_IDLE           0
#define UIQ_PENDING        1
#define UIQ_PROCESSING     2
#define UIQ_OK             3
#define UIQ_CANCEL         4

int uiquery_textentry(char *title, char *text, int maxlen, int flags);
int uiquery_progress(int icon, char *title, char *text, int percent);
int uiquery_nprogress(int icon, char *title, char *text, int percent);
void uiquery_update(int seq, char *text, int percent);
void uiquery_event(int type, int par1, int par2);
int uiquery_status(int seq, char *buf, int size);
void uiquery_dismiss(int seq);
void update_net_timeout(int timeout);

#endif
