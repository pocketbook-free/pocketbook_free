
/****************************************************************************
 *
 *
 * FILE NANE:       dev-led.c
 *
 *
 * PURPOSE:       		for samsung 64xx platform led device
 *                    
 *
 *
 * FILE REFERENCES:     none
 * Name                             IO          Description
 * ------------                     --          -------------------------   
 *
 *
 * EXTERNAL VARIABLES:  none    
 * Source:<>
 * Name                         Type        IO      Description
 * -------------                -------     --      ---------------------
 *
 *
 * EXTERNAL REFERENCES:     
 *
 * Name                         Description
 * ------------------       ----------------------------------------------  
 * 
 *                      
 * ABNORMAL TERMINATION CONDITIONS, ERROR AND WARNING MESSAGES:
 * none
 *
 *
 * ASSUMPTIONS, CONSTRAINTS, RESTRICTIONS: none
 *
 *
 * NOTES:
 *
 *
 * REQUIREMENTS/FUNCTIONAL SPECIFICATIONS REFERENCES:
 *
 *
 * Development History:
 * Date             Name            ChangeID    Release     Description 
 * --------         -------------   ------      -------     -----------------
 * 2010-07-22            Áºº£É­(William)                         original version
 *
 *
 * Algorithm (PDL):none
 ***************************************************************************/


#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <mach/leds-gpio.h>
#include <mach/gpio.h>


static struct s3c_led_platdata pdata_led1 = {
	.gpio		= S3C64XX_GPK(0),			//GPK0
#if defined(CONFIG_HW_EP1_EVT) || defined (CONFIG_HW_EP2_EVT)	|| defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)
	.flags		= S3C_LEDF_ACTLOW, // | S3C_LEDF_TRISTATE,
#else
	.flags    = S3C_LEDF_ACTHIGH, //S3C_LEDF_TRISTATE,
#endif
	.name		= "led_green",				//Green
	.def_trigger	= "timer",
};
//struct platform_device
struct platform_device platform_led1 = 
{
	.name		= S3C_LEDS_NAME, // "s3c_led",
	.id 		= 0,
	.dev 		=	{
							.platform_data = &pdata_led1,
						},	
};

static struct s3c_led_platdata pdata_led2 = {
	.gpio		= S3C64XX_GPK(1),			//GPK1
#if defined(CONFIG_HW_EP1_EVT) || defined (CONFIG_HW_EP2_EVT)	|| defined(CONFIG_HW_EP3_EVT) || defined(CONFIG_HW_EP4_EVT)
	.flags		= S3C_LEDF_ACTLOW , //| S3C_LEDF_TRISTATE,
#else
	.flags    = S3C_LEDF_ACTHIGH, //S3C_LEDF_TRISTATE,
#endif
	.name		= "led_orange",				//Oranger
	.def_trigger	= "timer",
};

struct platform_device platform_led2 = 
{
	.name		= S3C_LEDS_NAME, //"s3c_led",
	.id 		= 1,
	.dev 		=	{
							.platform_data = &pdata_led2,
						},	
};