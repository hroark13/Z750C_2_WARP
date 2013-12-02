/*
 *  max17040_battery.c
 *  fuel-gauge systems for lithium-ion (Li+) batteries
 *
 *  Copyright (C) 2009 Samsung Electronics
 *  Minkyu Kang <mk7.kang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/max17040_battery.h>
#include <linux/slab.h>

#define MAX17040_VCELL_MSB	0x02
#define MAX17040_VCELL_LSB	0x03
#define MAX17040_SOC_MSB	0x04
#define MAX17040_SOC_LSB	0x05
#define MAX17040_MODE_MSB	0x06
#define MAX17040_MODE_LSB	0x07
#define MAX17040_VER_MSB	0x08
#define MAX17040_VER_LSB	0x09
#define MAX17040_RCOMP_MSB	0x0C
#define MAX17040_RCOMP_LSB	0x0D
/* ZTEBSP yuanjinxing 20121130, max17040 driver, load customer model, start */
#define MAX17040_OCV_MSB	0x0E
#define MAX17040_OCV_LSB	0x0F
/* ZTEBSP yuanjinxing 20121130, max17040 driver, load customer model, end */
#define MAX17040_CMD_MSB	0xFE
#define MAX17040_CMD_LSB	0xFF

#define MAX17040_DELAY		1000
#define MAX17040_BATTERY_FULL	95

struct i2c_client *max17040_bak_client;    /* ZTEBSP yuanjinxing 20121130, max17040 driver, define a back client */

struct max17040_chip {
	struct i2c_client		*client;
	struct delayed_work		work;
	struct power_supply		battery;
	struct max17040_platform_data	*pdata;

	/* State Of Connect */
	int online;
	/* battery voltage */
	int vcell;
	/* battery capacity */
	int soc;
	/* State Of Charge */
	int status;
	/* State Of load model */
	int load_status;          /* ZTEBSP yuanjinxing 20121130, max17040 driver, add the state of load model */
};

static int max17040_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct max17040_chip *chip = container_of(psy,
				struct max17040_chip, battery);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = chip->status;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = chip->online;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = chip->vcell;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = chip->soc;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/* ZTEBSP yuanjinxing 20121130, max17040 driver, delete not used function, start */
#if 0
static int max17040_write_reg(struct i2c_client *client, int reg, u8 value)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, value);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}
#endif
/* ZTEBSP yuanjinxing 20121130, max17040 driver, delete not used function, end */

static int max17040_read_reg(struct i2c_client *client, int reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

/* ZTEBSP yuanjinxing 20121130, max17040 driver, the access function of max17040, start */
/*
 * max17040_write_word() - Write a word to max17040 register
 * @client:	The i2c client
 * @reg	  : Register to be write
 * @value : New register value
 */
static int max17040_write_word(struct i2c_client *client, u8 reg, u16 value)
{
	int ret;
	u8 data[2];
	data[0] = (value >> 8) & 0xFF;
	data[1] = value & 0xFF;

	ret = i2c_smbus_write_i2c_block_data(client, reg, 2, data);

	if (ret < 0) {
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);
	}

	return ret;
}

/*
 * max17040_write_word() - Read a word from max17040 register
 * @client:	The i2c client
 * @reg	  : Register to be read
 */
static int max17040_read_word(struct i2c_client *client, u8 reg)
{
	int ret;
	u8 data[2];

	ret = i2c_smbus_read_i2c_block_data(client, reg, 2, data);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ((data[0] << 8) | data[1]);
}

/*
 * max17040_write_block_data() - Write a blcok data to max17040 memory
 * @client:	The i2c client
 * @reg	  : Start memory addr to be write
 * @len   : Block data length
 * @value : Block data addr
 */
static int max17040_write_block_data(struct i2c_client *client, u8 reg, u8 len, const u8 *value)
{
	int ret;
	
	ret = i2c_smbus_write_i2c_block_data(client, reg, len, value);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}
/* ZTEBSP yuanjinxing 20121130, max17040 driver, the access function of max17040, end */

#if 0    /* ZTEBSP yuanjinxing 20121130, max17040 driver, as maxim suggest, should not reset the device */
static void max17040_reset(struct i2c_client *client)
{
	max17040_write_reg(client, MAX17040_CMD_MSB, 0x54);
	max17040_write_reg(client, MAX17040_CMD_LSB, 0x00);
}
#endif

