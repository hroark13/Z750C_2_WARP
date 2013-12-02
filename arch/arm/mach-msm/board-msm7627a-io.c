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

#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio_event.h>
#include <linux/leds.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <linux/i2c.h>
#include <linux/input/rmi_platformdata.h>
#include <linux/input/rmi_i2c.h>
#include <linux/delay.h>
#include <linux/atmel_maxtouch.h>
#include <linux/input/ft5x06_ts.h>
#include <linux/leds-msm-tricolor.h>
#include <asm/gpio.h>
#include <asm/mach-types.h>
#include <mach/rpc_server_handset.h>
#include <mach/pmic.h>
/*[ECID:000000] ZTEBSP yaotierui start  20120425, 8x25 bring up  */
#include <mach/vreg.h>
/*[ECID:000000] ZTEBSP yaotierui end  20120425, P825A20 bring up */

#include "devices.h"
#include "board-msm7627a.h"
#include "devices-msm7x2xa.h"

/*[ECID:000000] ZTEBSP change for touch screen, start*/
#ifdef USE_QUALCOMM_ORIGINAL_CODE
#define ATMEL_TS_I2C_NAME "maXTouch"
#define ATMEL_X_OFFSET 13
#define ATMEL_Y_OFFSET 0
#endif /*USE_QUALCOMM_ORIGINAL_CODE*/
/*[ECID:000000] ZTEBSP change for touch screen, end*/

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_RMI4_I2C) || \
defined(CONFIG_TOUCHSCREEN_SYNAPTICS_RMI4_I2C_MODULE)

#ifndef CLEARPAD3000_ATTEN_GPIO
#define CLEARPAD3000_ATTEN_GPIO (48)
#endif

#ifndef CLEARPAD3000_RESET_GPIO
#define CLEARPAD3000_RESET_GPIO (26)
#endif

/*[ECID:000000] ZTEBSP zhangbo change for side keys, start*/
#define KP_INDEX(row, col) ((row)*ARRAY_SIZE(kp_col_gpios) + (col))
#ifdef CONFIG_PROJECT_V72A

static struct gpio_event_direct_entry zte_keypad_key_map[] = {
	{
		.gpio	= 41,
		.code	= KEY_VOLUMEUP,
	},
	{
		.gpio	= 42,
		.code	= KEY_VOLUMEDOWN,
	},	
};

static struct gpio_event_input_info zte_keypad_key_info = {
	.info.func = gpio_event_input_func,
	.info.no_suspend = true,
	.debounce_time.tv64 = 5 * NSEC_PER_MSEC,  //wgh  modify
	.flags = 0,
	.type = EV_KEY,
	.keymap = zte_keypad_key_map,
	.keymap_size = ARRAY_SIZE(zte_keypad_key_map)
};
static struct gpio_event_info *kp_info[] = {
	&zte_keypad_key_info.info
};
#else
static unsigned int kp_row_gpios[] = {31,32};
static unsigned int kp_col_gpios[] = {36, 37};

static const unsigned short keymap[ARRAY_SIZE(kp_col_gpios) *
					  ARRAY_SIZE(kp_row_gpios)] = {
	[KP_INDEX(0, 0)] = KEY_VOLUMEUP,
	[KP_INDEX(0, 1)] = KEY_VOLUMEDOWN,
	
	[KP_INDEX(1, 0)] = KEY_RESERVED,//jiaobaocun changed
	[KP_INDEX(1, 1)] = KEY_RESERVED,
};

/* SURF keypad platform device information */
static struct gpio_event_matrix_info kp_matrix_info = {
	.info.func	= gpio_event_matrix_func,
	.keymap		= keymap,
	.output_gpios	= kp_row_gpios,
	.input_gpios	= kp_col_gpios,
	.noutputs	= ARRAY_SIZE(kp_row_gpios),
	.ninputs	= ARRAY_SIZE(kp_col_gpios),
	.settle_time.tv64 = 40 * NSEC_PER_USEC,
	.poll_time.tv64 = 20 * NSEC_PER_MSEC,
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
			  GPIOKPF_PRINT_UNMAPPED_KEYS,
};

static struct gpio_event_info *kp_info[] = {
	&kp_matrix_info.info
};
#endif
static struct gpio_event_platform_data kp_pdata = {
	.name		= "7x27a_kp",
	.info		= kp_info,
	.info_count	= ARRAY_SIZE(kp_info)
};

static struct platform_device kp_pdev = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &kp_pdata,
	},
};
/*[ECID:000000] ZTEBSP zhangbo change for side keys, end*/
/* 8625 keypad device information */
static unsigned int kp_row_gpios_8625[] = {31};
static unsigned int kp_col_gpios_8625[] = {36, 37};

static const unsigned short keymap_8625[] = {
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,
};

static const unsigned short keymap_8625_evt[] = {
	KEY_VOLUMEDOWN,
	KEY_VOLUMEUP,
};

static struct gpio_event_matrix_info kp_matrix_info_8625 = {
	.info.func      = gpio_event_matrix_func,
	.keymap         = keymap_8625,
	.output_gpios   = kp_row_gpios_8625,
	.input_gpios    = kp_col_gpios_8625,
	.noutputs       = ARRAY_SIZE(kp_row_gpios_8625),
	.ninputs        = ARRAY_SIZE(kp_col_gpios_8625),
	.settle_time.tv64 = 40 * NSEC_PER_USEC,
	.poll_time.tv64 = 20 * NSEC_PER_MSEC,
	.flags          = GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
			  GPIOKPF_PRINT_UNMAPPED_KEYS,
};

static struct gpio_event_info *kp_info_8625[] = {
	&kp_matrix_info_8625.info,
};

static struct gpio_event_platform_data kp_pdata_8625 = {
	.name           = "7x27a_kp",
	.info           = kp_info_8625,
	.info_count     = ARRAY_SIZE(kp_info_8625)
};

