/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <asm/mach-types.h>
#include <asm/io.h>
#include <mach/msm_bus_board.h>
#include <mach/msm_memtypes.h>
#include <mach/board.h>
#include <mach/gpiomux.h>
#include <mach/socinfo.h>
#include <mach/rpc_pmapp.h>
#include "devices.h"
#include "board-msm7627a.h"

/* [ECID:000000] ZTEBSP wangjianping, 20120914 add p865f02 lcd driver, start */
#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
//[ECID:000000] ZTEBSP  shihuiqin, 20120925 add for P825F02++ 
//#if defined(CONFIG_PROJECT_P865F02)
#if defined(CONFIG_PROJECT_P865F03)
//[ECID:000000] ZTEBSP  shihuiqin, 20120925 add for P825F02-- 
#define MSM_FB_SIZE		0x600000
#define MSM7x25A_MSM_FB_SIZE    0x1C2000
#define MSM8x25_MSM_FB_SIZE	0x600000
#elif defined(CONFIG_PROJECT_V72A)
#define MSM_FB_SIZE		0x708000
#define MSM7x25A_MSM_FB_SIZE    0x1C2000
#define MSM8x25_MSM_FB_SIZE	0x708000
#else
#define MSM_FB_SIZE		0x4BF000
#define MSM7x25A_MSM_FB_SIZE    0x1C2000
#define MSM8x25_MSM_FB_SIZE	0x5FA000
#endif
#else
#define MSM_FB_SIZE		0x32A000
#define MSM7x25A_MSM_FB_SIZE	0x12C000
#define MSM8x25_MSM_FB_SIZE	0x3FC000
#endif
/* [ECID:000000] ZTEBSP wangjianping, 20120914 add p865f02 lcd driver, end */

/*
 * Reserve enough v4l2 space for a double buffered full screen
 * res image (864x480x1.5x2)
 */
#define MSM_V4L2_VIDEO_OVERLAY_BUF_SIZE 1244160

static unsigned fb_size = MSM_FB_SIZE;
static int __init fb_size_setup(char *p)
{
	fb_size = memparse(p, NULL);
	return 0;
}

early_param("fb_size", fb_size_setup);

static uint32_t lcdc_truly_gpio_initialized;
static struct regulator_bulk_data regs_truly_lcdc[] = {
	{ .supply = "rfrx1",   .min_uV = 1800000, .max_uV = 1800000 },
};

#define SKU3_LCDC_GPIO_DISPLAY_RESET	90
#define SKU3_LCDC_GPIO_SPI_MOSI		19
#define SKU3_LCDC_GPIO_SPI_CLK		20
#define SKU3_LCDC_GPIO_SPI_CS0_N	21
#define SKU3_LCDC_LCD_CAMERA_LDO_2V8	35  /*LCD_CAMERA_LDO_2V8*/
#define SKU3_LCDC_LCD_CAMERA_LDO_1V8	34  /*LCD_CAMERA_LDO_1V8*/
#define SKU3_1_LCDC_LCD_CAMERA_LDO_1V8	58  /*LCD_CAMERA_LDO_1V8*/

static uint32_t lcdc_truly_gpio_table[] = {
	19,
	20,
	21,
	89,
	90,
};

static char *lcdc_gpio_name_table[5] = {
	"spi_mosi",
	"spi_clk",
	"spi_cs",
	"gpio_bkl_en",
	"gpio_disp_reset",
};

