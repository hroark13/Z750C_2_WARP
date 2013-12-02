/* drivers/input/touchscreen/gt816_818.c
 * 
 * 2010 - 2012 Goodix Technology.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be a reference 
 * to you, when you are integrating the GOODiX's CTP IC into your system, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details.
 * 
 * Version:1.2
 * Author:andrew@goodix.com
 * Release Date:2012/06/08
 * Revision record:
 *      V1.0:2012/05/01,create file.
 *      V1.2:2012/06/08,add slot report mode;
 *                      modify some return value.
 */

#include <linux/irq.h>
#include "gt816_818.h"
#include <../../../../arch/arm/plat-samsung/include/plat/gpio-cfg.h>   //haoweiwei add
#if GTP_ICS_SLOT_REPORT
    #include <linux/input/mt.h>
#endif

static const char *goodix_ts_name = "Goodix Capacitive TouchScreen";
static struct workqueue_struct *goodix_wq;
struct i2c_client * i2c_connect_client = NULL; 
static u8 config[GTP_CONFIG_LENGTH + GTP_ADDR_LENGTH]
                = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};

#if GTP_HAVE_TOUCH_KEY
	static const u16 touch_key_array[] = GTP_KEY_TAB;
	#define GTP_MAX_KEY_NUM	 (sizeof(touch_key_array)/sizeof(touch_key_array[0]))
#endif

static s8 gtp_i2c_test(struct i2c_client *client);	
#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h);
static void goodix_ts_late_resume(struct early_suspend *h);
#endif
 
#if GTP_CREATE_WR_NODE
extern s32 init_wr_node(struct i2c_client*);
extern void uninit_wr_node(void);
#endif

#if GTP_AUTO_UPDATE
extern s32 gt818_enter_update_mode(struct i2c_client *client);
extern void gt818_leave_update_mode(void);
extern s32 gt818_update_proc(void *dir);
extern s32 gt818x_enter_update_mode(struct i2c_client *client);
extern void gt818x_leave_update_mode(void);
extern s32 gt818x_update_proc(void *dir);

s32 (*gup_enter_update_mode)(struct i2c_client *client) = gt818x_enter_update_mode;
void (*gup_leave_update_mode)(void) = gt818x_leave_update_mode;
s32 (*gup_update_proc)(void *dir) = gt818x_update_proc;

extern u8 gt818_init_update_proc(struct goodix_ts_data *);
extern u8 gt818x_init_update_proc(struct goodix_ts_data *);
#endif

#if GTP_ESD_PROTECT
static struct delayed_work gtp_esd_check_work;
static struct workqueue_struct * gtp_esd_check_workqueue = NULL;
static void gtp_esd_check_func(struct work_struct *);
#endif

/*******************************************************	
Function:
	Read data from the i2c slave device.

Input:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:read data buffer.
	len:operate length.
	
Output:
	numbers of i2c_msgs to transfer
*********************************************************/
s32 gtp_i2c_read(struct i2c_client *client, u8 *buf, s32 len)
{
    struct i2c_msg msgs[2];
    s32 ret=-1;
    s32 retries = 0;

    GTP_DEBUG_FUNC();

    msgs[0].flags = !I2C_M_RD;
    msgs[0].addr  = client->addr;
    msgs[0].len   = GTP_ADDR_LENGTH;
    msgs[0].buf   = &buf[0];

    msgs[1].flags = I2C_M_RD;
    msgs[1].addr  = client->addr;
    msgs[1].len   = len - GTP_ADDR_LENGTH;
    msgs[1].buf   = &buf[GTP_ADDR_LENGTH];

    while(retries < 5)
    {
        ret = i2c_transfer(client->adapter, msgs, 2);
        if (ret == 2)break;
        retries++;
    }
    return ret;
}

/*******************************************************	
Function:
	write data to the i2c slave device.

Input:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:write data buffer.
	len:operate length.
	
Output:
	numbers of i2c_msgs to transfer.
*********************************************************/
s32 gtp_i2c_write(struct i2c_client *client,u8 *buf,s32 len)
{
    struct i2c_msg msg;
    s32 ret=-1;
    s32 retries = 0;

    GTP_DEBUG_FUNC();

    msg.flags = !I2C_M_RD;
    msg.addr  = client->addr;
    msg.len   = len;
    msg.buf   = buf;

    while(retries < 5)
    {
        ret = i2c_transfer(client->adapter, &msg, 1);
        if (ret == 1)break;
        retries++;
    }

    return ret;
}

/*******************************************************
Function:	Send a prefix command	
Parameters:	ts: client private data structure	
return:	Results of the implementation code, 0 for normal execution
*******************************************************/
s32 gtp_i2c_pre_cmd(struct i2c_client *client)
{   
    struct goodix_ts_data* ts = i2c_get_clientdata(client);      //haoweiwei add
    s32 ret = -1;
    u8 pre_cmd_data[2]={0x0f, 0xff};

    GTP_DEBUG_FUNC();
	if(GT818 != ts->chip_type)
    {
		return 0;
    }
    ret = gtp_i2c_write(client, pre_cmd_data, 2);

    return ret;
}

/*******************************************************	
Function:
	write i2c end cmd.

Input:
	client:	i2c device.
	
Output:
	numbers of i2c_msgs to transfer.
*********************************************************/
s32 gtp_i2c_end_cmd(struct i2c_client *client)
{
    s32 ret = -1;
    u8 end_cmd_data[2]={0x80, 0x00}; 
    
    GTP_DEBUG_FUNC();

    ret = gtp_i2c_write(client, end_cmd_data, 2);

    return ret;
}