static struct platform_device kp_pdev_8625 = {
	.name   = GPIO_EVENT_DEV_NAME,
	.id     = -1,
	.dev    = {
		.platform_data  = &kp_pdata_8625,
	},
};

#define LED_GPIO_PDM 96

#define MXT_TS_IRQ_GPIO         48
#define MXT_TS_RESET_GPIO       26
#define MAX_VKEY_LEN		100

static ssize_t mxt_virtual_keys_register(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	char *virtual_keys = __stringify(EV_KEY) ":" __stringify(KEY_MENU) \
		":60:840:120:80" ":" __stringify(EV_KEY) \
		":" __stringify(KEY_HOME)   ":180:840:120:80" \
		":" __stringify(EV_KEY) ":" \
		__stringify(KEY_BACK) ":300:840:120:80" \
		":" __stringify(EV_KEY) ":" \
		__stringify(KEY_SEARCH)   ":420:840:120:80" "\n";

	return snprintf(buf, strnlen(virtual_keys, MAX_VKEY_LEN) + 1 , "%s",
			virtual_keys);
}

static struct kobj_attribute mxt_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.atmel_mxt_ts",
		.mode = S_IRUGO,
	},
	.show = &mxt_virtual_keys_register,
};

static struct attribute *mxt_virtual_key_properties_attrs[] = {
	&mxt_virtual_keys_attr.attr,
	NULL,
};

static struct attribute_group mxt_virtual_key_properties_attr_group = {
	.attrs = mxt_virtual_key_properties_attrs,
};

struct kobject *mxt_virtual_key_properties_kobj;

static int mxt_vkey_setup(void)
{
	int retval = 0;

	mxt_virtual_key_properties_kobj =
		kobject_create_and_add("board_properties", NULL);
	if (mxt_virtual_key_properties_kobj)
		retval = sysfs_create_group(mxt_virtual_key_properties_kobj,
				&mxt_virtual_key_properties_attr_group);
	if (!mxt_virtual_key_properties_kobj || retval)
		pr_err("failed to create mxt board_properties\n");

	return retval;
}

static const u8 mxt_config_data[] = {
	/* T6 Object */
	0, 0, 0, 0, 0, 0,
	/* T38 Object */
	16, 1, 0, 0, 0, 0, 0, 0,
	/* T7 Object */
	32, 16, 50,
	/* T8 Object */
	30, 0, 20, 20, 0, 0, 20, 0, 50, 0,
	/* T9 Object */
	3, 0, 0, 18, 11, 0, 32, 75, 3, 3,
	0, 1, 1, 0, 10, 10, 10, 10, 31, 3,
	223, 1, 11, 11, 15, 15, 151, 43, 145, 80,
	100, 15, 0, 0, 0,
	/* T15 Object */
	131, 0, 11, 11, 1, 1, 0, 45, 3, 0,
	0,
	/* T18 Object */
	0, 0,
	/* T19 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	/* T23 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/* T25 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0,
	/* T40 Object */
	0, 0, 0, 0, 0,
	/* T42 Object */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* T46 Object */
	0, 2, 32, 48, 0, 0, 0, 0, 0,
	/* T47 Object */
	1, 20, 60, 5, 2, 50, 40, 0, 0, 40,
	/* T48 Object */
	1, 12, 80, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
	10, 0, 20, 5, 0, 38, 0, 20, 0, 0,
	0, 0, 0, 0, 16, 65, 3, 1, 1, 0,
	10, 10, 10, 0, 0, 15, 15, 154, 58, 145,
	80, 100, 15, 3,
};

static const u8 mxt_config_data_evt[] = {
	/* T6 Object */
	0, 0, 0, 0, 0, 0,
	/* T38 Object */
	20, 1, 0, 25, 9, 12, 0, 0,
	/* T7 Object */
	24, 12, 10,
	/* T8 Object */
	30, 0, 20, 20, 0, 0, 0, 0, 10, 192,
	/* T9 Object */
	131, 0, 0, 18, 11, 0, 16, 70, 2, 1,
	0, 2, 1, 62, 10, 10, 10, 10, 107, 3,
	223, 1, 2, 2, 20, 20, 172, 40, 139, 110,
	10, 15, 0, 0, 0,
	/* T15 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,
	/* T18 Object */
	0, 0,
	/* T19 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	/* T23 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/* T25 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0,
	/* T40 Object */
	0, 0, 0, 0, 0,
	/* T42 Object */
	3, 20, 45, 40, 128, 0, 0, 0,
	/* T46 Object */
	0, 2, 16, 16, 0, 0, 0, 0, 0,
	/* T47 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* T48 Object */
	1, 12, 64, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
	10, 0, 20, 5, 0, 38, 0, 20, 0, 0,
	0, 0, 0, 0, 16, 65, 3, 1, 1, 0,
	10, 10, 10, 0, 0, 15, 15, 154, 58, 145,
	80, 100, 15, 3,
};

static struct mxt_config_info mxt_config_array[] = {
	{
		.config		= mxt_config_data,
		.config_length	= ARRAY_SIZE(mxt_config_data),
		.family_id	= 0x81,
		.variant_id	= 0x01,
		.version	= 0x10,
		.build		= 0xAA,
	},
};

static int mxt_key_codes[MXT_KEYARRAY_MAX_KEYS] = {
	[0] = KEY_HOME,
	[1] = KEY_MENU,
	[9] = KEY_BACK,
	[10] = KEY_SEARCH,
};

static struct mxt_platform_data mxt_platform_data = {
	.config_array		= mxt_config_array,
	.config_array_size	= ARRAY_SIZE(mxt_config_array),
	.panel_minx		= 0,
	.panel_maxx		= 479,
	.panel_miny		= 0,
	.panel_maxy		= 799,
	.disp_minx		= 0,
	.disp_maxx		= 479,
	.disp_miny		= 0,
	.disp_maxy		= 799,
	.irqflags		= IRQF_TRIGGER_FALLING,
	.i2c_pull_up		= true,
	.reset_gpio		= MXT_TS_RESET_GPIO,
	.irq_gpio		= MXT_TS_IRQ_GPIO,
	.key_codes		= mxt_key_codes,
};

