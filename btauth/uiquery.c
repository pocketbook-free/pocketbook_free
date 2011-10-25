#include "inkinternal.h"

static iv_mpctl *ivmpc=NULL;
static char *cdata;
static int  *idata;
static int cseq;

static int uiq_init() {

	if (ivmpc != NULL) return 1;

	int mpc_shm = shmget(0xa12302, sizeof(iv_mpctl), IPC_CREAT | 0666);
	if (mpc_shm < 0) return 0;
	ivmpc = (iv_mpctl *) shmat(mpc_shm, NULL, 0);
	if (ivmpc == (void*)-1) return 0;
	cdata = (char *) (ivmpc->uidata);
	idata = (int *) (ivmpc->uidata);
	srandom(time(NULL));
	cseq = random() & 0x000fffff;
	return 1;

}

static int nextseq() {

	return (getpid() << 20) | ++cseq;

}

int uiquery_textentry(char *title, char *text, int maxlen, int flags) {

	int tlen, txlen;

	if (! uiq_init()) return 0;
	if (title == NULL) title = "";
	if (text == NULL) text = "";
	idata[0] = tlen = strlen(title)+1;
	idata[1] = txlen = strlen(text)+1;
	idata[2] = maxlen;
	idata[3] = flags;
	strcpy(cdata+16, title);
	strcpy(cdata+16+tlen, text);
	ivmpc->uisequence = nextseq();
	ivmpc->uiquery = UIQ_TEXTENTRY;
	ivmpc->uistatus = UIQ_PENDING;
	return ivmpc->uisequence;

}

static int uiquery_progress_internal(int icon, char *title, char *text, int percent, int nc) {

	int tlen, txlen;

	if (! uiq_init()) return 0;
	if (title == NULL) title = "";
	if (text == NULL) text = "";
	idata[0] = icon;
	idata[1] = tlen = strlen(title)+1;
	idata[2] = txlen = strlen(text)+1;
	idata[3] = percent;
	strcpy(cdata+16, title);
	strcpy(cdata+16+tlen, text);
	ivmpc->uisequence = nextseq();
	ivmpc->uiquery = nc ? UIQ_NPROGRESSBAR : UIQ_PROGRESSBAR;
	ivmpc->uistatus = UIQ_PENDING;
	return ivmpc->uisequence;

}

int uiquery_progress(int icon, char *title, char *text, int percent) {
	return uiquery_progress_internal(icon, title, text, percent, 0);
}

int uiquery_nprogress(int icon, char *title, char *text, int percent) {
	return uiquery_progress_internal(icon, title, text, percent, 1);
}

void uiquery_update(int seq, char *text, int percent) {

	int txlen;

	if (! uiq_init()) return;
	if (seq != ivmpc->uisequence) return;
	if (text == NULL) text = "";
	idata[0] = txlen = strlen(text)+1;
	idata[1] = percent;
	strcpy(cdata+8, text);
	ivmpc->uiquery = UIQ_UPDATE;
	ivmpc->uistatus = UIQ_PENDING;

}

int uiquery_status(int seq, char *buf, int size) {

	if (! uiq_init()) return UIQ_CANCEL;
	if (seq != ivmpc->uisequence) return UIQ_CANCEL;
	if (ivmpc->uistatus == UIQ_OK && buf != NULL) {
		strncpy(buf, cdata, size-1);
		buf[size-1] = 0;
	}
	return ivmpc->uistatus;

}

void uiquery_dismiss(int seq) {

	if (! uiq_init()) return;
	if (seq != ivmpc->uisequence) return;
	ivmpc->uiquery = UIQ_DISMISS;
	ivmpc->uistatus = UIQ_PENDING;

}