/*******************************************************
Function:
	Send config Function.

Input:
	client:	i2c client.

Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
s32 gtp_send_cfg(struct i2c_client *client)
{
    s32 ret = -1;
    s32 retry = 0;

    for (retry = 0; retry < 5; retry++)
    {
        gtp_i2c_pre_cmd(client);
        ret = gtp_i2c_write(client, config , GTP_CONFIG_LENGTH + GTP_ADDR_LENGTH);        
        gtp_i2c_end_cmd(client);

        if (ret > 0)
        {
            break;
        }
    }

    return ret;
}

/*******************************************************
Function:
	Enable IRQ Function.

Input:
	ts:	i2c client private struct.
	
Output:
	None.
*******************************************************/
void gtp_irq_disable(struct goodix_ts_data *ts)
{	
    unsigned long irqflags;
	
    GTP_DEBUG_FUNC();
	
    spin_lock_irqsave(&ts->irq_lock, irqflags);
    if (!ts->irq_is_disable)
    {
        ts->irq_is_disable = 1; 
        disable_irq_nosync(ts->client->irq);
    }
    spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

/*******************************************************
Function:
	Disable IRQ Function.

Input:
	ts:	i2c client private struct.
	
Output:
	None.
*******************************************************/
void gtp_irq_enable(struct goodix_ts_data *ts)
{	
    unsigned long irqflags;
	
    GTP_DEBUG_FUNC();
		
    spin_lock_irqsave(&ts->irq_lock, irqflags);
    if (ts->irq_is_disable) 
    {
        enable_irq(ts->client->irq);
        ts->irq_is_disable = 0;	
    }
    spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

/*******************************************************
Function:
	Touch down report function.

Input:
	ts:private data.
	id:tracking id.
	x:input x.
	y:input y.
	w:input weight.
	
Output:
	None.
*******************************************************/
static void gtp_touch_down(struct goodix_ts_data* ts,s32 id,s32 x,s32 y,s32 w)
{
#if GTP_CHANGE_X2Y
    GTP_SWAP(x, y);
#endif

#if GTP_ICS_SLOT_REPORT                                        //slot模式，android上报模式
//    input_report_key(ts->input_dev,BTN_TOUCH, 1);          //haoweiwei add
    input_mt_slot(ts->input_dev, id);
    input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
//    input_report_abs(ts->input_dev, ABS_MT_PRESSURE, w);      //haoweiwei delect to slove 
#else
//    input_report_key(ts->input_dev,BTN_TOUCH, 1);          //haoweiwei add
    input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);
    input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
    input_mt_sync(ts->input_dev);
#endif

//    GTP_DEBUG("ID:%d, X:%d, Y:%d, W:%d", id, x, y, w);     //haoweiwei delect
}

/*******************************************************
Function:
	Touch up report function.

Input:
	ts:private data.
	
Output:
	None.
*******************************************************/
static void gtp_touch_up(struct goodix_ts_data* ts, s32 id)
{
#if GTP_ICS_SLOT_REPORT
    input_mt_slot(ts->input_dev, id);
    input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, -1);
    GTP_DEBUG("Touch id[%2d] release!", id);
#else
    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
    input_mt_sync(ts->input_dev);
#endif
}