static struct i2c_board_info mxt_device_info[] __initdata = {
	{
		I2C_BOARD_INFO("atmel_mxt_ts", 0x4a),
		.platform_data = &mxt_platform_data,
		.irq = MSM_GPIO_TO_INT(MXT_TS_IRQ_GPIO),
	},
};

static int synaptics_touchpad_setup(void);

static struct msm_gpio clearpad3000_cfg_data[] = {
	{GPIO_CFG(CLEARPAD3000_ATTEN_GPIO, 0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_6MA), "rmi4_attn"},
	{GPIO_CFG(CLEARPAD3000_RESET_GPIO, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_PULL_DOWN, GPIO_CFG_8MA), "rmi4_reset"},
};

static struct rmi_XY_pair rmi_offset = {.x = 0, .y = 0};
static struct rmi_range rmi_clipx = {.min = 48, .max = 980};
static struct rmi_range rmi_clipy = {.min = 7, .max = 1647};
static struct rmi_f11_functiondata synaptics_f11_data = {
	.swap_axes = false,
	.flipX = false,
	.flipY = false,
	.offset = &rmi_offset,
	.button_height = 113,
	.clipX = &rmi_clipx,
	.clipY = &rmi_clipy,
};

#define MAX_LEN		100

static ssize_t clearpad3000_virtual_keys_register(struct kobject *kobj,
		     struct kobj_attribute *attr, char *buf)
{
	char *virtual_keys = __stringify(EV_KEY) ":" __stringify(KEY_MENU) \
			     ":60:830:120:60" ":" __stringify(EV_KEY) \
			     ":" __stringify(KEY_HOME)   ":180:830:120:60" \
				":" __stringify(EV_KEY) ":" \
				__stringify(KEY_SEARCH) ":300:830:120:60" \
				":" __stringify(EV_KEY) ":" \
			__stringify(KEY_BACK)   ":420:830:120:60" "\n";

	return snprintf(buf, strnlen(virtual_keys, MAX_LEN) + 1 , "%s",
			virtual_keys);
}

static struct kobj_attribute clearpad3000_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.sensor00fn11",
		.mode = S_IRUGO,
	},
	.show = &clearpad3000_virtual_keys_register,
};

static struct attribute *virtual_key_properties_attrs[] = {
	&clearpad3000_virtual_keys_attr.attr,
	NULL
};

static struct attribute_group virtual_key_properties_attr_group = {
	.attrs = virtual_key_properties_attrs,
};

struct kobject *virtual_key_properties_kobj;

static struct rmi_functiondata synaptics_functiondata[] = {
	{
		.function_index = RMI_F11_INDEX,
		.data = &synaptics_f11_data,
	},
};

static struct rmi_functiondata_list synaptics_perfunctiondata = {
	.count = ARRAY_SIZE(synaptics_functiondata),
	.functiondata = synaptics_functiondata,
};

static struct rmi_sensordata synaptics_sensordata = {
	.perfunctiondata = &synaptics_perfunctiondata,
	.rmi_sensor_setup	= synaptics_touchpad_setup,
};

static struct rmi_i2c_platformdata synaptics_platformdata = {
	.i2c_address = 0x2c,
	.irq_type = IORESOURCE_IRQ_LOWLEVEL,
	.sensordata = &synaptics_sensordata,
};

static struct i2c_board_info synaptic_i2c_clearpad3k[] = {
	{
	I2C_BOARD_INFO("rmi4_ts", 0x2c),
	.platform_data = &synaptics_platformdata,
	},
};

static int synaptics_touchpad_setup(void)
{
	int retval = 0;

	virtual_key_properties_kobj =
		kobject_create_and_add("board_properties", NULL);
	if (virtual_key_properties_kobj)
		retval = sysfs_create_group(virtual_key_properties_kobj,
				&virtual_key_properties_attr_group);
	if (!virtual_key_properties_kobj || retval)
		pr_err("failed to create ft5202 board_properties\n");

	retval = msm_gpios_request_enable(clearpad3000_cfg_data,
		    sizeof(clearpad3000_cfg_data)/sizeof(struct msm_gpio));
	if (retval) {
		pr_err("%s:Failed to obtain touchpad GPIO %d. Code: %d.",
				__func__, CLEARPAD3000_ATTEN_GPIO, retval);
		retval = 0; /* ignore the err */
	}
	synaptics_platformdata.irq = gpio_to_irq(CLEARPAD3000_ATTEN_GPIO);

	gpio_set_value(CLEARPAD3000_RESET_GPIO, 0);
	usleep(10000);
	gpio_set_value(CLEARPAD3000_RESET_GPIO, 1);
	usleep(50000);

	return retval;
}
#endif

/*[ECID:000000] ZTEBSP change for touch screen, start*/
#ifdef USE_QUALCOMM_ORIGINAL_CODE
static struct regulator_bulk_data regs_atmel[] = {
	{ .supply = "ldo2",  .min_uV = 2850000, .max_uV = 2850000 },
	{ .supply = "smps3", .min_uV = 1800000, .max_uV = 1800000 },
};

#define ATMEL_TS_GPIO_IRQ 82

static int atmel_ts_power_on(bool on)
{
	int rc = on ?
		regulator_bulk_enable(ARRAY_SIZE(regs_atmel), regs_atmel) :
		regulator_bulk_disable(ARRAY_SIZE(regs_atmel), regs_atmel);

	if (rc)
		pr_err("%s: could not %sable regulators: %d\n",
				__func__, on ? "en" : "dis", rc);
	else
		msleep(50);

	return rc;
}

