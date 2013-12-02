/* drivers/input/keyboard/synaptics_i2c_rmi.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (C) 2008 Texas Instrument Inc.
 * Copyright (C) 2009 Synaptics, Inc.
 *
 * provides device files /dev/input/event#
 * for named device files, use udev
 * 2D sensors report ABS_X_FINGER(0), ABS_Y_FINGER(0) through ABS_X_FINGER(7), ABS_Y_FINGER(7)
 * NOTE: requires updated input.h, which should be included with this driver
 * 1D/Buttons report BTN_0 through BTN_0 + button_count
 * TODO: report REL_X, REL_Y for flick, BTN_TOUCH for tap (on 1D/0D; done for 2D)
 * TODO: check ioctl (EVIOCGABS) to query 2D max X & Y, 1D button count
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/synaptics_i2c_rmi.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>
#include <linux/proc_fs.h>

#include <linux/slab.h> 

#define GPIO_TOUCH_EN_1V8_OUT  5//ZTEBSP 2012-4-19 zhangzhao FOR 825a10
#define GPIO_TOUCH_RST_OUT  26 //ZTEBSP 2012-4-19 zhangzhao FOR 825a10
#define GPIO_TOUCH_INT_WAKEUP	48 //ZTEBSP 2012-4-19 zhangzhao FOR 825a10
#define GPIO_TOUCH_EN_2V8_OUT	107 //ZTEBSP 2012-4-19 zhangzhao FOR 825a10


#define BTN_F19 BTN_0
#define BTN_F30 BTN_0
#define SCROLL_ORIENTATION REL_Y
//#define TSP_DEBUG
static struct workqueue_struct *synaptics_wq;
static struct i2c_driver synaptics_rmi4_driver;
static int synaptics_rmi4_read_pdt(struct synaptics_rmi4 *ts);
/* Register: EGR_0 */
#define EGR_PINCH_REG		0
#define EGR_PINCH 		(1 << 6)
#define EGR_PRESS_REG 		0
#define EGR_PRESS 		(1 << 5)
#define EGR_FLICK_REG 		0
#define EGR_FLICK 		(1 << 4)
#define EGR_EARLY_TAP_REG	0
#define EGR_EARLY_TAP		(1 << 3)
#define EGR_DOUBLE_TAP_REG	0
#define EGR_DOUBLE_TAP		(1 << 2)
#define EGR_TAP_AND_HOLD_REG	0
#define EGR_TAP_AND_HOLD	(1 << 1)
#define EGR_SINGLE_TAP_REG	0
#define EGR_SINGLE_TAP		(1 << 0)
/* Register: EGR_1 */
#define EGR_PALM_DETECT_REG	1
#define EGR_PALM_DETECT		(1 << 0)

struct synaptics_function_descriptor {
	__u8 queryBase;
	__u8 commandBase;
	__u8 controlBase;
	__u8 dataBase;
	__u8 intSrc;
	__u8 functionNumber;
};
#define FUNCTION_VERSION(x) ((x >> 5) & 3)
#define INTERRUPT_SOURCE_COUNT(x) (x & 7)

#define FD_ADDR_MAX 0xE9
#define FD_ADDR_MIN 0xDD
#define FD_BYTE_COUNT 6

#define MIN_ACTIVE_SPEED 5

#define CONFIG_SYNA_MULTI_TOUCH1

//extern int prop_add( struct device *dev, char *item, char *value);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_rmi4_early_suspend(struct early_suspend *h);
static void synaptics_rmi4_late_resume(struct early_suspend *h);
#endif

#if defined(CONFIG_TOUCHSCREEN_VIRTUAL_KEYS)
/*virtual key support begin*/
static ssize_t synaptics_vkeys_show(struct kobject *kobj, struct kobj_attribute *attr,char *buf)
{

	/*key_type : key_value : center_x : cener_y : area_width : area_height*/
	return sprintf(buf,
	__stringify(EV_KEY) ":" __stringify(KEY_MENU) ":70:850:70:59"
	":" __stringify(EV_KEY) ":" __stringify(KEY_HOME) ":193:850:70:59"
	":" __stringify(EV_KEY) ":" __stringify(KEY_BACK) ":309:850:70:59"
	":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":415:850:70:59"
	"\n");
}
static struct kobj_attribute synaptics_vkeys_attr = {
	.attr = {
		.mode = S_IRUGO,
	},
	.show = &synaptics_vkeys_show,
};

