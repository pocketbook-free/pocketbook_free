/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2000-2001  Qualcomm Incorporated
 *  Copyright (C) 2002-2003  Maxim Krasnyansky <maxk@qualcomm.com>
 *  Copyright (C) 2002-2009  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "inkinternal.h"

#include "hci.h"
#include "hci_lib.h"
#include "uiquery.h"

static iv_mpctl *mpc;

static int nodaemon = 0;
static int hcidev = -1;
static struct hci_dev_info *devinfo;
static char *ksfilename;

static char pincode[17];

#define info(fmt...) { if (nodaemon) { fprintf(stderr, fmt); fprintf(stderr, "\n"); } }
#define error(fmt...) { fprintf(stderr, fmt); fprintf(stderr, "\n"); }

typedef struct keystoreitem {
	bdaddr_t sba;
	bdaddr_t dba;
	uint8_t key[16];
	uint8_t type;
	struct keystoreitem *next;
} keystoreitem_t;

keystoreitem_t *keystore = NULL;

static keystoreitem_t *find_link_key(bdaddr_t *sba, bdaddr_t *dba) {

	keystoreitem_t *ksi = keystore;
	while (ksi != NULL) {
		if (memcmp(sba, &(ksi->sba), sizeof(bdaddr_t)) == 0 &&
		    memcmp(dba, &(ksi->dba), sizeof(bdaddr_t)) == 0)
			return ksi;
		ksi = ksi->next;
	}
	return NULL;

}

static void store_link_key(bdaddr_t *sba, bdaddr_t *dba, uint8_t *key, uint8_t type) {
	
	keystoreitem_t *ksi = find_link_key(sba, dba);
	if (ksi == NULL) {
		ksi = (keystoreitem_t *) calloc(sizeof(keystoreitem_t), 1);
		memcpy(&(ksi->sba), sba, sizeof(bdaddr_t));
		memcpy(&(ksi->dba), dba, sizeof(bdaddr_t));
		ksi->next = keystore;
		keystore = ksi;
	}
	memcpy(ksi->key, key, 16);
	ksi->type = type;

}

static int read_link_key(bdaddr_t *sba, bdaddr_t *dba, uint8_t *key, uint8_t *type) {

	keystoreitem_t *ksi = find_link_key(sba, dba);
	if (ksi == NULL) return -1;
	memcpy(key, ksi->key, 16);
	*type = ksi->type;
	return 0;

}

static void load_keys() {

	unsigned char buf[256], *p;
	bdaddr_t sba, dba;
	unsigned char key[16];
	uint8_t type;
	FILE *f;

	int alen = sizeof(bdaddr_t);
	int recordlen = alen + alen + 16 + 2;

	f = fopen(ksfilename, "rb");
	if (f == NULL) return;

	while (fread(buf, 1, 2, f) == 2) {
		if (buf[0] != 0xa1) break;
		if (buf[1] != recordlen) break;
		if (fread(buf, 1, recordlen, f) != recordlen) break;
		p = buf;
		memcpy(&sba, p, alen);
		p += alen;
		memcpy(&dba, p, alen);
		p += alen;
		memcpy(key, p, 16);
		p += 16;
		type = *p;
		store_link_key(&sba, &dba, key, type);
	}

	fclose(f);

}		

static void save_keys() {

	unsigned char buf[256], *p;
	FILE *f;

	int alen = sizeof(bdaddr_t);
	int recordlen = alen + alen + 16 + 2;

	f = fopen(ksfilename, "wb");
	if (f == NULL) {
		info ("cannot open key store file\n");
		return;
	}

	keystoreitem_t *ksi = keystore;
	while (ksi != NULL) {
		buf[0] = 0xa1;
		buf[1] = recordlen;
		p = buf+2;
		memcpy(p, &(ksi->sba), alen);
		p += alen;
		memcpy(p, &(ksi->dba), alen);
		p += alen;
		memcpy(p, ksi->key, 16);
		p += 16;
		*(p++) = ksi->type;
		*p = 0;
		if (fwrite(buf, 1, 2+recordlen, f) != 2+recordlen) {
			info ("error writing to key store file\n");
			break;
		}
		ksi = ksi->next;
	}

	fclose(f);
	sync();

}

