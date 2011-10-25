
/****************************************************************************
 *
 *
 * FILE NANE:  				leds-s3c64xx.c     
 *
 *
 * PURPOSE:       		for samsung platform led driver
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
 * 2010-07-22       Áºº£É­(William)                         original version
 *
 *
 * Algorithm (PDL):none
 ***************************************************************************/


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <mach/hardware.h>
#include <mach/leds-gpio.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

struct s3c_gpio_led {
	struct led_classdev		 cdev;
	struct s3c_led_platdata	*pdata;
};

static inline struct s3c_gpio_led *pdev_to_gpio(struct platform_device *dev)
{
	return platform_get_drvdata(dev);
}


static inline struct s3c_gpio_led *to_gpio(struct led_classdev *led_cdev)
{
	return container_of(led_cdev, struct s3c_gpio_led, cdev);
}

/* according to include/linux/leds.h define */
/* Set LED brightness level */
/* Must not sleep, use a workqueue if needed */
static void s3c_led_set(struct led_classdev *led_cdev,
			    enum led_brightness value)
{
	struct s3c_gpio_led *led = to_gpio(led_cdev);
	struct s3c_led_platdata *pd = led->pdata;
	
	/* there will be a short delay between setting the output and
	  going from output to input when using tristate.
	 */
	//s3c_gpio_setpull(pd->gpio, S3C_GPIO_PULL_UP);
	
	gpio_set_value(pd->gpio, (value ? 1 : 0) ^
			    (pd->flags & S3C_LEDF_ACTLOW));

	if (pd->flags & S3C_LEDF_TRISTATE)
		s3c_gpio_cfgpin(pd->gpio,
			value ? S3C_GPIO_OUTPUT : S3C_GPIO_INPUT);
}
#if 0
static 	enum led_brightness sec_brightness_get(struct led_classdev *led_cdev)
{

	
}
#endif

static int s3c64xx_led_remove(struct platform_device *dev)
{
	struct s3c_gpio_led *led = pdev_to_gpio(dev);

	led_classdev_unregister(&led->cdev);
	kfree(led);

	return 0;
}


static int s3c64xx_led_probe(struct platform_device *dev)
{
	struct s3c_led_platdata *pdata = dev->dev.platform_data;
	struct s3c_gpio_led *led;
	int ret;
	printk("%s \n", __func__);
	led = kzalloc(sizeof(struct s3c_gpio_led), GFP_KERNEL);
	if (led == NULL) {
		dev_err(&dev->dev, "No memory for device\n");
		return -ENOMEM;
	}

	platform_set_drvdata(dev, led);

	led->cdev.brightness_set = s3c_led_set;
	led->cdev.default_trigger = pdata->def_trigger;
	led->cdev.name = pdata->name;
	led->cdev.flags |= LED_CORE_SUSPENDRESUME;

	led->pdata = pdata;
	
	printk("pdata->gpio is %d \n", pdata->gpio);
	printk("pdata->flags is 0x%x \n", pdata->flags);
	/* no point in having a pull-up if we are always driving */

	if (pdata->flags & S3C_LEDF_TRISTATE) {
		//gpio_set_value(pdata->gpio, );
		//s3c2410_gpio_setpin(pdata->gpio, 0);
		s3c_gpio_cfgpin(pdata->gpio, S3C_GPIO_INPUT);
	} else {
	if( 105 == pdata->gpio )
	    {	
			s3c_gpio_setpull(pdata->gpio, S3C_GPIO_PULL_UP);
			printk("Set LED off\n");
			gpio_set_value(pdata->gpio, pdata->flags & S3C_LEDF_ACTLOW ? 1 : 0);
			s3c_gpio_cfgpin(pdata->gpio, S3C_GPIO_OUTPUT);
	    }
	}

	/* register our new led device */

	ret = led_classdev_register(&dev->dev, &led->cdev);
	if (ret < 0) {
		dev_err(&dev->dev, "led_classdev_register failed\n");
		goto exit_err1;
	}

	return 0;

 exit_err1:
	kfree(led);
	return ret;
}


static struct platform_driver s3c64xx_led_driver = {
	.probe		= s3c64xx_led_probe,
	.remove		= s3c64xx_led_remove,
	.driver		= {
		.name		= S3C_LEDS_NAME, //"s3c_led",
		.owner		= THIS_MODULE,
	},
};

static int __init s3c64xx_led_init(void)
{
	return platform_driver_register(&s3c64xx_led_driver);
}

static void __exit s3c64xx_led_exit(void)
{
	platform_driver_unregister(&s3c64xx_led_driver);
}

module_init(s3c64xx_led_init);
module_exit(s3c64xx_led_exit);

MODULE_AUTHOR("lianghaisen <william.hs.liang@foxconn.com>");
MODULE_DESCRIPTION("S3C64XX LED driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:s3c64xx_led");