/*******************************************************
Function:
	Goodix touchscreen work function.

Input:
	work:	work_struct of goodix_wq.
	
Output:
	None.
*******************************************************/
static void goodix_ts_work_func(struct work_struct *work)
{
    u8  index_data[3] = {(u8)(GTP_REG_INDEX>>8),(u8)GTP_REG_INDEX,0};
    u8  point_data[2 + 1 + 8 * GTP_MAX_TOUCH] = {GTP_READ_COOR_ADDR>>8, GTP_READ_COOR_ADDR & 0xFF};  
    u8  touch_num = 0;
    static u8 pre_touch = 0;
    u8  key_value = 0;
    u8* coor_data = NULL;
    s32 input_x = 0;
    s32 input_y = 0;
    s32 input_w = 0;
    s32 input_id = 0;
    s32 idx = 0;
    s32 ret = -1;
    struct goodix_ts_data *ts = NULL;

    GTP_DEBUG_FUNC();
    
    ts = container_of(work, struct goodix_ts_data, work);
    if (ts->enter_update)
    {
        goto exit_work_func;
    }

    gtp_i2c_pre_cmd(ts->client);
    ret = gtp_i2c_read(ts->client, index_data, 3);
    if (ret < 0)
    {
        GTP_ERROR("I2C transfer error. errno:%d\n ", ret);
        goto exit_work_func;
    }
	
    touch_num = index_data[GTP_ADDR_LENGTH] & 0x0f;
    if(touch_num > 5)
    {
        touch_num = 5;
    }
    ret = gtp_i2c_read(ts->client, point_data, 2 + 8 * touch_num + 1);
    if(ret < 0)	
    {
        GTP_ERROR("I2C transfer error. Number:%d\n ", ret);
        goto exit_work_func;
    }
    gtp_i2c_end_cmd(ts->client);

    GTP_DEBUG_ARRAY(index_data, 3);
    GTP_DEBUG("touch num:%x", touch_num);
    GTP_DEBUG_ARRAY(point_data, 2 + 8 * touch_num + 1);

    if ((index_data[GTP_ADDR_LENGTH] & 0x0f) == 0x0f)
    {
        ret = gtp_send_cfg(ts->client);
        if (ret < 0)
        {
            GTP_DEBUG("Reload config failed!\n");
        }
        goto exit_work_func;
    }
    
    if ((index_data[GTP_ADDR_LENGTH] & 0x30) != 0x20)
    {
        GTP_INFO("Data not ready!");
        goto exit_work_func;
    }

#if GTP_HAVE_TOUCH_KEY
    key_value = point_data[GTP_ADDR_LENGTH] & 0x0f;

    for (idx = 0; idx < GTP_MAX_KEY_NUM; idx++)
    {
        input_report_key(ts->input_dev, touch_key_array[idx], key_value & (0x01<<idx));   
    }
#endif
    coor_data = &point_data[3];

#if GTP_ICS_SLOT_REPORT
    if (pre_touch || touch_num)
    {
        s32 pos = 0;

        for (idx = 0; idx < GTP_MAX_TOUCH; idx++)
        {
            input_id = coor_data[pos] - 1;
            if (input_id == idx)
            {
                input_x = (coor_data[pos + 2] << 8) | coor_data[pos + 1];
                input_y = (coor_data[pos + 4] << 8) | coor_data[pos + 3];
                input_w = 200;

                pos += 8;

                gtp_touch_down(ts, idx, input_x, input_y, input_w);
                pre_touch |= 0x01 << idx;
            }
            else if (pre_touch & (0x01 << idx))
            {
                gtp_touch_up(ts, idx);
                pre_touch &= ~(0x01 << idx);
				//pre_touch &= ^(0x01 << idx);
            }
        }
    }

#else
    if (touch_num)
    {
        s32 pos = 0;
        
        for (idx = 0; idx < touch_num; idx++)
        {
            input_id = coor_data[pos] - 1;
            input_x = (coor_data[pos + 2] << 8) | coor_data[pos + 1];
            input_y = (coor_data[pos + 4] << 8) | coor_data[pos + 3];
            input_w = 20;

            pos += 8;

            if ((input_x > ts->abs_x_max)||(input_y > ts->abs_y_max))
            {
                continue;
            }
            gtp_touch_down(ts, input_id, input_x, input_y, input_w);
        }
    }
    else if (pre_touch)
    {
        GTP_DEBUG("Touch Release!");
        gtp_touch_up(ts, 0);
    }
	
    pre_touch = touch_num;
    input_report_key(ts->input_dev, BTN_TOUCH, (touch_num || key_value));
#endif

    input_sync(ts->input_dev);

exit_work_func:
    if (ts->use_irq)
    {
        gtp_irq_enable(ts);
    }
}


/*******************************************************
Function:
	Timer interrupt service routine.

Input:
	timer:	timer struct pointer.
	
Output:
	Timer work mode. HRTIMER_NORESTART---not restart mode
*******************************************************/
static enum hrtimer_restart goodix_ts_timer_handler(struct hrtimer *timer)
{
    struct goodix_ts_data *ts = container_of(timer, struct goodix_ts_data, timer);
	
    GTP_DEBUG_FUNC();

    queue_work(goodix_wq, &ts->work);
    hrtimer_start(&ts->timer, ktime_set(0, (GTP_POLL_TIME+6)*1000000), HRTIMER_MODE_REL);
    return HRTIMER_NORESTART;
}

/*******************************************************
Function:
	External interrupt service routine.

Input:
	irq:	interrupt number.
	dev_id: private data pointer.
	
Output:
	irq execute status.
*******************************************************/
static irqreturn_t goodix_ts_irq_handler(int irq, void *dev_id)
{
    struct goodix_ts_data *ts = dev_id;

    GTP_DEBUG_FUNC();

    gtp_irq_disable(ts);
    queue_work(goodix_wq, &ts->work);

    return IRQ_HANDLED;
}

/*******************************************************
Function:
	Reset chip Function.

Input:
	ms:reset time.
	
Output:
	None.
*******************************************************/
void gtp_reset_guitar(s32 ms)
{
    GTP_DEBUG_FUNC();
    GTP_GPIO_OUTPUT(GTP_RST_PORT, 0);
    msleep(ms);
	GTP_GPIO_OUTPUT(GTP_RST_PORT, 1);
    //GTP_GPIO_AS_INPUT(GTP_RST_PORT);
    msleep(50);

    return;
}

