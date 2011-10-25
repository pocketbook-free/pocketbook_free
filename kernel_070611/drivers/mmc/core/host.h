/*
 *  linux/drivers/mmc/core/host.h
 *
 *  Copyright (C) 2003 Russell King, All Rights Reserved.
 *  Copyright 2007 Pierre Ossman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _MMC_CORE_HOST_H
#define _MMC_CORE_HOST_H

int mmc_register_host_class(void);
void mmc_unregister_host_class(void);

/*****2010/8/18 ADD BY JONO*****/
#define BCM_CARD_DETECT 1
#ifdef BCM_CARD_DETECT
extern struct mmc_host *mmc_get_sdio_host(void);
#endif
/*****END****** ADD BY JONO*****/

#endif

