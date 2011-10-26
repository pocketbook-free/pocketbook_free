/**
	\file usbobex.h
	USB OBEX, USB transport for OBEX.
	OpenOBEX library - Free implementation of the Object Exchange protocol.

	Copyright (c) 2005 Alex Kanavin, All Rights Reserved.

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

#ifndef USBOBEX_H
#define USBOBEX_H

#include <openobex/obex_const.h>
#include <usb.h>

/* Information about a USB OBEX interface present on the system */
struct obex_usb_intf_transport_t {
	struct obex_usb_intf_transport_t *prev, *next;	/* Next and previous interfaces in the list */
	struct usb_device *device;		/* USB device that has the interface */
	int configuration;			/* Device configuration */
	int configuration_description;		/* Configuration string descriptor number */
	int control_interface;			/* OBEX master interface */
	int control_setting;			/* OBEX master interface setting */
	int control_interface_description;	/* OBEX master interface string descriptor number 
						 * If non-zero, use usb_get_string_simple() from 
						 * libusb to retrieve human-readable description
						 */
	int data_interface;			/* OBEX data/slave interface */
	int data_idle_setting;			/* OBEX data/slave idle setting */
	int data_interface_idle_description;	/* OBEX data/slave interface string descriptor number
						 * in idle setting */
	int data_active_setting;		/* OBEX data/slave active setting */
	int data_interface_active_description;	/* OBEX data/slave interface string descriptor number
						 * in active setting */
	int data_endpoint_read;			/* OBEX data/slave interface read endpoint */
	int data_endpoint_write;		/* OBEX data/slave interface write endpoint */
	usb_dev_handle *dev_control;		/* libusb handler for control interace */
	usb_dev_handle *dev_data;		/* libusb handler for data interface */
};

/* "Union Functional Descriptor" from CDC spec 5.2.3.X
 * used to find data/slave OBEX interface */
#pragma pack(1)
struct cdc_union_desc {
	uint8_t      bLength;
	uint8_t      bDescriptorType;
	uint8_t      bDescriptorSubType;

	uint8_t      bMasterInterface0;
	uint8_t      bSlaveInterface0;
};
#pragma pack()

/* CDC class and subclass types */
#define USB_CDC_CLASS			0x02
#define USB_CDC_OBEX_SUBCLASS		0x0b

/* class and subclass specific descriptor types */
#define CDC_HEADER_TYPE			0x00
#define CDC_CALL_MANAGEMENT_TYPE	0x01
#define CDC_AC_MANAGEMENT_TYPE		0x02
#define CDC_UNION_TYPE			0x06
#define CDC_COUNTRY_TYPE		0x07
#define CDC_OBEX_TYPE			0x15

/* Interface descriptor */
#define USB_DT_CS_INTERFACE		0x24
#define CDC_DATA_INTERFACE_TYPE		0x0a

#define USB_MAX_STRING_SIZE		256
#define USB_OBEX_TIMEOUT		10000 /* 10 seconds */

void usbobex_prepare_connect(obex_t *self, struct obex_usb_intf_transport_t *intf);
int usbobex_connect_request(obex_t *self);
int usbobex_disconnect_request(obex_t *self);
int usbobex_find_interfaces(obex_interface_t **interfaces);
void usbobex_free_interfaces(int num, obex_interface_t *intf);
#endif
