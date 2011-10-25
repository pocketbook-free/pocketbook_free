/*
 * Wi-Fi built-in chip control
 *
 * Copyright (c) 2010 Foxconn TMSBG
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/rfkill.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/map.h>
#include <linux/delay.h>

#include <plat/regs-gpio.h>
#include <plat/devs.h>

#include <linux/workqueue.h>
#include <linux/mmc/host.h>

///Peter ++
extern int sdhci_resume_host(struct sdhci_host *host);
extern int sdhci_suspend_host(struct sdhci_host *host, pm_message_t state);
static int wifi_power_state = 0; 
///Peter --

static void wifi_power_up(void)
{
///Peter ++
		struct sdhci_host *mmc2_sdhci_host = platform_get_drvdata(&s3c_device_hsmmc2);
///Peter --

	  //printk(">>> RFKILL_WIFI, wifi_power_up\n");
    
          //  WF_MAC_WAKE to LOW
#ifdef CONFIG_EX3_HARDWARE_DVT
          gpio_set_value(S3C64XX_GPM(2), 0);
#else
          gpio_set_value(S3C64XX_GPL(8), 0);
#endif

          mdelay(160);

          // WF_RSTn to HIGH
          printk("  RFKILL_WIFI: WF_RSTn to HIGH\n");          
          gpio_set_value(S3C64XX_GPK(14), 1);

///Peter ++
		  if (wifi_power_state) {
				wifi_power_state = 0;
		  	sdhci_resume_host(mmc2_sdhci_host);	
		  }
		  printk("[MMC2]schedule_work(&sec->sdhci_host.mmc.detect)\n");
///Peter --
}

static void wifi_power_down(void)
{
///Peter ++	
		pm_message_t state;
		struct sdhci_host *mmc2_sdhci_host = platform_get_drvdata(&s3c_device_hsmmc2);
///Peter --

    //printk(">>> RFKILL_WIFI, power_off\n");
    
          //  WF_MAC_WAKE to LOW
#ifdef CONFIG_EX3_HARDWARE_DVT
          gpio_set_value(S3C64XX_GPM(2), 1);
#else
          gpio_set_value(S3C64XX_GPL(8), 1);
#endif          

          mdelay(50);

          // WF_RSTn to LOW
          printk("  RFKILL_WIFI: WF_RSTn to LOW\n");
          gpio_set_value(S3C64XX_GPK(14), 0);
					
///Peter ++
					if (wifi_power_state == 0)
                            {
						// set WF_SD_CLK, WF_SD_CMD, WF_SD_D1 ~ WF_SD_D4 to keep previous state in Sleep
						s3c_gpio_slp_cfgpin(S3C64XX_GPH(6), 3);		
						s3c_gpio_slp_cfgpin(S3C64XX_GPH(7), 3);
						s3c_gpio_slp_cfgpin(S3C64XX_GPH(8), 3);
						s3c_gpio_slp_cfgpin(S3C64XX_GPH(9), 3);	
						s3c_gpio_slp_cfgpin(S3C64XX_GPC(4), 3);	
						s3c_gpio_slp_cfgpin(S3C64XX_GPC(5), 3);
	
						s3c_gpio_slp_setpull_updown(S3C64XX_GPH(6), 0);	
						s3c_gpio_slp_setpull_updown(S3C64XX_GPH(7), 0);	
						s3c_gpio_slp_setpull_updown(S3C64XX_GPH(8), 0);	
						s3c_gpio_slp_setpull_updown(S3C64XX_GPH(9), 0);	
						s3c_gpio_slp_setpull_updown(S3C64XX_GPC(4), 0);	
						s3c_gpio_slp_setpull_updown(S3C64XX_GPC(5), 0);
						
						wifi_power_state = 1;	
						state.event =0;
						sdhci_suspend_host(mmc2_sdhci_host, state);
					}
///Peter --
}

static int wifi_toggle_radio(void *data, enum rfkill_state state)
{
    //printk(">>> RFKILL_WIFI: wifi_toggle_radio\n");
      
	pr_info("RFKILL: WIFI going %s\n", state == RFKILL_STATE_ON ? "On" : "Off");

	if (state == RFKILL_STATE_ON)
	{
		//pr_info("RFKILL_WIFI: going ON\n");
		wifi_power_up();
	}
	else
	{
		//pr_info("RFKILL_WIFI: going OFF\n");
		wifi_power_down();
	}
	
    //printk("<<< RFKILL_WIFI: wifi_toggle_radio\n");
	return 0;
}

static int wifi_probe(struct platform_device *dev)
{
	int rc;
	struct rfkill *rfk;
    
    printk(">>> RFKILL: wifi_probe\n");
    
	rfk = rfkill_allocate(&dev->dev, RFKILL_TYPE_WLAN);
	if (!rfk)
	{
		rc = -ENOMEM;
		goto err_rfk_alloc;
	}

	rfk->name = "BCM4319-WiFi";
	rfk->toggle_radio = wifi_toggle_radio;
      rfk->state = RFKILL_STATE_SOFT_BLOCKED;

#ifdef CONFIG_RFKILL_LEDS
	rfk->led_trigger.name = "BCM4319-WiFi";
#endif

	rc = rfkill_register(rfk);

	if (rc)
		goto err_rfkill;

	platform_set_drvdata(dev, rfk);

    printk("<<< RFKILL: wifi_probe\n");
	return 0;

err_rfkill:
	if (rfk)
		rfkill_free(rfk);

	rfk = NULL;
    
err_rfk_alloc:
	// printk("<<< RFKILL: wifi_probe, err_rfk_alloc\n");
	wifi_power_down();

	return rc;
}

static int __devexit wifi_remove(struct platform_device *dev)
{
	struct rfkill *rfk = platform_get_drvdata(dev);
	
    printk(">>> RFKILL: wifi_remove\n");

	platform_set_drvdata(dev, NULL);

	if (rfk)
		rfkill_unregister(rfk);
	
	rfk = NULL;

	wifi_power_down();

    printk("<<< RFKILL: wifi_remove\n");
	return 0;
}

static struct platform_driver wifi_driver = {
	.probe = wifi_probe,
	.remove = __devexit_p(wifi_remove),

	.driver = {
		.name = "wifi_rfkill",
		.owner = THIS_MODULE,
	},
};


static int __init wifi_power_init(void)
{
	int _result;
	
      printk(">>> RFKILL: wifi_power_init\n");
	
	_result = platform_driver_register(&wifi_driver);
      printk("<<< RFKILL: wifi_power_init\n");
	return _result;	
}

static void __exit wifi_power_exit(void)
{
      printk(">>> RFKILL: wifi_power_exit\n");
	platform_driver_unregister(&wifi_driver);
      printk("<<< RFKILL: wifi_power_exit\n");
}

module_init(wifi_power_init);
module_exit(wifi_power_exit);