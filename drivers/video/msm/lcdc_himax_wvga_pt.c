/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <mach/gpio.h>
#include <mach/pmic.h>
#include "msm_fb.h"




#include <linux/pwm.h>

//#include <kernel/drivers/mfd/pm8xxx-pwm.c>
//#include <linux/mfd/pm8xxx/pmic8058.h>
//#include <linux/pwm.h>
//extern int pwm_config(struct pwm_device *pwm, int duty_us, int period_us);
//extern struct pwm_device *pwm_request(int pwm_id, const char *label);
//extern int pwm_enable(struct pwm_device *pwm)

/*
static struct pwm_device *bl_pwm;
*/
#define PWM_FREQ_HZ 50000
#define PWM_PERIOD_USEC (USEC_PER_SEC / PWM_FREQ_HZ)
#define PWM_DUTY_LEVEL (PWM_PERIOD_USEC / PWM_LEVEL)
#define PWM_LEVEL 100

struct himax_state_type{
	boolean display_on;
	boolean display_off;
};

static struct himax_state_type himax_state = { 0 };
static struct msm_panel_common_pdata *lcdc_himax_pdata;


static int lcdc_himax_panel_on(struct platform_device *pdev)
{
	printk("himaxlcdc_himax_panel_on  on=%d\n ",himax_state.display_on);
	if (!himax_state.display_on) {
		/* Configure reset GPIO that drives DAC */
		if (lcdc_himax_pdata->panel_config_gpio)
			lcdc_himax_pdata->panel_config_gpio(1);
		
		himax_state.display_on=TRUE;
	}
	return 0;
}

static int lcdc_himax_panel_off(struct platform_device *pdev)
{
	printk("himax lcdc_himax_panel_off  on=%d\n ",himax_state.display_on);
	if (himax_state.display_on) {
		/* Main panel power off (Deep standby in) */

		if (lcdc_himax_pdata->panel_config_gpio)
			lcdc_himax_pdata->panel_config_gpio(0);
		himax_state.display_on = FALSE;
	}
	return 0;
}
#if 0
static void lcdc_himax_set_backlight(struct msm_fb_data_type *mfd)
{

	int ret;
	int bl_level;

	bl_level = mfd->bl_level;
	if (lcdc_himax_pdata && lcdc_himax_pdata->pmic_backlight)
		ret = lcdc_himax_pdata->pmic_backlight(bl_level);
	else
		pr_err("%s(): Backlight level set failed", __func__);

	return;
	
#if 0
	int ret;
	int level;
	level =mfd->bl_level;
	
	printk("enter lcdc_himax_set_backlight =%d \n" ,level);
	if (bl_pwm) {
		ret = pwm_config(bl_pwm, PWM_DUTY_LEVEL * level,
			PWM_PERIOD_USEC);
		if (ret) {
			pr_err("%s: pwm_config on pwm failed %d\n",
					__func__, ret);
			return;
		}

		ret = pwm_enable(bl_pwm);
		if (ret) {
			pr_err("%s: pwm_enable on pwm failed %d\n",
					__func__, ret);
			return;
		}
	}
#endif
}
#endif
static int __devinit himax_probe(struct platform_device *pdev)
{
	
	printk("enter himax_probe \n");
	
	if (pdev->id == 0) 
	{
		lcdc_himax_pdata = pdev->dev.platform_data;
		return 0;
	}

/*
	bl_pwm = pwm_request(lcdc_himax_pdata->gpio, "backlight");
	if (bl_pwm == NULL || IS_ERR(bl_pwm)) {
		pr_err("%s pwm_request() failed\n", __func__);
		bl_pwm = NULL;
	}

*/	
	msm_fb_add_device(pdev);
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = himax_probe,
	.driver = {
		.name   = "lcdc_himax_wvga",
	},
};
extern  void mipi_zte_set_backlight(struct msm_fb_data_type *mfd);
static struct msm_fb_panel_data himax_panel_data = {
	.on = lcdc_himax_panel_on,
	.off = lcdc_himax_panel_off,
	.set_backlight = mipi_zte_set_backlight,
};

static struct platform_device this_device = {
	.name   = "lcdc_himax_wvga",
	.id	= 1,
	.dev	= {
		.platform_data = &himax_panel_data,
	}
};

static int __init lcdc_himax_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

	ret = platform_driver_register(&this_driver);
	if (ret)
		return ret;

	pinfo = &himax_panel_data.panel_info;
	pinfo->xres = 1024;
	pinfo->yres = 600;
	MSM_FB_SINGLE_MODE_PANEL(pinfo);
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 24;
	pinfo->fb_num = 2;
	/* 30Mhz mdp_lcdc_pclk and mdp_lcdc_pad_pcl */
	//pinfo->clk_rate = 30720000;
	pinfo->clk_rate = 80000000;
	pinfo->bl_max = 32;
	pinfo->bl_min = 1;
//[ECID:000000] ZTEBSP weiguohua 20130110 for frame rate  start
	pinfo->frame_rate = 60;	
//[ECID:000000] ZTEBSP weiguohua 20130110 for frame rate end

//[ECID:000000] ZTEBSP zhangqi 20120105 for disp log start
#if 1

	pinfo->lcdc.h_back_porch = 150;
	pinfo->lcdc.h_front_porch = 180;
	pinfo->lcdc.h_pulse_width = 5;
	pinfo->lcdc.v_back_porch = 20;
	pinfo->lcdc.v_front_porch = 27;
	pinfo->lcdc.v_pulse_width = 2;
#else
	pinfo->lcdc.h_back_porch = 120;
	pinfo->lcdc.h_front_porch = 160;
	pinfo->lcdc.h_pulse_width = 40;
	pinfo->lcdc.v_back_porch = 20;
	pinfo->lcdc.v_front_porch = 12;
	pinfo->lcdc.v_pulse_width = 3;
#endif
//[ECID:000000] ZTEBSP zhangqi 20120105 for disp log end
	pinfo->lcdc.border_clr = 0;
	pinfo->lcdc.underflow_clr = 0xff;
	pinfo->lcdc.hsync_skew = 0;

	ret = platform_device_register(&this_device);
	if (ret) {
		printk(KERN_ERR "%s not able to register the device\n",
			 __func__);
		goto fail_driver;
	}
	return ret;

#ifdef CONFIG_SPI_QSD
fail_device:
	platform_device_unregister(&this_device);
#endif
fail_driver:
	platform_driver_unregister(&this_driver);
	return ret;
}

device_initcall(lcdc_himax_panel_init);