static struct attribute *synaptics_properties_attrs[] = {
	&synaptics_vkeys_attr.attr,
	NULL
};

static struct attribute_group synaptics_properties_attr_group = {
	.attrs = synaptics_properties_attrs,
};
 
struct kobject *kobj_synaptics;
static void ts_key_report_synaptics_init(void)
{
	int rc = -EINVAL;

	/* virtual keys */
	synaptics_vkeys_attr.attr.name = "virtualkeys.Synaptics RMI4";
	kobj_synaptics = kobject_create_and_add("board_properties",
				NULL);
	if (kobj_synaptics)
		rc = sysfs_create_group(kobj_synaptics,
			&synaptics_properties_attr_group);
	if (!kobj_synaptics || rc)
		pr_err("%s: failed to create board_properties\n",
				__func__);
}
/*virtual key support end*/
#endif

static int synaptics_i2c_write(struct i2c_client *client, int reg, u8 data)
{
    u8 buf[2];
    int rc;
    int ret = 0;

    buf[0] = reg;
    buf[1] = data;
    rc = i2c_master_send(client, buf, 2);
    if (rc != 2)
    {
        dev_err(&client->dev, "synaptics_i2c_write FAILED: writing to reg %d\n", reg);
        ret = -1;
    }
    return ret;
}