/*******************************************************
Function:
	Eter sleep function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_enter_sleep(struct goodix_ts_data * ts)
{
    s8 ret = -1;
    s8 retry = 0;
    u8 i2c_control_buf[3] = {(u8)(GTP_REG_SLEEP >> 8), (u8)GTP_REG_SLEEP, 0x01};

    GTP_DEBUG_FUNC();

    while(retry++ < 5)
    {
        gtp_i2c_pre_cmd(ts->client);
        ret = gtp_i2c_write(ts->client, i2c_control_buf, 3);
        gtp_i2c_end_cmd(ts->client);
        if (ret > 0)
        {
            GTP_DEBUG("GTP enter sleep!");
            return ret;
        }
        msleep(10);
    }
    GTP_ERROR("GTP send sleep cmd failed.");
    return ret;
}

/*******************************************************
Function:
	Wakeup from sleep mode Function.

Input:
	ts:	private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_wakeup_sleep(struct goodix_ts_data * ts)
{
    u8 retry = 0;
    s8 ret = -1;

    GTP_DEBUG_FUNC();

#if GTP_POWER_CTRL_SLEEP
    while(retry++ < 5)
    {
        gtp_reset_guitar(20);
        ret = gtp_send_cfg(ts->client);
        if (ret > 0)
        {
            GTP_DEBUG("Wakeup sleep send config success.");
            return ret;
        }
    }
#else
    GTP_GPIO_OUTPUT(GTP_INT_PORT, 0);
    msleep(2);
    GTP_GPIO_AS_INPUT(GTP_INT_PORT);
    msleep(2);
    GTP_GPIO_OUTPUT(GTP_INT_PORT, 0);
    msleep(2);
    GTP_GPIO_AS_INT(GTP_INT_PORT);
    msleep(50);
    while(retry++ < 10)
    {
        ret = gtp_i2c_test(ts->client);
        if (ret > 0)
        {
            GTP_DEBUG("GTP wakeup sleep.");
            return ret;
        }
        gtp_reset_guitar(20);
    }
#endif

    GTP_ERROR("GTP wakeup sleep failed.");
    return ret;
}

s32 gt868_get_sensor_id(struct i2c_client *client, u8* id)
{
    u8 buf[8];
    u8 i;
    u8 Count;
 
        //设置SensorID为输入
    buf[0] = 0x16;
    buf[1] = 0x00;
    gtp_i2c_read(client, buf, 3);
    buf[2] &= 0xfd;
    gtp_i2c_write(client, buf, 3);
    
    //设置SensorID为上拉,关掉SensorID下拉
    buf[0] = 0x16;
    buf[1] = 0x06;
    gtp_i2c_read(client, buf, 4);
    buf[2] |= 0x02;
    buf[3] &= 0xFD;
    gtp_i2c_write(client, buf, 4);
    
    msleep(1);
    Count = 0;
    for(i = 0;i < 200;i++)
    {
            buf[0] = 0x16;
            buf[1] = 0x02;   
            gtp_i2c_read(client, buf, 3);
            buf[2] &= 0x02;
            if (buf[2] == 0)
            {
                    Count++;
            }
      }
      if(Count >= 100)
      {
            *id = 2;    
            goto SENSOR_ID_NONC;
      }
 
    //设置SensorID为下拉,关掉SensorID上拉
    buf[0] = 0x16;
    buf[1] = 0x06;
    gtp_i2c_read(client, buf, 4);
    buf[2] &= 0xFD;
    buf[3] |= 0x02;
    gtp_i2c_write(client, buf, 4);
 
        msleep(1);
    Count = 0;
    for(i = 0;i < 200;i++)
    {
            buf[0] = 0x16;
            buf[1] = 0x02;   
            gtp_i2c_read(client, buf, 3);
            buf[2] &= 0x02;
            if (buf[2] != 0)
            {
                    Count++;
            }
      }
      if(Count >= 100)
      {
            *id = 1;    
            goto SENSOR_ID_NONC;
      }
      
    *id = 0;
    goto SENSOR_ID_NC;
    
SENSOR_ID_NONC:
        //如果外部有强上下拉，内部将SensorID设置成悬浮输入态
    buf[0] = 0x16;
    buf[1] = 0x06;   
    gtp_i2c_read(client, buf, 4);
    buf[2] &= 0xfd;
    buf[3] &= 0xfd;
    gtp_i2c_write(client, buf, 4);
 
SENSOR_ID_NC:
    return 0;
}
/*******************************************************
Function:
	GTP initialize function.

Input:
	ts:	i2c client private struct.
	
Output:
	Executive outcomes.0---succeed.
*******************************************************/
static s32 gtp_init_panel(struct goodix_ts_data *ts)
{
    s32 ret = -1;
#if GTP_DRIVER_SEND_CFG
    u8 rd_cfg_buf[16];

    u8 gt818_cfg_info_group1[] = GT818_CTP_CFG_GROUP1;
    u8 gt818_cfg_info_group2[] = GT818_CTP_CFG_GROUP2;
    u8 gt818_cfg_info_group3[] = GT818_CTP_CFG_GROUP3;

    u8 gt818x_cfg_info_group1[] = GT818X_CTP_CFG_GROUP1;
    u8 gt818x_cfg_info_group2[] = GT818X_CTP_CFG_GROUP2;
    u8 gt818x_cfg_info_group3[] = GT818X_CTP_CFG_GROUP3;

	/*haoweiwei add 调频功能*/
//	  u8 gt818x_cfg_info_group1_charge[] = GT818X_CTP_CFG_GROUP1_CHARGE;
//    u8 gt818x_cfg_info_group2_charge[] = GT818X_CTP_CFG_GROUP2_CHARGE;
//    u8 gt818x_cfg_info_group3_charge[] = GT818X_CTP_CFG_GROUP3_CHARGE;

    u8 gt868_cfg_info_group1[] = GT868_CTP_CFG_GROUP1;
    u8 gt868_cfg_info_group2[] = GT868_CTP_CFG_GROUP2;
    u8 gt868_cfg_info_group3[] = GT868_CTP_CFG_GROUP3;
    u8 *send_cfg_buf[3];
    u8 cfg_info_len[3];
    if(GT818 == ts->chip_type)
    {
        send_cfg_buf[0] = gt818_cfg_info_group1;
        send_cfg_buf[1] = gt818_cfg_info_group2;
        send_cfg_buf[2] = gt818_cfg_info_group3;
        cfg_info_len[0] = sizeof(gt818_cfg_info_group1)/sizeof(gt818_cfg_info_group1[0]);
        cfg_info_len[1] = sizeof(gt818_cfg_info_group2)/sizeof(gt818_cfg_info_group2[0]);
        cfg_info_len[2] = sizeof(gt818_cfg_info_group3)/sizeof(gt818_cfg_info_group3[0]);       
    }
    else if(GT818X == ts->chip_type)
    {
        send_cfg_buf[0] = gt818x_cfg_info_group1;
        send_cfg_buf[1] = gt818x_cfg_info_group2;
        send_cfg_buf[2] = gt818x_cfg_info_group3;

        cfg_info_len[0] = sizeof(gt818x_cfg_info_group1)/sizeof(gt818x_cfg_info_group1[0]);
        cfg_info_len[1] = sizeof(gt818x_cfg_info_group2)/sizeof(gt818x_cfg_info_group2[0]);
        cfg_info_len[2] = sizeof(gt818x_cfg_info_group3)/sizeof(gt818x_cfg_info_group3[0]);
    }
    else 
    {
        send_cfg_buf[0] = gt868_cfg_info_group1;
        send_cfg_buf[1] = gt868_cfg_info_group2;
        send_cfg_buf[2] = gt868_cfg_info_group3;
        cfg_info_len[0] = sizeof(gt868_cfg_info_group1)/sizeof(gt868_cfg_info_group1[0]);
        cfg_info_len[1] = sizeof(gt868_cfg_info_group2)/sizeof(gt868_cfg_info_group2[0]);
        cfg_info_len[2] = sizeof(gt868_cfg_info_group3)/sizeof(gt868_cfg_info_group3[0]);       
    }
    
    GTP_DEBUG("len1=%d,len2=%d,len3=%d",cfg_info_len[0],cfg_info_len[1],cfg_info_len[2]);
    if ((!cfg_info_len[1]) && (!cfg_info_len[2]))
    {	
        rd_cfg_buf[GTP_ADDR_LENGTH] = 0; 
    }
    else
    {
        if (GT868 == ts->chip_type)
        {
            GTP_GPIO_OUTPUT(GTP_INT_PORT, 0);
            msleep(2);      
            gtp_reset_guitar(5);
            
            gt868_get_sensor_id(ts->client,&rd_cfg_buf[GTP_ADDR_LENGTH]);

            GTP_GPIO_AS_INT(GTP_INT_PORT);
            msleep(5);
            gtp_reset_guitar(5);            
    }
    else
    {
        rd_cfg_buf[0] = GTP_REG_SENSOR_ID >> 8;
        rd_cfg_buf[1] = GTP_REG_SENSOR_ID & 0xff;
        gtp_i2c_pre_cmd(ts->client);
        ret = gtp_i2c_read(ts->client, rd_cfg_buf, 3);
        gtp_i2c_end_cmd(ts->client);
        if (ret < 0)
        {
            GTP_ERROR("Read SENSOR ID failed,default use group1 config!");
            rd_cfg_buf[GTP_ADDR_LENGTH] = 0;
        }
        rd_cfg_buf[GTP_ADDR_LENGTH] &= 0x03;
    }
    }
    GTP_DEBUG("SENSOR ID:%d", rd_cfg_buf[GTP_ADDR_LENGTH]);
    memcpy(&config[GTP_ADDR_LENGTH], send_cfg_buf[rd_cfg_buf[GTP_ADDR_LENGTH]], GTP_CONFIG_LENGTH);

#if GTP_CUSTOM_CFG
    config[RESOLUTION_LOC]     = (u8)(GTP_MAX_WIDTH);
    config[RESOLUTION_LOC + 1] = (u8)(GTP_MAX_WIDTH>>8);
    config[RESOLUTION_LOC + 2] = (u8)GTP_MAX_HEIGHT;
    config[RESOLUTION_LOC + 3] = (u8)(GTP_MAX_HEIGHT>>8);
#endif  //endif GTP_CUSTOM_CFG
    
    if (GTP_INT_TRIGGER == 0)  //FALLING
    {
        config[TRIGGER_LOC] &= 0xf7; 
    }
    else if (GTP_INT_TRIGGER == 1)  //RISING
    {
        config[TRIGGER_LOC] |= 0x08;
    }

#else //else DRIVER NEED NOT SEND CONFIG

    ret = gtp_i2c_read(ts->client, config, GTP_CONFIG_LENGTH + GTP_ADDR_LENGTH);
    gtp_i2c_end_cmd(ts->client);
    if (ret < 0)
    {
        GTP_ERROR("GTP read resolution & max_touch_num failed, use default value!");
        ts->abs_x_max = GTP_MAX_WIDTH;
        ts->abs_y_max = GTP_MAX_HEIGHT;
        ts->int_trigger_type = GTP_INT_TRIGGER;
    }
#endif //endif GTP_DRIVER_SEND_CFG
    GTP_DEBUG_FUNC();

    ts->abs_x_max = (config[RESOLUTION_LOC + 1] << 8) + config[RESOLUTION_LOC];
    ts->abs_y_max = (config[RESOLUTION_LOC + 3] << 8) + config[RESOLUTION_LOC + 2];
    ts->int_trigger_type = (config[TRIGGER_LOC] >> 3) & 0x01;
    if ((!ts->abs_x_max)||(!ts->abs_y_max))
    {
        GTP_ERROR("GTP resolution & max_touch_num invalid, use default value!");
        ts->abs_x_max = GTP_MAX_WIDTH;
        ts->abs_y_max = GTP_MAX_HEIGHT;
    }

    ret = gtp_send_cfg(ts->client);
    if (ret < 0)
    {
        GTP_ERROR("Send config error. ret = %d", ret);
    }

    GTP_DEBUG("X_MAX = %d,Y_MAX = %d,TRIGGER = 0x%02x",
             ts->abs_x_max,ts->abs_y_max,ts->int_trigger_type);

    msleep(10);

    return 0;
}