//static void max17040_get_vcell(struct i2c_client *client)
int max17040_get_vcell(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);
	unsigned int msb;
	unsigned int lsb;
    
	msb = max17040_read_reg(client, MAX17040_VCELL_MSB);
	lsb = max17040_read_reg(client, MAX17040_VCELL_LSB);
    
    /* ZTEBSP yuanjinxing 20121130, max17040 driver, vcell unit is 1.25 mv and return vcell, start */
	chip->vcell = (((msb*256 + lsb) >> 4) * 1250)/1000;
    if(chip->vcell > 4200)
    {
    	  chip->vcell =4200;
    }
    /* ZTEBSP yuanjinxing 20121130, max17040 driver, vcell unit is 1.25 mv and return vcell, end */
	
    return chip->vcell;
}

//static void max17040_get_soc(struct i2c_client *client)
int max17040_get_soc(struct i2c_client *client)  //huxb fixed, 2011.02.06
{
	struct max17040_chip *chip = i2c_get_clientdata(client);
	u8 msb;
	u8 lsb;

	/* ZTEBSP yuanjinxing 20121130, max17040 driver, during loading mode return the last soc value, start */
	if (chip->load_status == -1)
		return chip->soc;
	/* ZTEBSP yuanjinxing 20121130, max17040 driver, during loading mode return the last soc value, end */
	
	msb = max17040_read_reg(client, MAX17040_SOC_MSB);
	lsb = max17040_read_reg(client, MAX17040_SOC_LSB);
	
	/* ZTEBSP yuanjinxing 20121130, max17040 driver, loading customer model, start */
#if 0
	chip->soc = msb;
#else
	if (chip->pdata->ini_data.bits == 19)
		chip->soc = msb / 2;
	else
		chip->soc = msb;
	
	if(chip->soc > 100)
		chip->soc = 100;
#endif
	/* ZTEBSP yuanjinxing 20121130, max17040 driver, loading customer model, end */

    return chip->soc;
}

static void max17040_get_version(struct i2c_client *client)
{
	u8 msb;
	u8 lsb;

	msb = max17040_read_reg(client, MAX17040_VER_MSB);
	lsb = max17040_read_reg(client, MAX17040_VER_LSB);

	dev_info(&client->dev, "MAX17040 Fuel-Gauge Ver %d%d\n", msb, lsb);
}

/* ZTEBSP yuanjinxing 20121130, max17040 driver, load the battery model, strat */
/*
 * max17040_load_model() - load a customer model to max17040
 * @client:	The i2c client
 *
 * It is recommended to periodically reload the model in case
 * an event occurred that might corrupt the custom model.
 * 
 * The model can be refreshed once per hour.
 */
static int max17040_load_model(struct i2c_client *client)
{
	struct max17040_chip *chip;
	struct max17040_ini_data *ini_data;	/* customer model data */
	int rcomp, ocv;		/* saved value of rcomp and ocv register */
	int soc1;			/* saved SOC register high and low byte */
	int i;
	int load_status;

	chip = i2c_get_clientdata(client);
	ini_data = &chip->pdata->ini_data;
	dev_info(&client->dev, "@ini_data=%p, title=%s\n", ini_data, ini_data->title);
	
	/* 
	 * 1. Unlock Model Access
	 *
	 * To unlock access to the model the host software must write 0x4A to
 	 * memory location 0x3E and write 0x57 to memory location 0x3F.
	 */
	max17040_write_word(client, 0x3E, 0x4A57);

	/* 
	 * 2. Read RCOMP and OCV
	 */
	rcomp = max17040_read_word(client, MAX17040_RCOMP_MSB);
	ocv = max17040_read_word(client, MAX17040_OCV_MSB);
	dev_info(&client->dev, "rcomp=0x%x, ocv=0x%x\n", rcomp, ocv);

	/* 
	 * 3. Write OCV
	 */
	max17040_write_word(client, MAX17040_OCV_MSB, ini_data->ocvtest);

	/* 
	 * 4. Write RCOMP to a Maximum value of 0xFF00
	 */
	max17040_write_word(client, MAX17040_RCOMP_MSB, 0xFF00);

	/* 
	 * 5. Write Model
	 *
	 * The 64byte model is located between memory locations 0x40 and 0x7F
	 */
	for (i = 0x40; i < 0x80; i += 0x10) {
		max17040_write_block_data(client, i, 16, &ini_data->data[i - 0x20]); /* model data from 32byte */
	}

	/* 
	 * 6. Delay at least 150ms
	 *
	 */
	msleep(200);

	/* 
	 * 7. Write OCV
	 */
	max17040_write_word(client, MAX17040_OCV_MSB, ini_data->ocvtest);

	/* 
	 * 8. Delay between 150ms and 600ms
	 *
	 */
	msleep(200);

	/* 
	 * 9. Read SOC register and compare to expected result
	 *
	 */
	soc1 = max17040_read_reg(client, MAX17040_SOC_MSB);	
	if ((soc1 >= ini_data->socchecka) && (soc1 <= ini_data->soccheckb)) {
		load_status = 1;
		printk(KERN_CRIT "MAX17040 model load SUCCESS!\n");
	}else {
		load_status = 0;
		printk(KERN_CRIT "MAX17040 model load FAILURE!\n");
	}

	/* 
	 * 10. Restore RCOMP and OCV
	 *
	 */
	max17040_write_word(client, MAX17040_RCOMP_MSB, rcomp);
	max17040_write_word(client, MAX17040_OCV_MSB, ocv);

	/* 
	 * 11. Lock Model Access
	 *
	 * To lock access to the model the host software must write 0x00 to
 	 * memory location 0x3E and write 0x00 to memory location 0x3F.
	 */
	max17040_write_word(client, 0x3E, 0x0000);

	return load_status;
}