static char lcdc_splash_is_enabled(void);
static int lcdc_truly_gpio_init(void)
{
	int i;
	int rc = 0;

	if (!lcdc_truly_gpio_initialized) {
		for (i = 0; i < ARRAY_SIZE(lcdc_truly_gpio_table); i++) {
			rc = gpio_request(lcdc_truly_gpio_table[i],
				lcdc_gpio_name_table[i]);
			if (rc < 0) {
				pr_err("Error request gpio %s\n",
					lcdc_gpio_name_table[i]);
				goto truly_gpio_fail;
			}
			rc = gpio_tlmm_config(GPIO_CFG(lcdc_truly_gpio_table[i],
				0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			if (rc < 0) {
				pr_err("Error config lcdc gpio:%d\n",
					lcdc_truly_gpio_table[i]);
				goto truly_gpio_fail;
			}
			if (lcdc_splash_is_enabled())
				rc = gpio_direction_output(
					lcdc_truly_gpio_table[i], 1);
			else
				rc = gpio_direction_output(
					lcdc_truly_gpio_table[i], 0);
			if (rc < 0) {
				pr_err("Error direct lcdc gpio:%d\n",
					lcdc_truly_gpio_table[i]);
				goto truly_gpio_fail;
			}
		}

			lcdc_truly_gpio_initialized = 1;
	}

	return rc;

truly_gpio_fail:
	for (; i >= 0; i--) {
		pr_err("Freeing GPIO: %d", lcdc_truly_gpio_table[i]);
		gpio_free(lcdc_truly_gpio_table[i]);
	}

	lcdc_truly_gpio_initialized = 0;
	return rc;
}


void sku3_lcdc_lcd_camera_power_init(void)
{
	int rc = 0;
	u32 socinfo = socinfo_get_platform_type();

	  /* LDO_EXT2V8 */
	if (gpio_request(SKU3_LCDC_LCD_CAMERA_LDO_2V8, "lcd_camera_ldo_2v8")) {
		pr_err("failed to request gpio lcd_camera_ldo_2v8\n");
		return;
	}

	rc = gpio_tlmm_config(GPIO_CFG(SKU3_LCDC_LCD_CAMERA_LDO_2V8, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
		GPIO_CFG_ENABLE);

	if (rc < 0) {
		pr_err("%s:unable to enable lcd_camera_ldo_2v8!\n", __func__);
		goto fail_gpio2;
	}

	/* LDO_EVT1V8 */
	if (socinfo == 0x0B) {
		if (gpio_request(SKU3_LCDC_LCD_CAMERA_LDO_1V8,
				"lcd_camera_ldo_1v8")) {
			pr_err("failed to request gpio lcd_camera_ldo_1v8\n");
			goto fail_gpio1;
		}

		rc = gpio_tlmm_config(GPIO_CFG(SKU3_LCDC_LCD_CAMERA_LDO_1V8, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);

		if (rc < 0) {
			pr_err("%s: unable to enable lcdc_camera_ldo_1v8!\n",
				__func__);
			goto fail_gpio1;
		}
	} else if (socinfo == 0x0F || machine_is_msm8625_qrd7()) {
		if (gpio_request(SKU3_1_LCDC_LCD_CAMERA_LDO_1V8,
				"lcd_camera_ldo_1v8")) {
			pr_err("failed to request gpio lcd_camera_ldo_1v8\n");
			goto fail_gpio1;
		}

		rc = gpio_tlmm_config(GPIO_CFG(SKU3_1_LCDC_LCD_CAMERA_LDO_1V8,
			0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);

		if (rc < 0) {
			pr_err("%s: unable to enable lcdc_camera_ldo_1v8!\n",
				__func__);
			goto fail_gpio1;
		}
	}

	rc = regulator_bulk_get(NULL, ARRAY_SIZE(regs_truly_lcdc),
			regs_truly_lcdc);
	if (rc)
		pr_err("%s: could not get regulators: %d\n", __func__, rc);

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs_truly_lcdc),
			regs_truly_lcdc);
	if (rc)
		pr_err("%s: could not set voltages: %d\n", __func__, rc);

	return;

fail_gpio1:
	if (socinfo == 0x0B)
		gpio_free(SKU3_LCDC_LCD_CAMERA_LDO_1V8);
	else if (socinfo == 0x0F || machine_is_msm8625_qrd7())
		gpio_free(SKU3_1_LCDC_LCD_CAMERA_LDO_1V8);
fail_gpio2:
	gpio_free(SKU3_LCDC_LCD_CAMERA_LDO_2V8);
	return;
}

int sku3_lcdc_lcd_camera_power_onoff(int on)
{
	int rc = 0;
	u32 socinfo = socinfo_get_platform_type();

	if (on) {
		if (socinfo == 0x0B)
			gpio_set_value_cansleep(SKU3_LCDC_LCD_CAMERA_LDO_1V8,
				1);
		else if (socinfo == 0x0F || machine_is_msm8625_qrd7())
			gpio_set_value_cansleep(SKU3_1_LCDC_LCD_CAMERA_LDO_1V8,
				1);

		gpio_set_value_cansleep(SKU3_LCDC_LCD_CAMERA_LDO_2V8, 1);

		rc = regulator_bulk_enable(ARRAY_SIZE(regs_truly_lcdc),
				regs_truly_lcdc);
		if (rc)
			pr_err("%s: could not enable regulators: %d\n",
				__func__, rc);
	} else {
		if (socinfo == 0x0B)
			gpio_set_value_cansleep(SKU3_LCDC_LCD_CAMERA_LDO_1V8,
				0);
		else if (socinfo == 0x0F || machine_is_msm8625_qrd7())
			gpio_set_value_cansleep(SKU3_1_LCDC_LCD_CAMERA_LDO_1V8,
				0);

		gpio_set_value_cansleep(SKU3_LCDC_LCD_CAMERA_LDO_2V8, 0);

		rc = regulator_bulk_disable(ARRAY_SIZE(regs_truly_lcdc),
				regs_truly_lcdc);
		if (rc)
			pr_err("%s: could not disable regulators: %d\n",
				__func__, rc);
	}

	return rc;
}

static int sku3_lcdc_power_save(int on)
{
	int rc = 0;
	static int cont_splash_done;

	if (on) {
		sku3_lcdc_lcd_camera_power_onoff(1);
		rc = lcdc_truly_gpio_init();
		if (rc < 0) {
			pr_err("%s(): Truly GPIO initializations failed",
				__func__);
			return rc;
		}

		if (lcdc_splash_is_enabled() && !cont_splash_done) {
			cont_splash_done = 1;
			return rc;
		}

		if (lcdc_truly_gpio_initialized) {
			/*LCD reset*/
			gpio_set_value(SKU3_LCDC_GPIO_DISPLAY_RESET, 1);
			msleep(20);
			gpio_set_value(SKU3_LCDC_GPIO_DISPLAY_RESET, 0);
			msleep(20);
			gpio_set_value(SKU3_LCDC_GPIO_DISPLAY_RESET, 1);
			msleep(20);
		}
	} else {
		/* pull down LCD IO to avoid current leakage */
		gpio_set_value(SKU3_LCDC_GPIO_SPI_MOSI, 0);
		gpio_set_value(SKU3_LCDC_GPIO_SPI_CLK, 0);
		gpio_set_value(SKU3_LCDC_GPIO_SPI_CS0_N, 0);
		gpio_set_value(SKU3_LCDC_GPIO_DISPLAY_RESET, 0);

		sku3_lcdc_lcd_camera_power_onoff(0);
	}
	return rc;
}

static struct msm_panel_common_pdata lcdc_truly_panel_data = {
	.panel_config_gpio = NULL,
	.gpio_num	  = lcdc_truly_gpio_table,
};

static struct platform_device lcdc_truly_panel_device = {
	.name   = "lcdc_truly_hvga_ips3p2335_pt",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_truly_panel_data,
	}
};

#ifndef CONFIG_ZTE_UART_USE_RGB_LCD_LVDS
static struct regulator_bulk_data regs_lcdc[] = {
	/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
	{ .supply = "bt",   .min_uV = 2600000, .max_uV = 2600000 },
	//{ .supply = "gp2",   .min_uV = 2850000, .max_uV = 2850000 },
	//{ .supply = "msme1", .min_uV = 1800000, .max_uV = 1800000 },
	/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */
};
static uint32_t lcdc_gpio_initialized;

static void lcdc_toshiba_gpio_init(void)
{
	int rc = 0;
	if (!lcdc_gpio_initialized) {
		/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
		if (gpio_request(124, "spi_clk")) {
			pr_err("failed to request gpio spi_clk\n");
			return;
		}
		if (gpio_request(122, "spi_cs")) {
			pr_err("failed to request gpio spi_cs0_N\n");
			goto fail_gpio6;
		}
		if (gpio_request(109, "spi_mosi")) {
			pr_err("failed to request gpio spi_mosi\n");
			goto fail_gpio5;
		}
		if (gpio_request(123, "spi_miso")) {
			pr_err("failed to request gpio spi_miso\n");
			goto fail_gpio4;
		}
		/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */
		
		/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
		/* if (gpio_request(89, "gpio_disp_rst")) { */
		/* 	pr_err("failed to request gpio_disp_pwr\n"); */
		/* 	goto fail_gpio3; */
		/* } */
		//if (gpio_request(GPIO_BACKLIGHT_EN, "gpio_bkl_en")) {
		//	pr_err("failed to request gpio_bkl_en\n");
		//	goto fail_gpio2;
		//}
		//pmapp_disp_backlight_init();
		/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */

		rc = regulator_bulk_get(NULL, ARRAY_SIZE(regs_lcdc),
					regs_lcdc);
		if (rc) {
			pr_err("%s: could not get regulators: %d\n",
					__func__, rc);
			goto fail_gpio1;
		}

		rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs_lcdc),
				regs_lcdc);
		if (rc) {
			pr_err("%s: could not set voltages: %d\n",
					__func__, rc);
			goto fail_vreg;
		}
		lcdc_gpio_initialized = 1;
	}
	return;
fail_vreg:
	regulator_bulk_free(ARRAY_SIZE(regs_lcdc), regs_lcdc);
fail_gpio1:

/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
//	gpio_free(GPIO_BACKLIGHT_EN);
//fail_gpio2:
	//gpio_free(89);
	//fail_gpio3:
	gpio_free(123);
fail_gpio4:
	gpio_free(109);
fail_gpio5:
	gpio_free(122);
fail_gpio6:
	gpio_free(124);
	lcdc_gpio_initialized = 0;
/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */
}

/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
static uint32_t lcdc_gpio_table[] = {
	124,
	122,
	109,
	0xff,//	89,
	//GPIO_BACKLIGHT_EN,
	123,
};
/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */

static void config_lcdc_gpio_table(uint32_t *table, int len, unsigned enable)
{
	int n;

	if (lcdc_gpio_initialized) {
		/* All are IO Expander GPIOs */
		for (n = 0; n < (len - 1); n++)
			gpio_direction_output(table[n], 1);
	}
}

static void lcdc_toshiba_config_gpios(int enable)
{
	config_lcdc_gpio_table(lcdc_gpio_table,
		ARRAY_SIZE(lcdc_gpio_table), enable);
}
#endif
#ifdef CONFIG_FB_MSM_LCDC_HIMAX_WVGA_PT
//[ECID:000000] ZTEBSP zhangqi modify for lcd 20120510 end
//[ECID:000000] ZTEBSP zhangqi add for lcd 20111216 start
int v70_lcd_vcc_init =0;

static struct regulator_bulk_data himax_regs_lcdc[] = {
	{ .supply = "rfrx1",   .min_uV = 3000000, .max_uV = 3000000 },
};