static void pin_code_request(int dev, bdaddr_t *sba, bdaddr_t *dba)
{
	pin_code_reply_cp pr;
	char buf[64];
	char sa[18], da[18];
	int tm, seq;

	memset(&pr, 0, sizeof(pr));
	bacpy(&pr.bdaddr, dba);

	ba2str(sba, sa); ba2str(dba, da);
	info("pin_code_request (sba=%s, dba=%s)", sa, da);

	seq = uiquery_textentry("@BtPin", "", 8, KBD_PHONE);

	tm = 600;
	while (1) {
		if (uiquery_status(seq, NULL, 0) > UIQ_PROCESSING) break;
		if (--tm <= 0) break;
		usleep(100000);
	}

	if (uiquery_status(seq, buf, sizeof(buf)) == UIQ_OK) {
		memcpy(pr.pin_code, mpc->uidata, 15);
		pr.pin_code[15] = 0;
		pr.pin_len = strlen(pr.pin_code);
		hci_send_cmd(dev, OGF_LINK_CTL, OCF_PIN_CODE_REPLY, PIN_CODE_REPLY_CP_SIZE, &pr);
	} else {
		uiquery_dismiss(seq);
		int counter = 100;
		while((uiquery_status(seq,NULL,0) != UIQ_NONE) && counter--) usleep(10000);
		hci_send_cmd(dev, OGF_LINK_CTL, OCF_PIN_CODE_NEG_REPLY, 6, &pr);
	}

	mpc->uiquery = UIQ_IDLE;

}

static void link_key_notify(int dev, bdaddr_t *sba, void *ptr)
{
	evt_link_key_notify *evt = ptr;
	bdaddr_t *dba = &evt->bdaddr;
	char sa[18], da[18];

	ba2str(sba, sa); ba2str(dba, da);
	info("link_key_notify (sba=%s, dba=%s, type=%d)", sa, da, evt->key_type);

	store_link_key(sba, dba, evt->link_key, evt->key_type);
	save_keys();

}

static void link_key_request(int dev, bdaddr_t *sba, bdaddr_t *dba)
{
	unsigned char key[16];
	char sa[18], da[18];
	uint8_t type;
	int err;

	ba2str(sba, sa); ba2str(dba, da);
	info("link_key_request (sba=%s, dba=%s)", sa, da);

	err = read_link_key(sba, dba, key, &type);
	if (err < 0) {
		/* Link key not found */
		hci_send_cmd(dev, OGF_LINK_CTL, OCF_LINK_KEY_NEG_REPLY, 6, dba);
	} else {
		/* Link key found */
		link_key_reply_cp lr;
		memcpy(lr.link_key, key, 16);
		bacpy(&lr.bdaddr, dba);
		hci_send_cmd(dev, OGF_LINK_CTL, OCF_LINK_KEY_REPLY,
					LINK_KEY_REPLY_CP_SIZE, &lr);
	}

}

static int receive_data()
{
	unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr = buf;
	int type;
	hci_event_hdr *eh;

	if (read(hcidev, buf, sizeof(buf)) <= 0) {
		return (errno == EAGAIN) ? 1 : -1;
	}

	type = *ptr++;

	//fprintf(stderr, "HCI event: 0x%02x\n", type);

	if (type != HCI_EVENT_PKT) return 0;

	eh = (hci_event_hdr *) ptr;
	ptr += HCI_EVENT_HDR_SIZE;

	ioctl(hcidev, HCIGETDEVINFO, (void *) devinfo);

	if (hci_test_bit(HCI_RAW, &devinfo->flags))
		return 1;

	fprintf(stderr, "Event: %i\n", eh->evt);

	switch (eh->evt) {
		case EVT_PIN_CODE_REQ:
			pin_code_request(hcidev, &devinfo->bdaddr, (bdaddr_t *) ptr);
			break;
		case EVT_LINK_KEY_REQ:
			link_key_request(hcidev, &devinfo->bdaddr, (bdaddr_t *) ptr);
			break;
		case EVT_LINK_KEY_NOTIFY:
			link_key_notify(hcidev, &devinfo->bdaddr, ptr);
			break;
	}

	return 0;
}

void usage() {

	fprintf(stderr, "usage: btauth <device> <pin> <keystore>\n");
	exit(1);

}