/*
 * show_load_model_status() - Show last load model status
 */
static ssize_t show_load_model_status(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct max17040_chip *chip;
	struct power_supply *psy;
	
	psy = dev_get_drvdata(dev);
	chip = container_of(psy, struct max17040_chip, battery);

	return sprintf(buf, "%u\n", chip->load_status);
}

/*
 * store_load_model() - Force to load model
 */
static ssize_t store_load_model(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct max17040_chip *chip;
	struct power_supply *psy;
	char *after;
	unsigned long num;
	
	psy = dev_get_drvdata(dev);
	chip = container_of(psy, struct max17040_chip, battery);
	
	num = simple_strtoul(buf, &after, 10);
	if (num)
	{
		/* adjust RCOMP register first */
		max17040_write_word(chip->client, MAX17040_RCOMP_MSB, (chip->pdata->ini_data.rcomp0 << 8));
		chip->load_status = max17040_load_model(chip->client);
	}

	return count;
}

static struct device_attribute load_model_attr =
	__ATTR(load_model, S_IRUGO | S_IWUSR, show_load_model_status, store_load_model);

#if 0
/*
* max17040_update_rcomp() - update rcomp register of max17040.
* MAXIM recommend update rcomp register when temperature is change.
*/
static int max17040_update_rcomp(struct max17040_chip *chip)
{
    int rcomp = 0;
	static u32 temp_last = -1000;
	static u32 temp_now = 0;

    /* first time update rcomp, only save the temp */
	if(-1000 == temp_last) {
	    temp_last = temp_of_cell;
	    return 0;
	}

    temp_now = temp_of_cell;
	printk(KERN_ERR "JXY DEBUG: cell temp is %dC\n", temp_of_cell);
	
    if(temp_now == temp_last)
		return 0;
	
    if(temp_now > 20) {
	    rcomp = (chip->pdata->ini_data.rcomp0 << 8)*100 + (temp_now - 20)*(chip->pdata->ini_data.tempcodown);
		rcomp /= 100;
    } else if(temp_now < 20) {
        rcomp = (chip->pdata->ini_data.rcomp0 << 8)*100 + (temp_now - 20)*(chip->pdata->ini_data.tempcoup);
		rcomp /= 100;
	}
	else {
		rcomp = chip->pdata->ini_data.rcomp0;
	}

	if(rcomp > 255) {
		rcomp = 255;
	} else if(rcomp < 0) {
	    rcomp = 0;
	}

	temp_last = temp_of_cell;
		
    max17040_write_word(chip->client, MAX17040_RCOMP_MSB, rcomp);

	return 0;
}
/* ZTEBSP yuanjinxing 20121130, max17040 driver, load the battery model, end */
#endif

static void max17040_get_online(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

	if (chip->pdata->battery_online)
		chip->online = chip->pdata->battery_online();
	else
		chip->online = 1;
}

static void max17040_get_status(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

	if (!chip->pdata->charger_online || !chip->pdata->charger_enable) {
		chip->status = POWER_SUPPLY_STATUS_UNKNOWN;
		return;
	}

	if (chip->pdata->charger_online()) {
		if (chip->pdata->charger_enable())
			chip->status = POWER_SUPPLY_STATUS_CHARGING;
		else
			chip->status = POWER_SUPPLY_STATUS_NOT_CHARGING;
	} else {
		chip->status = POWER_SUPPLY_STATUS_DISCHARGING;
	}

	if (chip->soc > MAX17040_BATTERY_FULL)
		chip->status = POWER_SUPPLY_STATUS_FULL;
}

