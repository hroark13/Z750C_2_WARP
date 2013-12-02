/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio_event.h>
#include <linux/memblock.h>
#include <asm/mach-types.h>
#include <linux/memblock.h>
#include <asm/mach/arch.h>
#include <asm/hardware/gic.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/msm_hsusb.h>
#include <mach/rpc_hsusb.h>
#include <mach/rpc_pmapp.h>
#include <mach/usbdiag.h>
#include <mach/msm_memtypes.h>
#include <mach/msm_serial_hs.h>
#include <linux/usb/android.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <mach/vreg.h>
#include <mach/pmic.h>
#include <mach/socinfo.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <asm/mach/mmc.h>
#include <linux/i2c.h>
#include <linux/i2c/sx150x.h>
#include <linux/gpio.h>
#include <linux/android_pmem.h>
#include <linux/bootmem.h>
#include <linux/mfd/marimba.h>
#include <mach/vreg.h>
#include <linux/power_supply.h>
#include <linux/regulator/consumer.h>
#include <mach/rpc_pmapp.h>
#include <mach/msm_battery.h>
#include <linux/smsc911x.h>
#include <linux/atmel_maxtouch.h>
#include <linux/fmem.h>
#include <linux/msm_adc.h>
#include <linux/ion.h>
#include "devices.h"
#include "timer.h"
#include "board-msm7x27a-regulator.h"
#include "devices-msm7x2xa.h"
#include "pm.h"
#include <mach/rpc_server_handset.h>
#include <mach/socinfo.h>
#include "pm-boot.h"
#include "board-msm7627a.h"
/*ZTEBSP yuanjinxing 20121130, max17040 driver, include max17040_battery.h, start */
#ifdef CONFIG_BATTERY_MAX17040
#include <linux/max17040_battery.h>
#endif
/*ZTEBSP yuanjinxing 20121130, max17040 driver, include max17040_battery.h, end */

//[ECID:000000] ZTEBSP wanghaifei start 20120330, add for sensor
#include <linux/mpu.h>
#include <linux/taos_common.h>
#include <linux/kxtik.h>
#include <linux/akm8963.h>
#include <linux/lis3dh.h>
#include <linux/l3g4200d.h>
#include <linux/kionix_accel.h>
//[ECID:000000] ZTEBSP wanghaifei end 20120330, add for sensor
//added by yangze for SX9502 sensor test 20121015 start
//#include <mach/attiny44a.h>
#include <linux/input/smtc/misc/sx9500_platform_data.h>
#include <linux/input/smtc/misc/sx95xx_i2c_reg.h>
#include <linux/input/smtc/misc/smtc_sar_test_platform_data.h>
#include <linux/input.h>
//added by yangze for SX9502 sensor test 20121015 end
#include <linux/persistent_ram.h>

/*[ECID:000000] ZTEBSP zhangbo add for nfc, start*/
#ifdef CONFIG_PN544_NFC
#include <linux/nfc/pn544.h>
#endif
/*[ECID:000000] ZTEBSP zhangbo add for nfc, end*/

#define PMEM_KERNEL_EBI1_SIZE	0x3A000
#define MSM_PMEM_AUDIO_SIZE	0x1F4000
//zte-modify,zhangbo,20120709,add ram_console func,begin
#ifdef CONFIG_ANDROID_RAM_CONSOLE
  #ifdef CONFIG_ANDROID_1G_RAM_SIZE // 1G RAM
    #define MSM_RAM_CONSOLE_PHYS   0x3FF05000
    #define MSM_RAM_CONSOLE_SIZE  0XFB000  
  #else //512M RAM
    #define MSM_RAM_CONSOLE_PHYS   0x2FF05000
    #define MSM_RAM_CONSOLE_SIZE  0XFB000
  #endif
#endif
//zte-modify,zhangbo,20120709,add ram_console func,eng

extern int zte_get_hw_board_id(void);// added by yangze 20120925

#if defined(CONFIG_GPIO_SX150X)
enum {
	SX150X_CORE,
};

static struct sx150x_platform_data sx150x_data[] __initdata = {
	[SX150X_CORE]	= {
		.gpio_base		= GPIO_CORE_EXPANDER_BASE,
		.oscio_is_gpo		= false,
		.io_pullup_ena		= 0,
		.io_pulldn_ena		= 0x02,
		.io_open_drain_ena	= 0xfef8,
		.irq_summary		= -1,
	},
};
#endif

/*[ECID:000000]  fanjiankang removed wlan register in board begin*/
#if 0
static struct platform_device msm_wlan_ar6000_pm_device = {
	.name           = "wlan_ar6000_pm_dev",
	.id             = -1,
};
#endif
/*[ECID:000000] ZTEBSP fanjiankang removed wlan register in board begin*/

//[ECID:0000]ZTE_BSP maxiaoping 20120330 add leds feature,start.
#ifdef CONFIG_LEDS_MSM_PMIC// for enabling CONFIG_LEDS_MSM_PMIC 
static struct platform_device msm_device_pmic_leds = {
	.name   = "pmic-leds",
	.id = -1,
};
#endif
//[ECID:0000]ZTE_BSP maxiaoping 20120330 add leds feature,end.

//[ECID:0000]ZTE_BSP maxiaoping 20121205 for button led driver,start.
#ifndef CONFIG_LEDS_NO_KB_GPIO
static struct gpio_led android_led_list[] = {
	{
		.name = "button-backlight",
		#if defined(CONFIG_PROJECT_P865E02)	
		.gpio = 116,
		#elif (defined(CONFIG_PROJECT_P865F03) || defined(CONFIG_PROJECT_P865V30) ||defined ( CONFIG_PROJECT_P865V20))
		.gpio = 34,		
		#else
		.gpio = 85,
		#endif
	},
};

static struct gpio_led_platform_data android_leds_data = {
	.num_leds	= ARRAY_SIZE(android_led_list),
	.leds		= android_led_list,
};

static struct platform_device android_leds = {
	.name		= "leds-gpio",
	.id		= -1,
	.dev		= {
		.platform_data = &android_leds_data,
	},
};
#endif
//[ECID:0000]ZTE_BSP maxiaoping 20121205 for button led driver,end.

#if defined(CONFIG_I2C) && defined(CONFIG_GPIO_SX150X)
/*[ECID:000000] ZTEBSP zhangbo comment it for conflicting with touch screen, start*/
#ifdef USE_QUALCOMM_ORIGINAL_CODE
static struct i2c_board_info core_exp_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("sx1509q", 0x3e),
	},
};
#endif /*USE_QUALCOMM_ORIGINAL_CODE*/
/*[ECID:000000] ZTEBSP zhangbo comment it for conflicting with touch screen, end*/

/* ZTEBSP yuanjinxing 20121130, max17040 driver, add max17040 device data, start */
#ifdef CONFIG_BATTERY_MAX17040
static struct max17040_platform_data max17040_data  = {
	.ini_data = {
		.title = "607_2_052511",
		.emptyadjustment = 0,
		.fulladjustment = 100,
		.rcomp0 = 109,
		.tempcoup = -2010,	/* should div 100, real is -20.1 */
		.tempcodown = -550,	/* should div 100, real is -5.5 */
		.ocvtest = 55888,
		.socchecka = 246,
		.soccheckb = 248,
		.bits = 19,
		.data = {
			0x57, 0x73, 0x94, 0xBA, 0xC4, 0xCE, 0xDA, 0xDF,
			0xE6, 0xF0, 0x06, 0x1D, 0x56, 0x91, 0xCB, 0x05,
			0xE7, 0x00, 0xF1, 0x08, 0x01, 0x0F, 0x11, 0x0D,
			0x51, 0x05, 0x91, 0x0C, 0xC1, 0x0C, 0x01, 0x0C,
			0xA5, 0x70, 0xB7, 0x30, 0xB9, 0x40, 0xBB, 0xA0,
			0xBC, 0x40, 0xBC, 0xE0, 0xBD, 0xA0, 0xBD, 0xF0,
			0xBE, 0x60, 0xBF, 0x00, 0xC0, 0x60, 0xC1, 0xD0,
			0xC5, 0x60, 0xC9, 0x10, 0xCC, 0xB0, 0xD0, 0x50,
			0x01, 0xF0, 0x24, 0xF0, 0x19, 0x20, 0x50, 0x10,
			0x46, 0x20, 0x62, 0x90, 0x00, 0x30, 0x02, 0x90,
			0x72, 0x00, 0x1B, 0x80, 0x1A, 0xF0, 0x19, 0xD0,
			0x17, 0x50, 0x12, 0xC0, 0x12, 0xC0, 0x12, 0xC0,
			0x50, 0x0F, 0x72, 0x0F, 0x91, 0x02, 0xB5, 0x01,
			0xC4, 0x02, 0xC6, 0x09, 0xD0, 0x03, 0xD0, 0x09,
			0x1F, 0x4F, 0x92, 0x01, 0x62, 0x29, 0x03, 0x29,
			0x20, 0xB8, 0xAF, 0x9D, 0x75, 0x2C, 0x2C, 0x2C}
	},
};
static struct i2c_board_info max17040_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("max17040", 0x36),
		.platform_data = &max17040_data,
	},
};
#endif
/* ZTEBSP yuanjinxing 20121130, max17040 driver, add max17040 device data, end */