static int atmel_ts_platform_init(struct i2c_client *client)
{
	int rc;
	struct device *dev = &client->dev;

	rc = regulator_bulk_get(dev, ARRAY_SIZE(regs_atmel), regs_atmel);
	if (rc) {
		dev_err(dev, "%s: could not get regulators: %d\n",
				__func__, rc);
		goto out;
	}

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs_atmel), regs_atmel);
	if (rc) {
		dev_err(dev, "%s: could not set voltages: %d\n",
				__func__, rc);
		goto reg_free;
	}

	rc = gpio_tlmm_config(GPIO_CFG(ATMEL_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (rc) {
		dev_err(dev, "%s: gpio_tlmm_config for %d failed\n",
			__func__, ATMEL_TS_GPIO_IRQ);
		goto reg_free;
	}

	/* configure touchscreen interrupt gpio */
	rc = gpio_request(ATMEL_TS_GPIO_IRQ, "atmel_maxtouch_gpio");
	if (rc) {
		dev_err(dev, "%s: unable to request gpio %d\n",
			__func__, ATMEL_TS_GPIO_IRQ);
		goto ts_gpio_tlmm_unconfig;
	}

	rc = gpio_direction_input(ATMEL_TS_GPIO_IRQ);
	if (rc < 0) {
		dev_err(dev, "%s: unable to set the direction of gpio %d\n",
			__func__, ATMEL_TS_GPIO_IRQ);
		goto free_ts_gpio;
	}
	return 0;

free_ts_gpio:
	gpio_free(ATMEL_TS_GPIO_IRQ);
ts_gpio_tlmm_unconfig:
	gpio_tlmm_config(GPIO_CFG(ATMEL_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);
reg_free:
	regulator_bulk_free(ARRAY_SIZE(regs_atmel), regs_atmel);
out:
	return rc;
}

static int atmel_ts_platform_exit(struct i2c_client *client)
{
	gpio_free(ATMEL_TS_GPIO_IRQ);
	gpio_tlmm_config(GPIO_CFG(ATMEL_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);
	regulator_bulk_free(ARRAY_SIZE(regs_atmel), regs_atmel);
	return 0;
}

static u8 atmel_ts_read_chg(void)
{
	return gpio_get_value(ATMEL_TS_GPIO_IRQ);
}

static u8 atmel_ts_valid_interrupt(void)
{
	return !atmel_ts_read_chg();
}


static struct maxtouch_platform_data atmel_ts_pdata = {
	.numtouch = 4,
	.init_platform_hw = atmel_ts_platform_init,
	.exit_platform_hw = atmel_ts_platform_exit,
	.power_on = atmel_ts_power_on,
	.display_res_x = 480,
	.display_res_y = 864,
	.min_x = ATMEL_X_OFFSET,
	.max_x = (505 - ATMEL_X_OFFSET),
	.min_y = ATMEL_Y_OFFSET,
	.max_y = (863 - ATMEL_Y_OFFSET),
	.valid_interrupt = atmel_ts_valid_interrupt,
	.read_chg = atmel_ts_read_chg,
};

static struct i2c_board_info atmel_ts_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO(ATMEL_TS_I2C_NAME, 0x4a),
		.platform_data = &atmel_ts_pdata,
		.irq = MSM_GPIO_TO_INT(ATMEL_TS_GPIO_IRQ),
	},
};
#endif /*USE_QUALCOMM_ORIGINAL_CODE*/
/*[ECID:000000] ZTEBSP change for touch screen, end*/

static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "7k_handset",
	.pwr_key_delay_ms = 0, //[ECID:000000] ZTEBSP wanghaifei 20120625, 0 to disable end key
};

static struct platform_device hs_pdev = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};

#define FT5X06_IRQ_GPIO		48
#define FT5X06_RESET_GPIO	26

static ssize_t
ft5x06_virtual_keys_register(struct kobject *kobj,
			     struct kobj_attribute *attr,
			     char *buf)
{
	return snprintf(buf, 200,
	__stringify(EV_KEY) ":" __stringify(KEY_MENU)  ":40:510:80:60"
	":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":120:510:80:60"
	":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":200:510:80:60"
	":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":280:510:80:60"
	"\n");
}

static struct kobj_attribute ft5x06_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.ft5x06_ts",
		.mode = S_IRUGO,
	},
	.show = &ft5x06_virtual_keys_register,
};

static struct attribute *ft5x06_virtual_key_properties_attrs[] = {
	&ft5x06_virtual_keys_attr.attr,
	NULL,
};

static struct attribute_group ft5x06_virtual_key_properties_attr_group = {
	.attrs = ft5x06_virtual_key_properties_attrs,
};

struct kobject *ft5x06_virtual_key_properties_kobj;

static struct ft5x06_ts_platform_data ft5x06_platformdata = {
	.x_max		= 320,
	.y_max		= 480,
	.reset_gpio	= FT5X06_RESET_GPIO,
	.irq_gpio	= FT5X06_IRQ_GPIO,
	.irqflags	= IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
};

static struct i2c_board_info ft5x06_device_info[] __initdata = {
	{
		I2C_BOARD_INFO("ft5x06_ts", 0x38),
		.platform_data = &ft5x06_platformdata,
		.irq = MSM_GPIO_TO_INT(FT5X06_IRQ_GPIO),
	},
};