/*******************************************************
Function:
	Read goodix touchscreen version function.

Input:
	client:	i2c client struct.
	version:address to store version info
	
Output:
	Executive outcomes.0---succeed.
*******************************************************/
s32 gtp_read_version(struct i2c_client *client, u16* version)
{
    s32 ret = -1;
    u8 buf[8] = {GTP_REG_VERSION >> 8, GTP_REG_VERSION & 0xff};
    struct goodix_ts_data* ts = i2c_get_clientdata(client);

    GTP_DEBUG_FUNC();

    gtp_i2c_pre_cmd(client);
    ret = gtp_i2c_read(client, buf, 6);
    gtp_i2c_end_cmd(client);
    if (ret < 0)
    {
        GTP_ERROR("GTP read version failed"); 
        return ret;
    }

    if (version)
    {
        *version = (buf[5] << 8) | buf[4];
    }

    GTP_INFO("IC VERSION:%02x%02x_%02x%02x", buf[3], buf[2], buf[5], buf[4]);
    if(0x68 == buf[3])
    {
        ts->chip_type = GT868;
        GTP_INFO("Chip_type: GT868/GT968M\n");
    }
    else
    { 
        if(buf[2] >= 0xF0)
        {
            ts->chip_type = GT818X;
            GTP_INFO("Chip_type: GT818X\n");
        }
        else
        {
            ts->chip_type = GT818;
            GTP_INFO("Chip_type: GT818/GT816\n");
        }
    }

    return ret;
}

