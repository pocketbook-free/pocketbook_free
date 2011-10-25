
/****************************************************************************
 *
 *
 * FILE NANE:   				leds-gpio.h        
 *
 *
 * PURPOSE:             for samsung platform led data struct
 *
 *
 * GLOBAL VARIABLES:
 *
 * Variables            Type        Description
 * -------------        --------    ---------------------------
 *
 *
 * EXTERNAL FUNCTION
 *
 * Name                     Description
 * ------------------       ----------------------------------------------
 *
 *
 * DEVELOPMENT HISTORY:
 *
 * Date         Author          ChangeID    Release     Description of Change 
 * --------     --------        ------      -------     ---------------------
 * 2010-07-22   Áºº£É­(William)                            original version
 *
 ***************************************************************************/


#ifndef __ASM_ARCH_LEDSGPIO_H
#define __ASM_ARCH_LEDSGPIO_H "leds-gpio.h"

#define S3C_LEDS_NAME		"s3c_led"

#define S3C_LEDF_ACTLOW	(1<<0)		/* LED is on when GPIO low */
#define S3C_LEDF_TRISTATE	(1<<1)		/* tristate to turn off */
#define S3C_LEDF_ACTHIGH		0

struct s3c_led_platdata {
	unsigned int		 gpio;
	unsigned int		 flags;

	char			*name;
	char			*def_trigger;
};

#endif /* __ASM_ARCH_LEDSGPIO_H */