int main(int argc, char **argv)
{
	struct hci_filter flt;
	read_stored_link_key_cp cp;
	int mpc_shm;
	int hdev, r;
	char *p;

	if (argc > 1 && strcmp(argv[1], "-n") == 0) {
		nodaemon = 1;
		argc--;
		argv++;
	}

	if (argc < 4) usage();

	mpc_shm = shmget(0xa12302, sizeof(iv_mpctl), IPC_CREAT | 0666);
	if (mpc_shm == -1) {
		fprintf(stderr, "cannot get shared memory segment\n");
		return 1;
	}
	mpc = (iv_mpctl *) shmat(mpc_shm, NULL, 0);
	if (mpc == (void*)-1) {
		fprintf(stderr, "cannot attach shared memory segment\n");
		return 1;
	}

	p = argv[1];
	if (strncmp(p, "hci", 3) == 0) p += 3;
	if (*p < '0' || *p > '9' || *(p+1) != 0) usage();
	hdev = *p - '0';

	if (strlen(argv[2]) > 16) {
		error("Pin is too long\n");
		return 1;
	}
	strcpy(pincode, argv[2]);

	ksfilename = argv[3];
	
	info("Starting security manager %d", hdev);

	if ((hcidev = hci_open_dev(hdev)) < 0) {
		error("Can't open device hci%d: %s (%d)",
						hdev, strerror(errno), errno);
		return 1;
	}

	/* Set filter */
	hci_filter_clear(&flt);
	hci_filter_set_ptype(HCI_EVENT_PKT, &flt);
	hci_filter_set_event(EVT_CMD_STATUS, &flt);
	hci_filter_set_event(EVT_CMD_COMPLETE, &flt);
	hci_filter_set_event(EVT_PIN_CODE_REQ, &flt);
	hci_filter_set_event(EVT_LINK_KEY_REQ, &flt);
	hci_filter_set_event(EVT_LINK_KEY_NOTIFY, &flt);
	hci_filter_set_event(EVT_RETURN_LINK_KEYS, &flt);
	hci_filter_set_event(EVT_IO_CAPABILITY_REQUEST, &flt);
	hci_filter_set_event(EVT_IO_CAPABILITY_RESPONSE, &flt);
	hci_filter_set_event(EVT_USER_CONFIRM_REQUEST, &flt);
	hci_filter_set_event(EVT_USER_PASSKEY_REQUEST, &flt);
	hci_filter_set_event(EVT_REMOTE_OOB_DATA_REQUEST, &flt);
	hci_filter_set_event(EVT_USER_PASSKEY_NOTIFY, &flt);
	hci_filter_set_event(EVT_KEYPRESS_NOTIFY, &flt);
	hci_filter_set_event(EVT_SIMPLE_PAIRING_COMPLETE, &flt);
	hci_filter_set_event(EVT_AUTH_COMPLETE, &flt);
	hci_filter_set_event(EVT_REMOTE_NAME_REQ_COMPLETE, &flt);
	hci_filter_set_event(EVT_READ_REMOTE_VERSION_COMPLETE, &flt);
	hci_filter_set_event(EVT_READ_REMOTE_FEATURES_COMPLETE, &flt);
	hci_filter_set_event(EVT_REMOTE_HOST_FEATURES_NOTIFY, &flt);
	hci_filter_set_event(EVT_INQUIRY_COMPLETE, &flt);
	hci_filter_set_event(EVT_INQUIRY_RESULT, &flt);
	hci_filter_set_event(EVT_INQUIRY_RESULT_WITH_RSSI, &flt);
	hci_filter_set_event(EVT_EXTENDED_INQUIRY_RESULT, &flt);
	hci_filter_set_event(EVT_CONN_REQUEST, &flt);
	hci_filter_set_event(EVT_CONN_COMPLETE, &flt);
	hci_filter_set_event(EVT_DISCONN_COMPLETE, &flt);
	if (setsockopt(hcidev, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
		error("Can't set filter on hci%d: %s (%d)",
						hdev, strerror(errno), errno);
		close(hcidev);
		return 1;
	}

	devinfo = calloc(sizeof(struct hci_dev_info), 1);
	if (hci_devinfo(hdev, devinfo) < 0) {
		error("Can't get device info: %s (%d)",
							strerror(errno), errno);
		close(hcidev);
		free(devinfo);
		return 1;
	}

	if (hci_test_bit(HCI_RAW, &devinfo->flags)) {
		error("Not HCI_RAW");
		return 1;
	}

	bacpy(&cp.bdaddr, BDADDR_ANY);
	cp.read_all = 1;

	hci_send_cmd(hcidev, OGF_HOST_CTL, OCF_READ_STORED_LINK_KEY,
			READ_STORED_LINK_KEY_CP_SIZE, (void *) &cp);

	load_keys();

	if (! nodaemon) {
		if (daemon(0, 0) != 0) {
			error("Could not start daemon process");
			return 1;
		}
	}

	while ((r = receive_data()) >= 0) {
		if (r > 0) usleep(100000);
	}

	return 0;

}