/*******************************************************
Function:
	I2c test Function.

Input:
	client:i2c client.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_i2c_test(struct i2c_client *client)
{
    u8 retry = 0;
    s8 ret = -1;
  
    GTP_DEBUG_FUNC();
  
    while(retry++ < 5)
    {
        gtp_i2c_pre_cmd(client);
        ret = gtp_i2c_end_cmd(client);
        if (ret > 0)
        {
            return ret;
        }
        GTP_ERROR("GTP i2c test failed time %d.",retry);
        msleep(10);
    }
    return ret;
}

/*******************************************************
Function:
	Request gpio Function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_request_io_port(struct goodix_ts_data *ts)
{
    s32 ret = 0;

    ret = GTP_GPIO_REQUEST(GTP_INT_PORT, "GTP_INT_IRQ");
    if (ret < 0) 
    {
        GTP_ERROR("Failed to request GPIO:%d, ERRNO:%d", (s32)GTP_INT_PORT, ret);
        ret = -ENODEV;
    }
    else
    {
        GTP_GPIO_AS_INT(GTP_INT_PORT);	
        ts->client->irq = GTP_INT_IRQ;
    }

    ret = GTP_GPIO_REQUEST(GTP_RST_PORT, "GTP_RST_PORT");
    if (ret < 0) 
    {
        GTP_ERROR("Failed to request GPIO:%d, ERRNO:%d",(s32)GTP_RST_PORT,ret);
        ret = -ENODEV;
    }
	GTP_GPIO_OUTPUT(GTP_RST_PORT, 1);
    //GTP_GPIO_AS_INPUT(GTP_RST_PORT);
    gtp_reset_guitar(20);
    
    if(ret < 0)
    {
        GTP_GPIO_FREE(GTP_RST_PORT);
        GTP_GPIO_FREE(GTP_INT_PORT);
    }

    return ret;
}

/*******************************************************
Function:
	Request irq Function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_request_irq(struct goodix_ts_data *ts)
{
    s32 ret = -1;
    const u8 irq_table[2] = GTP_IRQ_TAB;

    GTP_DEBUG("INT trigger type:%x", ts->int_trigger_type);

    ret  = request_irq(ts->client->irq, 
                       goodix_ts_irq_handler,
                       irq_table[ts->int_trigger_type],
                       ts->client->name,
                       ts);
    if (ret)
    {
        GTP_ERROR("Request IRQ failed!ERRNO:%d.", ret);
        GTP_GPIO_AS_INPUT(GTP_INT_PORT);
        GTP_GPIO_FREE(GTP_INT_PORT);

        hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        ts->timer.function = goodix_ts_timer_handler;
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
        return -1;
    }
    else 
    {
        gtp_irq_disable(ts);
        ts->use_irq = 1;
        return 0;
    }
}

/*******************************************************
Function:
	Request input device Function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_request_input_dev(struct goodix_ts_data *ts)
{
    s8 ret = -1;
    s8 phys[32];
    u16 report_max_x = 0;
    u16 report_max_y = 0;
#if GTP_HAVE_TOUCH_KEY
    u8 index = 0;
#endif
  
    GTP_DEBUG_FUNC();
  
    ts->input_dev = input_allocate_device();
    if (ts->input_dev == NULL)
    {
        GTP_ERROR("Failed to allocate input device.");
        return -ENOMEM;
    }

    ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
    ts->input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);
#if GTP_ICS_SLOT_REPORT
    __set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);
    input_mt_init_slots(ts->input_dev, 255);
#else
    ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
#endif

#if GTP_HAVE_TOUCH_KEY
    for (index = 0; index < GTP_MAX_KEY_NUM; index++)
    {
        input_set_capability(ts->input_dev,EV_KEY,touch_key_array[index]);	
    }
#endif

    report_max_x = ts->abs_x_max;
    report_max_y = ts->abs_y_max;
#if GTP_CHANGE_X2Y
    GTP_SWAP(report_max_x, report_max_y);
#endif

    input_set_abs_params(ts->input_dev, ABS_X, 0, report_max_x, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_Y, 0, report_max_y, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, report_max_x, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, report_max_y, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
//    input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);	  //haoweiwei delect
    input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, 255, 0, 0);

    sprintf(phys, "input/ts");
    ts->input_dev->name = goodix_ts_name;
    ts->input_dev->phys = phys;
    ts->input_dev->id.bustype = BUS_I2C;
    ts->input_dev->id.vendor = 0xDEAD;
    ts->input_dev->id.product = 0xBEEF;
    ts->input_dev->id.version = 10427;
	
    ret = input_register_device(ts->input_dev);
    if (ret)
    {
        GTP_ERROR("Register %s input device failed", ts->input_dev->name);
        return -ENODEV;
    }
    
#ifdef CONFIG_HAS_EARLYSUSPEND
    ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    ts->early_suspend.suspend = goodix_ts_early_suspend;
    ts->early_suspend.resume = goodix_ts_late_resume;
    register_early_suspend(&ts->early_suspend);
#endif

    return 0;
}

/*******************************************************
Function:
	Goodix touchscreen probe function.

Input:
	client:	i2c device struct.
	id:device id.
	
Output:
	Executive outcomes. 0---succeed.
*******************************************************/
static int goodix_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    s32 ret = -1;
    struct goodix_ts_data *ts;
    u16 version_info;

    GTP_DEBUG_FUNC();
    client->addr = 0x5d;

    //do NOT remove these output log
    GTP_INFO("GTP Driver Version:%s",GTP_DRIVER_VERSION);
    GTP_INFO("GTP Driver build@%s,%s", __TIME__,__DATE__);
    GTP_INFO("GTP I2C address:0x%02x", client->addr);

    i2c_connect_client = client;
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
    {
        GTP_ERROR("I2C check functionality failed.");
        return -ENODEV;
    }
    ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    if (ts == NULL)
    {
        GTP_ERROR("Alloc GFP_KERNEL memory failed.");
        return -ENOMEM;
    }
    
    memset(ts, 0, sizeof(*ts));
    INIT_WORK(&ts->work, goodix_ts_work_func);
    ts->client = client;
    i2c_set_clientdata(client, ts);
    spin_lock_init(&ts->irq_lock);

    ret = gtp_request_io_port(ts);
    if (ret < 0)
    {
        GTP_ERROR("GTP request IO port failed.");
        kfree(ts);
        return ret;
    }

    ret = gtp_i2c_test(client);
    if (ret < 0)
    {
        GTP_ERROR("I2C communication ERROR!");
    }

    ret = gtp_read_version(client, &version_info);
    if (ret < 0)
    {
        GTP_ERROR("Read version failed.");
    }
    else
    {
        GTP_INFO("IC VERSION:%04x", version_info);
    }