static int synaptics_i2c_read(struct i2c_client *client, u8 addr, u8 *buf, u8 len)
{
	struct i2c_msg i2c_msg[2];
	int ret;
	int retry_count = 0;

	i2c_msg[0].addr = client->addr;
	i2c_msg[0].flags = 0;
	i2c_msg[0].buf = &addr;
	i2c_msg[0].len = 1;

	i2c_msg[1].addr = client->addr;
	i2c_msg[1].flags = I2C_M_RD;
	i2c_msg[1].buf = buf;
	i2c_msg[1].len = len;
	
retry:
	ret = i2c_transfer(client->adapter, i2c_msg, 2);
	if (ret < 0) 
	{
		if (++retry_count == 5) 
		{
			printk(KERN_ERR "[TSP] I2C read failed\n");
		       return ret;
		} else 
		{
			msleep(1);
			goto retry;
		}
	}
	return 0;
}
static int synaptics_rmi4_read_pdt(struct synaptics_rmi4 *ts)
{
	int ret = 0;
	int nFd = 0;
	int interruptCount = 0;
	__u8 data_length;
	struct i2c_msg fd_i2c_msg[2];
	__u8 fd_reg;
	struct i2c_msg query_i2c_msg[2];
	__u8 query[14];
	struct synaptics_function_descriptor fd;

	fd_i2c_msg[0].addr = ts->client->addr;
	fd_i2c_msg[0].flags = 0;
	fd_i2c_msg[0].buf = &fd_reg;
	fd_i2c_msg[0].len = 1;

	fd_i2c_msg[1].addr = ts->client->addr;
	fd_i2c_msg[1].flags = I2C_M_RD;
	fd_i2c_msg[1].buf = (__u8 *)(&fd);
	fd_i2c_msg[1].len = FD_BYTE_COUNT;

	query_i2c_msg[0].addr = ts->client->addr;
	query_i2c_msg[0].flags = 0;
	query_i2c_msg[0].buf = &fd.queryBase;
	query_i2c_msg[0].len = 1;

	query_i2c_msg[1].addr = ts->client->addr;
	query_i2c_msg[1].flags = I2C_M_RD;
	query_i2c_msg[1].buf = query;
	query_i2c_msg[1].len = sizeof(query);

	ts->hasF11 = false;
	ts->hasF19 = false;
	ts->hasF30 = false;
	ts->data_reg = 0xff;
	ts->data_length = 0;

	for (fd_reg = FD_ADDR_MAX; fd_reg >= FD_ADDR_MIN; fd_reg -= FD_BYTE_COUNT) {
		printk("[TSP] read from address 0x%x\n", fd_reg);
		ret = i2c_transfer(ts->client->adapter, fd_i2c_msg, 2);
		if (ret < 0) {
			printk(KERN_ERR "[TSP] I2C read failed querying RMI4 $%02X capabilities\n", ts->client->addr);
			return ret;
		}
		if (!fd.functionNumber) {
			/* End of PDT */
			ret = nFd;
			printk("[TSP] Read %d functions from PDT\n", nFd);
			printk("[TSP] end of PDT with string 0x%x; 0x%x; 0x%x; 0x%x; 0x%x; 0x%x\n", \
					fd.functionNumber, fd.queryBase, fd.commandBase, fd.controlBase, fd.dataBase, fd.intSrc);
			break;
		}
                            
		++nFd;
		printk("[TSP] fd.functionNumber = 0x%x\n", fd.functionNumber);
		printk("[TSP] fd.queryBase = 0x%x\n", fd.queryBase);
		printk("[TSP] fd.commandBase = 0x%x\n", fd.commandBase);
		printk("[TSP] fd.controlBase = 0x%x\n", fd.controlBase);
		printk("[TSP] fd.dataBase = 0x%x\n", fd.dataBase);
		printk("[TSP] fd.intSrc = 0x%x\n\n", fd.intSrc);
		
		switch (fd.functionNumber) {
			case 0x01: /* Interrupt */
				ts->f01.data_offset = fd.dataBase;
				ts->f01.ctrl_base	= fd.controlBase;
				printk("[TSP] : f01.ctrl_base=0x%x\n",ts->f01.ctrl_base);

				/*
				 * Can't determine data_length
				 * until whole PDT has been read to count interrupt sources
				 * and calculate number of interrupt status registers.
				 * Setting to 0 safely "ignores" for now.
				 */
				data_length = 0;
				break;
			case 0x11: /* 2D */
				ts->hasF11 = true;
				ts->f11.data_offset = fd.dataBase;
				ts->f11.interrupt_offset = interruptCount / 8;
				ts->f11.interrupt_mask = ((1 << INTERRUPT_SOURCE_COUNT(fd.intSrc)) - 1) << (interruptCount % 8);
                                                        
				ret = i2c_transfer(ts->client->adapter, query_i2c_msg, 2);
				if (ret < 0)
					printk(KERN_ERR "[TSP] Error reading F11 query registers\n");

				ts->f11.points_supported = (query[1] & 7) + 1;
				if (ts->f11.points_supported == 6)
					ts->f11.points_supported = 10;

				ts->f11.data_length = data_length =
					/* finger status, four fingers per register */
					((ts->f11.points_supported + 3) / 4)
					/* absolute data, 5 per finger */
					+ 5 * ts->f11.points_supported;
				
				ts->f11_max_x=i2c_smbus_read_word_data(ts->client, fd.controlBase+6);
				ts->f11_max_y=i2c_smbus_read_word_data(ts->client, fd.controlBase+8);
				printk("[TSP] : max_x=%d, max_y=%d\n",ts->f11_max_x,ts->f11_max_y);
				
				printk("[TSP] MultiTouch:%d; DataLength:%d; DataOffset:%d; IntSrc:%d; IntMask:%d\n", \
						ts->f11.points_supported, data_length, ts->f11.data_offset, fd.intSrc, ts->f11.interrupt_mask);
				break;
				
			case 0x19: /* Cap Buttons */
				ts->hasF19 = true;
				ts->f19.data_offset = fd.dataBase;
				ts->f19.interrupt_offset = interruptCount / 8;
				ts->f19.interrupt_mask = ((1 < INTERRUPT_SOURCE_COUNT(fd.intSrc)) - 1) << (interruptCount % 8);
				//ret = i2c_transfer(ts->client->adapter, query_i2c_msg, 2);
				if (ret < 0)
					printk(KERN_ERR "[TSP] Error reading F19 query registers\n");

				ts->f19.points_supported = query[1] & 0x1F;
				ts->f19.data_length = data_length = (ts->f19.points_supported + 7) / 8;

				printk(KERN_NOTICE "[TSP] $%02X F19 has %d buttons\n", ts->client->addr, ts->f19.points_supported);

				break;
				
			case 0x30: /* GPIO */
				ts->hasF30 = true;
				ts->f30.data_offset = fd.dataBase;
				ts->f30.interrupt_offset = interruptCount / 8;
				ts->f30.interrupt_mask = ((1 < INTERRUPT_SOURCE_COUNT(fd.intSrc)) - 1) << (interruptCount % 8);

				ret = i2c_transfer(ts->client->adapter, query_i2c_msg, 2);
				if (ret < 0)
					printk(KERN_ERR "[TSP] Error reading F30 query registers\n");

				ts->f30.points_supported = query[1] & 0x1F;
				ts->f30.data_length = data_length = (ts->f30.points_supported + 7) / 8;

				break;
				
			default:
				goto pdt_next_iter;				
		}

		// Change to end address for comparison
		// NOTE: make sure final value of ts->data_reg is subtracted
		data_length += fd.dataBase;
		if (data_length > ts->data_length) {
			ts->data_length = data_length;
		}

		if (fd.dataBase < ts->data_reg) {
			ts->data_reg = fd.dataBase;
		}

pdt_next_iter:
		interruptCount += INTERRUPT_SOURCE_COUNT(fd.intSrc);
	}

	// Now that PDT has been read, interrupt count determined, F01 data length can be determined.
	ts->f01.data_length = data_length = 1 + ((interruptCount + 7) / 8);
	// Change to end address for comparison
	// NOTE: make sure final value of ts->data_reg is subtracted
	data_length += ts->f01.data_offset;
	if (data_length > ts->data_length) {
		ts->data_length = data_length;
	}

	// Change data_length back from end address to length
	// NOTE: make sure this was an address
	ts->data_length -= ts->data_reg;

	// Change all data offsets to be relative to first register read
	//  TODO: add __u8 *data (= &ts->data[ts->f##.data_offset]) to struct rmi_function_info?
	ts->f01.data_offset -= ts->data_reg;
	ts->f11.data_offset -= ts->data_reg;
	ts->f19.data_offset -= ts->data_reg;
	ts->f30.data_offset -= ts->data_reg;

	ts->data = kcalloc(ts->data_length, sizeof(*ts->data), GFP_KERNEL);

	if (ts->data == NULL) {
		printk( "[TSP] Not enough memory to allocate space for RMI4 data\n");
		ret = -ENOMEM;
	}
	printk("[TSP] ts->datalength:%d; F01_DataOffset:0x%x; F11_DataOffset:0x%x; F19_DataOffset:0x%x; F30_DataOffset:0x%x;\n", \
			ts->data_length, ts->f01.data_offset, ts->f11.data_offset, ts->f19.data_offset, ts->f30.data_offset);
	ts->data_i2c_msg[0].addr = ts->client->addr;
	ts->data_i2c_msg[0].flags = 0;
	ts->data_i2c_msg[0].len = 1;
	ts->data_i2c_msg[0].buf = &ts->data_reg;

	ts->data_i2c_msg[1].addr = ts->client->addr;
	ts->data_i2c_msg[1].flags = I2C_M_RD;
	ts->data_i2c_msg[1].len = ts->data_length;
	ts->data_i2c_msg[1].buf = ts->data;

	printk("[TSP] RMI4 $%02X data read: $%02X + %d\n",
		ts->client->addr, ts->data_reg, ts->data_length);

	return ret;
}