static void __init register_i2c_devices(void)
{
	if (machine_is_msm7x27a_surf() || machine_is_msm7625a_surf() ||
			machine_is_msm8625_surf())
		sx150x_data[SX150X_CORE].io_open_drain_ena = 0xe0f0;
		
	/*[ECID:000000] ZTEBSP zhangbo comment it for conflicting with touch screen, start*/
	#ifdef USE_QUALCOMM_ORIGINAL_CODE
	core_exp_i2c_info[0].platform_data =
			&sx150x_data[SX150X_CORE];

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				core_exp_i2c_info,
				ARRAY_SIZE(core_exp_i2c_info));
	#endif /*USE_QUALCOMM_ORIGINAL_CODE*/
	/*[ECID:000000] ZTEBSP zhangbo comment it for conflicting with touch screen, end*/

    /* ZTEBSP yuanjinxing 20121130, max17040 driver, register max17040 i2c device, start */
	#ifdef CONFIG_BATTERY_MAX17040
    i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				max17040_i2c_info,
				ARRAY_SIZE(max17040_i2c_info));
	#endif
	/* ZTEBSP yuanjinxing 20121130, max17040 driver, register max17040 i2c device, end */
}
#endif

static struct msm_gpio qup_i2c_gpios_io[] = {
/*ECID:0000 2012-6-18 zhangzhao modified the I2C frequency start*/
	{ GPIO_CFG(60, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
		"qup_scl" },
	{ GPIO_CFG(61, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
		"qup_sda" },
	{ GPIO_CFG(131, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
		"qup_scl" },
	{ GPIO_CFG(132, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
		"qup_sda" },
};

static struct msm_gpio qup_i2c_gpios_hw[] = {
	{ GPIO_CFG(60, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
		"qup_scl" },
	{ GPIO_CFG(61, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
		"qup_sda" },
	{ GPIO_CFG(131, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
		"qup_scl" },
	{ GPIO_CFG(132, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),
		"qup_sda" },
};
/*ECID:0000 2012-6-18 zhangzhao modified the I2C frequency end*/


//zte-modify,zhangbo,20120709,add ram_console func,begin
#ifdef CONFIG_ANDROID_RAM_CONSOLE
static struct platform_device ram_console_device = {
	.name = "ram_console",
	.id = -1,
};

struct persistent_ram_descriptor ram_console_desc = {
	.name = "ram_console",
	.size = MSM_RAM_CONSOLE_SIZE,
};

struct persistent_ram ram_console_ram = {
	.start = MSM_RAM_CONSOLE_PHYS,
	.size = MSM_RAM_CONSOLE_SIZE,
	.num_descs = 1,
	.descs = &ram_console_desc,
};
#endif
//zte-modify,zhangbo,20120709,add ram_console func,end


static void gsbi_qup_i2c_gpio_config(int adap_id, int config_type)
{
	int rc;

	if (adap_id < 0 || adap_id > 1)
		return;

	/* Each adapter gets 2 lines from the table */
	if (config_type)
		rc = msm_gpios_request_enable(&qup_i2c_gpios_hw[adap_id*2], 2);
	else
		rc = msm_gpios_request_enable(&qup_i2c_gpios_io[adap_id*2], 2);
	if (rc < 0)
		pr_err("QUP GPIO request/enable failed: %d\n", rc);
}
#if defined(CONFIG_PROJECT_P865E01)||defined(CONFIG_PROJECT_P865E02)||defined(CONFIG_PROJECT_P825B20) //zhangzhao 2012-12-6 optimize the camera start up time for B20
static struct msm_i2c_platform_data msm_gsbi0_qup_i2c_pdata = {
	.clk_freq		= 200000,/*ECID:0000 2012-6-18 zhangzhao modified the I2C frequency start*/
	.msm_i2c_config_gpio	= gsbi_qup_i2c_gpio_config,
};
#else
static struct msm_i2c_platform_data msm_gsbi0_qup_i2c_pdata = {
	.clk_freq		= 100000,/*ECID:0000 2012-6-18 zhangzhao modified the I2C frequency start*/
	.msm_i2c_config_gpio	= gsbi_qup_i2c_gpio_config,
};
#endif
static struct msm_i2c_platform_data msm_gsbi1_qup_i2c_pdata = {
	.clk_freq		= 100000,
	.msm_i2c_config_gpio	= gsbi_qup_i2c_gpio_config,
};

#ifdef CONFIG_ARCH_MSM7X27A
#define MSM_PMEM_MDP_SIZE       0x2300000
#define MSM7x25A_MSM_PMEM_MDP_SIZE       0x1500000

#define MSM_PMEM_ADSP_SIZE      0x1200000
#define MSM7x25A_MSM_PMEM_ADSP_SIZE      0xB91000
#define CAMERA_ZSL_SIZE		(SZ_1M * 60)
#endif

#ifdef CONFIG_ION_MSM
#define MSM_ION_HEAP_NUM        4
static struct platform_device ion_dev;
static int msm_ion_camera_size;
static int msm_ion_audio_size;
static int msm_ion_sf_size;
#endif


static struct android_usb_platform_data android_usb_pdata = {
	.update_pid_and_serial_num = usb_diag_update_pid_and_serial_num,
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &android_usb_pdata,
	},
};

#ifdef CONFIG_USB_EHCI_MSM_72K
static void msm_hsusb_vbus_power(unsigned phy_info, int on)
{
	int rc = 0;
	unsigned gpio;
/*[ECID:000000] ZTEBSP weiguohua change for zte otg, start 20121210*/
#if defined(CONFIG_PROJECT_V72A)	
	static int first_time = 0;
		pr_err("msm_hsusb_vbus_power config GPIO  %d \n", on);	
	if(first_time == 0)
	{
		rc = gpio_request(ZTE_CHARGE_CEN, "charge_en");
	if (rc < 0) {
		pr_err("failed to request %d GPIO\n", ZTE_CHARGE_CEN);
		return;
	}
	first_time =1;
	}
        rc = gpio_tlmm_config(GPIO_CFG(ZTE_CHARGE_CEN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
        if (rc < 0) {
                pr_err("%s: gpio_direction_input light_sensor_irq failed %d\n", __func__, rc);
        }	
	gpio_direction_output(ZTE_CHARGE_CEN, on);	
	gpio_set_value_cansleep(ZTE_CHARGE_CEN, on);	
#endif
/*[ECID:000000] ZTEBSP weiguohua change for zte otg, end 20121210*/
/*[ECID:000000] ZTEBSP weiguohua change for zte otg, start 20121206*/
#if defined(CONFIG_PROJECT_V72A)
      gpio = ZTE_OTG_LDO_EN;
#else
     gpio = GPIO_HOST_VBUS_EN;
#endif
/*[ECID:000000] ZTEBSP weiguohua change for zte otg, end 20121206*/
	rc = gpio_request(gpio, "i2c_host_vbus_en");
	if (rc < 0) {
		pr_err("failed to request %d GPIO\n", gpio);
		return;
	}
	gpio_direction_output(gpio, !!on);
	gpio_set_value_cansleep(gpio, !!on);
	gpio_free(gpio);
}

static struct msm_usb_host_platform_data msm_usb_host_pdata = {
	.phy_info       = (USB_PHY_INTEGRATED | USB_PHY_MODEL_45NM),
};

static void __init msm7x2x_init_host(void)
{
	msm_add_host(0, &msm_usb_host_pdata);
}
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
static int hsusb_rpc_connect(int connect)
{
	if (connect)
		return msm_hsusb_rpc_connect();
	else
		return msm_hsusb_rpc_close();
}

static struct regulator *reg_hsusb;
static int msm_hsusb_ldo_init(int init)
{
	int rc = 0;

	if (init) {
		reg_hsusb = regulator_get(NULL, "usb");
		if (IS_ERR(reg_hsusb)) {
			rc = PTR_ERR(reg_hsusb);
			pr_err("%s: could not get regulator: %d\n",
					__func__, rc);
			goto out;
		}

		rc = regulator_set_voltage(reg_hsusb, 3300000, 3300000);
		if (rc) {
			pr_err("%s: could not set voltage: %d\n",
					__func__, rc);
			goto reg_free;
		}

		return 0;
	}
	/* else fall through */
reg_free:
	regulator_put(reg_hsusb);
out:
	reg_hsusb = NULL;
	return rc;
}

static int msm_hsusb_ldo_enable(int enable)
{
	static int ldo_status;

	if (IS_ERR_OR_NULL(reg_hsusb))
		return reg_hsusb ? PTR_ERR(reg_hsusb) : -ENODEV;

	if (ldo_status == enable)
		return 0;

	ldo_status = enable;

	return enable ?
		regulator_enable(reg_hsusb) :
		regulator_disable(reg_hsusb);
}

#ifndef CONFIG_USB_EHCI_MSM_72K
static int msm_hsusb_pmic_notif_init(void (*callback)(int online), int init)
{
	int ret = 0;

	if (init)
		ret = msm_pm_app_rpc_init(callback);
	else
		msm_pm_app_rpc_deinit(callback);

	return ret;
}
#endif

static struct msm_otg_platform_data msm_otg_pdata = {
#ifndef CONFIG_USB_EHCI_MSM_72K
	.pmic_vbus_notif_init	 = msm_hsusb_pmic_notif_init,
#else
	.vbus_power		 = msm_hsusb_vbus_power,
#endif
	.rpc_connect		 = hsusb_rpc_connect,
	.pemp_level		 = PRE_EMPHASIS_WITH_20_PERCENT,
	.cdr_autoreset		 = CDR_AUTO_RESET_DISABLE,
	.drv_ampl		 = HS_DRV_AMPLITUDE_DEFAULT,
	.se1_gating		 = SE1_GATING_DISABLE,
	.ldo_init		 = msm_hsusb_ldo_init,
	.ldo_enable		 = msm_hsusb_ldo_enable,
	.chg_init		 = hsusb_chg_init,
	.chg_connected		 = hsusb_chg_connected,
	.chg_vbus_draw		 = hsusb_chg_vbus_draw,
};
#endif

static struct msm_hsusb_gadget_platform_data msm_gadget_pdata = {
	.is_phy_status_timer_on = 1,
};

static struct resource smc91x_resources[] = {
	[0] = {
		.start = 0x90000300,
		.end   = 0x900003ff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = MSM_GPIO_TO_INT(4),
		.end   = MSM_GPIO_TO_INT(4),
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device smc91x_device = {
	.name           = "smc91x",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(smc91x_resources),
	.resource       = smc91x_resources,
};

#ifdef CONFIG_SERIAL_MSM_HS
static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
	.inject_rx_on_wakeup	= 1,
	.rx_to_inject		= 0xFD,
};
#endif
static struct msm_pm_platform_data msm7x27a_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 16000,
					.residency = 20000,
	},
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 12000,
					.residency = 20000,
	},
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 0,
					.suspend_enabled = 1,
					.latency = 2000,
					.residency = 0,
	},
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 2,
					.residency = 0,
	},
};

static struct msm_pm_boot_platform_data msm_pm_boot_pdata __initdata = {
	.mode = MSM_PM_BOOT_CONFIG_RESET_VECTOR_PHYS,
	.p_addr = 0,
};

/* 8625 PM platform data */
static struct msm_pm_platform_data msm8625_pm_data[MSM_PM_SLEEP_MODE_NR * 2] = {
	/* CORE0 entries */
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 0,
					.suspend_enabled = 0,
					.latency = 16000,
					.residency = 20000,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 0,
					.suspend_enabled = 0,
					.latency = 12000,
					.residency = 20000,
	},

	/* picked latency & redisdency values from 7x30 */
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 0,
					.suspend_enabled = 0,
					.latency = 500,
					.residency = 6000,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 2,
					.residency = 10,
	},

	/* picked latency & redisdency values from 7x30 */
	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 0,
					.suspend_enabled = 0,
					.latency = 500,
					.residency = 6000,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 2,
					.residency = 10,
	},

};

static struct msm_pm_boot_platform_data msm_pm_8625_boot_pdata __initdata = {
	.mode = MSM_PM_BOOT_CONFIG_REMAP_BOOT_ADDR,
	.v_addr = MSM_CFG_CTL_BASE,
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
	.memory_type = MEMTYPE_EBI1,
	.request_region = request_fmem_c_region,
	.release_region = release_fmem_c_region,
	.reusable = 1,
};

static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};

