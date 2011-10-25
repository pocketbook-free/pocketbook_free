//-----------------------------------------------------------------------------
//
// linux/drivers/video/epson/s1d13521ifgpio.c -- Gumstix specific GPIO
// interface code for Epson S1D13521 LCD controllerframe buffer driver.
//
// Code based on E-Ink demo source code.
//
// Copyright(c) Seiko Epson Corporation 2008.
// All rights reserved.
//
// This file is subject to the terms and conditions of the GNU General Public
// License. See the file COPYING in the main directory of this archive for
// more details.
//
//----------------------------------------------------------------------------

#ifdef CONFIG_FB_EPSON_NTX_EBOOK_2440
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include "s1d13521fb.h"

#include <video/ntx_s3c2440_epson_gpio.h>

#ifdef CONFIG_FB_EPSON_PROC
typedef struct
{
        const int gpio;
        const char *gpiostr;
}GPIONAMEST;

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
//
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
int dump_gpio(char *buf)
{
static GPIONAMEST aGpio[] =
        {
//        {GPIO_CNFX,     "GPIO_CNFX    (?)"},
//        {GPIO_CNF1,     "GPIO_CNF1    (?)"},
        {_GPIO_EPSON_PIN_PWR,   "Epson_Pwr    (1)"},

#ifdef CONFIG_FB_EPSON_HRDY_OK
        {_GPIO_EPSON_PIN_RDY,   "Epson_RDY    (0)"},
#endif
        {_GPIO_EPSON_PIN_DC,    "Epson_DC(Data=1)"},
        {_GPIO_EPSON_PIN_RESET, "Epson_RESET  (0)"},
        {_GPIO_EPSON_PIN_RD,    "Epson_Read   (0)"},
        {_GPIO_EPSON_PIN_WR,    "Epson_Write  (0)"},
        {_GPIO_EPSON_PIN_CS,    "Epson_CS     (0)"},
        {_GPIO_EPSON_PIN_IRQ,   "Epson_IRQ    (0)"},
        {_GPIO_EPSON_PIN_D0,    "Epson_DA0       "},
        {_GPIO_EPSON_PIN_D1,    "Epson_DA1       "},
        {_GPIO_EPSON_PIN_D2,    "Epson_DA2       "},
        {_GPIO_EPSON_PIN_D3,    "Epson_DA3       "},
        {_GPIO_EPSON_PIN_D4,    "Epson_DA4       "},
        {_GPIO_EPSON_PIN_D5,    "Epson_DA5       "},
        {_GPIO_EPSON_PIN_D6,    "Epson_DA6       "},
        {_GPIO_EPSON_PIN_D7,    "Epson_DA7       "},
        {_GPIO_EPSON_PIN_D8,    "Epson_DA8       "},
        {_GPIO_EPSON_PIN_D9,    "Epson_DA9       "},
        {_GPIO_EPSON_PIN_D10,   "Epson_DA10      "},
        {_GPIO_EPSON_PIN_D11,   "Epson_DA11      "},
        {_GPIO_EPSON_PIN_D12,   "Epson_DA12      "},
        {_GPIO_EPSON_PIN_D13,   "Epson_DA13      "},
        {_GPIO_EPSON_PIN_D14,   "Epson_DA14      "},
        {_GPIO_EPSON_PIN_D15,   "Epson_DA15      "}
        };

        int i;
        char *bufin = buf;

        buf +=sprintf(buf,"GPIO            MODE  DIR  VAL\n");
        buf +=sprintf(buf,"--------------------------\n");

        for (i = 0; i < sizeof(aGpio)/sizeof(aGpio[0]); i++)
        {
        	buf +=sprintf(buf,"%s              %d    %d    %d\n",
        		aGpio[i].gpiostr,
            	get_gpio_mode(aGpio[i].gpio),
                get_gpio_dir(aGpio[i].gpio),
                get_gpio_val(aGpio[i].gpio));
        }

        return strlen(bufin);
}
#endif //CONFIG_FB_EPSON_PROC

#endif // CONFIG_FB_EPSON_GPIO_GUMSTIX