static int
proc_read_val(char *page, char **start, off_t off, int count, int *eof,
	  void *data)
{
	int len = 0;
	
	len += sprintf(page + len, "%s\n", "touchscreen module");
	len += sprintf(page + len, "name     : %s\n", "synaptics");
	len += sprintf(page + len, "i2c address  : 0x%x\n", 0x22);
	len += sprintf(page + len, "IC type    : %s\n", "S2202 series");
	len += sprintf(page + len, "firmware version    : %s\n", "TCP-43E61");
	len += sprintf(page + len, "module : %s\n", "synaptics + TPK");
	
	if (off + count >= len)
		*eof = 1;
	if (len < off)
	{
		return 0;

	}
	*start = page + off;
	return ((count < len - off) ? count : len - off);
}

static int proc_write_val(struct file *file, const char *buffer,
           unsigned long count, void *data)
{
		unsigned long val;
		sscanf(buffer, "%lu", &val);
		if (val >= 0) {
			return count;
		}
		return -EINVAL;
}

static void synaptics_rmi4_work_func(struct work_struct *work)
{
	int ret;
	struct synaptics_rmi4 *ts = container_of(work, struct synaptics_rmi4, work);

       ret = synaptics_i2c_read(ts->client, *(ts->data_i2c_msg[0].buf), ts->data_i2c_msg[1].buf, ts->data_i2c_msg[1].len);

	if (ret < 0) {
		printk( "[TSP] %s: i2c_transfer failed\n", __func__);
	} else {
		__u8 *interrupt = &ts->data[ts->f01.data_offset + 1];
		#ifdef TSP_DEBUG
		printk("[TSP] int status: 0x%x,IntMask:0x%x\n", interrupt[ts->f11.interrupt_offset], ts->f11.interrupt_mask);
		#endif
		if (ts->hasF11 && (interrupt[ts->f11.interrupt_offset] & ts->f11.interrupt_mask))     //mengzf
		{
		       /* number of touch points - fingers down in this case */
 	              int fingerDownCount = 0;
			__u8 fsr_len = (ts->f11.points_supported + 3) / 4;
			__u8 *f11_data = &ts->data[ts->f11.data_offset];
	             int f;

	             for (f = 0; f < ts->f11.points_supported; ++f) 
	             {
				/*finger status*/
				__u8 finger_status_reg = 0;
				__u8 finger_status;

				finger_status_reg = f11_data[f / 4];
				finger_status = (finger_status_reg >> ((f % 4) * 2)) & 3;
				if (finger_status == f11_finger_accurate ) 	{
					fingerDownCount++;
					ts->wasdown = true;
				}
	                   if (finger_status == f11_finger_accurate) {
			             __u8 reg;
			             __u8 *finger_reg;
			             u12 x;
			             u12 y;
					u4 wx = 0;
					u4 wy = 0;
					int z=0;
					
					reg = fsr_len + 5 * f;
				       finger_reg = &f11_data[reg];
					x = (finger_reg[0] * 0x10) | (finger_reg[2] % 0x10);	
				       y = (finger_reg[1] * 0x10) | (finger_reg[2] / 0x10);
					wx = finger_reg[3] % 0x10;
					wy = finger_reg[3] / 0x10;
					z = finger_reg[4];
					#ifdef TSP_DEBUG
					printk(  "[TSP] f: %d, s:%d, x: %d, y: %d\n", f,finger_status,x,y);
					#endif
				//#ifdef CONFIG_SYNA_MULTI_TOUCH1
					/* Report Multi-Touch events for each finger */
					/* major axis of touch area ellipse */
					input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, z);
					/* minor axis of touch area ellipse */
					input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,
							max(wx, wy));
					/* Currently only 2 supported - 1 or 0 */
					input_report_abs(ts->input_dev, ABS_MT_ORIENTATION,
						(wx > wy ? 1 : 0));
					input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
					input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
					/* TODO: Tracking ID needs to be reported but not used yet. */
					/* Could be formed by keeping an id per position and assiging */
					/* a new id when fingerStatus changes for that position.*/
					input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, f);
              			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, z);
					input_report_key(ts->input_dev, BTN_TOUCH, 1);
					/* MT sync between fingers */
					input_mt_sync(ts->input_dev);
				//#endif
				}
			}

		/* if we had a finger down before and now we don't have any send a button up. */
			if ((fingerDownCount == 0) && ts->wasdown) {
				ts->wasdown = false;
			//#ifdef CONFIG_SYNA_MULTI_TOUCH1
				input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
				input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_X, ts->oldX);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, ts->oldY);
				input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, f);
				input_report_key(ts->input_dev, BTN_TOUCH, 0);

				input_mt_sync(ts->input_dev);
			//#endif
				#ifdef TSP_DEBUG
				printk( "[TSP] %s: Finger up.", __func__);
				#endif
			}
	              input_sync(ts->input_dev);     /* sync after groups of events */
		} 
	}
	if (ts->use_irq) {
		enable_irq(ts->client->irq);
	}
}

