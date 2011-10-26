/**
	\file usbobex.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_USB

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>

#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include <string.h>
#include <unistd.h>
#include <stdio.h>		/* perror */
#include <errno.h>		/* errno and EADDRNOTAVAIL */

#include <usb.h>

#include "obex_main.h"
#include "usbobex.h"

/*
 * Function usbobex_prepare_connect (self, interface)
 *
 *    Prepare for USB OBEX connect
 *
 */
void usbobex_prepare_connect(obex_t *self, struct obex_usb_intf_transport_t *intf)
{
#ifndef _WIN32
	self->trans.self.usb = *intf;
#endif /* _WIN32 */
}

/*
 * Helper function to usbobex_find_interfaces
 */
static void find_eps(struct obex_usb_intf_transport_t *intf, struct usb_interface_descriptor data_intf, int *found_active, int *found_idle)
{
	struct usb_endpoint_descriptor *ep0, *ep1;

	if (data_intf.bNumEndpoints == 2) {
		ep0 = data_intf.endpoint;
		ep1 = data_intf.endpoint + 1;
		if ((ep0->bEndpointAddress & USB_ENDPOINT_IN) &&
		    ((ep0->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK) &&
		    !(ep1->bEndpointAddress & USB_ENDPOINT_IN) &&
		    ((ep1->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK)) {
			*found_active = 1;
			intf->data_active_setting = data_intf.bAlternateSetting;
			intf->data_interface_active_description = data_intf.iInterface;
			intf->data_endpoint_read = ep0->bEndpointAddress;
			intf->data_endpoint_write = ep1->bEndpointAddress;
		}
		if (!(ep0->bEndpointAddress & USB_ENDPOINT_IN) &&
		    ((ep0->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK) &&
		    (ep1->bEndpointAddress & USB_ENDPOINT_IN) &&
		    ((ep1->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK)) {
			*found_active = 1;
			intf->data_active_setting = data_intf.bAlternateSetting;
			intf->data_interface_active_description = data_intf.iInterface;
			intf->data_endpoint_read = ep1->bEndpointAddress;
			intf->data_endpoint_write = ep0->bEndpointAddress;
		}
	}
	if (data_intf.bNumEndpoints == 0) {
		*found_idle = 1;
		intf->data_idle_setting = data_intf.bAlternateSetting;
		intf->data_interface_idle_description = data_intf.iInterface;
	}
}

/*
 * Helper function to usbobex_find_interfaces
 */
static int find_obex_data_interface(unsigned char *buffer, int buflen, struct usb_config_descriptor config, struct obex_usb_intf_transport_t *intf)
{
	struct cdc_union_desc *union_header = NULL;
	int i, a;
	int found_active = 0;
	int found_idle = 0;

	if (!buffer) {
		DEBUG(2,"Weird descriptor references");
		return -EINVAL;
	}

	while (buflen > 0) {
		if (buffer [1] != USB_DT_CS_INTERFACE) {
			DEBUG(2,"skipping garbage");
			goto next_desc;
		}
		switch (buffer [2]) {
		case CDC_UNION_TYPE: /* we've found it */
			if (union_header) {
				DEBUG(2,"More than one union descriptor, skiping ...");
				goto next_desc;
			}
			union_header = (struct cdc_union_desc *)buffer;
			break;
		case CDC_OBEX_TYPE: /* maybe check version */
		case CDC_HEADER_TYPE:
			break; /* for now we ignore it */
		default:
			DEBUG(2, "Ignoring extra header, type %d, length %d", buffer[2], buffer[0]);
			break;
		}
next_desc:
		buflen -= buffer[0];
		buffer += buffer[0];
	}

	if (!union_header) {
		DEBUG(2,"No union descriptor, giving up\n");
		return -ENODEV;
	}
	/* Found the slave interface, now find active/idle settings and endpoints */
	intf->data_interface = union_header->bSlaveInterface0;
	/* Loop through all of the interfaces */
	for (i = 0; i < config.bNumInterfaces; i++) {
		/* Loop through all of the alternate settings */
		for (a = 0; a < config.interface[i].num_altsetting; a++) {
			/* Check if this interface is OBEX data interface*/
			/* and find endpoints */
			if (config.interface[i].altsetting[a].bInterfaceNumber == intf->data_interface)
				find_eps(intf, config.interface[i].altsetting[a], &found_active, &found_idle);
		}
	}
	if (!found_idle) {
		DEBUG(2,"No idle setting\n");
		return -ENODEV;
	}
	if (!found_active) {
		DEBUG(2,"No active setting\n");
		return -ENODEV;
	}

	return 0;
}

/*
 * Helper function to usbobex_find_interfaces
 */
static int get_intf_string(struct usb_dev_handle *usb_handle, char **string, int id)
{
	if (id) {
		if ((*string = malloc(USB_MAX_STRING_SIZE)) == NULL)
			return -ENOMEM;
		*string[0] = '\0';
		return usb_get_string_simple(usb_handle, id, *string, USB_MAX_STRING_SIZE);
	}

	return 0;
}

/*
 * Helper function to usbobex_find_interfaces
 */
static struct obex_usb_intf_transport_t *check_intf(struct usb_device *dev,
					int c, int i, int a,
					struct obex_usb_intf_transport_t *current)
{
	struct obex_usb_intf_transport_t *next = NULL;

	if ((dev->config[c].interface[i].altsetting[a].bInterfaceClass == USB_CDC_CLASS)
	    && (dev->config[c].interface[i].altsetting[a].bInterfaceSubClass == USB_CDC_OBEX_SUBCLASS)) {
		int err;
		unsigned char *buffer = dev->config[c].interface[i].altsetting[a].extra;
		int buflen = dev->config[c].interface[i].altsetting[a].extralen;

		next = malloc(sizeof(struct obex_usb_intf_transport_t));
		if (next == NULL)
			return current;
		next->device = dev;
		next->configuration = dev->config[c].bConfigurationValue;
		next->configuration_description = dev->config[c].iConfiguration;
		next->control_interface = dev->config[c].interface[i].altsetting[a].bInterfaceNumber;
		next->control_interface_description = dev->config[c].interface[i].altsetting[a].iInterface;
		next->control_setting = dev->config[c].interface[i].altsetting[a].bAlternateSetting;

		err = find_obex_data_interface(buffer, buflen, dev->config[c], next);
		if (err)
			free(next);
		else {
			if (current)
				current->next = next;
			next->prev = current;
			next->next = NULL;
			current = next;
		}
	}

	return current;
}

/*
 * Function usbobex_find_interfaces ()
 *
 *    Find available USBOBEX interfaces on the system
 */
int usbobex_find_interfaces(obex_interface_t **interfaces)
{
	struct usb_bus *busses;
	struct usb_bus *bus;
	struct usb_device *dev;
	int c, i, a, num;
	struct obex_usb_intf_transport_t *current = NULL;
	struct obex_usb_intf_transport_t *tmp = NULL;
	obex_interface_t *intf_array = NULL;
	struct usb_dev_handle *usb_handle;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	busses = usb_get_busses();

	for (bus = busses; bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			/* Loop through all of the configurations */
			for (c = 0; c < dev->descriptor.bNumConfigurations; c++) {
				/* Loop through all of the interfaces */
				for (i = 0; i < dev->config[c].bNumInterfaces; i++) {
					/* Loop through all of the alternate settings */
					for (a = 0; a < dev->config[c].interface[i].num_altsetting; a++) {
						/* Check if this interface is OBEX */
						/* and find data interface */
						current = check_intf(dev, c, i, a, current);
					}
				}
			}
		}
	}
	num = 0;
	if (current)
		num++;
	while (current && current->prev) {
		current = current->prev;
		num++;
	}
	intf_array = malloc(sizeof(obex_interface_t) * num);
	if (intf_array == NULL)
		goto cleanup_list;
	memset(intf_array, 0, sizeof(obex_interface_t) * num);
	num = 0;
	while (current) {
		intf_array[num].usb.intf = current;
		usb_handle = usb_open(current->device);
		get_intf_string(usb_handle, &intf_array[num].usb.manufacturer,
				current->device->descriptor.iManufacturer);
		get_intf_string(usb_handle, &intf_array[num].usb.product,
				current->device->descriptor.iProduct);
		get_intf_string(usb_handle, &intf_array[num].usb.serial,
				current->device->descriptor.iSerialNumber);
		get_intf_string(usb_handle, &intf_array[num].usb.configuration,
				current->configuration_description);
		get_intf_string(usb_handle, &intf_array[num].usb.control_interface,
				current->control_interface_description);
		get_intf_string(usb_handle, &intf_array[num].usb.data_interface_idle,
				current->data_interface_idle_description);
		get_intf_string(usb_handle, &intf_array[num].usb.data_interface_active,
				current->data_interface_active_description);
		usb_close(usb_handle);
		current = current->next; num++;
	}
	*interfaces = intf_array;
	return num;

cleanup_list:
	while (current) {
		tmp = current->next;
		free(current);
		current = tmp;
	}
	return 0;
}

/*
 * Function usbobex_free_interfaces ()
 *
 *    Free the list of discovered USBOBEX interfaces on the system
 */
void usbobex_free_interfaces(int num, obex_interface_t *intf)
{
	int i;

	if (intf == NULL)
		return;

	for (i = 0; i < num; i++) {
		free(intf[i].usb.manufacturer);
		free(intf[i].usb.product);
		free(intf[i].usb.serial);
		free(intf[i].usb.configuration);
		free(intf[i].usb.control_interface);
		free(intf[i].usb.data_interface_idle);
		free(intf[i].usb.data_interface_active);
		free(intf[i].usb.intf);
	}

	free(intf);
}

/*
 * Function usbobex_connect_request (self)
 *
 *    Open the USB connection
 *
 */
int usbobex_connect_request(obex_t *self)
{
	int ret;

	DEBUG(4, "\n");

	self->trans.self.usb.dev_control = usb_open(self->trans.self.usb.device);
	self->trans.self.usb.dev_data = usb_open(self->trans.self.usb.device);

	ret = usb_set_configuration(self->trans.self.usb.dev_control, self->trans.self.usb.configuration);
	if (ret < 0)
		DEBUG(4, "Can't set configuration %d", ret);

	ret = usb_claim_interface(self->trans.self.usb.dev_control, self->trans.self.usb.control_interface);
	if (ret < 0) {
		DEBUG(4, "Can't claim control interface %d", ret);
		goto err1;
	}

	ret = usb_set_altinterface(self->trans.self.usb.dev_control, self->trans.self.usb.control_setting);
	if (ret < 0) {
		DEBUG(4, "Can't set control setting %d", ret);
		goto err2;
	}

	ret = usb_claim_interface(self->trans.self.usb.dev_data, self->trans.self.usb.data_interface);
	if (ret < 0) {
		DEBUG(4, "Can't claim data interface %d", ret);
		goto err2;
	}

	ret = usb_set_altinterface(self->trans.self.usb.dev_data, self->trans.self.usb.data_active_setting);
	if (ret < 0) {
		DEBUG(4, "Can't set data active setting %d", ret);
		goto err3;
	}

	self->trans.mtu = OBEX_MAXIMUM_MTU;
	DEBUG(2, "transport mtu=%d\n", self->trans.mtu);
	return 1;

err3:
	usb_release_interface(self->trans.self.usb.dev_data, self->trans.self.usb.data_interface);
err2:
	usb_release_interface(self->trans.self.usb.dev_control, self->trans.self.usb.control_interface);
err1:
	usb_close(self->trans.self.usb.dev_data);
	usb_close(self->trans.self.usb.dev_control);
	return ret;
}

/*
 * Function usbobex_disconnect_request (self)
 *
 *    Shutdown the USB link
 *
 */
int usbobex_disconnect_request(obex_t *self)
{
	int ret;
	if (self->trans.connected == FALSE)
		return 0;

	DEBUG(4, "\n");
	ret = usb_set_altinterface(self->trans.self.usb.dev_data, self->trans.self.usb.data_idle_setting);
	if (ret < 0)
		DEBUG(4, "Can't set data idle setting %d", ret);
	ret = usb_release_interface(self->trans.self.usb.dev_data, self->trans.self.usb.data_interface);
	if (ret < 0)
		DEBUG(4, "Can't release data interface %d", ret);
	ret = usb_release_interface(self->trans.self.usb.dev_control, self->trans.self.usb.control_interface);
	if (ret < 0)
		DEBUG(4, "Can't release control interface %d", ret);
	ret = usb_close(self->trans.self.usb.dev_data);
	if (ret < 0)
		DEBUG(4, "Can't close data interface %d", ret);
	ret = usb_close(self->trans.self.usb.dev_control);
	if (ret < 0)
		DEBUG(4, "Can't close control interface %d", ret);

	return ret;
}

#endif /* HAVE_USB */