static void max17040_work(struct work_struct *work)
{
	struct max17040_chip *chip;
	static int load_time = 0;	/* ZTEBSP yuanjinxing 20121130, max17040 driver, define for load battery model */

	chip = container_of(work, struct max17040_chip, work.work);

	max17040_get_vcell(chip->client);
	max17040_get_soc(chip->client);
	max17040_get_online(chip->client);
	max17040_get_status(chip->client);

	/* ZTEBSP yuanjinxing 20121130, max17040 driver, load battery model per hour, start */
	if ((load_time % (3600 * HZ)) == 0) {
		chip->load_status = -1;
		chip->load_status = max17040_load_model(chip->client);
	}
	load_time += MAX17040_DELAY;
	/* ZTEBSP yuanjinxing 20121130, max17040 driver, load battery model per hour, end */

	/* ZTEBSP yuanjinxing 20121130, max17040 driver, update RCOMP according temperature, start */
	//max17040_update_rcomp(chip);
	/* ZTEBSP yuanjinxing 20121130, max17040 driver, update RCOMP according temperature, end */
	
	schedule_delayed_work(&chip->work, MAX17040_DELAY);
}

static enum power_supply_property max17040_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
};

static int __devinit max17040_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct max17040_chip *chip;
	int ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, chip);

	chip->battery.name		= "max17040_gauge";    /* ZTEBSP yuanjinxing 20121130, max17040 driver, change the battery name. */
	chip->battery.type		= POWER_SUPPLY_TYPE_BATTERY;
	chip->battery.get_property	= max17040_get_property;
	chip->battery.properties	= max17040_battery_props;
	chip->battery.num_properties	= ARRAY_SIZE(max17040_battery_props);

	ret = power_supply_register(&client->dev, &chip->battery);
	if (ret) {
		dev_err(&client->dev, "failed: power supply register\n");
		kfree(chip);
		return ret;
	}
	
	/* ZTEBSP yuanjinxing 20121130, max17040 driver, creat the device file and update RCOMP, start */
	ret = device_create_file(chip->battery.dev, &load_model_attr);
	if (ret) {
		dev_err(&client->dev, "failed: create loadmodel file\n");
		printk(KERN_ERR "JXY DEBUG: create loadmodel file failure!\n");  /* ZTEBSP yuanjinxing 20121203, for debug */
	}

	//max17040_reset(client);            /* ZTEBSP yuanjinxing 20121130, max17040 driver, as maxim suggest, should not reset the device */
	max17040_write_word(client, MAX17040_RCOMP_MSB, (chip->pdata->ini_data.rcomp0 << 8));
	/* ZTEBSP yuanjinxing 20121130, max17040 driver, creat the device file and update RCOMP, end */
	
	max17040_get_version(client);
	max17040_bak_client = client;     /* ZTEBSP yuanjinxing 20121130, max17040 driver, backup the max17040 client */

	INIT_DELAYED_WORK_DEFERRABLE(&chip->work, max17040_work);
	schedule_delayed_work(&chip->work, MAX17040_DELAY);

	return 0;
}

static int __devexit max17040_remove(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

	device_remove_file(chip->battery.dev, &load_model_attr);	/** ZTE_MODIFY liuzhongzhi added for load customer model, liuzhongzhi0008 */
	power_supply_unregister(&chip->battery);
	cancel_delayed_work(&chip->work);
	kfree(chip);
	return 0;
}

#ifdef CONFIG_PM

static int max17040_suspend(struct i2c_client *client,
		pm_message_t state)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

	cancel_delayed_work(&chip->work);
	return 0;
}

static int max17040_resume(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);

	schedule_delayed_work(&chip->work, MAX17040_DELAY);
	return 0;
}

#else

#define max17040_suspend NULL
#define max17040_resume NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id max17040_id[] = {
	{ "max17040", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max17040_id);

static struct i2c_driver max17040_i2c_driver = {
	.driver	= {
		.name	= "max17040",
	},
	.probe		= max17040_probe,
	.remove		= __devexit_p(max17040_remove),
	.suspend	= max17040_suspend,
	.resume		= max17040_resume,
	.id_table	= max17040_id,
};

/* ZTEBSP yuanjinxing 20121130, max17040 driver, export the soc vcell and bak client, start  */
EXPORT_SYMBOL(max17040_get_vcell);
EXPORT_SYMBOL(max17040_get_soc);
EXPORT_SYMBOL(max17040_bak_client);
/* ZTEBSP yuanjinxing 20121130, max17040 driver, export the soc vcell and bak client, end  */

module_i2c_driver(max17040_i2c_driver);

MODULE_AUTHOR("Minkyu Kang <mk7.kang@samsung.com>");
MODULE_DESCRIPTION("MAX17040 Fuel Gauge");
MODULE_LICENSE("GPL");