#if GTP_AUTO_UPDATE
	#if 0   //ZTEBSP BEGIN
    if(GT818 == ts->chip_type)
    {
    	
    	gup_enter_update_mode= gt818_enter_update_mode;         //haoweiwei add 
      gup_leave_update_mode= gt818_leave_update_mode;
      gup_update_proc= gt818_update_proc;
        ret = gt818_init_update_proc(ts);
    if (ret < 0)
    {
            GTP_ERROR("Create gt818 update thread error.");
        }
    }
	/*haoweiwei delect 818x fw updata*/
//	#if 0
    else if(GT818X == ts->chip_type)
    {
        ret = gt818x_init_update_proc(ts);
        if (ret < 0)
        {
            GTP_ERROR("Create gt818x update thread error.");
        }
    }
	#endif
	/*haoweiwei delect 818x fw updata*/
#endif  //ZTEBSP END

    ret = gtp_init_panel(ts);
    if (ret < 0)
    {
        GTP_ERROR("GTP init panel failed.");
    }

    ret = gtp_request_input_dev(ts);
    if (ret < 0)
    {
        GTP_ERROR("GTP request input dev failed");
    }
    
    ret = gtp_request_irq(ts); 
    if (ret < 0)
    {
        GTP_INFO("GTP works in polling mode.");
    }
    else
    {
        GTP_INFO("GTP works in interrupt mode.");
    }

    gtp_irq_enable(ts);

#if GTP_CREATE_WR_NODE
    init_wr_node(client);
#endif