static enum hrtimer_restart synaptics_rmi4_timer_func(struct hrtimer *timer)
{
	struct synaptics_rmi4 *ts = container_of(timer, \
					struct synaptics_rmi4, timer);

	queue_work(synaptics_wq, &ts->work);

	hrtimer_start(&ts->timer, ktime_set(0, 12 * NSEC_PER_MSEC), HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;
}

irqreturn_t synaptics_rmi4_irq_handler(int irq, void *dev_id)
{
	struct synaptics_rmi4 *ts = dev_id;
#ifdef TSP_DEBUG
	printk("[TSP] %s enter\n", __func__);
#endif
	disable_irq_nosync(ts->client->irq);
	queue_work(synaptics_wq, &ts->work);

	return IRQ_HANDLED;
}

static int ts_power_on(void)
{
	int ret = 0;
	printk("%s: enter----\n", __func__);

/*
	ret = gpio_request(GPIO_TOUCH_RST_OUT, "touch reset");
	if (ret)
	{	
		printk("%s: gpio %d request is error!\n", __func__, GPIO_TOUCH_RST_OUT);
		goto err_check_functionality_failed;
	}   

	gpio_direction_output(GPIO_TOUCH_RST_OUT, 1);
	msleep(10);
	printk("%s: GPIO_TOUCH_RST_OUT(%d) = %d\n", __func__, GPIO_TOUCH_RST_OUT, gpio_get_value(GPIO_TOUCH_RST_OUT));
*/

	ret = gpio_request(GPIO_TOUCH_EN_2V8_OUT, "touch voltage 3V");
	if (ret)
	{	
		printk("%s: gpio %d request is error!\n", __func__, GPIO_TOUCH_EN_2V8_OUT);
		goto err_check_functionality_failed;
	}   
	ret = gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_EN_2V8_OUT, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (ret < 0) {
		pr_err("%s: unable to enable 2V8 gpio for TSC!\n", __func__);
		gpio_free(GPIO_TOUCH_EN_2V8_OUT);
		goto err_check_functionality_failed;

	}

	gpio_direction_output(GPIO_TOUCH_EN_2V8_OUT, 1);

	gpio_set_value(GPIO_TOUCH_EN_2V8_OUT,1);
	//msleep(300);//xym
	printk("%s: GPIO_TOUCH_EN_2V8_OUT(%d) = %d\n", __func__, GPIO_TOUCH_EN_2V8_OUT, gpio_get_value(GPIO_TOUCH_EN_2V8_OUT));



	ret = gpio_request(GPIO_TOUCH_EN_1V8_OUT, "touch voltage");
	if (ret)
	{	
		printk("%s: gpio %d request is error!\n", __func__, GPIO_TOUCH_EN_1V8_OUT);
		goto err_check_functionality_failed;

	}   
	ret = gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_EN_1V8_OUT, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (ret < 0) {
		pr_err("%s: unable to enable GPIO_TOUCH_EN_1V8_OUT gpio for TSC!\n", __func__);
		gpio_free(GPIO_TOUCH_EN_1V8_OUT);
		goto err_check_functionality_failed;
	}

	gpio_direction_output(GPIO_TOUCH_EN_1V8_OUT, 1);
	gpio_set_value(GPIO_TOUCH_EN_1V8_OUT,1);
	msleep(300);//xym
	printk("%s: GPIO_TOUCH_EN_1V8_OUT(%d) = %d\n", __func__, GPIO_TOUCH_EN_1V8_OUT, gpio_get_value(GPIO_TOUCH_EN_1V8_OUT));


	return 0;


	err_check_functionality_failed:
	//gpio_free(GPIO_TOUCH_EN_1V8_OUT);
	//gpio_free(GPIO_TOUCH_EN_2V8_OUT);

		return ret;


}
static int synaptics_rmi4_probe(
	struct i2c_client *client, const struct i2c_device_id *id)
{
	int i;
	int ret = 0;
      int min_x = 0;
      int min_y = 0;
	struct synaptics_rmi4 *ts;
	struct proc_dir_entry *dir, *refresh;
              