static unsigned lcd_gpio[] = {
		GPIO_CFG(GPIO_LCDC_CABC_EN, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* LCDC_CABC_EN */
		GPIO_CFG(GPIO_LCDC_DBCM0, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* LCDC_DBCM0 */
		GPIO_CFG(GPIO_LCDC_DBCM1, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* LCDC_DBCM1 */
		GPIO_CFG(GPIO_LVDS_RSTB, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* LCDC_CABC_EN */
		GPIO_CFG(GPIO_LVDS_STANDBY, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* LCDC_DBCM0 */
		GPIO_CFG(GPIO_LVDS_XRST, 0, GPIO_CFG_OUTPUT,
		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* LCDC_DBCM1 */		
};

static void lcdc_himax_config_gpios(int enable)
{
	int rc=0;
	static int lcdc_first_time=0; //[ECID:000000] ZTEBSP zhangqi 20120105 for disp log start
	
	printk("lcdc lcdc_himax_config_gpios  enable =%d v70_lcd_vcc_init=%d\n",enable,v70_lcd_vcc_init);

	if (!v70_lcd_vcc_init)
	{
		rc = regulator_bulk_get(NULL, ARRAY_SIZE(himax_regs_lcdc), himax_regs_lcdc);
		if (rc) {
			pr_err("%s: could not get regulators: %d\n",
					__func__, rc);
			goto failed;
		}

		rc = regulator_bulk_set_voltage(ARRAY_SIZE(himax_regs_lcdc),
				himax_regs_lcdc);
		if (rc) {
			pr_err("%s: could not set voltages: %d\n",
					__func__, rc);
			goto failed;
		}

		gpio_request(GPIO_LCDC_CABC_EN, "lcd_cabc_en");
		gpio_tlmm_config(lcd_gpio[0], GPIO_CFG_ENABLE);		
		gpio_request(GPIO_LCDC_DBCM0, "lcd_dbcm0");
		gpio_tlmm_config(lcd_gpio[1], GPIO_CFG_ENABLE);		
		gpio_request(GPIO_LCDC_DBCM1, "lcd_dbcm1");		
		gpio_tlmm_config(lcd_gpio[2], GPIO_CFG_ENABLE);

		gpio_request(GPIO_LVDS_RSTB, "lvds_rstb");				
		gpio_tlmm_config(lcd_gpio[3], GPIO_CFG_ENABLE);		
		gpio_request(GPIO_LVDS_STANDBY, "lcd_standby");		
		gpio_tlmm_config(lcd_gpio[4], GPIO_CFG_ENABLE);		
		gpio_request(GPIO_LVDS_XRST, "lvds_xrst");
		gpio_tlmm_config(lcd_gpio[5], GPIO_CFG_ENABLE);			
		
		v70_lcd_vcc_init=1;
	}

	if (enable)
	{    
	    rc = regulator_bulk_enable(
		   ARRAY_SIZE(himax_regs_lcdc), himax_regs_lcdc);
		gpio_set_value(GPIO_LCDC_CABC_EN, 1);	
		msleep(10);		
	    gpio_set_value(GPIO_LCDC_DBCM0, 1);	
		msleep(10);		
		gpio_set_value(GPIO_LCDC_DBCM1, 1);
//[ECID:000000] ZTEBSP zhangqi 20120105 for disp log start

	if (lcdc_first_time)
		{
		
		gpio_set_value(GPIO_LVDS_XRST, 1);	
		msleep(20);
		gpio_set_value(GPIO_LVDS_XRST, 0);			
		msleep(3);		
		gpio_set_value(GPIO_LVDS_RSTB, 1);	
		msleep(20);
		gpio_set_value(GPIO_LVDS_RSTB, 0);
		}
	lcdc_first_time=1;
//[ECID:000000] ZTEBSP zhangqi 20120105 for disp log end

		msleep(3);		
		gpio_set_value(GPIO_LVDS_STANDBY, 0);	
	}
	else
	{
	    rc = regulator_bulk_disable(
		       ARRAY_SIZE(himax_regs_lcdc), himax_regs_lcdc);
		gpio_set_value(GPIO_LCDC_CABC_EN, 1);				
		msleep(10);		
		gpio_set_value(GPIO_LCDC_DBCM0, 1);					
		msleep(10);		
		gpio_set_value(GPIO_LCDC_DBCM1,1);
		msleep(10);
		gpio_set_value(GPIO_LVDS_STANDBY, 1);	
	}
        //[ECID:000000] ZTEBSP liwen 20120924,modify  for lcdc on off  delay end		
	return;
failed:
	printk("lcdc_himax_config_gpios error \n");
}
//[ECID:000000] ZTEBSP zhangqi add for lcd 20111216 end
#endif

static int msm_fb_lcdc_power_save(int on)
{
	int rc = 0;
	/* Doing the init of the LCDC GPIOs very late as they are from
		an I2C-controlled IO Expander */
#ifndef CONFIG_ZTE_UART_USE_RGB_LCD_LVDS
	lcdc_toshiba_gpio_init();

	if (lcdc_gpio_initialized) {
		/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
		//gpio_set_value_cansleep(GPIO_DISPLAY_PWR_EN, on);
		//gpio_set_value_cansleep(GPIO_BACKLIGHT_EN, on);
		/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */

		rc = on ? regulator_bulk_enable(
				ARRAY_SIZE(regs_lcdc), regs_lcdc) :
			  regulator_bulk_disable(
				ARRAY_SIZE(regs_lcdc), regs_lcdc);

		if (rc)
			pr_err("%s: could not %sable regulators: %d\n",
					__func__, on ? "en" : "dis", rc);
	}
#endif
	return rc;
}
#ifndef CONFIG_ZTE_UART_USE_RGB_LCD_LVDS

static int lcdc_toshiba_set_bl(int level)
{
	int ret;
/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
	return 0;
/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */

	ret = pmapp_disp_backlight_set_brightness(level);
	if (ret)
		pr_err("%s: can't set lcd backlight!\n", __func__);

	return ret;
}
#endif
#ifdef CONFIG_FB_MSM_LCDC_HIMAX_WVGA_PT
//[ECID:000000] ZTEBSP zhangqi add  lcd 20111218 start
static int lcdc_himax_set_bl(int level)
{

/*[ECID:000000] ZTEBSP weiguohua start  20121129,  for V72A not use pmapp backlight */
	return 0;
/*[ECID:000000] ZTEBSP weiguohua end  20121129, for V72A not use pmapp backlight */

}
//[ECID:000000] ZTEBSP zhangqi add  lcd 20111218 end
#endif
static int msm_lcdc_power_save(int on)
{
	int rc = 0;
	if (machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7())
		rc = sku3_lcdc_power_save(on);
	else
		rc = msm_fb_lcdc_power_save(on);

	return rc;
}
static struct lcdc_platform_data lcdc_pdata = {
	.lcdc_gpio_config = NULL,
	.lcdc_power_save   = msm_lcdc_power_save,
};
#ifndef CONFIG_FB_MSM_LCDC_HIMAX_WVGA_PT
/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
static int lcd_panel_spi_gpio_num[] = {
		124,   /* spi_clk */
		122, /* spi_cs  */
		109,  /* spi_sdoi */
		123,  /* spi_sdi */
		0xff, /* rst */
};
/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */

static struct msm_panel_common_pdata lcdc_toshiba_panel_data = {
	.panel_config_gpio = lcdc_toshiba_config_gpios,
	.pmic_backlight = lcdc_toshiba_set_bl,
	.gpio_num	 = lcd_panel_spi_gpio_num,
};

static struct platform_device lcdc_toshiba_panel_device = {
	.name   = "lcdc_toshiba_fwvga_pt",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_toshiba_panel_data,
	}
};
#endif
#ifdef CONFIG_FB_MSM_LCDC_HIMAX_WVGA_PT
//[ECID:000000] ZTEBSP zhangqi add for lcd 20111216 start
static struct msm_panel_common_pdata lcdc_himax_panel_data = {
	.panel_config_gpio = lcdc_himax_config_gpios,
	//.gpio=4,
	.pmic_backlight = lcdc_himax_set_bl,
	//.gpio_num	  = lcd_panel_spi_gpio_num,
};

static struct platform_device lcdc_himax_panel_device = {
	.name   = "lcdc_himax_wvga",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_himax_panel_data,
	}
};

//[ECID:000000] ZTEBSP zhangqi add for lcd 20111216 end
#endif
static struct resource msm_fb_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	}
};

#ifdef CONFIG_MSM_V4L2_VIDEO_OVERLAY_DEVICE
static struct resource msm_v4l2_video_overlay_resources[] = {
	{
		.flags = IORESOURCE_DMA,
	}
};
#endif

#define LCDC_TOSHIBA_FWVGA_PANEL_NAME   "lcdc_toshiba_fwvga_pt"
#define MIPI_CMD_RENESAS_FWVGA_PANEL_NAME       "mipi_cmd_renesas_fwvga"
//wangminrong add ++
#define VIDEO_WVGA_PANEL_NAME   "video_wvga_pt"
#define VIDEO_FWVGA_PANEL_NAME   "video_fwvga_pt"
#define VIDEO_QHD_PANEL_NAME   "video_qhd_pt"
//wangminrong add --
static int msm_fb_detect_panel(const char *name)
{
	int ret = -ENODEV;
	//wangminrong modify ++
	#if 0
	if (machine_is_msm7x27a_surf() || machine_is_msm7625a_surf() ||
			machine_is_msm8625_surf()) {
		if (!strncmp(name, "lcdc_toshiba_fwvga_pt", 21) ||
				!strncmp(name, "mipi_cmd_renesas_fwvga", 22))
			ret = 0;
	} else if (machine_is_msm7x27a_ffa() || machine_is_msm7625a_ffa()
					|| machine_is_msm8625_ffa()) {
		if (!strncmp(name, "mipi_cmd_renesas_fwvga", 22))
			ret = 0;
	} else if (machine_is_msm7627a_qrd1()) {
		if (!strncmp(name, "mipi_video_truly_wvga", 21))
			ret = 0;
	} else if (machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7()) {
		if (!strncmp(name, "lcdc_truly_hvga_ips3p2335_pt", 28))
			ret = 0;
	} else if (machine_is_msm7627a_evb() || machine_is_msm8625_evb() ||
			machine_is_msm8625_evt()) {
		if (!strncmp(name, "mipi_cmd_nt35510_wvga", 21))
			ret = 0;
	}

#if !defined(CONFIG_FB_MSM_LCDC_AUTO_DETECT) && \
	!defined(CONFIG_FB_MSM_MIPI_PANEL_AUTO_DETECT) && \
	!defined(CONFIG_FB_MSM_LCDC_MIPI_PANEL_AUTO_DETECT)
		if (machine_is_msm7x27a_surf() ||
			machine_is_msm7625a_surf() ||
			machine_is_msm8625_surf()) {
			if (!strncmp(name, LCDC_TOSHIBA_FWVGA_PANEL_NAME,
				strnlen(LCDC_TOSHIBA_FWVGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
				return 0;
		}
#endif
	#endif
	//wangminrong --
//wangminrong ++
//DOUMINGMING 20130125
//#if  defined(CONFIG_PROJECT_P825A20) || defined(CONFIG_PROJECT_P865E01) || defined(CONFIG_PROJECT_P865E02)
#if  defined(CONFIG_PROJECT_P825A20) || defined(CONFIG_PROJECT_P865E01) || defined(CONFIG_PROJECT_P865E02) || defined(CONFIG_PROJECT_P865V20)
	if (!strncmp(name, VIDEO_WVGA_PANEL_NAME,strnlen(VIDEO_WVGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
				return 0;
#endif
#if  defined(CONFIG_PROJECT_P825F01) || defined(CONFIG_PROJECT_P865F01) || defined(CONFIG_PROJECT_P865V30)
	if (!strncmp(name, VIDEO_FWVGA_PANEL_NAME,strnlen(VIDEO_FWVGA_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
				return 0;
#endif 
#if  defined(CONFIG_PROJECT_P865F03) 
	if (!strncmp(name, VIDEO_QHD_PANEL_NAME,strnlen(VIDEO_QHD_PANEL_NAME,
					PANEL_NAME_MAX_LEN)))
				return 0;
#endif

//wangminrong --
	return ret;
}

/*[ECID:000000] ZTEBSP yaotierui start  20120320, 8x25 bring up  */
#if 0
static int mipi_truly_set_bl(int on)
{
	gpio_set_value_cansleep(QRD_GPIO_BACKLIGHT_EN, !!on);

	return 1;
}
#endif
/*[ECID:000000] ZTEBSP yaotierui end  20120320, P825A20 bring up */

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_fb_resources),
	.resource       = msm_fb_resources,
	.dev    = {
		.platform_data = &msm_fb_pdata,
	}
};

#ifdef CONFIG_MSM_V4L2_VIDEO_OVERLAY_DEVICE
static struct platform_device msm_v4l2_video_overlay_device = {
		.name   = "msm_v4l2_overlay_pd",
		.id     = 0,
		.num_resources  = ARRAY_SIZE(msm_v4l2_video_overlay_resources),
		.resource       = msm_v4l2_video_overlay_resources,
	};
#endif


#ifdef CONFIG_FB_MSM_MIPI_DSI
/*[ECID:000000] ZTEBSP yaotierui start  20120320, 8x25 bring up  */
#if 0
static int mipi_renesas_set_bl(int level)
{
	int ret;

	ret = pmapp_disp_backlight_set_brightness(level);

	if (ret)
		pr_err("%s: can't set lcd backlight!\n", __func__);

	return ret;
}
#endif
/*[ECID:000000] ZTEBSP yaotierui end  20120320, P825A20 bring up */
static struct msm_panel_common_pdata mipi_renesas_pdata = {
/*[ECID:000000] ZTEBSP yaotierui start  20120320, 8x25 bring up  */
	//.pmic_backlight = mipi_renesas_set_bl,
		.pmic_backlight = NULL,
/*[ECID:000000] ZTEBSP yaotierui end  20120320, P825A20 bring up */
};


static struct platform_device mipi_dsi_renesas_panel_device = {
	.name =  "mipi_lead",   //"mipi_renesas",   /*[ECID:000000] ZTEBSP yaotierui start  20120320, 8x25 bring up  */
	.id = 0,
	.dev    = {
		.platform_data = &mipi_renesas_pdata,
	}
};
#endif

static int evb_backlight_control(int level, int mode)
{

	int i = 0;
	int remainder, ret = 0;
	u32 socinfo = socinfo_get_version();

	/* device address byte = 0x72 */
	if (!mode) {
		if (socinfo != 0x10000 && level == 0)
			level = 10;
		gpio_set_value(96, 0);
		udelay(67);
		gpio_set_value(96, 1);
		udelay(33);
		gpio_set_value(96, 0);
		udelay(33);
		gpio_set_value(96, 1);
		udelay(67);
		gpio_set_value(96, 0);
		udelay(33);
		gpio_set_value(96, 1);
		udelay(67);
		gpio_set_value(96, 0);
		udelay(33);
		gpio_set_value(96, 1);
		udelay(67);
		gpio_set_value(96, 0);
		udelay(67);
		gpio_set_value(96, 1);
		udelay(33);
		gpio_set_value(96, 0);
		udelay(67);
		gpio_set_value(96, 1);
		udelay(33);
		gpio_set_value(96, 0);
		udelay(33);
		gpio_set_value(96, 1);
		udelay(67);
		gpio_set_value(96, 0);
		udelay(67);
		gpio_set_value(96, 1);
		udelay(33);

		/* t-EOS and t-start */
		gpio_set_value(96, 0);
		ndelay(4200);
		gpio_set_value(96, 1);
		ndelay(9000);

		/* data byte */
		/* RFA = 0 */
		gpio_set_value(96, 0);
		udelay(67);
		gpio_set_value(96, 1);
		udelay(33);

		/* Address bits */
		gpio_set_value(96, 0);
		udelay(67);
		gpio_set_value(96, 1);
		udelay(33);
		gpio_set_value(96, 0);
		udelay(67);
		gpio_set_value(96, 1);
		udelay(33);

		/* Data bits */
		for (i = 0; i < 5; i++) {
			remainder = (level) & (16);
			if (remainder) {
				gpio_set_value(96, 0);
				udelay(33);
				gpio_set_value(96, 1);
				udelay(67);
			} else {
				gpio_set_value(96, 0);
				udelay(67);
				gpio_set_value(96, 1);
				udelay(33);
			}
			level = level << 1;
		}

		/* t-EOS */
		gpio_set_value(96, 0);
		ndelay(12000);
		gpio_set_value(96, 1);
	} else {
		ret = pmapp_disp_backlight_set_brightness(level);
		 if (ret)
			pr_err("%s: can't set lcd backlight!\n", __func__);
	}

	return ret;
}

static int mipi_NT35510_rotate_panel(void)
{
	int rotate = 0;
	if (machine_is_msm8625_evt())
		rotate = 1;

	return rotate;
}

static struct msm_panel_common_pdata mipi_truly_pdata = {
/*[ECID:000000] ZTEBSP yaotierui start  20120320, 8x25 bring up  */
	//.pmic_backlight = mipi_truly_set_bl,
	.pmic_backlight = NULL,
/*[ECID:000000] ZTEBSP yaotierui end  20120320, P825A20 bring up */
};

static struct platform_device mipi_dsi_truly_panel_device = {
	.name   = "mipi_truly",
	.id     = 0,
	.dev    = {
		.platform_data = &mipi_truly_pdata,
	}
};

static struct msm_panel_common_pdata mipi_NT35510_pdata = {
	.backlight    = evb_backlight_control,
	.rotate_panel = mipi_NT35510_rotate_panel,
};

static struct platform_device mipi_dsi_NT35510_panel_device = {
	.name = "mipi_NT35510",
	.id   = 0,
	.dev  = {
		.platform_data = &mipi_NT35510_pdata,
	}
};

static struct msm_panel_common_pdata mipi_NT35516_pdata = {
	.backlight = evb_backlight_control,
};

static struct platform_device mipi_dsi_NT35516_panel_device = {
	.name   = "mipi_truly_tft540960_1_e",
	.id     = 0,
	.dev    = {
		.platform_data = &mipi_NT35516_pdata,
	}
};

static struct platform_device *msm_fb_devices[] __initdata = {
	&msm_fb_device,
#ifndef CONFIG_FB_MSM_LCDC_HIMAX_WVGA_PT		
	&lcdc_toshiba_panel_device,
#endif
#ifdef CONFIG_FB_MSM_LCDC_HIMAX_WVGA_PT
	&lcdc_himax_panel_device,
#endif      
#ifdef CONFIG_FB_MSM_MIPI_DSI
	&mipi_dsi_renesas_panel_device,
#endif
#ifdef CONFIG_MSM_V4L2_VIDEO_OVERLAY_DEVICE
	&msm_v4l2_video_overlay_device,
#endif
};

static struct platform_device *qrd_fb_devices[] __initdata = {
	&msm_fb_device,
	&mipi_dsi_truly_panel_device,
};

static struct platform_device *qrd3_fb_devices[] __initdata = {
	&msm_fb_device,
	&lcdc_truly_panel_device,
};

static struct platform_device *evb_fb_devices[] __initdata = {
	&msm_fb_device,
	&mipi_dsi_NT35510_panel_device,
	&mipi_dsi_NT35516_panel_device,
};

void __init msm_msm7627a_allocate_memory_regions(void)
{
	void *addr;
	unsigned long fb_size;

	if (machine_is_msm7625a_surf() || machine_is_msm7625a_ffa())
		fb_size = MSM7x25A_MSM_FB_SIZE;
	else if (machine_is_msm7627a_evb() || machine_is_msm8625_evb()
						|| machine_is_msm8625_evt())
		fb_size = MSM8x25_MSM_FB_SIZE;
	else
		fb_size = MSM_FB_SIZE;

	addr = alloc_bootmem_align(fb_size, 0x1000);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + fb_size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n", fb_size,
						addr, __pa(addr));

#ifdef CONFIG_MSM_V4L2_VIDEO_OVERLAY_DEVICE
	fb_size = MSM_V4L2_VIDEO_OVERLAY_BUF_SIZE;
	addr = alloc_bootmem_align(fb_size, 0x1000);
	msm_v4l2_video_overlay_resources[0].start = __pa(addr);
	msm_v4l2_video_overlay_resources[0].end =
		msm_v4l2_video_overlay_resources[0].start + fb_size - 1;
	pr_debug("allocating %lu bytes at %p (%lx physical) for v4l2\n",
		fb_size, addr, __pa(addr));
#endif

}

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = 97,
	.mdp_rev = MDP_REV_303,
#ifndef CONFIG_ZTE_UART_USE_RGB_LCD_LVDS
	.cont_splash_enabled = 0x1,
#endif
};

static char lcdc_splash_is_enabled()
{
	return mdp_pdata.cont_splash_enabled;
}

/*[ECID:000000] ZTEBSP wangminrong  start 20120419 for A20 or A10 or E01  */
//#define GPIO_LCDC_BRDG_PD	121  //128
#if defined ( CONFIG_PROJECT_P825A10 ) || defined ( CONFIG_PROJECT_P825F01 ) || defined ( CONFIG_PROJECT_P865F01 )  || defined ( CONFIG_PROJECT_P865E02 )|| defined ( CONFIG_PROJECT_P865V30 ) ||defined ( CONFIG_PROJECT_P865V20)

#define GPIO_LCD_BACKLIGHT_EN	96
#define GPIO_LCDC_BRDG_RESET_N	85

#else
#define GPIO_LCD_BACKLIGHT_EN	97
#define GPIO_LCDC_BRDG_RESET_N	129

#endif
/*[ECID:000000] ZTEBSP wangminrong  end 20120419 for A20 or A10 or E01  */
/*[ECID:000000] ZTEBSP yaotierui start  20120320, 8x25 bring up  */
//#define GPIO_LCD_DSI_SEL	125
/*[ECID:000000] ZTEBSP yaotierui end  20120320, P825A20 bring up */
#define LCDC_RESET_PHYS		0x90008014

/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
//#define MDP_303_VSYNC_GPIO 97
#define MDP_303_VSYNC_GPIO 121
/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */
#ifdef CONFIG_FB_MSM_MIPI_DSI
static  void __iomem *lcdc_reset_ptr;
/*[ECID:000000] ZTEBSP yaotierui start  20120320, 8x25 bring up  */
static unsigned mipi_dsi_gpio[] = {
		//GPIO_CFG(GPIO_LCDC_BRDG_RESET_N, 0, GPIO_CFG_OUTPUT,
		//GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* LCDC_BRDG_RESET_N */
		//GPIO_CFG(GPIO_LCDC_BRDG_PD, 0, GPIO_CFG_OUTPUT,
		//GPIO_CFG_NO_PULL, GPIO_CFG_2MA), /* LCDC_BRDG_PD */
	GPIO_CFG(GPIO_LCDC_BRDG_RESET_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
		GPIO_CFG_4MA),       /* LCDC_BRDG_RESET_N */
	GPIO_CFG(MDP_303_VSYNC_GPIO, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
		GPIO_CFG_4MA),       /* LCDC_BRDG_RESET_N */
	GPIO_CFG(GPIO_LCD_BACKLIGHT_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
		GPIO_CFG_2MA),       /* LCDC_BRDG_RESET_N */
};
/*[ECID:000000] ZTEBSP yaotierui end  20120320, P825A20 bring up */
enum {
	DSI_SINGLE_LANE = 1,
	DSI_TWO_LANES,
};

static int msm_fb_get_lane_config(void)
{
	/* For MSM7627A SURF/FFA and QRD */
	int rc = DSI_TWO_LANES;
	if (machine_is_msm7625a_surf() || machine_is_msm7625a_ffa()) {
		rc = DSI_SINGLE_LANE;
		pr_info("DSI_SINGLE_LANES\n");
	} else {
		pr_info("DSI_TWO_LANES\n");
	}
	return rc;
}

static int msm_fb_dsi_client_msm_reset(void)
{
	int rc = 0;

	rc = gpio_request(GPIO_LCDC_BRDG_RESET_N, "lcdc_brdg_reset_n");
	if (rc < 0) {
		pr_err("failed to request lcd brdg reset_n\n");
		return rc;
	}

	rc = gpio_request(MDP_303_VSYNC_GPIO, "lcdc_brdg_pd");
	if (rc < 0) {
		pr_err("failed to request lcd brdg pd\n");
		//return rc;
	}
	
/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
	rc = gpio_request(GPIO_LCD_BACKLIGHT_EN, "lcdc_brdg_pd");
	if (rc < 0) {
		pr_err("failed to request lcd backlight\n");
		return rc;
	}
/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */

	rc = gpio_tlmm_config(mipi_dsi_gpio[0], GPIO_CFG_ENABLE);
	if (rc) {
		pr_err("Failed to enable LCDC Bridge reset enable\n");
		goto gpio_error;
	}

	rc = gpio_tlmm_config(mipi_dsi_gpio[1], GPIO_CFG_ENABLE);
	if (rc) {
		pr_err("Failed to enable LCDC Bridge pd enable\n");
		goto gpio_error2;
	}

/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
	rc = gpio_tlmm_config(mipi_dsi_gpio[2], GPIO_CFG_ENABLE);
	if (rc) {
		pr_err("Failed to enable lcd backlight\n");
		//goto gpio_error2;
	}
/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */

	rc = gpio_direction_output(GPIO_LCDC_BRDG_RESET_N, 1);
/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
	//rc |= gpio_direction_output(GPIO_LCDC_BRDG_PD, 1);
	//gpio_set_value_cansleep(GPIO_LCDC_BRDG_PD, 0);
/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */

	if (!rc) {
		if (machine_is_msm7x27a_surf() || machine_is_msm7625a_surf()
				|| machine_is_msm8625_surf()) {
			lcdc_reset_ptr = ioremap_nocache(LCDC_RESET_PHYS,
				sizeof(uint32_t));

			if (!lcdc_reset_ptr)
				return 0;
		}
		return rc;
	} else {
		goto gpio_error;
	}

gpio_error2:
	pr_err("Failed GPIO bridge pd\n");
	/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
	//gpio_free(GPIO_LCDC_BRDG_PD);
	/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */

gpio_error:
	pr_err("Failed GPIO bridge reset\n");
	gpio_free(GPIO_LCDC_BRDG_RESET_N);
	return rc;
}
#endif
/*[ECID:000000] ZTEBSP yaotierui start  20120320, 8x25 bring up  */
#if 0
static int mipi_truly_sel_mode(int video_mode)
{
	int rc = 0;

	rc = gpio_request(GPIO_LCD_DSI_SEL, "lcd_dsi_sel");
	if (rc < 0)
		goto gpio_error;

	rc = gpio_tlmm_config(lcd_dsi_sel_gpio[0], GPIO_CFG_ENABLE);
	if (rc)
		goto gpio_error;

	rc = gpio_direction_output(GPIO_LCD_DSI_SEL, 1);
	if (!rc) {
		gpio_set_value_cansleep(GPIO_LCD_DSI_SEL, video_mode);
		return rc;
	} else {
		goto gpio_error;
	}

gpio_error:
	pr_err("mipi_truly_sel_mode failed\n");
	gpio_free(GPIO_LCD_DSI_SEL);
	return rc;
}

static int msm_fb_dsi_client_qrd1_reset(void)
{
	int rc = 0;

	rc = gpio_request(GPIO_LCDC_BRDG_RESET_N, "lcdc_brdg_reset_n");
	if (rc < 0) {
		pr_err("failed to request lcd brdg reset_n\n");
		return rc;
	}

	rc = gpio_tlmm_config(mipi_dsi_gpio[0], GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("Failed to enable LCDC Bridge reset enable\n");
		return rc;
	}

	rc = gpio_direction_output(GPIO_LCDC_BRDG_RESET_N, 1);
	if (rc < 0) {
		pr_err("Failed GPIO bridge pd\n");
		gpio_free(GPIO_LCDC_BRDG_RESET_N);
		return rc;
	}

	mipi_truly_sel_mode(1);

	return rc;
}
#endif
/*[ECID:000000] ZTEBSP yaotierui end  20120320, P825A20 bring up */

/*[ECID:000000] ZTEBSP yaotierui start  20120320, 8x25 bring up  */
#if 0
#define GPIO_QRD3_LCD_BRDG_RESET_N	85
#define GPIO_QRD3_LCD_BACKLIGHT_EN	96
#define GPIO_QRD3_LCD_EXT_2V85_EN	35
#define GPIO_QRD3_LCD_EXT_1V8_EN	40

static unsigned qrd3_mipi_dsi_gpio[] = {
	GPIO_CFG(GPIO_QRD3_LCD_BRDG_RESET_N, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL,
			GPIO_CFG_2MA), /* GPIO_QRD3_LCD_BRDG_RESET_N */
	GPIO_CFG(GPIO_QRD3_LCD_BACKLIGHT_EN, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL,
			GPIO_CFG_2MA), /* GPIO_QRD3_LCD_BACKLIGHT_EN */
	GPIO_CFG(GPIO_QRD3_LCD_EXT_2V85_EN, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL,
			GPIO_CFG_2MA), /* GPIO_QRD3_LCD_EXT_2V85_EN */
	GPIO_CFG(GPIO_QRD3_LCD_EXT_1V8_EN, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_NO_PULL,
			GPIO_CFG_2MA), /* GPIO_QRD3_LCD_EXT_1V8_EN */
};

static int msm_fb_dsi_client_qrd3_reset(void)
{
	int rc = 0;

	rc = gpio_request(GPIO_QRD3_LCD_BRDG_RESET_N, "qrd3_lcd_brdg_reset_n");
	if (rc < 0) {
		pr_err("failed to request qrd3 lcd brdg reset_n\n");
		return rc;
	}

	return rc;
}
#endif
/*[ECID:000000] ZTEBSP yaotierui end  20120320, P825A20 bring up */
#ifdef CONFIG_FB_MSM_MIPI_DSI
static int msm_fb_dsi_client_reset(void)
{
	int rc = 0;

/*[ECID:000000] ZTEBSP yaotierui start  20120320, 8x25 bring up  */
#if 0
	if (machine_is_msm7627a_qrd1())
		rc = msm_fb_dsi_client_qrd1_reset();
	else if (machine_is_msm7627a_evb() || machine_is_msm8625_evb()
						|| machine_is_msm8625_evt())
		rc = msm_fb_dsi_client_qrd3_reset();
	else
#endif
/*[ECID:000000] ZTEBSP yaotierui end  20120320, P825A20 bring up */
		rc = msm_fb_dsi_client_msm_reset();

	return rc;

}

static struct regulator_bulk_data regs_dsi[] = {
	/*[ECID:000000] ZTEBSP yaotierui start  20120320, 8x25 bring up  */
/*[ECID:000000] ZTEBSP wangbing, for P825A10 LCD, 20120420 start*/	
#if defined ( CONFIG_PROJECT_P825A10 ) || defined ( CONFIG_PROJECT_P825F01 ) || defined ( CONFIG_PROJECT_P865F01 ) || defined ( CONFIG_PROJECT_P865E02 )|| defined ( CONFIG_PROJECT_P865V30 ) ||defined ( CONFIG_PROJECT_P865V20)
	{ .supply = "gp2",   .min_uV = 2850000, .max_uV = 2850000 },	
#else
	{ .supply = "bt",   .min_uV = 2900000, .max_uV = 2900000 },//[ECID:000000] ZTEBSP wangminrong for lcd the same to sensor 20120511
	//{ .supply = "gp2",   .min_uV = 2850000, .max_uV = 2850000 },
	//{ .supply = "msme1", .min_uV = 1800000, .max_uV = 1800000 },
#endif	
/*[ECID:000000] ZTEBSP wangbing, for P825A10 LCD, 20120420 end*/
	/*[ECID:000000] ZTEBSP yaotierui end  20120320, P825A20 bring up */
};

static int dsi_gpio_initialized;
#endif
/*[ECID:000000] ZTEBSP wangbing, for P825A10 LCD, 20120420 */	
#if defined ( CONFIG_PROJECT_P825A10 ) || defined ( CONFIG_PROJECT_P825F01 ) || defined ( CONFIG_PROJECT_P865F01 ) || defined ( CONFIG_PROJECT_P865V30 ) ||defined ( CONFIG_PROJECT_P865V20)
#define GPIO_LCD_1V8 5
#endif
#ifdef CONFIG_FB_MSM_MIPI_DSI
static int mipi_dsi_panel_msm_power(int on)
{
	int rc = 0;
	uint32_t lcdc_reset_cfg;

	/* I2C-controlled GPIO Expander -init of the GPIOs very late */
	if (unlikely(!dsi_gpio_initialized)) {
		/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
		#if 0
		pmapp_disp_backlight_init();

		rc = gpio_request(GPIO_DISPLAY_PWR_EN, "gpio_disp_pwr");
		if (rc < 0) {
			pr_err("failed to request gpio_disp_pwr\n");
			return rc;
		}

		if (machine_is_msm7x27a_surf() || machine_is_msm7625a_surf()
				|| machine_is_msm8625_surf()) {
			rc = gpio_direction_output(GPIO_DISPLAY_PWR_EN, 1);
			if (rc < 0) {
				pr_err("failed to enable display pwr\n");
				goto fail_gpio1;
			}

			rc = gpio_request(GPIO_BACKLIGHT_EN, "gpio_bkl_en");
			if (rc < 0) {
				pr_err("failed to request gpio_bkl_en\n");
				goto fail_gpio1;
			}

			rc = gpio_direction_output(GPIO_BACKLIGHT_EN, 1);
			if (rc < 0) {
				pr_err("failed to enable backlight\n");
				goto fail_gpio2;
			}
		}
		#endif
              /*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */
              
/*[ECID:000000] ZTEBSP wangbing, for P825A10 LCD, 20120420 start*/	
#if defined ( CONFIG_PROJECT_P825A10 ) || defined ( CONFIG_PROJECT_P825F01 ) || defined ( CONFIG_PROJECT_P865F01 ) || defined ( CONFIG_PROJECT_P865V30 ) ||defined ( CONFIG_PROJECT_P865V20)
		rc = gpio_request(GPIO_LCD_1V8, "gpio_lcd_1v8");
		if (rc < 0) {
			pr_err("failed to request gpio_lcd_1v8\n");
			return rc;
		}

#if 0
	rc = gpio_tlmm_config(GPIO_CFG(GPIO_LCD_1V8, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to enable lcd_camera_ldo_2v8!\n", __func__);
		goto fail_gpio1;
	}	
#endif
		
		rc = gpio_direction_output(GPIO_LCD_1V8, 1);
		if (rc < 0) {
			pr_err("failed to enable display pwr\n");
			goto fail_gpio1;
		}		              
#endif
/*[ECID:000000] ZTEBSP wangbing, for P825A10 LCD, 20120420 end*/	
		
		rc = regulator_bulk_get(NULL, ARRAY_SIZE(regs_dsi), regs_dsi);
		if (rc) {
			pr_err("%s: could not get regulators: %d\n",
					__func__, rc);
			goto fail_gpio2;
		}

		rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs_dsi),
						regs_dsi);
		if (rc) {
			pr_err("%s: could not set voltages: %d\n",
					__func__, rc);
			goto fail_vreg;
		}
		/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
		#if 0
		if (pmapp_disp_backlight_set_brightness(100))
			pr_err("backlight set brightness failed\n");
		#endif
		/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */

		dsi_gpio_initialized = 1;
	}

	/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
	#if 0
	if (machine_is_msm7x27a_surf() || machine_is_msm7625a_surf() ||
			machine_is_msm8625_surf()) {
		gpio_set_value_cansleep(GPIO_DISPLAY_PWR_EN, on);
		gpio_set_value_cansleep(GPIO_BACKLIGHT_EN, on);
	} else if (machine_is_msm7x27a_ffa() || machine_is_msm7625a_ffa()
					|| machine_is_msm8625_ffa()) {
		if (on) {
			/* This line drives an active low pin on FFA */
			rc = gpio_direction_output(GPIO_DISPLAY_PWR_EN, !on);
			if (rc < 0)
				pr_err("failed to set direction for "
					"display pwr\n");
		} else {
			gpio_set_value_cansleep(GPIO_DISPLAY_PWR_EN, !on);
			rc = gpio_direction_input(GPIO_DISPLAY_PWR_EN);
			if (rc < 0)
				pr_err("failed to set direction for "
					"display pwr\n");
		}
	}
	#endif
	/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */

	if (on) {
		/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
		//gpio_set_value_cansleep(GPIO_LCDC_BRDG_PD, 0);
		/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */

		if (machine_is_msm7x27a_surf() ||
				 machine_is_msm7625a_surf() ||
				 machine_is_msm8625_surf()) {
			lcdc_reset_cfg = readl_relaxed(lcdc_reset_ptr);
			rmb();
			lcdc_reset_cfg &= ~1;

			writel_relaxed(lcdc_reset_cfg, lcdc_reset_ptr);
			msleep(20);
			wmb();
			lcdc_reset_cfg |= 1;
			writel_relaxed(lcdc_reset_cfg, lcdc_reset_ptr);
		} else {
			gpio_set_value_cansleep(GPIO_LCDC_BRDG_RESET_N, 0);
			msleep(20);
			gpio_set_value_cansleep(GPIO_LCDC_BRDG_RESET_N, 1);
		}
	} else {
	/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
		//gpio_set_value_cansleep(GPIO_LCDC_BRDG_PD, 1);
	/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */
	}

/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
/* sensor && lcd used together, do not to shut it down alone.we need to check it later!
*/
#if 0
	rc = on ? regulator_bulk_enable(ARRAY_SIZE(regs_dsi), regs_dsi) :
		  regulator_bulk_disable(ARRAY_SIZE(regs_dsi), regs_dsi);
#else
	rc = on ? regulator_bulk_enable(ARRAY_SIZE(regs_dsi), regs_dsi) :
		  0;
#endif
/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */

	if (rc)
		pr_err("%s: could not %sable regulators: %d\n",
				__func__, on ? "en" : "dis", rc);

	return rc;

/*[ECID:000000] ZTEBSP wangbing, for P825A10 LCD, 20120420 */		
#if defined ( CONFIG_PROJECT_P825A10 ) || defined ( CONFIG_PROJECT_P825F01 ) || defined ( CONFIG_PROJECT_P865F01 ) || defined ( CONFIG_PROJECT_P865V30 ) ||defined ( CONFIG_PROJECT_P865V20)
	gpio_set_value_cansleep(GPIO_LCD_1V8, 1);	
#endif	
	
fail_vreg:
	regulator_bulk_free(ARRAY_SIZE(regs_dsi), regs_dsi);
fail_gpio2:
/*[ECID:000000] ZTEBSP yaotierui start  20120319, 8x25 bring up  */
#if 0
	gpio_free(GPIO_BACKLIGHT_EN);
fail_gpio1:
	gpio_free(GPIO_DISPLAY_PWR_EN);
#endif
/*[ECID:000000] ZTEBSP yaotierui end  20120319, P825A20 bring up */

/*[ECID:000000] ZTEBSP wangbing, for P825A10 LCD, 20120420 */	
#if defined ( CONFIG_PROJECT_P825A10 ) || defined ( CONFIG_PROJECT_P825F01 ) || defined ( CONFIG_PROJECT_P865F01 ) || defined ( CONFIG_PROJECT_P865V30 ) ||defined ( CONFIG_PROJECT_P865V20)
fail_gpio1:
		gpio_free(GPIO_LCD_1V8);
#endif

	dsi_gpio_initialized = 0;
	return rc;
}
#endif
/*[ECID:000000] ZTEBSP yaotierui start  20120320, 8x25 bring up  */
#if 0
static int mipi_dsi_panel_qrd1_power(int on)
{
	int rc = 0;

	if (!dsi_gpio_initialized) {
		rc = gpio_request(QRD_GPIO_BACKLIGHT_EN, "gpio_bkl_en");
		if (rc < 0)
			return rc;

		rc = gpio_tlmm_config(GPIO_CFG(QRD_GPIO_BACKLIGHT_EN, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
		if (rc < 0) {
			pr_err("failed GPIO_BACKLIGHT_EN tlmm config\n");
			return rc;
		}

		rc = gpio_direction_output(QRD_GPIO_BACKLIGHT_EN, 1);
		if (rc < 0) {
			pr_err("failed to enable backlight\n");
			gpio_free(QRD_GPIO_BACKLIGHT_EN);
			return rc;
		}
		dsi_gpio_initialized = 1;
	}

	gpio_set_value_cansleep(QRD_GPIO_BACKLIGHT_EN, !!on);

	if (on) {
		gpio_set_value_cansleep(GPIO_LCDC_BRDG_RESET_N, 1);
		msleep(20);
		gpio_set_value_cansleep(GPIO_LCDC_BRDG_RESET_N, 0);
		msleep(20);
		gpio_set_value_cansleep(GPIO_LCDC_BRDG_RESET_N, 1);

	}

	return rc;
}
 
static int qrd3_dsi_gpio_initialized;
#endif //wanghaifei for JB bring up
static struct regulator *gpio_reg_2p85v, *gpio_reg_1p8v;
#if 0 //wanghaifei for JB bring up
static int mipi_dsi_panel_qrd3_power(int on)
{
	int rc = 0;

	if (!qrd3_dsi_gpio_initialized) {
		pmapp_disp_backlight_init();
		rc = gpio_request(GPIO_QRD3_LCD_BACKLIGHT_EN,
			"qrd3_gpio_bkl_en");
		if (rc < 0)
			return rc;

		qrd3_dsi_gpio_initialized = 1;

		if (mdp_pdata.cont_splash_enabled) {
			rc = gpio_tlmm_config(GPIO_CFG(
			     GPIO_QRD3_LCD_BACKLIGHT_EN, 0, GPIO_CFG_OUTPUT,
			     GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			if (rc < 0) {
				pr_err("failed QRD3 GPIO_BACKLIGHT_EN tlmm config\n");
				return rc;
			}
			rc = gpio_direction_output(GPIO_QRD3_LCD_BACKLIGHT_EN,
			     1);
			if (rc < 0) {
				pr_err("failed to enable backlight\n");
				gpio_free(GPIO_QRD3_LCD_BACKLIGHT_EN);
				return rc;
			}

			/*Configure LCD Bridge reset*/
			rc = gpio_tlmm_config(qrd3_mipi_dsi_gpio[0],
			     GPIO_CFG_ENABLE);
			if (rc < 0) {
				pr_err("Failed to enable LCD Bridge reset enable\n");
				return rc;
			}

			rc = gpio_direction_output(GPIO_QRD3_LCD_BRDG_RESET_N,
			     1);

			if (rc < 0) {
				pr_err("Failed GPIO bridge Reset\n");
				gpio_free(GPIO_QRD3_LCD_BRDG_RESET_N);
				return rc;
			}
			return 0;
		}
	}

	if (on) {
		rc = gpio_tlmm_config(GPIO_CFG(GPIO_QRD3_LCD_BACKLIGHT_EN, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
		if (rc < 0) {
			pr_err("failed QRD3 GPIO_BACKLIGHT_EN tlmm config\n");
			return rc;
		}
		rc = gpio_direction_output(GPIO_QRD3_LCD_BACKLIGHT_EN, 1);
		if (rc < 0) {
			pr_err("failed to enable backlight\n");
			gpio_free(GPIO_QRD3_LCD_BACKLIGHT_EN);
			return rc;
		}
		/*Toggle Backlight GPIO*/
		gpio_set_value_cansleep(GPIO_QRD3_LCD_BACKLIGHT_EN, 1);
		udelay(190);
		gpio_set_value_cansleep(GPIO_QRD3_LCD_BACKLIGHT_EN, 0);
		udelay(286);
		gpio_set_value_cansleep(GPIO_QRD3_LCD_BACKLIGHT_EN, 1);
		/* 1 wire mode starts from this low to high transition */
		udelay(50);

		/*Enable EXT_2.85 and 1.8 regulators*/
		rc = regulator_enable(gpio_reg_2p85v);
		if (rc < 0)
			pr_err("%s: reg enable failed\n", __func__);
		rc = regulator_enable(gpio_reg_1p8v);
		if (rc < 0)
			pr_err("%s: reg enable failed\n", __func__);

		/*Configure LCD Bridge reset*/
		rc = gpio_tlmm_config(qrd3_mipi_dsi_gpio[0], GPIO_CFG_ENABLE);
		if (rc < 0) {
			pr_err("Failed to enable LCD Bridge reset enable\n");
			return rc;
		}

		rc = gpio_direction_output(GPIO_QRD3_LCD_BRDG_RESET_N, 1);

		if (rc < 0) {
			pr_err("Failed GPIO bridge Reset\n");
			gpio_free(GPIO_QRD3_LCD_BRDG_RESET_N);
			return rc;
		}

		/*Toggle Bridge Reset GPIO*/
		msleep(20);
		gpio_set_value_cansleep(GPIO_QRD3_LCD_BRDG_RESET_N, 0);
		msleep(20);
		gpio_set_value_cansleep(GPIO_QRD3_LCD_BRDG_RESET_N, 1);
		msleep(20);

	} else {
		gpio_tlmm_config(GPIO_CFG(GPIO_QRD3_LCD_BACKLIGHT_EN, 0,
			GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
			GPIO_CFG_DISABLE);

		gpio_tlmm_config(GPIO_CFG(GPIO_QRD3_LCD_BRDG_RESET_N, 0,
			GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_DISABLE);

		rc = regulator_disable(gpio_reg_2p85v);
		if (rc < 0)
			pr_err("%s: reg disable failed\n", __func__);
		rc = regulator_disable(gpio_reg_1p8v);
		if (rc < 0)
			pr_err("%s: reg disable failed\n", __func__);

	}

	return rc;
}
#endif
/*[ECID:000000] ZTEBSP yaotierui end  20120320, P825A20 bring up */
#ifdef CONFIG_FB_MSM_MIPI_DSI
static char mipi_dsi_splash_is_enabled(void);
static int mipi_dsi_panel_power(int on)
{
	int rc = 0;

/*[ECID:000000] ZTEBSP yaotierui start  20120320, 8x25 bring up  */
#if 0
	if (machine_is_msm7627a_qrd1())
		rc = mipi_dsi_panel_qrd1_power(on);
	else if (machine_is_msm7627a_evb() || machine_is_msm8625_evb()
						|| machine_is_msm8625_evt())
		rc = mipi_dsi_panel_qrd3_power(on);
	else
#endif
/*[ECID:000000] ZTEBSP yaotierui end  20120320, P825A20 bring up */
		rc = mipi_dsi_panel_msm_power(on);
	return rc;
}
#endif

#ifdef CONFIG_FB_MSM_MIPI_DSI
static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio		= MDP_303_VSYNC_GPIO,
	.dsi_power_save		= mipi_dsi_panel_power,
	.dsi_client_reset       = msm_fb_dsi_client_reset,
	.get_lane_config	= msm_fb_get_lane_config,
	.splash_is_enabled	= mipi_dsi_splash_is_enabled,
};
#endif
#ifdef CONFIG_FB_MSM_MIPI_DSI
static char mipi_dsi_splash_is_enabled(void)
{
	return mdp_pdata.cont_splash_enabled;
}
#endif
static char prim_panel_name[PANEL_NAME_MAX_LEN];
static int __init prim_display_setup(char *param)
{
	if (strnlen(param, PANEL_NAME_MAX_LEN))
		strlcpy(prim_panel_name, param, PANEL_NAME_MAX_LEN);
	return 0;
}
early_param("prim_display", prim_display_setup);

static int disable_splash;

void msm7x27a_set_display_params(char *prim_panel)
{
	if (strnlen(prim_panel, PANEL_NAME_MAX_LEN)) {
		strlcpy(msm_fb_pdata.prim_panel_name, prim_panel,
			PANEL_NAME_MAX_LEN);
		pr_debug("msm_fb_pdata.prim_panel_name %s\n",
			msm_fb_pdata.prim_panel_name);
	}
	if (strnlen(msm_fb_pdata.prim_panel_name, PANEL_NAME_MAX_LEN)) {
		if (strncmp((char *)msm_fb_pdata.prim_panel_name,
			"mipi_cmd_nt35510_wvga",
			strnlen("mipi_cmd_nt35510_wvga",
				PANEL_NAME_MAX_LEN)) &&
		    strncmp((char *)msm_fb_pdata.prim_panel_name,
			"mipi_video_nt35510_wvga",
			strnlen("mipi_video_nt35510_wvga",
				PANEL_NAME_MAX_LEN)))
			disable_splash = 1;
	}
}

void __init msm_fb_add_devices(void)
{
	int rc = 0;
	msm7x27a_set_display_params(prim_panel_name);
	if (machine_is_msm7627a_qrd1())
		platform_add_devices(qrd_fb_devices,
				ARRAY_SIZE(qrd_fb_devices));
	else if (machine_is_msm7627a_evb() || machine_is_msm8625_evb()
						|| machine_is_msm8625_evt()) {
		mipi_NT35510_pdata.bl_lock = 1;
		mipi_NT35516_pdata.bl_lock = 1;
		if (disable_splash)
			mdp_pdata.cont_splash_enabled = 0x0;


		platform_add_devices(evb_fb_devices,
				ARRAY_SIZE(evb_fb_devices));
	} else if (machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7()) {
		sku3_lcdc_lcd_camera_power_init();
		mdp_pdata.cont_splash_enabled = 0x0;
		platform_add_devices(qrd3_fb_devices,
						ARRAY_SIZE(qrd3_fb_devices));
	} else {
		printk("wangminrong 3333\r\n");
#ifndef CONFIG_ZTE_UART_USE_RGB_LCD_LVDS
		mdp_pdata.cont_splash_enabled = 0x1;
#endif
		platform_add_devices(msm_fb_devices,
				ARRAY_SIZE(msm_fb_devices));
	}

	msm_fb_register_device("mdp", &mdp_pdata);
	if (machine_is_msm7625a_surf() || machine_is_msm7x27a_surf() ||
			machine_is_msm8625_surf() || machine_is_msm7627a_qrd3()
			|| machine_is_msm8625_qrd7())
		msm_fb_register_device("lcdc", &lcdc_pdata);
#ifdef CONFIG_FB_MSM_MIPI_DSI
	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
#endif
	if (machine_is_msm7627a_evb() || machine_is_msm8625_evb()
					|| machine_is_msm8625_evt()) {
		gpio_reg_2p85v = regulator_get(&mipi_dsi_device.dev,
								"lcd_vdd");
		if (IS_ERR(gpio_reg_2p85v))
			pr_err("%s:ext_2p85v regulator get failed", __func__);

		gpio_reg_1p8v = regulator_get(&mipi_dsi_device.dev,
								"lcd_vddi");
		if (IS_ERR(gpio_reg_1p8v))
			pr_err("%s:ext_1p8v regulator get failed", __func__);

		if (mdp_pdata.cont_splash_enabled) {
			/*Enable EXT_2.85 and 1.8 regulators*/
			rc = regulator_enable(gpio_reg_2p85v);
			if (rc < 0)
				pr_err("%s: reg enable failed\n", __func__);
			rc = regulator_enable(gpio_reg_1p8v);
			if (rc < 0)
				pr_err("%s: reg enable failed\n", __func__);
		}
	}
}
