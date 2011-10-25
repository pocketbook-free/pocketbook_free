#include "inkinternal.h"

//static iv_mpctl *shmpc=NULL;
static char *cdata;
static int  *idata;
static int cseq;

static int uiq_init() {

	if (shmpc != NULL) return 1;

	int mpc_shm = shmget(0xa12302, sizeof(iv_mpctl), IPC_CREAT | 0666);
	if (mpc_shm < 0) return 0;
	shmpc = (iv_mpctl *) shmat(mpc_shm, NULL, 0);
	if (shmpc == (void*)-1) return 0;
	cdata = (char *) (shmpc->uidata);
	idata = (int *) (shmpc->uidata);
	srandom(time(NULL));
	cseq = random() & 0x000fffff;
	return 1;

}

static int nextseq() {

	return (getpid() << 20) | ++cseq;

}

static void wait_for_free() {

	int i;
	for (i=0; i<20; i++) {
		if (shmpc->uistatus != UIQ_PENDING) return;
		usleep(100000);
	}

}

int uiquery_textentry(char *title, char *text, int maxlen, int flags) {

	int tlen, txlen;

	if (! uiq_init()) return 0;
	wait_for_free();
	if (title == NULL) title = "";
	if (text == NULL) text = "";
	idata[0] = tlen = strlen(title)+1;
	idata[1] = txlen = strlen(text)+1;
	idata[2] = maxlen;
	idata[3] = flags;
	strcpy(cdata+16, title);
	strcpy(cdata+16+tlen, text);
	shmpc->uisequence = nextseq();
	shmpc->uiquery = UIQ_TEXTENTRY;
	shmpc->uistatus = UIQ_PENDING;
	return shmpc->uisequence;

}

static int uiquery_progress_internal(int icon, char *title, char *text, int percent, int nc) {

	int tlen, txlen;

	if (! uiq_init()) return 0;
	wait_for_free();
	if (title == NULL) title = "";
	if (text == NULL) text = "";
	idata[0] = icon;
	idata[1] = tlen = strlen(title)+1;
	idata[2] = txlen = strlen(text)+1;
	idata[3] = percent;
	strcpy(cdata+16, title);
	strcpy(cdata+16+tlen, text);
	shmpc->uisequence = nextseq();
	shmpc->uiquery = nc ? UIQ_NPROGRESSBAR : UIQ_PROGRESSBAR;
	shmpc->uistatus = UIQ_PENDING;
	return shmpc->uisequence;

}

int uiquery_progress(int icon, char *title, char *text, int percent) {
	return uiquery_progress_internal(icon, title, text, percent, 0);
}

int uiquery_nprogress(int icon, char *title, char *text, int percent) {
	return uiquery_progress_internal(icon, title, text, percent, 1);
}

void uiquery_update(int seq, char *text, int percent) {

	static int lasttime = 0;
	int txlen, newtime;

	if (! uiq_init()) return;
	if (seq != shmpc->uisequence) return;
	if (shmpc->uistatus == UIQ_PENDING) return;
	newtime = time(NULL);
	if (newtime-lasttime < 2) return;
	lasttime = newtime;
	if (text == NULL) text = "";
	idata[0] = txlen = strlen(text)+1;
	idata[1] = percent;
	strcpy(cdata+8, text);
	shmpc->uiquery = UIQ_UPDATE;
	shmpc->uistatus = UIQ_PENDING;

}

void uiquery_event(int type, int par1, int par2) {

	if (! uiq_init()) return;
	wait_for_free();
	idata[0] = type;
	idata[1] = par1;
	idata[2] = par2;
	shmpc->uiquery = UIQ_EVENT_MAIN;
	shmpc->uistatus = UIQ_PENDING;

}

int uiquery_status(int seq, char *buf, int size) {

	if (! uiq_init()) return UIQ_CANCEL;
	if (seq != shmpc->uisequence) return UIQ_CANCEL;
	if (shmpc->uistatus == UIQ_OK && buf != NULL) {
		strncpy(buf, cdata, size-1);
		buf[size-1] = 0;
	}
	return shmpc->uistatus;

}

void uiquery_dismiss(int seq) {

	if (! uiq_init()) return;
	if (seq != shmpc->uisequence) return;
	shmpc->uiquery = UIQ_DISMISS;
	shmpc->uistatus = UIQ_PENDING;

}