	ret=ts_power_on();
	if(ret)
	{
          	pr_err("[TSP]gpio request error!\n");
		return -EIO;
	}
		
#ifdef TSP_DEBUG
	printk( "[TSP] probing for Synaptics RMI4 device %s at $%02X...\n", client->name, client->addr);
#endif
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "[TSP] %s: need I2C_FUNC_I2C\n", __func__);
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}
        	
	ts = kzalloc(sizeof(struct synaptics_rmi4), GFP_KERNEL);
	INIT_WORK(&ts->work, synaptics_rmi4_work_func);
	ts->client = client;
	i2c_set_clientdata(client, ts);
	client->driver = &synaptics_rmi4_driver;

       ret = synaptics_rmi4_read_pdt(ts);
	if (ret <= 0) {
		if (ret == 0)
			printk(KERN_ERR "[TSP] Empty PDT\n");

		printk(KERN_ERR "[TSP] Error identifying device (%d)\n", ret);
		ret = -ENODEV;
		goto err_pdt_read_failed;
	}
	
	ts->input_dev = input_allocate_device();
	if (!ts->input_dev) {
		printk(KERN_ERR "[TSP] failed to allocate input device.\n");
		ret = -EBUSY;
		goto err_alloc_dev_failed;
	}

	ts->input_dev->name = "Synaptics RMI4";
	ts->input_dev->phys = client->name;