static unsigned pmem_mdp_size = MSM_PMEM_MDP_SIZE;
static int __init pmem_mdp_size_setup(char *p)
{
	pmem_mdp_size = memparse(p, NULL);
	return 0;
}

early_param("pmem_mdp_size", pmem_mdp_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
static int __init pmem_adsp_size_setup(char *p)
{
	pmem_adsp_size = memparse(p, NULL);
	return 0;
}

early_param("pmem_adsp_size", pmem_adsp_size_setup);
//[ECID:000000] ZTEBSP wangminrong start for charger or bootup 20120627
 unsigned int lcd_id_type = 0;
static int __init zte_lcd_magic_setup(char *p)
{
	lcd_id_type = memparse(p, NULL);
	printk("wangminrong lcd_id_type is %d\r\n",lcd_id_type);
	return 0;
}

early_param("lcd_type", zte_lcd_magic_setup);

 int bootup_mode = 0;
static int __init mode_setup(char *str)
{
	printk("wangminrong is in mode_setup\r\n");
	if (!str)
		return -EINVAL;

	if (!strcmp(str, "charger")) {
		printk("wangminrong kernel is in power_off charger\r\n");
		bootup_mode = 1;
	} 
	else
	
		return -1;

	return 0;
}
early_param("androidboot.mode", mode_setup);
//[ECID:000000] ZTEBSP wangminrong end for charger or bootup 20120627



/* [ECID:000000] ZTEBSP yuanjinxing, for recongise recovery mode in kernel, 2012.07.13,, start */
static int __init rcm_mode_setup(char *str)
{
	printk("JXY DEBUD: RCM\r\n");
	if (!str)
		return -EINVAL;

	if (!strcmp(str, "true")) {
		printk("JXY DEBUG: kernel mode is RCM!\r\n");
		bootup_mode = 2;
	} 
	else
		return -1;

	return 0;
}
early_param("zte.rcmmod", rcm_mode_setup);
/* [ECID:000000] ZTEBSP yuanjinxing, for recongise recovery mode in kernel, 2012.07.13, start */

static struct android_pmem_platform_data android_pmem_audio_pdata = {
	.name = "pmem_audio",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.memory_type = MEMTYPE_EBI1,
};

static struct platform_device android_pmem_audio_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_audio_pdata },
};

static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
	.memory_type = MEMTYPE_EBI1,
};
static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};

static u32 msm_calculate_batt_capacity(u32 current_voltage);

static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.voltage_min_design     = 3200,
	.voltage_max_design     = 4200,
	.voltage_fail_safe      = 3340,
	.avail_chg_sources      = AC_CHG | USB_CHG ,
	.batt_technology        = POWER_SUPPLY_TECHNOLOGY_LION,
	.calculate_capacity     = &msm_calculate_batt_capacity,
};

static u32 msm_calculate_batt_capacity(u32 current_voltage)
{
	u32 low_voltage	 = msm_psy_batt_data.voltage_min_design;
	u32 high_voltage = msm_psy_batt_data.voltage_max_design;

	if (current_voltage <= low_voltage)
		return 0;
	else if (current_voltage >= high_voltage)
		return 100;
	else
		return (current_voltage - low_voltage) * 100
			/ (high_voltage - low_voltage);
}

static struct platform_device msm_batt_device = {
	.name               = "msm-battery",
	.id                 = -1,
	.dev.platform_data  = &msm_psy_batt_data,
};
/*[ECID:000000] ZTEBSP weiguohua change for not used gpio, start 20121122*/
#ifdef USE_QUALCOMM_ORIGINAL_CODE
static struct smsc911x_platform_config smsc911x_config = {
	.irq_polarity	= SMSC911X_IRQ_POLARITY_ACTIVE_HIGH,
	.irq_type	= SMSC911X_IRQ_TYPE_PUSH_PULL,
	.flags		= SMSC911X_USE_16BIT,
};

static struct resource smsc911x_resources[] = {
	[0] = {
		.start	= 0x90000000,
		.end	= 0x90007fff,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= MSM_GPIO_TO_INT(48),
		.end	= MSM_GPIO_TO_INT(48),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
	},
};

static struct platform_device smsc911x_device = {
	.name		= "smsc911x",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(smsc911x_resources),
	.resource	= smsc911x_resources,
	.dev		= {
		.platform_data	= &smsc911x_config,
	},
};

static struct msm_gpio smsc911x_gpios[] = {
	{ GPIO_CFG(48, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA),
							 "smsc911x_irq"  },
	{ GPIO_CFG(49, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA),
							 "eth_fifo_sel" },
};
#endif
/*[ECID:000000] ZTEBSP weiguohua change for not used gpio, end 20121122*/
static char *msm_adc_surf_device_names[] = {
	"XO_ADC",
};

static struct msm_adc_platform_data msm_adc_pdata = {
	.dev_names = msm_adc_surf_device_names,
	.num_adc = ARRAY_SIZE(msm_adc_surf_device_names),
	.target_hw = MSM_8x25,
};

static struct platform_device msm_adc_device = {
	.name   = "msm_adc",
	.id = -1,
	.dev = {
		.platform_data = &msm_adc_pdata,
	},
};
/*[ECID:000000] ZTEBSP weiguohua change for not used gpio, start 20121122*/
#ifdef USE_QUALCOMM_ORIGINAL_CODE
#define ETH_FIFO_SEL_GPIO	49
static void msm7x27a_cfg_smsc911x(void)
{
	int res;

	res = msm_gpios_request_enable(smsc911x_gpios,
				 ARRAY_SIZE(smsc911x_gpios));
	if (res) {
		pr_err("%s: unable to enable gpios for SMSC911x\n", __func__);
		return;
	}

	/* ETH_FIFO_SEL */
	res = gpio_direction_output(ETH_FIFO_SEL_GPIO, 0);
	if (res) {
		pr_err("%s: unable to get direction for gpio %d\n", __func__,
							 ETH_FIFO_SEL_GPIO);
		msm_gpios_disable_free(smsc911x_gpios,
						 ARRAY_SIZE(smsc911x_gpios));
		return;
	}
	gpio_set_value(ETH_FIFO_SEL_GPIO, 0);
}
#endif
/*[ECID:000000] ZTEBSP weiguohua change for not used gpio, end 20121122*/


