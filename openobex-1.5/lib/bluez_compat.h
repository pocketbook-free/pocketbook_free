/**
	\file bluez_compat.h
	Provides the Bluez API on other platforms.
	OpenOBEX library - Free implementation of the Object Exchange protocol.

	Copyright (c) 2007 Hendrik Sattler, All Rights Reserved.

	OpenOBEX is free software; you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as
	published by the Free Software Foundation; either version 2.1 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with OpenOBEX. If not, see <http://www.gnu.org/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef BLUEZ_COMPAT_H
#define BLUEZ_COMPAT_H

#if defined(HAVE_BLUETOOTH_WINDOWS)
/* you need the headers files from the Platform SDK */
#include <winsock2.h>
#include <ws2bth.h>
#define bdaddr_t    BTH_ADDR
#define sockaddr_rc _SOCKADDR_BTH
#define rc_family   addressFamily
#define rc_bdaddr   btAddr
#define rc_channel  port
#define PF_BLUETOOTH   PF_BTH
#define AF_BLUETOOTH   PF_BLUETOOTH
#define BTPROTO_RFCOMM BTHPROTO_RFCOMM
static bdaddr_t bluez_compat_bdaddr_any = {BTH_ADDR_NULL};
#define BDADDR_ANY     &bluez_compat_bdaddr_any
#define bacpy(dst,src) memcpy((dst),(src),sizeof(bdaddr_t))
#define bacmp(a,b)     memcmp((a),(b),sizeof(bdaddr_t))

#elif defined(HAVE_BLUETOOTH_LINUX)
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#elif defined(HAVE_BLUETOOTH_FREEBSD)
#include <bluetooth.h>
#define sockaddr_rc sockaddr_rfcomm
#define rc_family   rfcomm_family
#define rc_bdaddr   rfcomm_bdaddr
#define rc_channel  rfcomm_channel

#elif defined(HAVE_BLUETOOTH_NETBSD)
#include <bluetooth.h>
#include <netbt/rfcomm.h>
#define sockaddr_rc sockaddr_bt
#define rc_family   bt_family
#define rc_bdaddr   bt_bdaddr
#define rc_channel  bt_channel
#define BDADDR_ANY  NG_HCI_BDADDR_ANY

#endif /* HAVE_BLUETOOTH_* */

#endif /* BLUEZ_COMPAT_H */