//	ts->f11_max_x = 1139;//1153;
//	ts->f11_max_y = 1888;//1893;
	min_x = 0;
	min_y = 0;
              
	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	
	/* set dummy key to make driver work with virtual keys */
	input_set_capability(ts->input_dev, EV_KEY, KEY_PROG1);
	
	set_bit(ABS_MT_TOUCH_MAJOR, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, ts->input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, ts->input_dev->absbit);

	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(BTN_2, ts->input_dev->keybit);
	

	if (ts->hasF11) {
		printk(KERN_DEBUG "[TSP] %s: Set ranges X=[%d..%d] Y=[%d..%d].", __func__, min_x, ts->f11_max_x, min_y, ts->f11_max_y);
		input_set_abs_params(ts->input_dev, ABS_X, min_x, ts->f11_max_x,0, 0);
		input_set_abs_params(ts->input_dev, ABS_Y, min_y, ts->f11_max_y,0, 0);
		input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
		input_set_abs_params(ts->input_dev, ABS_TOOL_WIDTH, 0, 15, 0, 0);

	//#ifdef CONFIG_SYNA_MULTI_TOUCH1
		input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 15, 0, 0);   /*pressure of single-touch*/
		input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MINOR, 0, 15, 0, 0);
		input_set_abs_params(ts->input_dev, ABS_MT_ORIENTATION, 0, 1, 0, 0);
		input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 1, ts->f11.points_supported, 0, 0);
		input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);   /*ABS_TOOL_WIDTH of single-touch*/

		input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, min_x, ts->f11_max_x,0, 0);
		input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, min_y, ts->f11_max_y,0, 0);
	//#endif

		if (ts->hasEgrPalmDetect)
			set_bit(BTN_DEAD, ts->input_dev->keybit);
		if (ts->hasEgrFlick) {
			set_bit(REL_X, ts->input_dev->keybit);
			set_bit(REL_Y, ts->input_dev->keybit);
		}
		if (ts->hasEgrSingleTap)
			set_bit(BTN_TOUCH, ts->input_dev->keybit);
		if (ts->hasEgrDoubleTap)
			set_bit(BTN_TOOL_DOUBLETAP, ts->input_dev->keybit);
	}
	if (ts->hasF19) {
		set_bit(BTN_DEAD, ts->input_dev->keybit);
#ifdef CONFIG_SYNA_BUTTONS
		/* F19 does not (currently) report ABS_X but setting maximum X is a convenient way to indicate number of buttons */
		input_set_abs_params(ts->input_dev, ABS_X, 0, ts->f19.points_supported, 0, 0);
		for (i = 0; i < ts->f19.points_supported; ++i) {
			set_bit(BTN_F19 + i, ts->input_dev->keybit);
		}
#endif

#ifdef CONFIG_SYNA_BUTTONS_SCROLL
		set_bit(EV_REL, ts->input_dev->evbit);
		set_bit(SCROLL_ORIENTATION, ts->input_dev->relbit);
#endif
	}
	if (ts->hasF30) {
		for (i = 0; i < ts->f30.points_supported; ++i) {
			set_bit(BTN_F30 + i, ts->input_dev->keybit);
		}
	}

	/*
	 * Device will be /dev/input/event#
	 * For named device files, use udev
	 */
	ret = input_register_device(ts->input_dev);
	if (ret) {
		printk(KERN_ERR "[TSP] synaptics_rmi4_probe: Unable to register %s \
				input device\n", ts->input_dev->name);
		goto err_input_register_device_failed;
	} else {
		printk("[TSP] synaptics input device registered\n");
	}

	if (client->irq) {
		if (request_irq(client->irq, synaptics_rmi4_irq_handler,
				IRQF_TRIGGER_LOW, client->name, ts) >= 0) {
			ts->use_irq = 1;
		} else {
			printk("[TSP] Failed to request IRQ!\n");
		}
	}
              
	if (!ts->use_irq) {
		printk(KERN_ERR "[TSP] Synaptics RMI4 device %s in polling mode\n", client->name);
		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = synaptics_rmi4_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}

	ts->enable = 1;

	dev_set_drvdata(&ts->input_dev->dev, ts);

       dir = proc_mkdir("touchscreen", NULL);
	refresh = create_proc_entry("ts_information", 0644, dir);
	if (refresh) {
		refresh->data		= NULL;
		refresh->read_proc  = proc_read_val;
		refresh->write_proc = proc_write_val;
	}
	printk( "[TSP] synaptics_ts_probe: Start touchscreen %s in %s mode\n", ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");

	#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = synaptics_rmi4_early_suspend;
	ts->early_suspend.resume = synaptics_rmi4_late_resume;
	register_early_suspend(&ts->early_suspend);
	#endif
	
	#ifdef CONFIG_TOUCHSCREEN_VIRTUAL_KEYS
	ts_key_report_synaptics_init();
	#endif

	return 0;

err_input_register_device_failed:
	input_free_device(ts->input_dev);

err_alloc_dev_failed:
err_pdt_read_failed:
                
              if(ts != NULL)
              {
                        kfree(ts);
              }
err_check_functionality_failed:
	gpio_free(GPIO_TOUCH_EN_1V8_OUT);
	gpio_free(GPIO_TOUCH_EN_2V8_OUT);
	return ret;
}