#if defined(CONFIG_SERIAL_MSM_HSL_CONSOLE) \
		&& defined(CONFIG_MSM_SHARED_GPIO_FOR_UART2DM)
static struct msm_gpio uart2dm_gpios[] = {
	{GPIO_CFG(19, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
							"uart2dm_rfr_n" },
	{GPIO_CFG(20, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
							"uart2dm_cts_n" },
	{GPIO_CFG(21, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
							"uart2dm_rx"    },
	{GPIO_CFG(108, 2, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
							"uart2dm_tx"    },
};

static void msm7x27a_cfg_uart2dm_serial(void)
{
	int ret;
	ret = msm_gpios_request_enable(uart2dm_gpios,
					ARRAY_SIZE(uart2dm_gpios));
	if (ret)
		pr_err("%s: unable to enable gpios for uart2dm\n", __func__);
}
#else
static void msm7x27a_cfg_uart2dm_serial(void) { }
#endif

static struct fmem_platform_data fmem_pdata;

static struct platform_device fmem_device = {
	.name = "fmem",
	.id = 1,
	.dev = { .platform_data = &fmem_pdata },
};
#ifdef CONFIG_ZTE_UART_USE_PORT3
//[ECID:000000] ZTEBSP zhangqi for  utart debug 20111215 start
static unsigned uart_debug_gpio[] = {
GPIO_CFG(86, 1, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_12MA),
GPIO_CFG(87, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_12MA),
};
//[ECID:000000] ZTEBSP zhangqi for  utart debug 20111215 end
#endif
static struct platform_device *rumi_sim_devices[] __initdata = {
	&msm_device_dmov,
	&msm_device_smd,
	&smc91x_device,
	&msm_device_uart1,
	&msm_device_nand,
	&msm_device_uart_dm1,
	&msm_gsbi0_qup_i2c_device,
	&msm_gsbi1_qup_i2c_device,
};

static struct platform_device *msm8625_rumi3_devices[] __initdata = {
	&msm8625_device_dmov,
	&msm8625_device_smd,
	&msm8625_device_uart1,
	&msm8625_gsbi0_qup_i2c_device,
};

static struct platform_device *msm7627a_surf_ffa_devices[] __initdata = {
	&msm_device_dmov,
	&msm_device_smd,
	&msm_device_uart1,
	&msm_device_uart_dm1,
	&msm_device_uart_dm2,
	&msm_gsbi0_qup_i2c_device,
	&msm_gsbi1_qup_i2c_device,
	&msm_device_otg,
	&msm_device_gadget_peripheral,
/*[ECID:000000] ZTEBSP weiguohua change for not used gpio, start 20121122*/
#ifdef USE_QUALCOMM_ORIGINAL_CODE	
	&smsc911x_device,
#endif
/*[ECID:000000] ZTEBSP weiguohua change for not used gpio, end 20121122*/
	&msm_kgsl_3d0,
};

static struct platform_device *common_devices[] __initdata = {
	&android_usb_device,
	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_audio_device,
	&fmem_device,
	&msm_device_nand,
	&msm_device_snd,
	&msm_device_cad,
	&msm_device_adspdec,
	&asoc_msm_pcm,
	&asoc_msm_dai0,
	&asoc_msm_dai1,
	&msm_batt_device,
	&msm_adc_device,
#ifdef CONFIG_ION_MSM
	&ion_dev,
#endif
};

static struct platform_device *msm8625_surf_devices[] __initdata = {
	&msm8625_device_dmov,
#ifdef CONFIG_ZTE_UART_USE_PORT3
	&msm8625_device_uart3,	
#else
	&msm8625_device_uart1,
#endif
	&msm8625_device_uart_dm1,
	&msm8625_device_uart_dm2,
	&msm8625_gsbi0_qup_i2c_device,
	&msm8625_gsbi1_qup_i2c_device,
	&msm8625_device_smd,
	&msm8625_device_otg,
	&msm8625_device_gadget_peripheral,
	&msm8625_kgsl_3d0,
	//[ECID:0000]ZTE_BSP maxiaoping 20120330 add leds feature,start.
	#ifdef CONFIG_LEDS_MSM_PMIC// enabling LEDS devices
         &msm_device_pmic_leds,
	#endif
	//[ECID:0000]ZTE_BSP maxiaoping 20120330 add leds feature,end.

	//[ECID:0000]ZTE_BSP maxiaoping 20120420 add macro to control button led feature,start.
	//As some project such as P825A10 do not has keyboard leds.
	//[ECID:0000]ZTE_BSP maxiaoping 20120330 for button led driver,start.
	#ifndef CONFIG_LEDS_NO_KB_GPIO
	&android_leds,
	#endif
	//[ECID:0000]ZTE_BSP maxiaoping 20120330 for button led driver,end.
	//[ECID:0000]ZTE_BSP maxiaoping 20120420 add macro to control button led feature,end.

    //zte-modify,zhangbo,20120709,add ram_console func,begin	
	#ifdef CONFIG_ANDROID_RAM_CONSOLE
	&ram_console_device,
    #endif
    //zte-modify,zhangbo,20120709,add ram_console func,end	
};

static unsigned pmem_kernel_ebi1_size = PMEM_KERNEL_EBI1_SIZE;
static int __init pmem_kernel_ebi1_size_setup(char *p)
{
	pmem_kernel_ebi1_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_kernel_ebi1_size", pmem_kernel_ebi1_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;
static int __init pmem_audio_size_setup(char *p)
{
	pmem_audio_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_audio_size", pmem_audio_size_setup);

static void fix_sizes(void)
{
	if (machine_is_msm7625a_surf() || machine_is_msm7625a_ffa()) {
		pmem_mdp_size = MSM7x25A_MSM_PMEM_MDP_SIZE;
		pmem_adsp_size = MSM7x25A_MSM_PMEM_ADSP_SIZE;
	} else {
		pmem_mdp_size = MSM_PMEM_MDP_SIZE;
		pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
	}

	if (get_ddr_size() > SZ_512M)
		pmem_adsp_size = CAMERA_ZSL_SIZE;
#ifdef CONFIG_ION_MSM
	msm_ion_camera_size = pmem_adsp_size;
	msm_ion_audio_size = (MSM_PMEM_AUDIO_SIZE + PMEM_KERNEL_EBI1_SIZE);
	msm_ion_sf_size = pmem_mdp_size;
#endif
}

#ifdef CONFIG_ION_MSM
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct ion_co_heap_pdata co_ion_pdata = {
	.adjacent_mem_id = INVALID_HEAP_ID,
	.align = PAGE_SIZE,
};
#endif

/**
 * These heaps are listed in the order they will be allocated.
 * Don't swap the order unless you know what you are doing!
 */
static struct ion_platform_data ion_pdata = {
	.nr = MSM_ION_HEAP_NUM,
	.has_outer_cache = 1,
	.heaps = {
		{
			.id	= ION_SYSTEM_HEAP_ID,
			.type	= ION_HEAP_TYPE_SYSTEM,
			.name	= ION_VMALLOC_HEAP_NAME,
		},
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
		/* PMEM_ADSP = CAMERA */
		{
			.id	= ION_CAMERA_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_CAMERA_HEAP_NAME,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *)&co_ion_pdata,
		},
		/* PMEM_AUDIO */
		{
			.id	= ION_AUDIO_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_AUDIO_HEAP_NAME,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *)&co_ion_pdata,
		},
		/* PMEM_MDP = SF */
		{
			.id	= ION_SF_HEAP_ID,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= ION_SF_HEAP_NAME,
			.memory_type = ION_EBI_TYPE,
			.extra_data = (void *)&co_ion_pdata,
		},
#endif
	}
};

static struct platform_device ion_dev = {
	.name = "ion-msm",
	.id = 1,
	.dev = { .platform_data = &ion_pdata },
};
#endif

static struct memtype_reserve msm7x27a_reserve_table[] __initdata = {
	[MEMTYPE_SMI] = {
	},
	[MEMTYPE_EBI0] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
	[MEMTYPE_EBI1] = {
		.flags	=	MEMTYPE_FLAGS_1M_ALIGN,
	},
};

#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
static struct android_pmem_platform_data *pmem_pdata_array[] __initdata = {
		&android_pmem_adsp_pdata,
		&android_pmem_audio_pdata,
		&android_pmem_pdata,
};
#endif
#endif

static void __init size_pmem_devices(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	unsigned int i;
	unsigned int reusable_count = 0;

	android_pmem_adsp_pdata.size = pmem_adsp_size;
	android_pmem_pdata.size = pmem_mdp_size;
	android_pmem_audio_pdata.size = pmem_audio_size;

	fmem_pdata.size = 0;
	fmem_pdata.align = PAGE_SIZE;

	/* Find pmem devices that should use FMEM (reusable) memory.
	 */
	for (i = 0; i < ARRAY_SIZE(pmem_pdata_array); ++i) {
		struct android_pmem_platform_data *pdata = pmem_pdata_array[i];

		if (!reusable_count && pdata->reusable)
			fmem_pdata.size += pdata->size;

		reusable_count += (pdata->reusable) ? 1 : 0;

		if (pdata->reusable && reusable_count > 1) {
			pr_err("%s: Too many PMEM devices specified as reusable. PMEM device %s was not configured as reusable.\n",
				__func__, pdata->name);
			pdata->reusable = 0;
		}
	}
#endif
#endif
}

#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
static void __init reserve_memory_for(struct android_pmem_platform_data *p)
{
	msm7x27a_reserve_table[p->memory_type].size += p->size;
}
#endif
#endif

static void __init reserve_pmem_memory(void)
{
#ifdef CONFIG_ANDROID_PMEM
#ifndef CONFIG_MSM_MULTIMEDIA_USE_ION
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(pmem_pdata_array); ++i)
		reserve_memory_for(pmem_pdata_array[i]);

	msm7x27a_reserve_table[MEMTYPE_EBI1].size += pmem_kernel_ebi1_size;
#endif
#endif
}

static void __init size_ion_devices(void)
{
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
	ion_pdata.heaps[1].size = msm_ion_camera_size;
	ion_pdata.heaps[2].size = msm_ion_audio_size;
	ion_pdata.heaps[3].size = msm_ion_sf_size;
#endif
}

static void __init reserve_ion_memory(void)
{
#if defined(CONFIG_ION_MSM) && defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	msm7x27a_reserve_table[MEMTYPE_EBI1].size += msm_ion_camera_size;
	msm7x27a_reserve_table[MEMTYPE_EBI1].size += msm_ion_audio_size;
	msm7x27a_reserve_table[MEMTYPE_EBI1].size += msm_ion_sf_size;
#endif
}

static void __init msm7x27a_calculate_reserve_sizes(void)
{
	fix_sizes();
	size_pmem_devices();
	reserve_pmem_memory();
	size_ion_devices();
	reserve_ion_memory();
}

static int msm7x27a_paddr_to_memtype(unsigned int paddr)
{
	return MEMTYPE_EBI1;
}

static struct reserve_info msm7x27a_reserve_info __initdata = {
	.memtype_reserve_table = msm7x27a_reserve_table,
	.calculate_reserve_sizes = msm7x27a_calculate_reserve_sizes,
	.paddr_to_memtype = msm7x27a_paddr_to_memtype,
};

static void __init msm7x27a_reserve(void)
{
	reserve_info = &msm7x27a_reserve_info;
	msm_reserve();
}

static void __init msm8625_reserve(void)
{
	msm7x27a_reserve();
	memblock_remove(MSM8625_SECONDARY_PHYS, SZ_8);
	memblock_remove(MSM8625_WARM_BOOT_PHYS, SZ_32);
	memblock_remove(MSM8625_NON_CACHE_MEM, SZ_2K);
}

static void __init msm7x27a_device_i2c_init(void)
{
	msm_gsbi0_qup_i2c_device.dev.platform_data = &msm_gsbi0_qup_i2c_pdata;
	msm_gsbi1_qup_i2c_device.dev.platform_data = &msm_gsbi1_qup_i2c_pdata;
}

static void __init msm8625_device_i2c_init(void)
{
	msm8625_gsbi0_qup_i2c_device.dev.platform_data =
		&msm_gsbi0_qup_i2c_pdata;
	msm8625_gsbi1_qup_i2c_device.dev.platform_data =
		&msm_gsbi1_qup_i2c_pdata;
}

#define MSM_EBI2_PHYS			0xa0d00000
#define MSM_EBI2_XMEM_CS2_CFG1		0xa0d10030

static void __init msm7x27a_init_ebi2(void)
{
	uint32_t ebi2_cfg;
	void __iomem *ebi2_cfg_ptr;

	ebi2_cfg_ptr = ioremap_nocache(MSM_EBI2_PHYS, sizeof(uint32_t));
	if (!ebi2_cfg_ptr)
		return;

	ebi2_cfg = readl(ebi2_cfg_ptr);
	if (machine_is_msm7x27a_rumi3() || machine_is_msm7x27a_surf() ||
		machine_is_msm7625a_surf() || machine_is_msm8625_surf())
		ebi2_cfg |= (1 << 4); /* CS2 */

	writel(ebi2_cfg, ebi2_cfg_ptr);
	iounmap(ebi2_cfg_ptr);

	/* Enable A/D MUX[bit 31] from EBI2_XMEM_CS2_CFG1 */
	ebi2_cfg_ptr = ioremap_nocache(MSM_EBI2_XMEM_CS2_CFG1,
							 sizeof(uint32_t));
	if (!ebi2_cfg_ptr)
		return;

	ebi2_cfg = readl(ebi2_cfg_ptr);
	if (machine_is_msm7x27a_surf() || machine_is_msm7625a_surf())
		ebi2_cfg |= (1 << 31);

	writel(ebi2_cfg, ebi2_cfg_ptr);
	iounmap(ebi2_cfg_ptr);
}

static struct platform_device msm_proccomm_regulator_dev = {
	.name   = PROCCOMM_REGULATOR_DEV_NAME,
	.id     = -1,
	.dev    = {
		.platform_data = &msm7x27a_proccomm_regulator_data
	}
};

static void msm_adsp_add_pdev(void)
{
	int rc = 0;
	struct rpc_board_dev *rpc_adsp_pdev;

	rpc_adsp_pdev = kzalloc(sizeof(struct rpc_board_dev), GFP_KERNEL);
	if (rpc_adsp_pdev == NULL) {
		pr_err("%s: Memory Allocation failure\n", __func__);
		return;
	}
	rpc_adsp_pdev->prog = ADSP_RPC_PROG;

	if (cpu_is_msm8625())
		rpc_adsp_pdev->pdev = msm8625_device_adsp;
	else
		rpc_adsp_pdev->pdev = msm_adsp_device;
	rc = msm_rpc_add_board_dev(rpc_adsp_pdev, 1);
	if (rc < 0) {
		pr_err("%s: return val: %d\n",	__func__, rc);
		kfree(rpc_adsp_pdev);
	}
}

static void __init msm7627a_rumi3_init(void)
{
	msm7x27a_init_ebi2();
	platform_add_devices(rumi_sim_devices,
			ARRAY_SIZE(rumi_sim_devices));
}

static void __init msm8625_rumi3_init(void)
{
	msm7x2x_misc_init();
	msm_adsp_add_pdev();
	msm8625_device_i2c_init();
	platform_add_devices(msm8625_rumi3_devices,
			ARRAY_SIZE(msm8625_rumi3_devices));

	msm_pm_set_platform_data(msm8625_pm_data,
			 ARRAY_SIZE(msm8625_pm_data));
	BUG_ON(msm_pm_boot_init(&msm_pm_8625_boot_pdata));
	msm8x25_spm_device_init();
	msm_pm_register_cpr_ops();
}

#define LED_GPIO_PDM		96
#define UART1DM_RX_GPIO		45
/*[ECID:000000] $ZTEBSP fanjiankang removed wlan register in board begin*/

#if 0
#if defined(CONFIG_BT) && defined(CONFIG_MARIMBA_CORE)
static int __init msm7x27a_init_ar6000pm(void)
{
	msm_wlan_ar6000_pm_device.dev.platform_data = &ar600x_wlan_power;
	return platform_device_register(&msm_wlan_ar6000_pm_device);
}
#else
static int __init msm7x27a_init_ar6000pm(void) { return 0; }
#endif
#endif 

/*[ECID:000000]ZTEBSP fanjiankang removed wlan register in board end*/

static void __init msm7x27a_init_regulators(void)
{
	int rc = platform_device_register(&msm_proccomm_regulator_dev);
	if (rc)
		pr_err("%s: could not register regulator device: %d\n",
				__func__, rc);
}

static void __init msm7x27a_add_footswitch_devices(void)
{
	platform_add_devices(msm_footswitch_devices,
			msm_num_footswitch_devices);
}

static void __init msm7x27a_add_platform_devices(void)
{
	if (machine_is_msm8625_surf() || machine_is_msm8625_ffa()) {
		platform_add_devices(msm8625_surf_devices,
			ARRAY_SIZE(msm8625_surf_devices));
	} else {
		platform_add_devices(msm7627a_surf_ffa_devices,
			ARRAY_SIZE(msm7627a_surf_ffa_devices));
	}

	platform_add_devices(common_devices,
			ARRAY_SIZE(common_devices));
}

static void __init msm7x27a_uartdm_config(void)
{
	msm7x27a_cfg_uart2dm_serial();
	msm_uart_dm1_pdata.wakeup_irq = gpio_to_irq(UART1DM_RX_GPIO);
	if (cpu_is_msm8625())
		msm8625_device_uart_dm1.dev.platform_data =
			&msm_uart_dm1_pdata;
	else
		msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
}

static void __init msm7x27a_otg_gadget(void)
{
	if (cpu_is_msm8625()) {
		msm_otg_pdata.swfi_latency =
		msm8625_pm_data[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].latency;
		msm8625_device_otg.dev.platform_data = &msm_otg_pdata;
		msm8625_device_gadget_peripheral.dev.platform_data =
			&msm_gadget_pdata;
	} else {
		msm_otg_pdata.swfi_latency =
		msm7x27a_pm_data[
		MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
		msm_device_otg.dev.platform_data = &msm_otg_pdata;
		msm_device_gadget_peripheral.dev.platform_data =
			&msm_gadget_pdata;
	}
}

static void __init msm7x27a_pm_init(void)
{
	if (machine_is_msm8625_surf() || machine_is_msm8625_ffa()) {
		msm_pm_set_platform_data(msm8625_pm_data,
				ARRAY_SIZE(msm8625_pm_data));
		BUG_ON(msm_pm_boot_init(&msm_pm_8625_boot_pdata));
		msm8x25_spm_device_init();
		msm_pm_register_cpr_ops();
	} else {
		msm_pm_set_platform_data(msm7x27a_pm_data,
				ARRAY_SIZE(msm7x27a_pm_data));
		BUG_ON(msm_pm_boot_init(&msm_pm_boot_pdata));
	}

	msm_pm_register_irqs();
}


/*[ECID:000000] ZTEBSP fanjiankang add wlan power decluaration begin*/
int wlan_init_power(void);	 
/*[ECID:000000] ZTEBSP fanjiankang add wlan power decluaration end*/

#if defined(CONFIG_PROJECT_P825B20)||defined(CONFIG_PROJECT_P825A20)
#define ACC_SENSOR_I2C_NUM 0
#define AKM_SENSOR_I2C_NUM 0
#define TAOS_SENSOR_I2C_NUM 1
#define GYRO_SENSOR_I2C_NUM 0
#elif defined(CONFIG_PROJECT_P865E02)
#define ACC_SENSOR_I2C_NUM 1
#define AKM_SENSOR_I2C_NUM 1
#define TAOS_SENSOR_I2C_NUM 1
#define GYRO_SENSOR_I2C_NUM 1
#elif defined(CONFIG_PROJECT_V72A)
#define ACC_SENSOR_I2C_NUM 0
#define AKM_SENSOR_I2C_NUM 0
#define SAR_SENSOR_I2C_NUM 0
#else
#define ACC_SENSOR_I2C_NUM 1
#define AKM_SENSOR_I2C_NUM 1
#define TAOS_SENSOR_I2C_NUM 1
#define GYRO_SENSOR_I2C_NUM 1
#endif

#if defined(CONFIG_PROJECT_P865F01)
#define LIS3DH_I2C_ADDR 0x18
#else
#define LIS3DH_I2C_ADDR 0x19
#endif
#if defined(CONFIG_PROJECT_P825B20)||defined(CONFIG_PROJECT_P825A20)
#define AK8963_I2C_ADDR 0X0C
#elif defined(CONFIG_PROJECT_V72A)
#define AK8963_I2C_ADDR 0X0D
#else
#define AK8963_I2C_ADDR 0X0E
#endif
#define SAR_I2C_ADDR 0X28
#if defined(CONFIG_PROJECT_P865F01)
#define KIONIX_I2C_ADDR 0X0E
#else
#define KIONIX_I2C_ADDR 0X0F
#endif
#define L3G4200D_I2C_ADDR 0X68
#define TAOS_I2C_ADDR 0X39

#ifdef CONFIG_TI_ST_ACCEL_LIS3DH
static struct lis3dh_acc_platform_data lis3dh_pdata = {
	.poll_interval = 100,          //Driver polling interval as 100ms
	.min_interval = 5,    //Driver polling interval minimum 5ms
	.g_range = LIS3DH_ACC_G_2G,    //Full Scale of LSM303DLH Accelerometer  
#if defined(CONFIG_PROJECT_P865E02)||defined(CONFIG_PROJECT_P865F01)
	.axis_map_x = 0,      //x = x 
	.axis_map_y = 1,      //y = y 
	.axis_map_z = 2,      //z = z 
	.negate_x = 0,      //x = +x 
	.negate_y = 1,      //y = -y 
	.negate_z = 1,      //z = -z 
#elif defined(CONFIG_PROJECT_V72A)
	.axis_map_x = 0,      //x = x
	.axis_map_y = 1,      //y = y
	.axis_map_z = 2,      //z = z 
	.negate_x = 1,      //x = -x 
	.negate_y = 1,      //y = -y 
	.negate_z = 0,      //z = +z
#elif defined(CONFIG_PROJECT_P865F03)
	.axis_map_x = 1,      //x = x
	.axis_map_y = 0,      //y = y
	.axis_map_z = 2,      //z = z 
	.negate_x = 1,      //x = -x 
	.negate_y = 1,      //y = -y 
	.negate_z = 0,      //z = +z 
#elif defined(CONFIG_PROJECT_P865V30) ||defined ( CONFIG_PROJECT_P865V20)
	.axis_map_x = 1,      //x = x
	.axis_map_y = 0,      //y = y
	.axis_map_z = 2,      //z = z 
	.negate_x = 0,      //x = -x 
	.negate_y = 0,      //y = -y 
	.negate_z = 1,      //z = +z 	
#else
	.axis_map_x = 1,      //x = y 
	.axis_map_y = 0,      //y = x 
	.axis_map_z = 2,      //z = z 
	.negate_x = 0,      //x = +x 
	.negate_y = 0,      //y = +y 
	.negate_z = 1,      //z = -z 

#endif
	.gpio_int1 = -1,
	.gpio_int2 = -1,
}; 

static struct i2c_board_info lis3dh_i2c_boardinfo[] = {
        {
                I2C_BOARD_INFO(LIS3DH_ACC_DEV_NAME, LIS3DH_I2C_ADDR),
                .platform_data  = &lis3dh_pdata,
        },
};
#endif

#ifdef CONFIG_TI_ST_COMPASS_AK8963
static  struct akm8963_platform_data akm8963_pdata = { 
    #if defined(CONFIG_PROJECT_P865E02)||defined(CONFIG_PROJECT_P865F03)||defined(CONFIG_PROJECT_P865V30) ||defined ( CONFIG_PROJECT_P865V20)
        .layout = 7,
    #elif defined(CONFIG_PROJECT_V72A)
        .layout = 3,
    #elif defined(CONFIG_PROJECT_P825B20)||defined(CONFIG_PROJECT_P825A20)
        .layout = 5,
	#elif defined(CONFIG_PROJECT_P865F01)
		.layout = 8,
    #else
        .layout = 7,
    #endif
        .outbit = 1,
        .gpio_DRDY = 0,
        .gpio_RST = 0,
};

static struct i2c_board_info akm_compass_i2c_boardinfo[] = {
        {
                I2C_BOARD_INFO(AKM8963_I2C_NAME, AK8963_I2C_ADDR),
                .platform_data  = &akm8963_pdata,
		.irq = MSM_GPIO_TO_INT(ZTE_GPIO_COMPASS_SENSOR_IRQ),
        },
};

static void compass_sensor_init(void)
{
	 int ret = 0;
	 ret = gpio_request(ZTE_GPIO_COMPASS_SENSOR_IRQ, "ak8963");
        if (ret < 0) {
                pr_err("%s: gpio_request compass_irq failed %d\n", __func__, ret);
        }
        ret = gpio_tlmm_config(GPIO_CFG(ZTE_GPIO_COMPASS_SENSOR_IRQ, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
        if (ret < 0) {
                pr_err("%s: gpio_direction_input compass_irq failed %d\n", __func__, ret);
        }

	i2c_register_board_info(AKM_SENSOR_I2C_NUM, akm_compass_i2c_boardinfo,ARRAY_SIZE(akm_compass_i2c_boardinfo));
}
#endif

#ifdef CONFIG_ZTE_SENSORS_KXTF9

static struct kxtik_platform_data kxtik_pdata = {
        .min_interval   = 5,
        .poll_interval  = 200,

#if defined(CONFIG_PROJECT_P865E02)||defined(CONFIG_PROJECT_P865F01)
		.axis_map_x = 0,      //x = x 
		.axis_map_y = 1,      //y = y 
		.axis_map_z = 2,      //z = z 
		.negate_x = 0,      //x = +x 
		.negate_y = 1,      //y = -y 
		.negate_z = 1,      //z = -z 
#elif defined(CONFIG_PROJECT_V72A)
		.axis_map_x = 0,      //x = x
		.axis_map_y = 1,      //y = y
		.axis_map_z = 2,      //z = z 
		.negate_x = 1,      //x = -x 
		.negate_y = 1,      //y = -y 
		.negate_z = 0,      //z = +z
#elif defined(CONFIG_PROJECT_P865F03)
		.axis_map_x = 1,      //x = x
		.axis_map_y = 0,      //y = y
		.axis_map_z = 2,      //z = z 
		.negate_x = 1,      //x = -x 
		.negate_y = 1,      //y = -y 
		.negate_z = 0,      //z = +z 
#elif defined(CONFIG_PROJECT_P865V30) ||defined ( CONFIG_PROJECT_P865V20)
	.axis_map_x = 1,      //x = x
	.axis_map_y = 0,      //y = y
	.axis_map_z = 2,      //z = z 
	.negate_x = 0,      //x = -x 
	.negate_y = 0,      //y = -y 
	.negate_z = 1,      //z = +z 
#else
		.axis_map_x = 1,      //x = y 
		.axis_map_y = 0,      //y = x 
		.axis_map_z = 2,      //z = z 
		.negate_x = 0,      //x = +x 
		.negate_y = 0,      //y = +y 
		.negate_z = 1,      //z = -z 
#endif

        .res_12bit      = RES_12BIT,
        .g_range        = KXTIK_G_2G,
};

static struct i2c_board_info kionix_i2c_boardinfo[] = {
        {
                I2C_BOARD_INFO("kxtik", KIONIX_I2C_ADDR),
                .platform_data  = &kxtik_pdata,
        //      .irq            = MSM_GPIO_TO_INT(ZTE_GPIO_ACCEL_SENSOR_IRQ1),
        },
};
#endif

#ifdef CONFIG_INPUT_LIGHTSENSOR_TAOS
static struct i2c_board_info  taos_i2c_boardinfo[] __initdata ={
        {
                I2C_BOARD_INFO("taos", TAOS_I2C_ADDR),
                .irq = MSM_GPIO_TO_INT(ZTE_GPIO_LIGHT_SENSOR_IRQ),
        },
};

static void light_sensor_init(void)
{
	 int ret =0;
        ret = gpio_request(ZTE_GPIO_LIGHT_SENSOR_IRQ, "light_sensor_irq");
        if (ret < 0) {
                pr_err("%s: gpio_request light_sensor_irq failed %d\n", __func__, ret);
        }	
        ret = gpio_tlmm_config(GPIO_CFG(ZTE_GPIO_LIGHT_SENSOR_IRQ, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
        if (ret < 0) {
                pr_err("%s: gpio_direction_input light_sensor_irq failed %d\n", __func__, ret);
        }

	i2c_register_board_info(TAOS_SENSOR_I2C_NUM, taos_i2c_boardinfo,ARRAY_SIZE(taos_i2c_boardinfo));
}
#endif

#if defined(CONFIG_TI_ST_ACCEL_LIS3DH) || defined(CONFIG_ZTE_SENSORS_KXTF9)
static void accel_sensor_init(void)
{
	int ret =0;
	ret = gpio_request(ZTE_GPIO_ACCEL_SENSOR_IRQ1, "accel_irq");
	if (ret < 0) {
	        pr_err("%s: gpio_request accel_irq1 failed %d\n", __func__, ret);
	}
	ret = gpio_tlmm_config(GPIO_CFG(ZTE_GPIO_ACCEL_SENSOR_IRQ1, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (ret < 0) {
	        pr_err("%s: gpio_direction_input accel_irq1 failed %d\n", __func__, ret);
	}

	#ifdef CONFIG_TI_ST_ACCEL_LIS3DH
	i2c_register_board_info(ACC_SENSOR_I2C_NUM, lis3dh_i2c_boardinfo,ARRAY_SIZE(lis3dh_i2c_boardinfo));
	#endif

	#ifdef CONFIG_ZTE_SENSORS_KXTF9
	i2c_register_board_info(ACC_SENSOR_I2C_NUM, kionix_i2c_boardinfo,ARRAY_SIZE(kionix_i2c_boardinfo));
	#endif	
}
#endif

#ifdef CONFIG_TI_ST_GYRO_L3G4200D
static struct l3g4200d_gyr_platform_data l3g4200d_pdata = {
	.fs_range = L3G4200D_GYR_FS_2000DPS,
#if defined(CONFIG_PROJECT_P865F03)	
	.axis_map_x = 1, 
	.axis_map_y = 0,
	.axis_map_z = 2,
	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,
#else
	.axis_map_x = 0, 
	.axis_map_y = 1,
	.axis_map_z = 2,
	.negate_x = 1,
	.negate_y = 0,
	.negate_z = 1,
#endif
	.poll_interval=10,
	.min_interval=2,
};

static struct i2c_board_info  l3g4200d_i2c_boardinfo[] __initdata ={
        {
                I2C_BOARD_INFO(L3G4200D_GYR_DEV_NAME, L3G4200D_I2C_ADDR),
                .platform_data  = &l3g4200d_pdata,
        },
};

static void gyro_sensor_init(void)
{
	i2c_register_board_info(GYRO_SENSOR_I2C_NUM, l3g4200d_i2c_boardinfo,ARRAY_SIZE(l3g4200d_i2c_boardinfo));	
}
#endif

static void sensor_power_init(void)
{
	#ifdef CONFIG_PROJECT_P865E02
	const char *sensor_reg_ldo = "ldo12";
	#elif defined(CONFIG_PROJECT_P825B20)||defined(CONFIG_PROJECT_P825A20)
	const char *sensor_reg_ldo = "ldo17";
	#elif defined(CONFIG_PROJECT_V72A)
	const char *sensor_reg_ldo = "ldo15";
	#else
	const char *sensor_reg_ldo = NULL;
	#endif
	int ret = 0;
	struct regulator *sensor_reg = NULL;
	if(sensor_reg_ldo){
		sensor_reg = regulator_get(NULL , sensor_reg_ldo);
		if (IS_ERR(sensor_reg)) {
		        pr_err("%s: vreg get failed with : (%ld)\n",
		                __func__, PTR_ERR(sensor_reg));
		}
		
		/* Set the voltage level to 2.85V */
		ret = regulator_set_voltage(sensor_reg, 2850000, 2850000);
		if (ret < 0) {
		        pr_err("%s: set regulator level failed with :(%d)\n",
		                __func__, ret);
		        regulator_put(sensor_reg);
		}
		/* Enabling the 2.85V regulator */
		ret = regulator_enable(sensor_reg);
		if (ret) {
		        pr_err("%s: enable regulator failed with :(%d)\n",
		                __func__, ret);
		        regulator_put(sensor_reg);
		}
	}
} 

//added by yangze for SX9502 sensor test 20121015 start
#ifdef CONFIG_CAP_PROX_SX9500_ZTE
static int sx9500_get_nirq_state(void) 
{
	return !gpio_get_value(ZTE_GPIO_SAR_SENSOR_IRQ);
}

/* Define Registers that need to be initialized to values different than default*/
static struct smtc_reg_data sx9500_i2c_reg_setup[] = {	
	{
		.reg = 0x00,
		.val = 0x01,
	},
	{
		.reg = 0x01,
		.val = 0x00,
	},
	{
		.reg = 0x02,
		.val = 0x00,
	},
	{
		.reg = 0x04,
		.val = 0x00,
	},	
	{
		.reg = 0x05,
		.val = 0x00,
	},
	{
		.reg = 0x06,
		.val = 0x01,//0x03 -20121024 changed
	},		
	{
		.reg = SX950X_NOP0_REG,
		.val = 0x68,
	},
	{
		.reg = SX950X_NCPS1_REG,
		.val = 0x43,
	},
	{
		.reg = SX950X_NCPS2_REG,
		.val = 0x67,//0x43
	},
	{
		.reg = SX950X_NCPS3_REG,
		.val = 0x01,//0x08
	},
	{
		.reg = SX950X_NCPS4_REG,
		.val = 0x7D,//0xff
	},
	{
		.reg = SX950X_NCPS5_REG,
		.val = 0x4e,
	},
	{
		.reg = SX950X_NCPS6_REG,
		.val = 0x08,
	},
	{
		.reg = SX950X_NCPS7_REG,
		.val = 0x0A,//0x00 -20121024 changed for debounce
	},
	{
		.reg = SX950X_NCPS8_REG,
		.val = 0x00,
	},
};
/* Define SAR test configuration.*/
static smtc_sar_test_platform_data_t smtc_sar_test_cfg = {
  .keycode = KEY_0,
};

static sx9500_platform_data_t sx9500_config = {
	/* Function pointer to get the NIRQ state (1->NIRQ-low, 0->NIRQ-high) */
	.get_is_nirq_low = sx9500_get_nirq_state,
	/*  pointer to an initializer function. Here in case needed in the future */
	.init_platform_hw = 0,
	/*  pointer to an exit function. Here in case needed in the future */
	.exit_platform_hw = 0,
	.pi2c_reg = sx9500_i2c_reg_setup,
	.i2c_reg_num = ARRAY_SIZE(sx9500_i2c_reg_setup),
	.psar_platform_data = &smtc_sar_test_cfg,
};

static struct i2c_board_info cap_i2c_sensors_list[] = {
	{
	I2C_BOARD_INFO("sx9500", SAR_I2C_ADDR),
	.irq = MSM_GPIO_TO_INT(ZTE_GPIO_SAR_SENSOR_IRQ),
	.platform_data = &sx9500_config,
	},
};

static void prox_sensor_init(void)
{
	int err;
	printk("sx9500_init \r\n");

	err = gpio_request(ZTE_GPIO_SAR_SENSOR_IRQ, "sar_irq_pin");
	if (err < 0)
		printk("gpio_request(ZTE_GPIO_SAR_SENSOR_IRQ, sar_irq_pin) err \r\n");

	err = gpio_direction_input(ZTE_GPIO_SAR_SENSOR_IRQ);
	if (err < 0)
		printk("gpio_direction_input(ZTE_GPIO_SAR_SENSOR_IRQ) err \r\n");

	err = gpio_request(ZTE_GPIO_SAR_SENSOR_TXEN, "sar_txen_pin");
	if (err < 0)
		printk("gpio_request(ZTE_GPIO_SAR_SENSOR_TXEN, sar_txen_pin) err \r\n");

	err = gpio_direction_output(ZTE_GPIO_SAR_SENSOR_TXEN, 1);
	if (err < 0)
		printk("gpio_direction_output(ZTE_GPIO_SAR_SENSOR_TXEN, 1) err \r\n");
 
  i2c_register_board_info(SAR_SENSOR_I2C_NUM, cap_i2c_sensors_list, ARRAY_SIZE(cap_i2c_sensors_list));
}
#endif
//added by yangze for SX9502 sensor test 20121015 end

static void msm7x27a_sensor_init(void)
{
	sensor_power_init();
  /* ACCEL sensor_init */
	#if defined(CONFIG_TI_ST_ACCEL_LIS3DH) || defined(CONFIG_ZTE_SENSORS_KXTF9)
	accel_sensor_init();
	#endif
  /* COMPASS sensor init*/
	#ifdef CONFIG_TI_ST_COMPASS_AK8963
	compass_sensor_init();
	#endif
  /*Light&&Proximity sensor init*/
	#ifdef CONFIG_INPUT_LIGHTSENSOR_TAOS
	light_sensor_init();
	#endif
	#ifdef CONFIG_TI_ST_GYRO_L3G4200D
	gyro_sensor_init();
	#endif
	#ifdef CONFIG_CAP_PROX_SX9500_ZTE
	prox_sensor_init();
	#endif
}

/*[ECID:000000] ZTEBSP zhangbo add for nfc, start*/
#ifdef CONFIG_PN544_NFC

static struct pn544_i2c_platform_data pn544_data = {
	.irq_gpio = ZTE_GPIO_PN544_IRQ,
	.ven_gpio = ZTE_GPIO_PN544_VEN_EN,
	.firm_gpio = ZTE_GPIO_PN544_FIRM,
};

static struct i2c_board_info __initdata pn544_i2c0_boardinfo[] = {
	{
		I2C_BOARD_INFO("pn544", 0x28),
		.irq = MSM_GPIO_TO_INT(ZTE_GPIO_PN544_IRQ),
		.platform_data = &pn544_data,
	},
};

static void nfc_init(void)
{
	gpio_tlmm_config(GPIO_CFG(pn544_data.irq_gpio, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(pn544_data.ven_gpio, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(pn544_data.firm_gpio, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	gpio_export(pn544_data.irq_gpio, false);
	gpio_export(pn544_data.ven_gpio, false);
	gpio_export(pn544_data.firm_gpio, false);
	
	i2c_register_board_info(0, pn544_i2c0_boardinfo,
	ARRAY_SIZE(pn544_i2c0_boardinfo));
}
#endif
/*[ECID:000000] ZTEBSP zhangbo add for nfc, end*/


static void __init msm7x2x_init(void)
{
	msm7x2x_misc_init();

	/* Initialize regulators first so that other devices can use them */
	msm7x27a_init_regulators();
	msm_adsp_add_pdev();
	if (cpu_is_msm8625())
		msm8625_device_i2c_init();
	else
		msm7x27a_device_i2c_init();
	msm7x27a_init_ebi2();
	msm7x27a_uartdm_config();
//doumingming add 20121206 ++
#ifdef CONFIG_PROJECT_V72A
      printk("doumingming: enter msm7x2x_init");
      gpio_tlmm_config(GPIO_CFG(29, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_16MA),GPIO_CFG_ENABLE);
      gpio_request(ZTE_GPIO_CHARGE_CURRENT, "ZTE_GPIO_CHARGE_CURRENT");
      
//      gpio_direction_output(ZTE_GPIO_CHARGE_CURRENT, 1);
      printk("doumingming:msm7x2x_init max8903_chg_ac_type");
#endif
//doumingming add 20121206 --

#ifdef CONFIG_ZTE_UART_USE_PORT3
	gpio_request(86, "uart_rx");
	gpio_tlmm_config(uart_debug_gpio[0], GPIO_CFG_ENABLE);
	gpio_request(87, "uart_tx");
	gpio_tlmm_config(uart_debug_gpio[1], GPIO_CFG_ENABLE);
#endif
	msm7x27a_otg_gadget();
/*[ECID:000000] ZTEBSP weiguohua change for not used gpio, start 20121122*/
#ifdef USE_QUALCOMM_ORIGINAL_CODE
	msm7x27a_cfg_smsc911x();
#endif
/*[ECID:000000] ZTEBSP weiguohua change for not used gpio, end 20121122*/
	msm7x27a_add_footswitch_devices();
	msm7x27a_add_platform_devices();
	/* Ensure ar6000pm device is registered before MMC/SDC */
	/* modified by fanjiankang begin*/
/*[ECID:000000]ZTEBSP fanjiankang add wlan power init begin*/
	       wlan_init_power();
/*[ECID:000000] ZTEBSP fanjiankang add wlan power init begin*/
	msm7627a_init_mmc();
	msm_fb_add_devices();
	msm7x2x_init_host();
	msm7x27a_pm_init();
	register_i2c_devices();
#if defined(CONFIG_BT) && defined(CONFIG_MARIMBA_CORE)
	msm7627a_bt_power_init();
#endif
	msm7627a_camera_init();
	msm7627a_add_io_devices();
	/*7x25a kgsl initializations*/
	msm7x25a_kgsl_3d0_init();
	/*8x25 kgsl initializations*/
	msm8x25_kgsl_3d0_init();
//[ECID:000000] ZTEBSP wanghaifei start 20120330, sensor init
        msm7x27a_sensor_init();
//[ECID:000000] ZTEBSP wanghaifei end 20120330, sensor init
	/*[ECID:000000] ZTEBSP zhangbo add for nfc, start*/
	#ifdef CONFIG_PN544_NFC
	nfc_init();
	#endif
	/*[ECID:000000] ZTEBSP zhangbo add for nfc, end*/

}

static void __init msm7x2x_init_early(void)
{
	msm_msm7627a_allocate_memory_regions();
//zte-modify,zhangbo,20121211,add ram_console func,begin
#ifdef CONFIG_ANDROID_RAM_CONSOLE	
	persistent_ram_early_init(&ram_console_ram);
#endif
//zte-modify,zhangbo,20121211,add ram_console func,end
}

MACHINE_START(MSM7X27A_RUMI3, "QCT MSM7x27a RUMI3")
	.atag_offset	= 0x100,
	.map_io		= msm_common_io_init,
	.reserve	= msm7x27a_reserve,
	.init_irq	= msm_init_irq,
	.init_machine	= msm7627a_rumi3_init,
	.timer		= &msm_timer,
	.init_early     = msm7x2x_init_early,
	.handle_irq	= vic_handle_irq,
MACHINE_END
MACHINE_START(MSM7X27A_SURF, "QCT MSM7x27a SURF")
	.atag_offset	= 0x100,
	.map_io		= msm_common_io_init,
	.reserve	= msm7x27a_reserve,
	.init_irq	= msm_init_irq,
	.init_machine	= msm7x2x_init,
	.timer		= &msm_timer,
	.init_early     = msm7x2x_init_early,
	.handle_irq	= vic_handle_irq,
MACHINE_END
MACHINE_START(MSM7X27A_FFA, "QCT MSM7x27a FFA")
	.atag_offset	= 0x100,
	.map_io		= msm_common_io_init,
	.reserve	= msm7x27a_reserve,
	.init_irq	= msm_init_irq,
	.init_machine	= msm7x2x_init,
	.timer		= &msm_timer,
	.init_early     = msm7x2x_init_early,
	.handle_irq	= vic_handle_irq,
MACHINE_END
MACHINE_START(MSM7625A_SURF, "QCT MSM7625a SURF")
	.atag_offset    = 0x100,
	.map_io         = msm_common_io_init,
	.reserve        = msm7x27a_reserve,
	.init_irq       = msm_init_irq,
	.init_machine   = msm7x2x_init,
	.timer          = &msm_timer,
	.init_early     = msm7x2x_init_early,
	.handle_irq	= vic_handle_irq,
MACHINE_END
MACHINE_START(MSM7625A_FFA, "QCT MSM7625a FFA")
	.atag_offset    = 0x100,
	.map_io         = msm_common_io_init,
	.reserve        = msm7x27a_reserve,
	.init_irq       = msm_init_irq,
	.init_machine   = msm7x2x_init,
	.timer          = &msm_timer,
	.init_early     = msm7x2x_init_early,
	.handle_irq	= vic_handle_irq,
MACHINE_END
MACHINE_START(MSM8625_RUMI3, "QCT MSM8625 RUMI3")
	.atag_offset    = 0x100,
	.map_io         = msm8625_map_io,
	.reserve        = msm8625_reserve,
	.init_irq       = msm8625_init_irq,
	.init_machine   = msm8625_rumi3_init,
	.timer          = &msm_timer,
	.handle_irq	= gic_handle_irq,
MACHINE_END
MACHINE_START(MSM8625_SURF, "QCT MSM8625 SURF")
	.atag_offset    = 0x100,
	.map_io         = msm8625_map_io,
	.reserve        = msm8625_reserve,
	.init_irq       = msm8625_init_irq,
	.init_machine   = msm7x2x_init,
	.timer          = &msm_timer,
	.init_early     = msm7x2x_init_early,
	.handle_irq	= gic_handle_irq,
MACHINE_END
MACHINE_START(MSM8625_FFA, "QCT MSM8625 FFA")
	.atag_offset    = 0x100,
	.map_io         = msm8625_map_io,
	.reserve        = msm8625_reserve,
	.init_irq       = msm8625_init_irq,
	.init_machine   = msm7x2x_init,
	.timer          = &msm_timer,
	.init_early     = msm7x2x_init_early,
	.handle_irq	= gic_handle_irq,
MACHINE_END