#if GTP_ESD_PROTECT     //防静电，类似看门狗功能
    INIT_DELAYED_WORK(&gtp_esd_check_work, gtp_esd_check_func);
    gtp_esd_check_workqueue = create_workqueue("gtp_esd_check");
    queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE); 
#endif

    return 0;
}


/*******************************************************
Function:
	Goodix touchscreen driver release function.

Input:
	client:	i2c device struct.
	
Output:
	Executive outcomes. 0---succeed.
*******************************************************/
static int goodix_ts_remove(struct i2c_client *client)
{
    struct goodix_ts_data *ts = i2c_get_clientdata(client);
	
    GTP_DEBUG_FUNC();
	
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&ts->early_suspend);
#endif

#if GTP_CREATE_WR_NODE
    uninit_wr_node();
#endif

#if GTP_ESD_PROTECT
    destroy_workqueue(gtp_esd_check_workqueue);
#endif

    if (ts) 
    {
        if (ts->use_irq)
        {
            GTP_GPIO_AS_INPUT(GTP_INT_PORT);
            GTP_GPIO_FREE(GTP_INT_PORT);
            free_irq(client->irq, ts);
        }
        else
        {
            hrtimer_cancel(&ts->timer);
        }
    }	
	
    GTP_INFO("GTP driver is removing...");
    i2c_set_clientdata(client, NULL);
    input_unregister_device(ts->input_dev);
    kfree(ts);

    return 0;
}

/*******************************************************
Function:
	Early suspend function.

Input:
	h:early_suspend struct.
	
Output:
	None.
*******************************************************/
#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h)
{
    struct goodix_ts_data *ts;
    s8 ret = -1;	
    ts = container_of(h, struct goodix_ts_data, early_suspend);
	
    GTP_DEBUG_FUNC();

#if GTP_ESD_PROTECT
    ts->gtp_is_suspend = 1;
    cancel_delayed_work_sync(&gtp_esd_check_work);
#endif

    if (ts->use_irq)
    {
        gtp_irq_disable(ts);
    }
    else
    {
        hrtimer_cancel(&ts->timer);
    }
    ret = gtp_enter_sleep(ts);
    if (ret < 0)
    {
        GTP_ERROR("GTP early suspend failed.");
    }
}

/*******************************************************
Function:
	Late resume function.

Input:
	h:early_suspend struct.
	
Output:
	None.
*******************************************************/
static void goodix_ts_late_resume(struct early_suspend *h)
{
    struct goodix_ts_data *ts;
    s8 ret = -1;
    ts = container_of(h, struct goodix_ts_data, early_suspend);
	
    GTP_DEBUG_FUNC();
	
    ret = gtp_wakeup_sleep(ts);
    if (ret < 0)
    {
        GTP_ERROR("GTP later resume failed.");
    }

    if (ts->use_irq)
    {
        gtp_irq_enable(ts);
    }
    else
    {
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
    }

#if GTP_ESD_PROTECT
    ts->gtp_is_suspend = 0;
    queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE);
#endif
}
#endif

#if GTP_ESD_PROTECT
static void gtp_esd_check_func(struct work_struct *work)
{
    int i;
    int ret = -1;
    struct goodix_ts_data *ts = NULL;

    GTP_DEBUG_FUNC();
    
    ts = i2c_get_clientdata(i2c_connect_client);

    if (ts->gtp_is_suspend)
    {
        return;
    }

    for (i = 0; i < 3; i++)
    {
        ret = gtp_i2c_end_cmd(i2c_connect_client);
	    if (ret >= 0)
	    {
	        break;
	    }
	}
	
    if (i >= 3)
    {
        gtp_reset_guitar(50);
    }

    if(!ts->gtp_is_suspend)
    {
        queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE);
    }

    return;
}
#endif

static const struct i2c_device_id goodix_ts_id[] = {
    { GTP_I2C_NAME, 0 },
    { }
};

static struct i2c_driver goodix_ts_driver = {
    .probe      = goodix_ts_probe,
    .remove     = goodix_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
    .suspend    = goodix_ts_early_suspend,
    .resume     = goodix_ts_later_resume,
#endif
    .id_table   = goodix_ts_id,
    .driver = {
        .name     = GTP_I2C_NAME,
        .owner    = THIS_MODULE,
    },
};

/*******************************************************	
Function:
	Driver Install function.
Input:
  None.
Output:
	Executive Outcomes. 0---succeed.
********************************************************/
static int __devinit goodix_ts_init(void)
{
    s32 ret;

    GTP_DEBUG_FUNC();	
    GTP_INFO("GTP driver install.");
    goodix_wq = create_singlethread_workqueue("goodix_wq");
    if (!goodix_wq)
    {
        GTP_ERROR("Creat workqueue failed.");
        return -ENOMEM;
    }
    ret = i2c_add_driver(&goodix_ts_driver);
    return ret; 
}

/*******************************************************	
Function:
	Driver uninstall function.
Input:
  None.
Output:
	Executive Outcomes. 0---succeed.
********************************************************/
static void __exit goodix_ts_exit(void)
{
    GTP_DEBUG_FUNC();
    GTP_INFO("GTP driver exited.");
    i2c_del_driver(&goodix_ts_driver);
    if (goodix_wq)
    {
        destroy_workqueue(goodix_wq);
    }
}

late_initcall(goodix_ts_init);
module_exit(goodix_ts_exit);

MODULE_DESCRIPTION("GTP Series Driver");
MODULE_LICENSE("GPL");