static int synaptics_rmi4_remove(struct i2c_client *client)
{
       struct synaptics_rmi4 *ts = i2c_get_clientdata(client);
	unregister_early_suspend(&ts->early_suspend);
	if (ts->use_irq)
		free_irq(client->irq, ts);
	else
		hrtimer_cancel(&ts->timer);
	
	input_unregister_device(ts->input_dev);
       if(ts != NULL)
       {
                kfree(ts);
        }
	   
	return 0;
}

static int synaptics_rmi4_suspend(struct i2c_client *client, pm_message_t mesg)
{
       int ret;
	struct synaptics_rmi4 *ts = i2c_get_clientdata(client);

	if (ts->use_irq)
       {
		disable_irq(client->irq);
	}
	else
	{
		hrtimer_cancel(&ts->timer);
	}
	ret = cancel_work_sync(&ts->work);
	if (ret && ts->use_irq) /* if work was pending disable-count is now 2 */
	{
	    printk("[TSP] cancel_work_sync ret=%d",ret);
	    enable_irq(client->irq);
	}
				printk("[TSP] : f01.ctrl_base=0x%x\n",ts->f01.ctrl_base);

//synaptics_i2c_write(ts->client, 0x36, 0);     /* disable interrupt, */
synaptics_i2c_write(client, ts->f01.ctrl_base, 0x01);      /* deep sleep */

	ts->enable = 0;

	return 0;
}

static int synaptics_rmi4_resume(struct i2c_client *client)
{
    struct synaptics_rmi4 *ts = i2c_get_clientdata(client);
    
synaptics_i2c_write(client, ts->f01.ctrl_base, 0x0);      /* deep sleep */
				printk("[TSP] : f01.ctrl_base=0x%x\n",ts->f01.ctrl_base);

	if (ts->use_irq)
	{
	    enable_irq(ts->client->irq);
	    //synaptics_i2c_write(ts->client, 0x36, 4);     /* enable interrupt, */
	}
	else
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);

	ts->enable = 1;

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_rmi4_early_suspend(struct early_suspend *h)
{
	struct synaptics_rmi4 *ts;
	ts = container_of(h, struct synaptics_rmi4, early_suspend);
	synaptics_rmi4_suspend(ts->client, PMSG_SUSPEND);
}

static void synaptics_rmi4_late_resume(struct early_suspend *h)
{
	struct synaptics_rmi4 *ts;
	ts = container_of(h, struct synaptics_rmi4, early_suspend);
	synaptics_rmi4_resume(ts->client);
}
#endif


static const struct i2c_device_id synaptics_rmi4_id[] = {
	{ "synaptics-rmi-ts", 0 },
	{ }
};

static struct i2c_driver synaptics_rmi4_driver = {
	.probe		= synaptics_rmi4_probe,
	.remove		= synaptics_rmi4_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= synaptics_rmi4_suspend,
	.resume		= synaptics_rmi4_resume,
#endif
	.id_table	= synaptics_rmi4_id,
	.driver = {
		.name	= "synaptics-rmi-ts",
	},
};

static int __devinit synaptics_rmi4_init(void)
{
	synaptics_wq = create_singlethread_workqueue("synaptics_wq");
	if (!synaptics_wq)
	{
		printk(KERN_ERR "[TSP] Could not create work queue synaptics_wq: no memory");
		return -ENOMEM;
	}

	return i2c_add_driver(&synaptics_rmi4_driver);
}

static void __exit synaptics_rmi4_exit(void)
{
	i2c_del_driver(&synaptics_rmi4_driver);
	if (synaptics_wq)
		destroy_workqueue(synaptics_wq);
}

module_init(synaptics_rmi4_init);
module_exit(synaptics_rmi4_exit);

MODULE_DESCRIPTION("Synaptics RMI4 Driver");
MODULE_LICENSE("GPL");