static void __init ft5x06_touchpad_setup(void)
{
	int rc;

	rc = gpio_tlmm_config(GPIO_CFG(FT5X06_IRQ_GPIO, 0,
			GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
			GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (rc)
		pr_err("%s: gpio_tlmm_config for %d failed\n",
			__func__, FT5X06_IRQ_GPIO);

	rc = gpio_tlmm_config(GPIO_CFG(FT5X06_RESET_GPIO, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
			GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (rc)
		pr_err("%s: gpio_tlmm_config for %d failed\n",
			__func__, FT5X06_RESET_GPIO);

	ft5x06_virtual_key_properties_kobj =
			kobject_create_and_add("board_properties", NULL);

	if (ft5x06_virtual_key_properties_kobj)
		rc = sysfs_create_group(ft5x06_virtual_key_properties_kobj,
				&ft5x06_virtual_key_properties_attr_group);

	if (!ft5x06_virtual_key_properties_kobj || rc)
		pr_err("%s: failed to create board_properties\n", __func__);

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				ft5x06_device_info,
				ARRAY_SIZE(ft5x06_device_info));
}

/* SKU3/SKU7 keypad device information */
#define KP_INDEX_SKU3(row, col) ((row)*ARRAY_SIZE(kp_col_gpios_sku3) + (col))
static unsigned int kp_row_gpios_sku3[] = {31, 32};
static unsigned int kp_col_gpios_sku3[] = {36, 37};

static const unsigned short keymap_sku3[] = {
	[KP_INDEX_SKU3(0, 0)] = KEY_VOLUMEUP,
	[KP_INDEX_SKU3(0, 1)] = KEY_VOLUMEDOWN,
	[KP_INDEX_SKU3(1, 1)] = KEY_CAMERA,
};

static struct gpio_event_matrix_info kp_matrix_info_sku3 = {
	.info.func      = gpio_event_matrix_func,
	.keymap         = keymap_sku3,
	.output_gpios   = kp_row_gpios_sku3,
	.input_gpios    = kp_col_gpios_sku3,
	.noutputs       = ARRAY_SIZE(kp_row_gpios_sku3),
	.ninputs        = ARRAY_SIZE(kp_col_gpios_sku3),
	.settle_time.tv64 = 40 * NSEC_PER_USEC,
	.poll_time.tv64 = 20 * NSEC_PER_MSEC,
	.flags          = GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
				GPIOKPF_PRINT_UNMAPPED_KEYS,
};

static struct gpio_event_info *kp_info_sku3[] = {
	&kp_matrix_info_sku3.info,
};
static struct gpio_event_platform_data kp_pdata_sku3 = {
	.name           = "7x27a_kp",
	.info           = kp_info_sku3,
	.info_count     = ARRAY_SIZE(kp_info_sku3)
};

static struct platform_device kp_pdev_sku3 = {
	.name   = GPIO_EVENT_DEV_NAME,
	.id     = -1,
	.dev    = {
		.platform_data  = &kp_pdata_sku3,
	},
};

static struct led_info ctp_backlight_info = {
	.name           = "button-backlight",
	.flags          = PM_MPP__I_SINK__LEVEL_40mA << 16 | PM_MPP_7,
};

static struct led_platform_data ctp_backlight_pdata = {
	.leds = &ctp_backlight_info,
	.num_leds = 1,
};

static struct platform_device pmic_mpp_leds_pdev = {
	.name   = "pmic-mpp-leds",
	.id     = -1,
	.dev    = {
		.platform_data  = &ctp_backlight_pdata,
	},
};

static struct led_info tricolor_led_info[] = {
	[0] = {
		.name           = "red",
		.flags          = LED_COLOR_RED,
	},
	[1] = {
		.name           = "green",
		.flags          = LED_COLOR_GREEN,
	},
};

static struct led_platform_data tricolor_led_pdata = {
	.leds = tricolor_led_info,
	.num_leds = ARRAY_SIZE(tricolor_led_info),
};

static struct platform_device tricolor_leds_pdev = {
	.name   = "msm-tricolor-leds",
	.id     = -1,
	.dev    = {
		.platform_data  = &tricolor_led_pdata,
	},
};

/*[ECID:000000] ZTEBSP yaotierui start  20120321, 8x25 bring up  */
#ifdef CONFIG_TOUCHSCREEN_VIRTUAL_KEYS
struct kobject *android_touch_kobj;
static void touch_sysfs_init(void)
{
	android_touch_kobj = kobject_create_and_add("board_properties", NULL);
	if (android_touch_kobj == NULL) {
		printk(KERN_ERR "%s: subsystem_register failed\n", __func__);
	}
}
#endif

static void touch_vdd(void)
{
#ifndef CONFIG_PROJECT_V72A
	struct vreg *vreg_s3_sp=NULL;
	int ret;

//ECID:0000 wangbing 2012-4-26 modified for bt start
#if defined ( CONFIG_PROJECT_P825A10 ) || defined ( CONFIG_PROJECT_P825F01 ) || defined ( CONFIG_PROJECT_P865F01 ) || defined ( CONFIG_PROJECT_P865E02 )|| defined ( CONFIG_PROJECT_P865F03 )|| defined ( CONFIG_PROJECT_P865V30 ) ||defined ( CONFIG_PROJECT_P865V20)
	gpio_tlmm_config(GPIO_CFG(5, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	ret = gpio_request(5,"TS_1V8_GPIO");
	if (ret) printk(KERN_ERR "%s: gpio_request: %d failed!\n", __func__, 5);	
	gpio_direction_output(5, 1);
	gpio_free(5);
	
	gpio_tlmm_config(GPIO_CFG(107, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	ret = gpio_request(107,"TS_3V0_GPIO");
	if (ret) printk(KERN_ERR "%s: gpio_request: %d failed!\n", __func__, 107);	
	gpio_direction_output(107, 1);
	gpio_free(107);

	msleep(5);

	gpio_tlmm_config(GPIO_CFG(26, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	ret = gpio_request(26,"TS_RESET");
	if (ret) printk(KERN_ERR "%s: gpio_request: %d failed!\n", __func__, 26);	
	gpio_direction_output(26, 1);
	msleep(5);	
	gpio_direction_output(26, 0);
	msleep(5);	
	gpio_direction_output(26, 1);	
	gpio_free(26);
#endif	
//ECID:0000 wangbing 2012-4-26 modified for bt end	
			
	vreg_s3_sp = vreg_get(NULL, "msme1");
	if (IS_ERR(vreg_s3_sp)) 
	{
		pr_err("%s: vreg_get for S3 failed\n", __func__);
	}
	else
	{
		vreg_disable(vreg_s3_sp);
	}
	gpio_tlmm_config(GPIO_CFG(131, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(132, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	
	ret = gpio_request(131,"I2C_SCL");
	if (ret) printk(KERN_ERR "%s: gpio_request: %d failed!\n", __func__, 131);			
	gpio_direction_output(131, 0);
	ret = gpio_request(132,"I2C_SDA");
	if (ret) printk(KERN_ERR "%s: gpio_request: %d failed!\n", __func__, 132);	
	gpio_direction_output(132, 0);
	#ifndef CONFIG_PROJECT_P865E02	//P865E02项目13引脚用于摄像头
	ret = gpio_request(13,"TCHN_3V_EN");
	if (ret) printk(KERN_ERR "%s: gpio_request: %d failed!\n", __func__, 13);	
	gpio_direction_output(13, 0);//TCHN_3V_EN
	#endif
	ret = gpio_request(12,"TCHN_RST");
	if (ret) printk(KERN_ERR "%s: gpio_request: %d failed!\n", __func__, 12);	
	gpio_direction_output(12, 0);
	
	ret = gpio_request(82,"CAP_TS_INT");
	if (ret) printk(KERN_ERR "%s: gpio_request: %d failed!\n", __func__, 82);	
	gpio_direction_output(82, 0);//CAP_TS_INT
	msleep(500);
	
	gpio_direction_output(12, 1);
	#ifndef CONFIG_PROJECT_P865E02
	gpio_direction_output(13, 1);
	#endif
	if (!IS_ERR(vreg_s3_sp)) 
	{
		vreg_enable(vreg_s3_sp);
	}
	gpio_direction_input(82);		
	gpio_free(131);
	gpio_free(132);
	#ifndef CONFIG_PROJECT_P865E02
	gpio_free(13);
	#endif
	gpio_free(12);
	gpio_free(82);
#endif
}
/*[ECID:000000] ZTEBSP yaotierui end  20120321, P825A20 bring up */

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT
#define ATMEL_TS_I2C_NAME "atmel_mxt_ts"
static int atmel_mxt_ts_platform_init(struct i2c_client *client)
{
	int rc;
        //printk("atmel_ts_platform_init \n");
	struct device *dev = &client->dev;
pr_err(" atmel init begin\n");

rc =gpio_request(ZTE_TS_LDO_ENABLE,"ts_power");
if (rc) {
		dev_err(dev, "%s: unable to request gpio %d\n",
			__func__, ZTE_TS_LDO_ENABLE);
		goto ts_ldo_gpio_tlmm_unconfig;
	}
	rc = gpio_tlmm_config(GPIO_CFG(ZTE_TS_LDO_ENABLE, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to enable reset gpio for TSC!\n", __func__);
		gpio_free(ZTE_TS_RESET_PIN);
		goto out;
	}
	rc = gpio_direction_output(ZTE_TS_LDO_ENABLE, 0);	
	if (rc) 
	{		
		pr_err("%s: unable to set_direction for gpio %d\n",	__func__, ZTE_TS_LDO_ENABLE);
	}
        gpio_set_value(ZTE_TS_LDO_ENABLE, 1);	
		
	/* configure touchscreen interrupt gpio */
	rc = gpio_request(ZTE_TS_GPIO_IRQ, "ts_irq_gpio");
	if (rc) {
		dev_err(dev, "%s: unable to request gpio %d\n",
			__func__, ZTE_TS_GPIO_IRQ);
		goto ts_gpio_tlmm_unconfig;
	}

	rc = gpio_tlmm_config(GPIO_CFG(ZTE_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (rc) {
		pr_err("%s: gpio_tlmm_config for %d failed\n",
			__func__, ZTE_TS_GPIO_IRQ);
        gpio_free(ZTE_TS_GPIO_IRQ);
		goto out;
	}

	rc = gpio_request(ZTE_TS_RESET_PIN, "touch reset");
	if (rc)
	{	
		printk("%s: gpio %d request is error!\n", __func__, ZTE_TS_RESET_PIN);
		goto ts_reset_gpio_tlmm_unconfig;
	}   

	rc = gpio_tlmm_config(GPIO_CFG(ZTE_TS_RESET_PIN, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to enable reset gpio for TSC!\n", __func__);
		gpio_free(ZTE_TS_RESET_PIN);
		goto out;
	}
	rc = gpio_direction_output(ZTE_TS_RESET_PIN, 0);	
	if (rc) 
	{		
		pr_err("%s: unable to set_direction for gpio %d\n",	__func__, ZTE_TS_RESET_PIN);
	}
        gpio_set_value(ZTE_TS_RESET_PIN, 0);
	msleep(100);	
        gpio_set_value(ZTE_TS_RESET_PIN, 1);
	msleep(100);
	
    return rc;

ts_reset_gpio_tlmm_unconfig:
	gpio_tlmm_config(GPIO_CFG(ZTE_TS_RESET_PIN, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);	
ts_gpio_tlmm_unconfig:
	gpio_tlmm_config(GPIO_CFG(ZTE_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);
ts_ldo_gpio_tlmm_unconfig:
	gpio_tlmm_config(GPIO_CFG(ZTE_TS_LDO_ENABLE, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);	
out:
	return rc;
}
// [ECID:000000] ZTEBSP zhangqi add for V70 touch 20111229 end
// [ECID:000000] ZTEBSP zhangqi add for V70 touch 20111229 start
#if 0
static int atmel_mxt_ts_platform_exit(struct i2c_client *client)
{
        printk("atmel_ts_platform_exit \n");
      #if 0	
	gpio_free(ZTE_TS_GPIO_IRQ);
	gpio_tlmm_config(GPIO_CFG(ZTE_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);
	#endif
	return 0;
}
#endif
// [ECID:000000] ZTEBSP zhangqi add for V70 touch 20111229 end
// [ECID:000000] ZTEBSP zhangqi add for V70 touch 20111229 start
/*haoweiwei add start 2012-9-27*/
static void atmel_mxt_ts_platform_reset(void)
{
	gpio_set_value(ZTE_TS_RESET_PIN,1);
	gpio_set_value(ZTE_TS_RESET_PIN,0);
	msleep(30);
	gpio_set_value(ZTE_TS_RESET_PIN,1);
	msleep(300);
	
	printk("%s: [TPS] touch panel hareware reset !\n", __func__);

}
/*haoweiwei add end 2012-9-27*/
#if 0
static u8 atmel_ts_read_chg(void)
{
	return gpio_get_value(ZTE_TS_GPIO_IRQ);
}
#endif

static struct mxt_platform_data atmel_mxt_ts_pdata = {
	.irq_gpio = ZTE_TS_GPIO_IRQ,
	.reset_gpio = ZTE_TS_RESET_PIN,
	.irqflags = IRQF_TRIGGER_FALLING,
	.init_platform_hw = atmel_mxt_ts_platform_init,
	//.exit_platform_hw = atmel_mxt_ts_platform_exit,
	//.read_chg = atmel_ts_read_chg,
        .reset_ts = atmel_mxt_ts_platform_reset,
};

#endif

/*[ECID:000000] ZTEBSP yaotierui start  20120321, 8x25 bring up  */
//ZTE_TS_XYM_20110808 begin
static struct i2c_board_info i2c_touch_devices[] = {
#ifdef CONFIG_TOUCHSCREEN_FOCALTECH_NEW		
	{				
		.type         = "ft5x0x_ts",
		/*.flags        = ,*/
		.addr         = 0x3E,
#if defined ( CONFIG_PROJECT_P825A10 ) || defined ( CONFIG_PROJECT_P825F01 ) || defined ( CONFIG_PROJECT_P865F01 ) || defined ( CONFIG_PROJECT_P865E02 )|| defined ( CONFIG_PROJECT_P865V30 ) ||defined ( CONFIG_PROJECT_P865V20)//ZTEBSP 2012-4-19 zhangzhao for 825a10	
		.irq          = MSM_GPIO_TO_INT(48),	

#elif defined CONFIG_PROJECT_P825A20		 
		.irq          = MSM_GPIO_TO_INT(82),
#elif defined CONFIG_PROJECT_P865E01		 
		.irq          = MSM_GPIO_TO_INT(82),
#else
//#error		
#endif				
	},	
#endif
/*[ECID:000000] ZTEBSP shihuiqin start  20120424, synaptics touchscreen  */
#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI		
	{				
		.type         = "synaptics-rmi-ts",
		/*.flags        = ,*/
		.addr         = 0x22,
#if defined ( CONFIG_PROJECT_P825A10 ) || defined ( CONFIG_PROJECT_P825F01 ) || defined ( CONFIG_PROJECT_P865F01 ) || defined ( CONFIG_PROJECT_P865F03 )|| defined ( CONFIG_PROJECT_P865V30 ) ||defined ( CONFIG_PROJECT_P865V20)//ZTEBSP 2012-4-19 zhangzhao for 825a10	
		.irq          = MSM_GPIO_TO_INT(48),	

#elif defined CONFIG_PROJECT_P825A20		 
		.irq          = MSM_GPIO_TO_INT(82),
#elif defined CONFIG_PROJECT_P865E01		 
		.irq          = MSM_GPIO_TO_INT(82),
#else
//#error		
#endif				
	},
#endif
/*[ECID:000000] ZTEBSP shihuiqin end 20120424, synatic touch screen  */
/*[ECID:000000] ZTEBSP haoweiwei start  20120911, gt818 touchscreen  */
#ifdef CONFIG_TOUCHSCREEN_GT818_I2C		
	{				
		.type         = "Goodix-TS",
		/*.flags        = ,*/
		.addr         = 0x5d,
#if defined ( CONFIG_PROJECT_P825A10 ) || defined ( CONFIG_PROJECT_P825F01 ) || defined ( CONFIG_PROJECT_P865F01 ) || defined ( CONFIG_PROJECT_P865V30 ) ||defined ( CONFIG_PROJECT_P865V20)//ZTEBSP 2012-4-19 zhangzhao for 825a10	
		.irq          = MSM_GPIO_TO_INT(48),	

#elif defined CONFIG_PROJECT_P825A20		 
		.irq          = MSM_GPIO_TO_INT(82),
#elif defined CONFIG_PROJECT_P865E01		 
		.irq          = MSM_GPIO_TO_INT(82),
#elif defined CONFIG_PROJECT_P865F02
        .irq          = MSM_GPIO_TO_INT(82),	
#endif				
	},
#endif
/*haoweiwei add for Goodix 968 touchscreen 20121010 start*/
#ifdef CONFIG_TOUCHSCREEN_GT968_I2C		
	{				
		.type         = "Goodix-TS",
		/*.flags        = ,*/
		.addr         = 0x5d,
#if defined ( CONFIG_PROJECT_P825A10 ) || defined ( CONFIG_PROJECT_P825F01 ) || defined ( CONFIG_PROJECT_P865F01 ) || defined ( CONFIG_PROJECT_P865V30 ) ||defined ( CONFIG_PROJECT_P865V20)//ZTEBSP 2012-4-19 zhangzhao for 825a10	
		.irq          = MSM_GPIO_TO_INT(48),	

#elif defined CONFIG_PROJECT_P825A20		 
		.irq          = MSM_GPIO_TO_INT(82),
#elif defined CONFIG_PROJECT_P865E01		 
		.irq          = MSM_GPIO_TO_INT(82),
#else
#error		
#endif				
	},
#endif
/*haoweiwei add for Goodix 968 touchscreen 20121010 end*/

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT	
/*mudong touch screen just  I2C ADDR DIFF*/
	{				
		.type         = ATMEL_TS_I2C_NAME,
		/*.flags        = ,*/
		.addr         = 0x4b,
 		.platform_data = &atmel_mxt_ts_pdata,		
		.irq          = MSM_GPIO_TO_INT(ZTE_TS_GPIO_IRQ),				
	},
/*junda touch screen ,just  I2C ADDR DIFF*/	
	{				
		.type         = ATMEL_TS_I2C_NAME,
		/*.flags        = ,*/
		.addr         = 0x4a,
 		.platform_data = &atmel_mxt_ts_pdata,		
		.irq          = MSM_GPIO_TO_INT(ZTE_TS_GPIO_IRQ),				
	},	
#endif
};
/*[ECID:000000] ZTEBSP yaotierui end  20120321, P825A20 bring up */

void __init msm7627a_add_io_devices(void)
{
#ifdef CONFIG_PROJECT_V72A
	gpio_tlmm_config(GPIO_CFG(42, 0, GPIO_CFG_INPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
#endif
	/*[ECID:000000] ZTEBSP yaotierui start  20120321, 8x25 bring up  */
		touch_vdd();/*解决掉电不完全的问题*/
	/*[ECID:000000] ZTEBSP yaotierui end  20120321, P825A20 bring up */

	/*[ECID:000000] ZTEBSP yaotierui start  20120321, 8x25 bring up  */
	#ifdef CONFIG_TOUCHSCREEN_VIRTUAL_KEYS
	touch_sysfs_init();
       #endif
	/*[ECID:000000] ZTEBSP yaotierui end  20120321, P825A20 bring up */

	/*[ECID:000000] ZTEBSP yaotierui start  20120321, 8x25 bring up  */
#ifdef CONFIG_PROJECT_V72A
	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
	          i2c_touch_devices,ARRAY_SIZE(i2c_touch_devices));//XYM
#else
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
	          i2c_touch_devices,ARRAY_SIZE(i2c_touch_devices));//XYM

#endif
	/*[ECID:000000] ZTEBSP yaotierui end  20120321, P825A20 bring up */
	
	/* touchscreen */
	/*[ECID:000000] ZTEBSP zhangbo comment it for touchscreen, start*/
	#ifdef USE_QUALCOMM_ORIGINAL_CODE
	if (machine_is_msm7625a_surf() || machine_is_msm7625a_ffa()) {
		atmel_ts_pdata.min_x = 0;
		atmel_ts_pdata.max_x = 480;
		atmel_ts_pdata.min_y = 0;
		atmel_ts_pdata.max_y = 320;
	}

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				atmel_ts_i2c_info,
				ARRAY_SIZE(atmel_ts_i2c_info));
	#endif
	/*[ECID:000000] ZTEBSP zhangbo comment it for touchscreen, end*/	
	/* keypad */
	platform_device_register(&kp_pdev);

	/* headset */
	platform_device_register(&hs_pdev);

	//[ECID:0000]ZTE_BSP maxiaoping 20120330 for button led driver,start.
	#if 0
	/* LED: configure it as a pdm function */
	if (gpio_tlmm_config(GPIO_CFG(LED_GPIO_PDM, 3,
				GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE))
		pr_err("%s: gpio_tlmm_config for %d failed\n",
			__func__, LED_GPIO_PDM);
	else
		platform_device_register(&led_pdev);

	/* Vibrator */
	if (machine_is_msm7x27a_ffa() || machine_is_msm7625a_ffa()
					|| machine_is_msm8625_ffa())
		msm_init_pmic_vibrator();
	#endif
	//[ECID:0000]ZTE_BSP maxiaoping 20120330 for button led driver,end.
	
	//[ECID:0000]ZTE_BSP maxiaoping 20120330 add vibrator  feature,start.
	#ifdef CONFIG_MSM_RPC_VIBRATOR
	//if (machine_is_msm7x27a_ffa() || machine_is_msm7625a_ffa())
	msm_init_pmic_vibrator();
	#endif
	//[ECID:0000]ZTE_BSP maxiaoping 20120330 add vibrator  feature,end.

}

void __init qrd7627a_add_io_devices(void)
{
	int rc;

	/* touchscreen */
	if (machine_is_msm7627a_qrd1()) {
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
					synaptic_i2c_clearpad3k,
					ARRAY_SIZE(synaptic_i2c_clearpad3k));
	} else if (machine_is_msm7627a_evb() || machine_is_msm8625_evb() ||
			machine_is_msm8625_evt()) {
		/* Use configuration data for EVT */
		if (machine_is_msm8625_evt()) {
			mxt_config_array[0].config = mxt_config_data_evt;
			mxt_config_array[0].config_length =
					ARRAY_SIZE(mxt_config_data_evt);
			mxt_platform_data.panel_maxy = 875;
			mxt_platform_data.need_calibration = true;
			mxt_vkey_setup();
		}

		rc = gpio_tlmm_config(GPIO_CFG(MXT_TS_IRQ_GPIO, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config for %d failed\n",
				__func__, MXT_TS_IRQ_GPIO);
		}

		rc = gpio_tlmm_config(GPIO_CFG(MXT_TS_RESET_GPIO, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config for %d failed\n",
				__func__, MXT_TS_RESET_GPIO);
		}

		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
					mxt_device_info,
					ARRAY_SIZE(mxt_device_info));
	} else if (machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7()) {
		ft5x06_touchpad_setup();
	}

	/* headset and power key*/
	/* ignore end key as this target doesn't need it */
	hs_platform_data.ignore_end_key = true;
	platform_device_register(&hs_pdev);

	/* vibrator */
#ifdef CONFIG_MSM_RPC_VIBRATOR
	msm_init_pmic_vibrator();
#endif

	/* keypad */
	if (machine_is_msm8625_evt())
		kp_matrix_info_8625.keymap = keymap_8625_evt;

	if (machine_is_msm7627a_evb() || machine_is_msm8625_evb() ||
			machine_is_msm8625_evt())
		platform_device_register(&kp_pdev_8625);
	else if (machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7())
		platform_device_register(&kp_pdev_sku3);

	/* leds */
	if (machine_is_msm7627a_evb() || machine_is_msm8625_evb() ||
						machine_is_msm8625_evt()) {
		platform_device_register(&pmic_mpp_leds_pdev);
		platform_device_register(&tricolor_leds_pdev);
	}
}
