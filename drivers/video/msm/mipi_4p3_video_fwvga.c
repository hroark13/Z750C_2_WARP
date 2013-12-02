/* Copyright (c) 2008-2011, Code Aurora Forum. All rights reserved.
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

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_lead.h"
#include <mach/gpio.h>
#ifdef CONFIG_ZTE_PLATFORM
#include <mach/zte_memlog.h>
#endif

static struct dsi_buf fwvga_tx_buf;
static struct dsi_buf fwvga_rx_buf;
extern u32 LcdPanleID;
extern  unsigned int lcd_id_type;
extern void mipi_zte_set_backlight(struct msm_fb_data_type *mfd);
//[ECID 000000] zhangqi add for CABC begin
#ifdef CONFIG_BACKLIGHT_CABC
void mipi_set_backlight_4p3(struct msm_fb_data_type *mfd);
#endif
//[ECID 000000] zhangqi add for CABC end

// #ifdef CONFIG_FB_MSM_GPIO
#define GPIO_LCD_RESET 85
//[ECID:0000] ZTEBSP wangminrong start 20120517 for lcd jianrong 
//static  int lcd_gpio_config(void);
// #else
// #define GPIO_LCD_RESET 84
// #endif

// lcd_id0---1  lcd_id1---0    16 is tianma 
 // lcd_id0---0  lcd_id1---1   1 is yushun handstar 
// lcd_id0---1 lcd_id1---1     17 is lead handstar  
// lcd_id0---0  lcd_id1---0    0 is xinli handstar
#if 0
static  int lcd_gpio_config(void)
{
	int a,b,c;
	
	b = gpio_request(130,"lcd_id0");
	if(b)
		{
			
		printk("wangminrong request error\r\n");	
		}
	//gpio_request(128,"lcd_id1");
	
	//gpio_tlmm_config(GPIO_CFG(128, 0, GPIO_CFG_INPUT,
	//	GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
		
		c= gpio_tlmm_config(GPIO_CFG(130, 0, GPIO_CFG_INPUT,
		GPIO_CFG_PULL_UP, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
		if(c)
		{
			
		printk("wangminrong request error22\r\n");	
		}
	gpio_direction_input(130);
	//gpio_direction_input(128);
	
	a = gpio_get_value(130);
	printk("wangminrong a is %d\r\n",a);
	//b = gpio_get_value(128);
	//c = (a << 4) | b;
	
	//printk("wangminrong b is %d\r\n",b);
	
	
	//gpio_free(130);
	//gpio_free(128);
	return  a;
}
#endif
//[ECID:0000] ZTEBSP wangminrong end 20120517 for lcd jianrong 
//[ECID:0000] ZTEBSP wangminrong start 20120411 for lcd jianrong 
/*ic define*/
#define HIMAX_8363 		1
#define HIMAX_8369 		2
#define NOVATEK_35510	3
#define RENESAS_R61408	4
/* [ECID:000000] ZTEBSP wangjianping, 20120904 add OTM8009A LCD IC driver, start */
#define ORISE_OTM8009A  5
/* [ECID:000000] ZTEBSP wangjianping, 20120904 add OTM8009A LCD IC driver, end */

#define HIMAX8369_TIANMA_TN_ID		0xB1
#define HIMAX8369_TIANMA_IPS_ID		0xA5
#define HIMAX8369_LEAD_ID				0
#define HIMAX8369_LEAD_HANNSTAR_ID	0x88
#define NT35510_YUSHUN_ID				0
#define NT35510_LEAD_ID				0xA0

/*about icchip sleep and display on */
static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};
static char exit_sleep[2] = {0x11, 0x00};
static char display_on[2] = {0x29, 0x00};
static struct dsi_cmd_desc display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep}
};

/* [ECID:0000] ZTEBSP wangbing start 201205281 for lcd, start*/
/*
static char enter_deep_sleep[2] = {0x4f, 0x00};
static struct dsi_cmd_desc display_off_deep_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(enter_sleep), enter_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(enter_deep_sleep), enter_deep_sleep}	
};*/
/* [ECID:0000] ZTEBSP wangbing start 201205281 for lcd, end*/


/*about himax8363 chip id */

static char hx8363_setpassword_para[4]={0xB9,0xFF,0x83,0x63};
/*static char hx8363_icid_rd_para[2] = {0xB9, 0x00}; 
   
static struct dsi_cmd_desc hx8363_icid_rd_cmd = 
{
	DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(hx8363_icid_rd_para), hx8363_icid_rd_para
};
static struct dsi_cmd_desc hx8363_setpassword_cmd[] = 
{	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(hx8363_setpassword_para),hx8363_setpassword_para},

};
*/
/*about himax8369 chip id */

static char hx8369_setpassword_para[4]={0xB9,0xFF,0x83,0x69};
/*static char hx8369_icid_rd_para[2] = {0xB9, 0x00}; 
static char hx8369_panleid_rd_para[2] = {0xda, 0x00};    


static struct dsi_cmd_desc hx8369_icid_rd_cmd = 
{
	DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(hx8369_icid_rd_para), hx8369_icid_rd_para
};
static struct dsi_cmd_desc hx8369_setpassword_cmd[] = 
{	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(hx8369_setpassword_para),hx8369_setpassword_para},

};*/
/*
static struct dsi_cmd_desc hx8369_panleid_rd_cmd = 
{
	DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(hx8369_panleid_rd_para), hx8369_panleid_rd_para
};*/


/*about Novatek3511 chip id */
/*
static char nt3511_page_ff[5] = {0xff, 0xaa,0x55,0x25,0x01};
static char nt3511_page_f8[20] = {0xf8, 0x01,0x12,0x00,0x20,0x33,0x13,0x00,0x40,0x00,0x00,0x23,0x01,0x99,0xc8,0x00,0x00,0x01,0x00,0x00};
static char nt3511_icid_rd_para[2] = {0xc5, 0x00}; 
static char nt3511_panleid_rd_para[2] = {0xDA, 0x00};    //added by zte_gequn091966 for lead_nt35510,20111226

static struct dsi_cmd_desc nt3511_setpassword_cmd[] = {	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(nt3511_page_ff),nt3511_page_ff},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(nt3511_page_f8),nt3511_page_f8}
};
static struct dsi_cmd_desc nt3511_icid_rd_cmd = {
	DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(nt3511_icid_rd_para), nt3511_icid_rd_para};


static struct dsi_cmd_desc nt3511_panleid_rd_cmd = {
	DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(nt3511_panleid_rd_para), nt3511_panleid_rd_para
};
*/
/*about RENESAS r61408 chip id */
/*
static char r61408_setpassword_para[2]={0xb0,0x04};
static struct dsi_cmd_desc r61408_setpassword_cmd[] = 
{	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(r61408_setpassword_para),r61408_setpassword_para},

};
static char r61408_icid_rd_para[2] = {0xbf, 0x00}; 
static struct dsi_cmd_desc r61408_icid_rd_cmd = 
{
	DTYPE_GEN_READ1, 1, 0, 0, 1, sizeof(r61408_icid_rd_para), r61408_icid_rd_para
};
*/
//wangminrong add cabc 
#ifdef CONFIG_BACKLIGHT_CABC
static char otm8009a_para_CABC_0x51[2]={0x51,0xff};
static char otm8009a_para_CABC_0x53[2]={0x53,0x2c};
static char otm8009a_para_CABC_0x55[2]={0x55,0x03};
static struct dsi_cmd_desc otm8009a_display_on_CABC_backlight_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(otm8009a_para_CABC_0x51), otm8009a_para_CABC_0x51},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(otm8009a_para_CABC_0x53), otm8009a_para_CABC_0x53},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(otm8009a_para_CABC_0x55), otm8009a_para_CABC_0x55},
};
static char otm8009a_para_CABC_0x53_off[2]={0x53,0x00};
static struct dsi_cmd_desc otm8009a_display_off_CABC_backlight_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 100, sizeof(otm8009a_para_CABC_0x53_off), otm8009a_para_CABC_0x53_off}
};

/** ZTEBSP zhoufan modify for NT35110B LEAD IPS LCD CABC ++ 20130226 **/
static char nt35110_para_CABC_0x51[2]={0x51,0xff};
static char nt35110_para_CABC_0x53[2]={0x53,0x2c};
static char nt35110_para_CABC_0x55[2]={0x55,0x03};
static struct dsi_cmd_desc nt35110_display_on_CABC_backlight_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35110_para_CABC_0x51), nt35110_para_CABC_0x51},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35110_para_CABC_0x53), nt35110_para_CABC_0x53},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35110_para_CABC_0x55), nt35110_para_CABC_0x55},
};
static char nt35110_para_CABC_0x53_off[2]={0x53,0x00};
static struct dsi_cmd_desc nt35110_display_off_CABC_backlight_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 100, sizeof(nt35110_para_CABC_0x53_off), nt35110_para_CABC_0x53_off}
};
/** ZTEBSP zhoufan modify for NT35110B LEAD IPS LCD CABC -- 20130226 **/
// [ECID:0000] ZTEBSP liyeqing add for CABC of tianma hx8369a02 , begin --20121012
static char tianma_hx8369a02_para_CABC_0xc9[10]={0xc9,0x3e,0x00,0x00,0x01,0x0f,0x06,0x1e,0x1e,0x00};//liyeqing change 20121019
static char tianma_hx8369a02_para_CABC_0x51[2]={0x51,0xff};
static char tianma_hx8369a02_para_CABC_0x53[2]={0x53,0x24};
static char tianma_hx8369a02_para_CABC_0x55[2]={0x55,0x03};
static struct dsi_cmd_desc tianma_hx8369a02_display_on_CABC_backlight_cmds[] = {
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(tianma_hx8369a02_para_CABC_0xc9),tianma_hx8369a02_para_CABC_0xc9},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_hx8369a02_para_CABC_0x51),tianma_hx8369a02_para_CABC_0x51},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_hx8369a02_para_CABC_0x53),tianma_hx8369a02_para_CABC_0x53},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 5, sizeof(tianma_hx8369a02_para_CABC_0x55),tianma_hx8369a02_para_CABC_0x55},
};

static char tianma_hx8369a02_para_CABC_0x53_off[2]={0x53,0x00};
static struct dsi_cmd_desc tianma_hx8369a02_display_off_CABC_backlight_cmds[] = {
	{DTYPE_DCS_WRITE1,1,0,0,100,sizeof(tianma_hx8369a02_para_CABC_0x53_off),tianma_hx8369a02_para_CABC_0x53_off}
};
// [ECID:0000] ZTEBSP liyeqing add for CABC of tianma hx8369a02 , end --20121012
#endif
//wangminrong add cabc
/**************************************
1. hx8363 yassy start 
**************************************/
static char hx8363_yassy_para_0xb1[13]={0xB1,0x78,0x34,0x08,0x34,0x02,0x13,
								0x11,0x11,0x2d,0x35,0x3F,0x3F};  
static char hx8363_yassy_para_0xba[14]={0xBA,0x80,0x00,0x10,0x08,0x08,0x10,0x7c,0x6e,
								0x6d,0x0a,0x01,0x84,0x43};   //TWO LANE
static char hx8363_yassy_para_0x3a[2]={0x3a,0x77};
//static char hx8363_para_0x36[2]={0x36,0x0a};
static char hx8363_yassy_para_0xb2[4]={0xb2,0x33,0x33,0x22};
static char hx8363_yassy_para_0xb3[2]={0xb3,0x00};
static char hx8363_yassy_para_0xb4[10]={0xb4,0x08,0x12,0x72,0x12,0x06,0x03,0x54,0x03,0x4e};
static char hx8363_yassy_para_0xb6[2]={0xb6,0x2c};
static char hx8363_yassy_para_0xcc[2]={0xcc,0x09};
static char hx8363_yassy_para_0xe0[31]={0xe0,0x01,0x09,0x17,0x10,0x10,0x3e,0x07,
	0x8d,0x90,0x54,0x16,0xd5,0x55,0x53,0x19,0x01,0x09,0x17,0x10,0x10,0x3e,0x07,
	0x8d,0x90,0x54,0x16,0xd5,0x55,0x53,0x19};	

static struct dsi_cmd_desc hx8363_yassy_display_on_cmds[] = 
{

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_setpassword_para),hx8363_setpassword_para},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0xb1), hx8363_yassy_para_0xb1},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0xb2), hx8363_yassy_para_0xb2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0xba), hx8363_yassy_para_0xba},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0x3a), hx8363_yassy_para_0x3a},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0xb3), hx8363_yassy_para_0xb3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0xb4), hx8363_yassy_para_0xb4},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0xb6), hx8363_yassy_para_0xb6},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0xcc), hx8363_yassy_para_0xcc},
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 10, sizeof(hx8363_para_0x36), hx8363_para_0x36},		
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8363_yassy_para_0xe0), hx8363_yassy_para_0xe0},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(hx8363_para_0xc1), hx8363_para_0xc1},
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(hx8363_para_0xc2), hx8363_para_0xc2},	
	
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};



/**************************************
2. hx8369 lead start 
**************************************/
static char hx8369_lead_tn_para_0xb0[3]={0xb0,0x01,0x09};
static char hx8369_lead_tn_para_0xb1[20]={0xB1,0x01,0x00,0x34,0x07,0x00,0x0F,0x0F,
	0x21,0x28,0x3F,0x3F,0x07,0x23,0x01,0xE6,0xE6,0xE6,0xE6,0xE6};  
static char hx8369_lead_tn_para_0xb2[16]={0xB2,0x00,0x23,0x0A,0x0A,0x70,0x00,0xFF,
	0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x01};  //VIDEO MODE
//static char para_0xb2[16]={0xB2,0x00,0x20,0x0A,0x0A,0x70,0x00,0xFF,
//	0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x01};  //CMD MODE
static char hx8369_lead_tn_para_0xb4[6]={0xB4,0x00,0x0C,0x84,0x0C,0x01}; 
static char hx8369_lead_tn_para_0xb6[3]={0xB6,0x2c,0x2c};
static char hx8369_lead_tn_para_0xd5[27]={0xD5,0x00,0x05,0x03,0x00,0x01,0x09,0x10,
	0x80,0x37,0x37,0x20,0x31,0x46,0x8A,0x57,0x9B,0x20,0x31,0x46,0x8A,
	0x57,0x9B,0x07,0x0F,0x07,0x00}; 
static char hx8369_lead_tn_para_0xe0[35]={0xE0,0x00,0x06,0x06,0x29,0x2d,0x3F,0x13,0x32,
	0x08,0x0c,0x0D,0x11,0x14,0x11,0x14,0x0e,0x15,0x00,0x06,0x06,0x29,0x2d,
	0x3F,0x13,0x32,0x08,0x0c,0x0D,0x11,0x14,0x11,0x14,0x0e,0x15};
static char hx8369_lead_tn_para_0x3a[2]={0x3A,0x77}; 
static char hx8369_lead_tn_para_0xba[14]={0xBA,0x00,0xA0,0xC6,0x00,0x0A,0x00,0x10,0x30,
	0x6C,0x02,0x11,0x18,0x40};   //TWO LANE
//static char para_0xba[14]={0xBA,0x00,0xA0,0xC6,0x00,0x0A,0x00,0x10,0x30,
	//0x6C,0x02,0x10,0x18,0x40};   //ONE LANE

static struct dsi_cmd_desc hx8369_lead_display_on_cmds[] = {
	 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_setpassword_para),hx8369_setpassword_para},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0xb0), hx8369_lead_tn_para_0xb0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0xb1), hx8369_lead_tn_para_0xb1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0xb2), hx8369_lead_tn_para_0xb2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0xb4), hx8369_lead_tn_para_0xb4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0xb6), hx8369_lead_tn_para_0xb6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0xd5), hx8369_lead_tn_para_0xd5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0xe0), hx8369_lead_tn_para_0xe0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0x3a), hx8369_lead_tn_para_0x3a},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_tn_para_0xba), hx8369_lead_tn_para_0xba},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};
/**************************************
3. HANNSTAR hx8369 lead start 
**************************************/


static char hx8369_lead_hannstar_para_0xb1[20]={0xB1,0x01,0x00,0x34,0x07,0x00,0x0E,0x0E,
	0x21,0x29,0x3F,0x3F,0x01,0x63,0x01,0xE6,0xE6,0xE6,0xE6,0xE6};  
static char hx8369_lead_hannstar_para_0xb2[16]={0xB2,0x00,0x23,0x07,0x07,0x70,0x00,0xFF,
	0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x01};  //VIDEO MODE
static char hx8369_lead_hannstar_para_0xb4[6]={0xB4,0x02,0x18,0x80,0x10,0x01}; 
static char hx8369_lead_hannstar_para_0xb6[3]={0xB6,0x1F,0x1F};
static char hx8369_lead_hannstar_para_0xcc[2]={0xcc,0x00}; 
static char hx8369_lead_hannstar_para_0xd5[27]={0xD5,0x00,0x07,0x00,0x00,0x01,0x0a,0x10,
	0x60,0x33,0x37,0x23,0x01,0xB9,0x75,0xA8,0x64,0x00,0x00,0x41,0x06,
	0x50,0x07,0x07,0x0F,0x07,0x00}; 
static char hx8369_lead_hannstar_para_0xe0[35]={0xE0,0x00,0x03,0x00,0x09,0x09,0x21,0x1B,0x2D,
	0x06,0x0c,0x10,0x15,0x16,0x14,0x16,0x12,0x18,0x00,0x03,0x00,0x09,0x09,
	0x21,0x1B,0x2D,0x06,0x0c,0x10,0x15,0x16,0x14,0x16,0x12,0x18};
static char hx8369_lead_hannstar_para_0x3a[2]={0x3A,0x77}; 
static char hx8369_lead_hannstar_para_0xba[14]={0xBA,0x00,0xA0,0xC6,0x00,0x0A,0x02,0x10,0x30,
	0x6F,0x02,0x11,0x18,0x40};   //TWO LANE

static struct dsi_cmd_desc hx8369_lead_hannstar_display_on_cmds[] = {
	 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_setpassword_para),hx8369_setpassword_para},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0xb1), hx8369_lead_hannstar_para_0xb1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0xb2), hx8369_lead_hannstar_para_0xb2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0xb4), hx8369_lead_hannstar_para_0xb4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0xb6), hx8369_lead_hannstar_para_0xb6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0xcc), hx8369_lead_hannstar_para_0xcc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0xd5), hx8369_lead_hannstar_para_0xd5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0xe0), hx8369_lead_hannstar_para_0xe0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0x3a), hx8369_lead_hannstar_para_0x3a},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_lead_hannstar_para_0xba), hx8369_lead_hannstar_para_0xba},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};

/**************************************
4. hx8369 tianma TN start 
**************************************/
/*
static char hx8369_tianma_tn_para_0xb1[20]={0xB1,0x01,0x00,0x34,0x0A,0x00,0x11,0x12,0x21,0x29,0x3F,0x3F,
	0x01,0x1a,0x01,0xE6,0xE6,0xE6,0xE6,0xE6}; 
static char hx8369_tianma_tn_para_0xb2[16]={0xB2,0x00,0x23,0x03,0x03,0x70,0x00,0xFF,0x00,0x00,0x00,0x00,
	0x03,0x03,0x00,0x01};  //VIDEO MODE
static char hx8369_tianma_tn_para_0xb4[6]={0xB4,0x02,0x18,0x70,0x0f,0x01}; 
static char hx8369_tianma_tn_para_0xb6[3]={0xB6,0x4a,0x4a};
static char hx8369_tianma_tn_para_0xd5[27]={0xD5,0x00,0x09,0x03,0x29,0x01,0x0a,0x28,0x60,0x11,0x13,0x00,
	0x00,0x40,0x26,0x51,0x37,0x00,0x00,0x71,0x35,0x60,0x24,0x07,0x0F,0x04,0x04}; 
static char hx8369_tianma_tn_para_0xe0[35]={0xE0,0x00,0x02,0x0b,0x0a,0x09,0x18,0x1d,0x2a,0x08,0x11,0x0d,
	0x13,0x15,0x14,0x15,0x0f,0x14,0x00,0x02,0x0b,0x0a,0x09,0x18,0x1d,0x2a,0x08,0x11,0x0d,0x13,0x15,
	0x14,0x15,0x0f,0x14};
static char hx8369_tianma_tn_para_0xcc[2]={0xcc,0x00}; 
static char hx8369_tianma_tn_para_0x3a[2]={0x3A,0x77}; 
static char hx8369_tianma_tn_para_0xba[14]={0xBA,0x00,0xA0,0xC6,0x00,0x0A,0x00,0x10,0x30,0x6F,0x02,0x11,0x18,0x40};   //TWO LANE
static struct dsi_cmd_desc hx8369_tianma_tn_display_on_cmds[] = 
{

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_setpassword_para),hx8369_setpassword_para},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xb1), hx8369_tianma_tn_para_0xb1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xb2), hx8369_tianma_tn_para_0xb2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xb4), hx8369_tianma_tn_para_0xb4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xb6), hx8369_tianma_tn_para_0xb6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xd5),hx8369_tianma_tn_para_0xd5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xe0), hx8369_tianma_tn_para_0xe0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0x3a), hx8369_tianma_tn_para_0x3a},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xcc), hx8369_tianma_tn_para_0xcc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xba), hx8369_tianma_tn_para_0xba},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};
*/
//4.//tianma 4.3 for P825A10 ..... wangminrong/
static char hx8369_tianma_tn_para_0xb1[20]={0xB1,0xf5,0x00,0x44,0x0b,0x00,0x11,0x11,0x36,0x3E,0x3F,0x3F,
	0x01,0x23,0x01,0xE6,0xE6,0xE6,0xE6,0xE6}; 
static char hx8369_tianma_tn_para_0xb2[16]={0xB2,0x00,0x2b,0x03,0x03,0x70,0x00,0xFF,0x00,0x00,0x00,0x00,
	0x03,0x03,0x00,0x01};  //VIDEO MODE
static char hx8369_tianma_tn_para_0xb4[6]={0xB4,0x02,0x18,0x70,0x13,0x05}; 
static char hx8369_tianma_tn_para_0xb6[3]={0xB6,0x1E,0x1E};
//static char hx8369_tianma_tn_para_0x36[2]= {0x36,0x00};
static char hx8369_tianma_tn_para_0xd5[27]={0xD5,0x00,0x06,0x03,0x29,0x01,0x0A,0x00,0x80,0x11,0x13,0x02,
	0x13,0xF4,0xF6,0xF5,0xF7,0x31,0x20,0x05,0x07,0x04,0x06,0x07,0x0F,0x04,0x04}; 
static char hx8369_tianma_tn_para_0xe0[35]={0xE0,0x00,0x02,0x06,0x0E,0x0B,0x15,0x26,0x33,0x09,0x11,0x11,
	0x16,0x17,0x15,0x15,0x10,0x16,0x00,0x02,0x06,0x0E,0x0B,0x15,0x26,0x33,0x09,0x11,0x11,0x16,0x17,
	0x15,0x15,0x10,0x16};
static char hx8369_tianma_tn_para_0xcc[2]={0xcc,0x00}; 
static char hx8369_tianma_tn_para_0x3a[2]={0x3A,0x77}; 
static char hx8369_tianma_tn_para_0xba[14]={0xBA,0x00,0xA0,0xC6,0x00,0x0A,0x00,0x10,0x30,0x6F,0x02,0x11,0x18,0x40};   //TWO LANE
static struct dsi_cmd_desc hx8369_tianma_tn_display_on_cmds[] = 
{

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_setpassword_para),hx8369_setpassword_para},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xb1), hx8369_tianma_tn_para_0xb1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xb2), hx8369_tianma_tn_para_0xb2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xb4), hx8369_tianma_tn_para_0xb4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xb6), hx8369_tianma_tn_para_0xb6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xd5),hx8369_tianma_tn_para_0xd5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xe0), hx8369_tianma_tn_para_0xe0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0x3a), hx8369_tianma_tn_para_0x3a},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xcc), hx8369_tianma_tn_para_0xcc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_tn_para_0xba), hx8369_tianma_tn_para_0xba},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}
};
/**************************************
5. hx8369 tianma IPS start 
**************************************/
static char hx8369_tianma_ips_para_0xb1[20]={0xB1,0x01,0x00,0x34,0x07,0x00,0x0D,0x0D,0x1A,0x22,0x3F,
	0x3F,0x01,0x23,0x01,0xE6,0xE6,0xE6,0xE6,0xE6}; 
static char hx8369_tianma_ips_para_0xb2[16]={0xB2,0x00,0x23,0x05,0x05,0x70,0x00,0xFF,
	0x00,0x00,0x00,0x00,0x03,0x03,0x00,0x01};  //VIDEO MODE
static char hx8369_tianma_ips_para_0xb4[6]={0xB4,0x00,0x18,0x80,0x06,0x02}; 
static char hx8369_tianma_ips_para_0xb6[3]={0xB6,0x42,0x42};
static char hx8369_tianma_ips_para_0xd5[27]={0xD5,0x00,0x09,0x03,0x29,0x01,0x0A,0x28,0x70,
	0x11,0x13,0x00,0x00,0x40,0x26,0x51,0x37,0x00,0x00,0x71,0x35,0x60,0x24,0x07,0x0F,0x04,0x04}; 
static char hx8369_tianma_ips_para_0xe0[35]={0xE0,0x00,0x0A,0x0F,0x2E,0x33,0x3F,0x1D,0x3E,0x07,0x0D,0x0F,
	0x12,0x15,0x13,0x15,0x10,0x17,0x00,0x0A,0x0F,0x2E,0x33,0x3F,0x1D,0x3E,0x07,0x0D,
	0x0F,0x12,0x15,0x13,0x15,0x10,0x17};
static char hx8369_tianma_ips_para_0x3a[2]={0x3A,0x77}; 
static char hx8369_tianma_ips_para_0xba[14]={0xBA,0x00,0xA0,0xC6,0x00,0x0A,0x00,0x10,0x30,0x6F,0x02,0x11,0x18,0x40};   //TWO LANE

static struct dsi_cmd_desc hx8369_tianma_ips_display_on_cmds[] = 
{

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_setpassword_para),hx8369_setpassword_para},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_ips_para_0xb1), hx8369_tianma_ips_para_0xb1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_ips_para_0xb2), hx8369_tianma_ips_para_0xb2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_ips_para_0xb4), hx8369_tianma_ips_para_0xb4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_ips_para_0xb6), hx8369_tianma_ips_para_0xb6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_ips_para_0xd5), hx8369_tianma_ips_para_0xd5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_ips_para_0xe0), hx8369_tianma_ips_para_0xe0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(hx8369_tianma_ips_para_0x3a), hx8369_tianma_ips_para_0x3a},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(hx8369_tianma_ips_para_0xba), hx8369_tianma_ips_para_0xba},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};


/**************************************
6. nt35510 lead start 
**************************************/
#if 0
static char nt35510_lead_cmd_page1_f0[6] = {0xf0, 0x55,0xaa,0x52,0x08,0x01};
static char nt35510_lead_cmd_page1_bc[4] = {0xbc, 0x00,0x78,0x1A};
static char nt35510_lead_cmd_page1_bd[4] = {0xbd, 0x00,0x78,0x1A};
static char nt35510_lead_cmd_page1_be[3] = {0xbe, 0x00,0x4E};
static char nt35510_lead_cmd_page1_b0[4] = {0xb0, 0x00,0x00,0x00};
static char nt35510_lead_cmd_page1_b1[4] = {0xb1, 0x00,0x00,0x00};
//static char lead_cmd_page1_b7[4] = {0xb7, 0x44,0x44,0x44};
//static char lead_cmd_page1_b3[4] = {0xb1, 0x0B,0x0B,0x0B};
static char nt35510_lead_cmd_page1_b9[4] = {0xb9, 0x34,0x34,0x34};
static char nt35510_lead_cmd_page1_ba[4] = {0xba, 0x16,0x16,0x16};

static char nt35510_lead_cmd_page1_b6[4] = {0xb6, 0x36,0x36,0x36};
static char nt35510_lead_cmd_page1_b7[4] = {0xb7, 0x26,0x26,0x26};
static char nt35510_lead_cmd_page1_b8[4] = {0xb8, 0x26,0x26,0x26};
static char nt35510_lead_cmd_page1_d1[] = {0xD1,0x00,0x00,0x00,0x2D,0x00,0x5C,
0x00,0x80,0x00,0xAB,0x00,0xE4,0x01,0x15,0x01,0x5C,0x01,0x8E,0x01,0xD3,0x02,
0x03,0x02,0x45,0x02,0x77,0x02,0x78,0x02,0xA4,0x02,0xD1,0x02,0xEA,0x03,0x09,
0x03,0x1A,0x03,0x32,0x03,0x40,0x03,0x59,0x03,0x68,0x03,0x7C,0x03,0xB2,0x03,
0xD8};
static char nt35510_lead_cmd_page1_d2[] = {0xD2,0x00,0x00,0x00,0x2D,0x00,0x5C,
0x00,0x80,0x00,0xAB,0x00,0xE4,0x01,0x15,0x01,0x5C,0x01,0x8E,0x01,0xD3,0x02,
0x03,0x02,0x45,0x02,0x77,0x02,0x78,0x02,0xA4,0x02,0xD1,0x02,0xEA,0x03,0x09,
0x03,0x1A,0x03,0x32,0x03,0x40,0x03,0x59,0x03,0x68,0x03,0x7C,0x03,0xB2,0x03,
0xD8};
static char nt35510_lead_cmd_page1_d3[] = {0xD3,0x00,0x00,0x00,0x2D,0x00,0x5C,
0x00,0x80,0x00,0xAB,0x00,0xE4,0x01,0x15,0x01,0x5C,0x01,0x8E,0x01,0xD3,0x02,
0x03,0x02,0x45,0x02,0x77,0x02,0x78,0x02,0xA4,0x02,0xD1,0x02,0xEA,0x03,0x09,
0x03,0x1A,0x03,0x32,0x03,0x40,0x03,0x59,0x03,0x68,0x03,0x7C,0x03,0xB2,0x03,
0xD8};
static char nt35510_lead_cmd_page1_d4[] = {0xD4,0x00,0x00,0x00,0x2D,0x00,0x5C,
0x00,0x80,0x00,0xAB,0x00,0xE4,0x01,0x15,0x01,0x5C,0x01,0x8E,0x01,0xD3,0x02,
0x03,0x02,0x45,0x02,0x77,0x02,0x78,0x02,0xA4,0x02,0xD1,0x02,0xEA,0x03,0x09,
0x03,0x1A,0x03,0x32,0x03,0x40,0x03,0x59,0x03,0x68,0x03,0x7C,0x03,0xB2,0x03,
0xD8};
static char nt35510_lead_cmd_page1_d5[] = {0xD5,0x00,0x00,0x00,0x2D,0x00,0x5C,
0x00,0x80,0x00,0xAB,0x00,0xE4,0x01,0x15,0x01,0x5C,0x01,0x8E,0x01,0xD3,0x02,
0x03,0x02,0x45,0x02,0x77,0x02,0x78,0x02,0xA4,0x02,0xD1,0x02,0xEA,0x03,0x09,
0x03,0x1A,0x03,0x32,0x03,0x40,0x03,0x59,0x03,0x68,0x03,0x7C,0x03,0xB2,0x03,
0xD8};
static char nt35510_lead_cmd_page1_d6[] = {0xD6,0x00,0x00,0x00,0x2D,0x00,0x5C,
0x00,0x80,0x00,0xAB,0x00,0xE4,0x01,0x15,0x01,0x5C,0x01,0x8E,0x01,0xD3,0x02,
0x03,0x02,0x45,0x02,0x77,0x02,0x78,0x02,0xA4,0x02,0xD1,0x02,0xEA,0x03,0x09,
0x03,0x1A,0x03,0x32,0x03,0x40,0x03,0x59,0x03,0x68,0x03,0x7C,0x03,0xB2,0x03,
0xD8};

static char nt35510_lead_cmd_page0_f0[6] = {0xf0, 0x55,0xaa,0x52,0x08,0x00};
static char nt35510_lead_cmd_page0_b1[2] = {0xb1, 0xf8};   //0xcc
static char nt35510_lead_cmd_page0_b4[2] = {0xb4, 0x10};
static char nt35510_lead_cmd_page0_b6[2] = {0xb6, 0x02};
static char nt35510_lead_cmd_page0_b7[3] = {0xb7, 0x71,0x71};
static char nt35510_lead_cmd_page0_b8[5] = {0xb8, 0x01,0x0A,0x0A,0x0A};
static char nt35510_lead_cmd_page0_bc[4] = {0xbc, 0x05,0x05,0x05};
static char nt35510_lead_cmd_page0_bd[6] = {0xbd, 0x01,0x84,0x07,0x31,0x00};
static char nt35510_lead_cmd_page0_be[6] = {0xbe, 0x01,0x84,0x07,0x31,0x00};
static char nt35510_lead_cmd_page0_bf[6] = {0xbf, 0x01,0x84,0x07,0x31,0x00};

static char nt35510_lead_cmd_page0_c7[2] = {0xc7, 0x02};
static char nt35510_lead_cmd_page0_c9[5] = {0xc9, 0x11,0x00,0x00,0x00};
static char nt35510_lead_cmd_page0_3a[2] = {0x3a, 0x77};
static char nt35510_lead_cmd_page0_35[2] = {0x35, 0x00};
static char nt35510_lead_cmd_page0_21[2] = {0x21, 0x00};

static struct dsi_cmd_desc nt35510_lead_display_on_cmds[] = {

{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_f0), nt35510_lead_cmd_page1_f0},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_bc), nt35510_lead_cmd_page1_bc},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_bd), nt35510_lead_cmd_page1_bd},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_be), nt35510_lead_cmd_page1_be},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b0), nt35510_lead_cmd_page1_b0},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b1), nt35510_lead_cmd_page1_b1},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b9), nt35510_lead_cmd_page1_b9},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_ba), nt35510_lead_cmd_page1_ba},

{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b6), nt35510_lead_cmd_page1_b6},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b7), nt35510_lead_cmd_page1_b7},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b8), nt35510_lead_cmd_page1_b8},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d1), nt35510_lead_cmd_page1_d1},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d2), nt35510_lead_cmd_page1_d2},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d3), nt35510_lead_cmd_page1_d3},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d4), nt35510_lead_cmd_page1_d4},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d5), nt35510_lead_cmd_page1_d5},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d6), nt35510_lead_cmd_page1_d6},

{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_f0), nt35510_lead_cmd_page0_f0},
{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_b1), nt35510_lead_cmd_page0_b1},
{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_b4), nt35510_lead_cmd_page0_b4},
{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_b6), nt35510_lead_cmd_page0_b6},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_b7), nt35510_lead_cmd_page0_b7},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_b8), nt35510_lead_cmd_page0_b8},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_bc), nt35510_lead_cmd_page0_bc},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_bd), nt35510_lead_cmd_page0_bd},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_be), nt35510_lead_cmd_page0_be},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_bf), nt35510_lead_cmd_page0_bf},

{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_f0), nt35510_lead_cmd_page0_f0},
{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_c7), nt35510_lead_cmd_page0_c7},
{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_c9), nt35510_lead_cmd_page0_c9},
{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_3a), nt35510_lead_cmd_page0_3a},
{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_35), nt35510_lead_cmd_page0_35},
{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_21), nt35510_lead_cmd_page0_21},
//{DTYPE_DCS_WRITE1, 1, 0, 0, 1, sizeof(lead_cmd_page0_36), lead_cmd_page0_36},
{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};
#endif
/** ZTEBSP zhoufan modify for NT35110B LEAD IPS LCD++ 20130226**/
static char nt35510_lead_cmd_page1_f0[6] = {0xf0, 0x55,0xaa,0x52,0x08,0x01};
static char nt35510_lead_cmd_page1_b0[4] = {0xb0, 0x0d,0x0d,0x0d};
static char nt35510_lead_cmd_page1_b6[4] = {0xb6, 0x34,0x34,0x34};
static char nt35510_lead_cmd_page1_b1[4] = {0xb1, 0x0d,0x0d,0x0d};
static char nt35510_lead_cmd_page1_b7[4] = {0xb7, 0x35,0x35,0x35};
static char nt35510_lead_cmd_page1_b2[4] = {0xb2, 0x0,0x0,0x0};
static char nt35510_lead_cmd_page1_b8[4] = {0xb8, 0x24,0x24,0x24};
static char nt35510_lead_cmd_page1_bf[2] = {0xbf, 0x01};
static char nt35510_lead_cmd_page1_b3[4] = {0xb3, 0x08,0x08,0x08};
static char nt35510_lead_cmd_page1_b9[4] = {0xb9, 0x34,0x34,0x34};
static char nt35510_lead_cmd_page1_ba[4] = {0xba, 0x24,0x24,0x24};
static char nt35510_lead_cmd_page1_bc[4] = {0xbc, 0x00,0xA0,0x00};
static char nt35510_lead_cmd_page1_bd[4] = {0xbd, 0x00,0xA0,0x00};
static char nt35510_lead_cmd_page1_be[3] = {0xbe, 0x00,0x93};

static char nt35510_lead_cmd_page1_d1[] = {
	0xD1,
	0x00,0x05,0x00,0x17,
	0x00,0x35,0x00,0x5A,
	0x00,0x68,0x00,0x8C,
	0x00,0xB1,0x00,0xE2,
	0x01,0x0A,0x01,0x4A,
	0x01,0x7B,0x01,0xCB,
	0x02,0x0D,0x02,0x0E,
	0x02,0x4C,0x02,0x90,
       0x02,0xBB,0x02,0xF6,
       0x03,0x1D,0x03,0x50,
       0x03,0x79,0x03,0x9A,
       0x03,0xAE,0x03,0xEA,
       0x03,0xF4,0x03,0xFF};
static char nt35510_lead_cmd_page1_d2[] = {
	0xD2,
	0x00,0x05,0x00,0x17,
	0x00,0x35,0x00,0x5A,
	0x00,0x68,0x00,0x8C,
	0x00,0xB1,0x00,0xE2,
	0x01,0x0A,0x01,0x4A,
	0x01,0x7B,0x01,0xCB,
	0x02,0x0D,0x02,0x0E,
	0x02,0x4C,0x02,0x90,
       0x02,0xBB,0x02,0xF6,
       0x03,0x1D,0x03,0x50,
       0x03,0x79,0x03,0x9A,
       0x03,0xAE,0x03,0xEA,
       0x03,0xF4,0x03,0xFF};
static char nt35510_lead_cmd_page1_d3[] = {
	0xD3,
	0x00,0x05,0x00,0x17,
	0x00,0x35,0x00,0x5A,
	0x00,0x68,0x00,0x8C,
	0x00,0xB1,0x00,0xE2,
	0x01,0x0A,0x01,0x4A,
	0x01,0x7B,0x01,0xCB,
	0x02,0x0D,0x02,0x0E,
	0x02,0x4C,0x02,0x90,
       0x02,0xBB,0x02,0xF6,
       0x03,0x1D,0x03,0x50,
       0x03,0x79,0x03,0x9A,
       0x03,0xAE,0x03,0xEA,
       0x03,0xF4,0x03,0xFF};
static char nt35510_lead_cmd_page1_d4[] = {
	0xD4,
	0x00,0x05,0x00,0x17,
	0x00,0x35,0x00,0x5A,
	0x00,0x68,0x00,0x8C,
	0x00,0xB1,0x00,0xE2,
	0x01,0x0A,0x01,0x4A,
	0x01,0x7B,0x01,0xCB,
	0x02,0x0D,0x02,0x0E,
	0x02,0x4C,0x02,0x90,
       0x02,0xBB,0x02,0xF6,
       0x03,0x1D,0x03,0x50,
       0x03,0x79,0x03,0x9A,
       0x03,0xAE,0x03,0xEA,
       0x03,0xF4,0x03,0xFF};
static char nt35510_lead_cmd_page1_d5[] = {
	0xD5,
	0x00,0x05,0x00,0x17,
	0x00,0x35,0x00,0x5A,
	0x00,0x68,0x00,0x8C,
	0x00,0xB1,0x00,0xE2,
	0x01,0x0A,0x01,0x4A,
	0x01,0x7B,0x01,0xCB,
	0x02,0x0D,0x02,0x0E,
	0x02,0x4C,0x02,0x90,
       0x02,0xBB,0x02,0xF6,
       0x03,0x1D,0x03,0x50,
       0x03,0x79,0x03,0x9A,
       0x03,0xAE,0x03,0xEA,
       0x03,0xF4,0x03,0xFF};
static char nt35510_lead_cmd_page1_d6[] = {
	0xD6,
	0x00,0x05,0x00,0x17,
	0x00,0x35,0x00,0x5A,
	0x00,0x68,0x00,0x8C,
	0x00,0xB1,0x00,0xE2,
	0x01,0x0A,0x01,0x4A,
	0x01,0x7B,0x01,0xCB,
	0x02,0x0D,0x02,0x0E,
	0x02,0x4C,0x02,0x90,
       0x02,0xBB,0x02,0xF6,
       0x03,0x1D,0x03,0x50,
       0x03,0x79,0x03,0x9A,
       0x03,0xAE,0x03,0xEA,
       0x03,0xF4,0x03,0xFF};

static char nt35510_lead_cmd_page0_f0[6] = {0xf0, 0x55,0xaa,0x52,0x08,0x00};
static char nt35510_lead_cmd_page0_b1[3] = {0xb1, 0xfc,0x00};
static char nt35510_lead_cmd_page0_b5[2] = {0xb5, 0x6B};
static char nt35510_lead_cmd_page0_b6[2] = {0xb6, 0x05};
static char nt35510_lead_cmd_page0_b7[3] = {0xb7, 0x70,0x70};
static char nt35510_lead_cmd_page0_b8[5] = {0xb8,0x01,0x05,0x05,0x05};
static char nt35510_lead_cmd_page0_bc[4] = {0xbc, 0x00,0x00,0x00};
static char nt35510_lead_cmd_page0_c9[6] = {0xc9, 0xd0, 0x82,0x50,0x28,0x28};
static char nt35510_lead_cmd_page0_bd[6] = {0xbd, 0x01,0x70,0x1a,0x10,0x00};
//static char nt35510_lead_cmd_page0_b1_mirror[3] = {0xb1, 0xfc,0x02};
static char nt35510_lead_cmd_page0_e0[3]={0xe0,0x00,0x01};

static struct dsi_cmd_desc nt35510_lead_display_on_cmds[] = {

{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_f0), nt35510_lead_cmd_page1_f0},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b0), nt35510_lead_cmd_page1_b0},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b6), nt35510_lead_cmd_page1_b6},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b1), nt35510_lead_cmd_page1_b1},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b7), nt35510_lead_cmd_page1_b7},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b2), nt35510_lead_cmd_page1_b2},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b8), nt35510_lead_cmd_page1_b8},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_bf), nt35510_lead_cmd_page1_bf},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b3), nt35510_lead_cmd_page1_b3},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_b9), nt35510_lead_cmd_page1_b9},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_ba), nt35510_lead_cmd_page1_ba},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_bc), nt35510_lead_cmd_page1_bc},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_bd), nt35510_lead_cmd_page1_bd},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_be), nt35510_lead_cmd_page1_be},

{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d1), nt35510_lead_cmd_page1_d1},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d2), nt35510_lead_cmd_page1_d2},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d3), nt35510_lead_cmd_page1_d3},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d4), nt35510_lead_cmd_page1_d4},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d5), nt35510_lead_cmd_page1_d5},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page1_d6), nt35510_lead_cmd_page1_d6},

{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_f0), nt35510_lead_cmd_page0_f0},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_b1), nt35510_lead_cmd_page0_b1},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_b5), nt35510_lead_cmd_page0_b5},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_b6), nt35510_lead_cmd_page0_b6},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_b7), nt35510_lead_cmd_page0_b7},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_b8), nt35510_lead_cmd_page0_b8},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_bc), nt35510_lead_cmd_page0_bc},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_c9), nt35510_lead_cmd_page0_c9},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_bd), nt35510_lead_cmd_page0_bd},
//{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_b1_mirror), nt35510_lead_cmd_page0_b1_mirror},
{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(nt35510_lead_cmd_page0_e0), nt35510_lead_cmd_page0_e0},

{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};
/** ZTEBSP zhoufan modify for NT35110B LEAD IPS LCD-- 20130226**/

/**************************************
7. nt35510 yushun start 
**************************************/
/*
//static char cmd_page_f3[9] = {0xf3, 0x00,0x32,0x00,0x38,0x31,0x08,0x11,0x00};
static char nt3511_yushun_cmd_page0_f0[6] = {0xf0, 0x55,0xaa,0x52,0x08,0x00};
static char nt3511_yushun_cmd_page0_b0[6] = {0xb0, 0x04,0x0a,0x0e,0x09,0x04};
static char nt3511_yushun_cmd_page0_b1[3] = {0xb1, 0x18,0x04};
static char nt3511_yushun_cmd_page0_36[2] = {0x36, 0x90};
static char nt3511_yushun_cmd_page0_b3[2] = {0xb3, 0x00};
static char nt3511_yushun_cmd_page0_b6[2] = {0xb6, 0x03};
static char nt3511_yushun_cmd_page0_b7[3] = {0xb7, 0x70,0x70};
static char nt3511_yushun_cmd_page0_b8[5] = {0xb8, 0x00,0x06,0x06,0x06};
static char nt3511_yushun_cmd_page0_bc[4] = {0xbc, 0x00,0x00,0x00};
static char nt3511_yushun_cmd_page0_bd[6] = {0xbd, 0x01,0x84,0x06,0x50,0x00};
static char nt3511_yushun_cmd_page0_cc[4] = {0xcc, 0x03,0x01,0x06};

static char nt3511_yushun_cmd_page1_f0[6] = {0xf0, 0x55,0xaa,0x52,0x08,0x01};
static char nt3511_yushun_cmd_page1_b0[4] = {0xb0, 0x05,0x05,0x05};
static char nt3511_yushun_cmd_page1_b1[4] = {0xb1, 0x05,0x05,0x05};
static char nt3511_yushun_cmd_page1_b2[4] = {0xb2, 0x03,0x03,0x03};
static char nt3511_yushun_cmd_page1_b8[4] = {0xb8, 0x25,0x25,0x25};
static char nt3511_yushun_cmd_page1_b3[4] = {0xb3, 0x0b,0x0b,0x0b};
static char nt3511_yushun_cmd_page1_b9[4] = {0xb9, 0x34,0x34,0x34};
static char nt3511_yushun_cmd_page1_bf[2] = {0xbf, 0x01};
static char nt3511_yushun_cmd_page1_b5[4] = {0xb5, 0x08,0x08,0x08};
static char nt3511_yushun_cmd_page1_ba[4] = {0xba, 0x24,0x24,0x24};
static char nt3511_yushun_cmd_page1_b4[4] = {0xb4, 0x2e,0x2e,0x2e};
static char nt3511_yushun_cmd_page1_bc[4] = {0xbc, 0x00,0x68,0x00};
static char nt3511_yushun_cmd_page1_bd[4] = {0xbd, 0x00,0x7c,0x00};
static char nt3511_yushun_cmd_page1_be[3] = {0xbe, 0x00,0x45};
static char nt3511_yushun_cmd_page1_d0[5] = {0xd0, 0x0c,0x15,0x0b,0x0e};

static char nt3511_yushun_cmd_page1_d1[53] = {0xd1, 0x00,0x37,0x00,0x61,0x00,0x92,0x00,0xB4,0x00,0xCF,0x00
,0xF6,0x01,0x2F,0x01,0x7F,0x01,0x97,0x01,0xC0,0x01,0xE5,0x02,0x25,0x02,0x5E,0x02,0x60,0x02
,0x87,0x02,0xBE,0x02,0xE2,0x03,0x0F,0x03,0x30,0x03,0x5C,0x03,0x77,0x03,0x94,0x03,0x9F,0x03
,0xAC,0x03,0xBA,0x03,0xF1};
static char nt3511_yushun_cmd_page1_d2[53] = {0xd2, 0x00,0x37,0x00,0x61,0x00,0x92,0x00,0xB4,0x00,0xCF,0x00
,0xF6,0x01,0x2F,0x01,0x7F,0x01,0x97,0x01,0xC0,0x01,0xE5,0x02,0x25,0x02,0x5E,0x02,0x60,0x02
,0x87,0x02,0xBE,0x02,0xE2,0x03,0x0F,0x03,0x30,0x03,0x5C,0x03,0x77,0x03,0x94,0x03,0x9F,0x03
,0xAC,0x03,0xBA,0x03,0xF1};
static char nt3511_yushun_cmd_page1_d3[53] = {0xd3, 0x00,0x37,0x00,0x61,0x00,0x92,0x00,0xB4,0x00,0xCF,0x00
,0xF6,0x01,0x2F,0x01,0x7F,0x01,0x97,0x01,0xC0,0x01,0xE5,0x02,0x25,0x02,0x5E,0x02,0x60,0x02
,0x87,0x02,0xBE,0x02,0xE2,0x03,0x0F,0x03,0x30,0x03,0x5C,0x03,0x77,0x03,0x94,0x03,0x9F,0x03
,0xAC,0x03,0xBA,0x03,0xF1};
static char nt3511_yushun_cmd_page1_d4[53] = {0xd4, 0x00,0x37,0x00,0x50,0x00,0x89,0x00,0xA9,0x00,0xC0,0x00
,0xF6,0x01,0x14,0x01,0x48,0x01,0x6B,0x01,0xA7,0x01,0xD3,0x02,0x17,0x02,0x4F,0x02,0x51,0x02
,0x86,0x02,0xBD,0x02,0xE2,0x03,0x0F,0x03,0x30,0x03,0x5C,0x03,0x77,0x03,0x94,0x03,0x9F,0x03
,0xAC,0x03,0xBA,0x03,0xF1};
static char nt3511_yushun_cmd_page1_d5[53] = {0xd5, 0x00,0x37,0x00,0x50,0x00,0x89,0x00,0xA9,0x00,0xC0,0x00
,0xF6,0x01,0x14,0x01,0x48,0x01,0x6B,0x01,0xA7,0x01,0xD3,0x02,0x17,0x02,0x4F,0x02,0x51,0x02
,0x86,0x02,0xBD,0x02,0xE2,0x03,0x0F,0x03,0x30,0x03,0x5C,0x03,0x77,0x03,0x94,0x03,0x9F,0x03
,0xAC,0x03,0xBA,0x03,0xF1};
static char nt3511_yushun_cmd_page1_d6[53] = {0xd6, 0x00,0x37,0x00,0x50,0x00,0x89,0x00,0xA9,0x00,0xC0,0x00
,0xF6,0x01,0x14,0x01,0x48,0x01,0x6B,0x01,0xA7,0x01,0xD3,0x02,0x17,0x02,0x4F,0x02,0x51,0x02
,0x86,0x02,0xBD,0x02,0xE2,0x03,0x0F,0x03,0x30,0x03,0x5C,0x03,0x77,0x03,0x94,0x03,0x9F,0x03
,0xAC,0x03,0xBA,0x03,0xF1};

static struct dsi_cmd_desc nt3511_yushun_display_on_cmds[] = {

       // yushun nt35510
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(cmd_page_ff),cmd_page_ff},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(cmd_page_f3),cmd_page_f3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_f0),nt3511_yushun_cmd_page0_f0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_b0),nt3511_yushun_cmd_page0_b0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_b1),nt3511_yushun_cmd_page0_b1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_36),nt3511_yushun_cmd_page0_36},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_b3),nt3511_yushun_cmd_page0_b3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_b6),nt3511_yushun_cmd_page0_b6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_b7),nt3511_yushun_cmd_page0_b7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_b8),nt3511_yushun_cmd_page0_b8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_bc),nt3511_yushun_cmd_page0_bc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_bd),nt3511_yushun_cmd_page0_bd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_cc),nt3511_yushun_cmd_page0_cc},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_f0),nt3511_yushun_cmd_page1_f0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b0),nt3511_yushun_cmd_page1_b0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b1),nt3511_yushun_cmd_page1_b1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b2),nt3511_yushun_cmd_page1_b2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b8),nt3511_yushun_cmd_page1_b8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b3),nt3511_yushun_cmd_page1_b3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b9),nt3511_yushun_cmd_page1_b9},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_bf),nt3511_yushun_cmd_page1_bf},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b5),nt3511_yushun_cmd_page1_b5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_ba),nt3511_yushun_cmd_page1_ba},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b4),nt3511_yushun_cmd_page1_b4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_bc),nt3511_yushun_cmd_page1_bc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_bd),nt3511_yushun_cmd_page1_bd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_be),nt3511_yushun_cmd_page1_be},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d0),nt3511_yushun_cmd_page1_d0},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d1), nt3511_yushun_cmd_page1_d1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d2), nt3511_yushun_cmd_page1_d2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d3), nt3511_yushun_cmd_page1_d3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d4), nt3511_yushun_cmd_page1_d4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d5), nt3511_yushun_cmd_page1_d5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d6), nt3511_yushun_cmd_page1_d6},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};

*/
/**************************************
P825A10 4.3  yushun nt35510 mipi-4.3  wangminrong start 
**************************************/

//page 0
static char nt3511_yushun_cmd_page0_f0[6] = {0xf0, 0x55,0xaa,0x52,0x08,0x00};

static char nt3511_yushun_cmd_page0_b1[3] = {0xb1, 0xDC,0x00};

static char nt3511_yushun_cmd_page0_b6[2] = {0xb6, 0x05};
static char nt3511_yushun_cmd_page0_b7[3] = {0xb7, 0x70,0x70};
static char nt3511_yushun_cmd_page0_b8[5] = {0xb8, 0x01,0x03,0x03,0x03};
static char nt3511_yushun_cmd_page0_bc[4] = {0xbc, 0x02,0x00,0x00};

static char nt3511_yushun_cmd_page0_c9[6] = {0xc9, 0xD0,0x02,0x50,0x50,0x50};

//page 1
static char nt3511_yushun_cmd_page1_f0[6] = {0xf0, 0x55,0xaa,0x52,0x08,0x01};
static char nt3511_yushun_cmd_page1_b0[4] = {0xb0, 0x0D,0x0D,0x0D};
static char nt3511_yushun_cmd_page1_b6[4] = {0xb6, 0x44,0x44,0x44};
static char nt3511_yushun_cmd_page1_b1[4] = {0xb1, 0x0D,0x0D,0x0D};
static char nt3511_yushun_cmd_page1_b7[4] = {0xb7, 0x34,0x34,0x34};
static char nt3511_yushun_cmd_page1_b2[4] = {0xb2, 0x00,0x00,0x00};
static char nt3511_yushun_cmd_page1_b8[4] = {0xb8, 0x34,0x34,0x34};
static char nt3511_yushun_cmd_page1_bf[2] = {0xbf, 0x01};
static char nt3511_yushun_cmd_page1_b3[4] = {0xb3, 0x0f,0x0f,0x0f};
static char nt3511_yushun_cmd_page1_b9[4] = {0xb9, 0x34,0x34,0x34};

static char nt3511_yushun_cmd_page1_b5[4] = {0xb5, 0x08,0x08,0x08};
static char nt3511_yushun_cmd_page1_c2[2] = {0xc2, 0x03};

static char nt3511_yushun_cmd_page1_ba[4] = {0xba, 0x34,0x34,0x34};

static char nt3511_yushun_cmd_page1_bc[4] = {0xbc, 0x00,0x78,0x00};
static char nt3511_yushun_cmd_page1_bd[4] = {0xbd, 0x00,0x78,0x00};
static char nt3511_yushun_cmd_page1_be[3] = {0xbe, 0x00,0x64};
//static char nt3511_yushun_cmd_page1_d0[5] = {0xd0, 0x0c,0x15,0x0b,0x0e};

static char nt3511_yushun_cmd_page1_d1[53] = {0xd1, 0x00,0x33,0x00,0x34,0x00,0x3A,0x00,0x4A,0x00,0x5C,0x00
,0x81,0x00,0xA6,0x00,0xE5,0x01,0x13,0x01,0x54,0x01,0x82,0x01,0xCA,0x02,0x00,0x02,0x01,0x02
,0x34,0x02,0x67,0x02,0x84,0x02,0xA4,0x02,0xB7,0x02,0xCF,0x02,0xDE,0x02,0xF2,0x02,0xFE,0x03
,0x10,0x03,0x33,0x03,0x6D};
//
static char nt3511_yushun_cmd_page1_d2[53] = {0xd1, 0x00,0x33,0x00,0x34,0x00,0x3A,0x00,0x4A,0x00,0x5C,0x00
,0x81,0x00,0xA6,0x00,0xE5,0x01,0x13,0x01,0x54,0x01,0x82,0x01,0xCA,0x02,0x00,0x02,0x01,0x02
,0x34,0x02,0x67,0x02,0x84,0x02,0xA4,0x02,0xB7,0x02,0xCF,0x02,0xDE,0x02,0xF2,0x02,0xFE,0x03
,0x10,0x03,0x33,0x03,0x6D};
//
static char nt3511_yushun_cmd_page1_d3[53] = {0xd1, 0x00,0x33,0x00,0x34,0x00,0x3A,0x00,0x4A,0x00,0x5C,0x00
,0x81,0x00,0xA6,0x00,0xE5,0x01,0x13,0x01,0x54,0x01,0x82,0x01,0xCA,0x02,0x00,0x02,0x01,0x02
,0x34,0x02,0x67,0x02,0x84,0x02,0xA4,0x02,0xB7,0x02,0xCF,0x02,0xDE,0x02,0xF2,0x02,0xFE,0x03
,0x10,0x03,0x33,0x03,0x6D};
//
static char nt3511_yushun_cmd_page1_d4[53] = {0xd1, 0x00,0x33,0x00,0x34,0x00,0x3A,0x00,0x4A,0x00,0x5C,0x00
,0x81,0x00,0xA6,0x00,0xE5,0x01,0x13,0x01,0x54,0x01,0x82,0x01,0xCA,0x02,0x00,0x02,0x01,0x02
,0x34,0x02,0x67,0x02,0x84,0x02,0xA4,0x02,0xB7,0x02,0xCF,0x02,0xDE,0x02,0xF2,0x02,0xFE,0x03
,0x10,0x03,0x33,0x03,0x6D};
//
static char nt3511_yushun_cmd_page1_d5[53] = {0xd1, 0x00,0x33,0x00,0x34,0x00,0x3A,0x00,0x4A,0x00,0x5C,0x00
,0x81,0x00,0xA6,0x00,0xE5,0x01,0x13,0x01,0x54,0x01,0x82,0x01,0xCA,0x02,0x00,0x02,0x01,0x02
,0x34,0x02,0x67,0x02,0x84,0x02,0xA4,0x02,0xB7,0x02,0xCF,0x02,0xDE,0x02,0xF2,0x02,0xFE,0x03
,0x10,0x03,0x33,0x03,0x6D};
//
static char nt3511_yushun_cmd_page1_d6[53] = {0xd1, 0x00,0x33,0x00,0x34,0x00,0x3A,0x00,0x4A,0x00,0x5C,0x00
,0x81,0x00,0xA6,0x00,0xE5,0x01,0x13,0x01,0x54,0x01,0x82,0x01,0xCA,0x02,0x00,0x02,0x01,0x02
,0x34,0x02,0x67,0x02,0x84,0x02,0xA4,0x02,0xB7,0x02,0xCF,0x02,0xDE,0x02,0xF2,0x02,0xFE,0x03
,0x10,0x03,0x33,0x03,0x6D};
//inversion
//static char nt3511_yushun_cmd_page0_36[2] = {0x36,0xc0}; //for lcd inversion 20120517

//static char nt3511_yushun_cmd_page0_cc[4] = {0xcc, 0x03,0x01,0x06};

static struct dsi_cmd_desc nt3511_yushun_display_on_cmds[] = {

       // yushun nt35510
//wangminrong add 20120417 for yushun 4.3
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_f0),nt3511_yushun_cmd_page0_f0},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_b1),nt3511_yushun_cmd_page0_b1},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_b6),nt3511_yushun_cmd_page0_b6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_b7),nt3511_yushun_cmd_page0_b7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_b8),nt3511_yushun_cmd_page0_b8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_bc),nt3511_yushun_cmd_page0_bc},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_c9),nt3511_yushun_cmd_page0_c9},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page0_36),nt3511_yushun_cmd_page0_36}, //for lcd inversion 20120517

	

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_f0),nt3511_yushun_cmd_page1_f0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b0),nt3511_yushun_cmd_page1_b0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b6),nt3511_yushun_cmd_page1_b6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b1),nt3511_yushun_cmd_page1_b1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b7),nt3511_yushun_cmd_page1_b7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b2),nt3511_yushun_cmd_page1_b2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b8),nt3511_yushun_cmd_page1_b8},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_bf),nt3511_yushun_cmd_page1_bf},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b3),nt3511_yushun_cmd_page1_b3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b9),nt3511_yushun_cmd_page1_b9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_b5),nt3511_yushun_cmd_page1_b5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_c2),nt3511_yushun_cmd_page1_c2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_ba),nt3511_yushun_cmd_page1_ba},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_bc),nt3511_yushun_cmd_page1_bc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_bd),nt3511_yushun_cmd_page1_bd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_be),nt3511_yushun_cmd_page1_be},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d1), nt3511_yushun_cmd_page1_d1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d2), nt3511_yushun_cmd_page1_d2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d3), nt3511_yushun_cmd_page1_d3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d4), nt3511_yushun_cmd_page1_d4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d5), nt3511_yushun_cmd_page1_d5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun_cmd_page1_d6), nt3511_yushun_cmd_page1_d6},
	


	
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on},
	
};
/**************************************
P825F01 tianma NT35512 20120731
**************************************/


//static char nt3511_yushun2_cmd_page0_ff[5] = {0xff, 0xaa,0x55,0x25,0x01};
//static char nt3511_yushun2_cmd_page0_f8[14] = {0xf8, 0x01,0x02,0x00,0x20,0x33,0x13,0x00,0x40,0x00,0x00,0x23,0x02,0x19};
/*
//page 1
static char nt3511_yushun2_cmd_page1_f0[6] = {0xf0, 0x55,0xaa,0x52,0x08,0x01};
static char nt3511_yushun2_cmd_page1_b6[2] = {0xb6, 0x45};
static char nt3511_yushun2_cmd_page1_b0[4] = {0xb0, 0x0f,0x0f,0x0f};

//
static char nt3511_yushun2_cmd_page1_b7[3] = {0xb7, 0x35,0x35};
static char nt3511_yushun2_cmd_page1_b1[4] = {0xb1, 0x0f,0x0f,0x0f};
static char nt3511_yushun2_cmd_page1_ba[2] = {0xba, 0x14};
static char nt3511_yushun2_cmd_page1_b5[4] = {0xb5, 0x08,0x08,0x08};
static char nt3511_yushun2_cmd_page1_bc[4] = {0xbc, 0x00,0x78,0x00};
static char nt3511_yushun2_cmd_page1_bd[4] = {0xbd, 0x00,0x78,0x00};
static char nt3511_yushun2_cmd_page1_be[5] = {0xbe, 0x10,0x74,0x00,0x7A};
static char nt3511_yushun2_cmd_page1_b3[4] = {0xb3, 0x05,0x05,0x05};
static char nt3511_yushun2_cmd_page1_b9[4] = {0xb9, 0x14,0x14,0x14};
static char nt3511_yushun2_cmd_page1_b2[4] = {0xb2, 0x02,0x02,0x02};

static char nt3511_yushun2_cmd_page1_b8[4] = {0xb8, 0x34,0x34,0x34};











static char nt3511_yushun2_cmd_page1_d1[53] = {0xd1, 0x00,0x29,0x00,0x30,0x00,0x33,0x00,0x52,0x00,0x77,0x00
,0xB0,0x00,0xD7,0x01,0x10,0x01,0x3A,0x01,0x77,0x01,0xA2,0x01,0xE0,0x02,0x0F,0x02,0x10,0x02
,0x39,0x02,0x63,0x02,0x7A,0x02,0x94,0x02,0xA4,0x02,0xB8,0x02,0xC5,0x02,0xD4,0x02,0xDD,0x02
,0xE8,0x02,0xFB,0x03,0x60};
//
static char nt3511_yushun2_cmd_page1_d2[53] = {0xd2, 0x00,0x29,0x00,0x30,0x00,0x33,0x00,0x52,0x00,0x77,0x00
,0xB0,0x00,0xD7,0x01,0x10,0x01,0x3A,0x01,0x77,0x01,0xA2,0x01,0xE0,0x02,0x0F,0x02,0x10,0x02
,0x39,0x02,0x63,0x02,0x7A,0x02,0x94,0x02,0xA4,0x02,0xB8,0x02,0xC5,0x02,0xD4,0x02,0xDD,0x02
,0xE8,0x02,0xFB,0x03,0x00};
//
static char nt3511_yushun2_cmd_page1_d3[53] = {0xd3, 0x00,0x29,0x00,0x30,0x00,0x33,0x00,0x52,0x00,0x77,0x00
,0xB0,0x00,0xD7,0x01,0x10,0x01,0x3A,0x01,0x77,0x01,0xA2,0x01,0xE0,0x02,0x0F,0x02,0x10,0x02
,0x39,0x02,0x63,0x02,0x7A,0x02,0x94,0x02,0xA4,0x02,0xB8,0x02,0xC5,0x02,0xD4,0x02,0xDD,0x02
,0xE8,0x02,0xFB,0x02,0xD0};
//
static char nt3511_yushun2_cmd_page1_d4[53] = {0xd4, 0x00,0x29,0x00,0x30,0x00,0x33,0x00,0x52,0x00,0x77,0x00
,0xB0,0x00,0xD7,0x01,0x10,0x01,0x3A,0x01,0x77,0x01,0xA2,0x01,0xE0,0x02,0x0F,0x02,0x10,0x02
,0x39,0x02,0x63,0x02,0x7A,0x02,0x94,0x02,0xA4,0x02,0xB8,0x02,0xC5,0x02,0xD4,0x02,0xDD,0x02
,0xE8,0x02,0xFB,0x03,0x60};
//
static char nt3511_yushun2_cmd_page1_d5[53] = {0xd5, 0x00,0x29,0x00,0x30,0x00,0x33,0x00,0x52,0x00,0x77,0x00
,0xB0,0x00,0xD7,0x01,0x10,0x01,0x3A,0x01,0x77,0x01,0xA2,0x01,0xE0,0x02,0x0F,0x02,0x10,0x02
,0x39,0x02,0x63,0x02,0x7A,0x02,0x94,0x02,0xA4,0x02,0xB8,0x02,0xC5,0x02,0xD4,0x02,0xDD,0x02
,0xE8,0x02,0xFB,0x03,0x00};
//
static char nt3511_yushun2_cmd_page1_d6[53] = {0xd6, 0x00,0x29,0x00,0x30,0x00,0x33,0x00,0x52,0x00,0x77,0x00
,0xB0,0x00,0xD7,0x01,0x10,0x01,0x3A,0x01,0x77,0x01,0xA2,0x01,0xE0,0x02,0x0F,0x02,0x10,0x02
,0x39,0x02,0x63,0x02,0x7A,0x02,0x94,0x02,0xA4,0x02,0xB8,0x02,0xC5,0x02,0xD4,0x02,0xDD,0x02
,0xE8,0x02,0xFB,0x02,0xD0};
//page 0
static char nt3511_yushun2_cmd_page0_f0[6] = {0xf0, 0x55,0xaa,0x52,0x08,0x00};
static char nt3511_yushun2_cmd_page0_b4[2] = {0xb4, 0x10};
static char nt3511_yushun2_cmd_page0_c9[6] = {0xc9, 0x01,0x02,0x50,0x50,0x50};
static char nt3511_yushun2_cmd_page0_2a[5] = {0x2a, 0x00,0x00,0x01,0xDF};
static char nt3511_yushun2_cmd_page0_2b[5] = {0x2a, 0x00,0x00,0x03,0x55};
static char nt3511_yushun2_cmd_page0_b5[2] = {0xb5, 0x6c};//wangminrong
static char nt3511_yushun2_cmd_page0_b6[2] = {0xb6, 0x05};
static char nt3511_yushun2_cmd_page0_bc[2] = {0xbc, 0x02};//20120814
static char nt3511_yushun2_cmd_page0_b1[4] = {0xb1, 0x14,0x06,0x00};

static char nt3511_yushun2_cmd_page0_b7[3] = {0xb7, 0x80,0x80};
static char nt3511_yushun2_cmd_page0_b8[5] = {0xb8, 0x01,0x07,0x07,0x07};

static char nt3511_yushun2_cmd_page0_f7[16] = {0xf7, 0x63,0x40,0x00,0x00,0x00,0x01,0xc4,0xa2,0x00,0x02,0x64,0x54,0x48,0x00,0xd0};

static struct dsi_cmd_desc nt3511_tianma_display_on_cmds_1[] = {
     
//	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_ff),nt3511_yushun2_cmd_page0_ff},

//	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_f8),nt3511_yushun2_cmd_page0_f8},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_f0),nt3511_yushun2_cmd_page1_f0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b6),nt3511_yushun2_cmd_page1_b6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b0),nt3511_yushun2_cmd_page1_b0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b7),nt3511_yushun2_cmd_page1_b7},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b1),nt3511_yushun2_cmd_page1_b1},

	

	//{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_ba),nt3511_yushun2_cmd_page1_ba},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b5),nt3511_yushun2_cmd_page1_b5},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_bc),nt3511_yushun2_cmd_page1_bc},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b5),nt3511_yushun2_cmd_page1_b5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_ba),nt3511_yushun2_cmd_page1_ba},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_bc),nt3511_yushun2_cmd_page1_bc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_bd),nt3511_yushun2_cmd_page1_bd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_be),nt3511_yushun2_cmd_page1_be},
	//
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b3),nt3511_yushun2_cmd_page1_b3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b9),nt3511_yushun2_cmd_page1_b9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b2),nt3511_yushun2_cmd_page1_b2},

		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b8),nt3511_yushun2_cmd_page1_b8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_d1), nt3511_yushun2_cmd_page1_d1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_d2), nt3511_yushun2_cmd_page1_d2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_d3), nt3511_yushun2_cmd_page1_d3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_d4), nt3511_yushun2_cmd_page1_d4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_d5), nt3511_yushun2_cmd_page1_d5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_d6), nt3511_yushun2_cmd_page1_d6},
	

	//page 0
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_f0),nt3511_yushun2_cmd_page0_f0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_b4),nt3511_yushun2_cmd_page0_b4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_c9),nt3511_yushun2_cmd_page0_c9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_2a),nt3511_yushun2_cmd_page0_2a},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_2b),nt3511_yushun2_cmd_page0_2b},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_b5),nt3511_yushun2_cmd_page0_b5},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_b6),nt3511_yushun2_cmd_page0_b6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_bc),nt3511_yushun2_cmd_page0_bc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_b1),nt3511_yushun2_cmd_page0_b1},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_b7),nt3511_yushun2_cmd_page0_b7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_b8),nt3511_yushun2_cmd_page0_b8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_f7),nt3511_yushun2_cmd_page0_f7},
	

	
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}
	
};
*/
/**************************************
P825F01 tianma NT35512 20120814--------2222222
**************************************/


//static char nt3511_yushun2_cmd_page0_ff[5] = {0xff, 0xaa,0x55,0x25,0x01};
//static char nt3511_yushun2_cmd_page0_f8[14] = {0xf8, 0x01,0x02,0x00,0x20,0x33,0x13,0x00,0x40,0x00,0x00,0x23,0x02,0x19};

//page 1
static char nt3511_yushun2_cmd_page1_f0[6] = {0xf0, 0x55,0xaa,0x52,0x08,0x01};
static char nt3511_yushun2_cmd_page1_b6[2] = {0xb6, 0x54};//20120814
static char nt3511_yushun2_cmd_page1_b0[2] = {0xb0, 0x0a};//20120814

//

static char nt3511_yushun2_cmd_page1_b1[2] = {0xb1, 0x0a};
static char nt3511_yushun2_cmd_page1_b7[2] = {0xb7, 0x44};
static char nt3511_yushun2_cmd_page1_ba[2] = {0xba, 0x14};
static char nt3511_yushun2_cmd_page1_b5[4] = {0xb5, 0x08,0x08,0x08};
static char nt3511_yushun2_cmd_page1_bc[4] = {0xbc, 0x00,0x8a,0x00};
static char nt3511_yushun2_cmd_page1_bd[4] = {0xbd, 0x00,0x8a,0x00};
static char nt3511_yushun2_cmd_page1_be[3] = {0xbe, 0x00,0x86};
static char nt3511_yushun2_cmd_page1_bf[2] = {0xbf, 0x01};//
static char nt3511_yushun2_cmd_page1_b3[2] = {0xb3, 0x05};
static char nt3511_yushun2_cmd_page1_b9[2] = {0xb9, 0x37};
static char nt3511_yushun2_cmd_page1_b2[2] = {0xb2, 0x03};

static char nt3511_yushun2_cmd_page1_b8[2] = {0xb8, 0x25};


//gramma 2.6 
/*
static char nt3511_yushun2_cmd_page1_d1[53] = {0xd1, 0x00,0x00,0x00,0x20,0x00,0x40,0x00,0x65,0x00,0x80,0x00
,0x90,0x00,0xa0,0x00,0xc0,0x00,0xf3,0x01,0x7c,0x01,0xA0,0x01,0xdf,0x02,0x10,0x02,0x12,0x02
,0x40,0x02,0x70,0x02,0x8B,0x02,0xab,0x02,0xbd,0x02,0xd2,0x02,0xe4,0x02,0xf4,0x03,0x05,0x03
,0x1c,0x03,0xc0,0x03,0xff};
//
static char nt3511_yushun2_cmd_page1_d2[53] = {0xd2, 0x00,0x00,0x00,0x20,0x00,0x40,0x00,0x65,0x00,0x80,0x00
,0x90,0x00,0xa0,0x00,0xc0,0x00,0xf3,0x01,0x7c,0x01,0xA0,0x01,0xdf,0x02,0x10,0x02,0x12,0x02
,0x40,0x02,0x70,0x02,0x8B,0x02,0xab,0x02,0xbd,0x02,0xd2,0x02,0xe4,0x02,0xf4,0x03,0x05,0x03
,0x1c,0x03,0xc0,0x03,0xff};
//
static char nt3511_yushun2_cmd_page1_d3[53] = {0xd3, 0x00,0x00,0x00,0x20,0x00,0x40,0x00,0x65,0x00,0x80,0x00
,0x90,0x00,0xa0,0x00,0xc0,0x00,0xf3,0x01,0x7c,0x01,0xA0,0x01,0xdf,0x02,0x10,0x02,0x12,0x02
,0x40,0x02,0x70,0x02,0x8B,0x02,0xab,0x02,0xbd,0x02,0xd2,0x02,0xe4,0x02,0xf4,0x03,0x05,0x03
,0x1c,0x03,0xc0,0x03,0xff};
//
static char nt3511_yushun2_cmd_page1_d4[53] = {0xd4, 0x00,0x00,0x00,0x20,0x00,0x40,0x00,0x65,0x00,0x80,0x00
,0x90,0x00,0xa0,0x00,0xc0,0x00,0xf3,0x01,0x7c,0x01,0xA0,0x01,0xdf,0x02,0x10,0x02,0x12,0x02
,0x40,0x02,0x70,0x02,0x8B,0x02,0xab,0x02,0xbd,0x02,0xd2,0x02,0xe4,0x02,0xf4,0x03,0x05,0x03
,0x1c,0x03,0xc0,0x03,0xff};
//
static char nt3511_yushun2_cmd_page1_d5[53] = {0xd5, 0x00,0x00,0x00,0x20,0x00,0x40,0x00,0x65,0x00,0x80,0x00
,0x90,0x00,0xa0,0x00,0xc0,0x00,0xf3,0x01,0x7c,0x01,0xA0,0x01,0xdf,0x02,0x10,0x02,0x12,0x02
,0x40,0x02,0x70,0x02,0x8B,0x02,0xab,0x02,0xbd,0x02,0xd2,0x02,0xe4,0x02,0xf4,0x03,0x05,0x03
,0x1c,0x03,0xc0,0x03,0xff};
//
static char nt3511_yushun2_cmd_page1_d6[53] = {0xd6, 0x00,0x00,0x00,0x20,0x00,0x40,0x00,0x65,0x00,0x80,0x00
,0x90,0x00,0xa0,0x00,0xc0,0x00,0xf3,0x01,0x7c,0x01,0xA0,0x01,0xdf,0x02,0x10,0x02,0x12,0x02
,0x40,0x02,0x70,0x02,0x8B,0x02,0xab,0x02,0xbd,0x02,0xd2,0x02,0xe4,0x02,0xf4,0x03,0x05,0x03
,0x1c,0x03,0xc0,0x03,0xff};
*/
static char nt3511_yushun2_cmd_page1_d1[53] = {0xd1,0x00,0x00,0x00,0x20,0x00,0x40,0x00,0x65,0x00,0x80,0x00
,0x90,0x00,0xa0,0x00,0xc0,0x00,0xf3,0x01,0x4c,0x01,0x70,0x01,0xdf,0x02,0x10,0x02,0x12,0x02
,0x40,0x02,0x70,0x02,0x8B,0x02,0xab,0x02,0xbd,0x02,0xd2,0x02,0xe4,0x02,0xf4,0x03,0x05,0x03
,0x1c,0x03,0xc0,0x03,0xff};
//
static char nt3511_yushun2_cmd_page1_d2[53] = {0xd2,0x00,0x00,0x00,0x20,0x00,0x40,0x00,0x65,0x00,0x80,0x00
,0x90,0x00,0xa0,0x00,0xc0,0x00,0xf3,0x01,0x4c,0x01,0x70,0x01,0xdf,0x02,0x10,0x02,0x12,0x02
,0x40,0x02,0x70,0x02,0x8B,0x02,0xab,0x02,0xbd,0x02,0xd2,0x02,0xe4,0x02,0xf4,0x03,0x05,0x03
,0x1c,0x03,0xc0,0x03,0xff};
//
static char nt3511_yushun2_cmd_page1_d3[53] = {0xd3,0x00,0x00,0x00,0x20,0x00,0x40,0x00,0x65,0x00,0x80,0x00
,0x90,0x00,0xa0,0x00,0xc0,0x00,0xf3,0x01,0x4c,0x01,0x70,0x01,0xdf,0x02,0x10,0x02,0x12,0x02
,0x40,0x02,0x70,0x02,0x8B,0x02,0xab,0x02,0xbd,0x02,0xd2,0x02,0xe4,0x02,0xf4,0x03,0x05,0x03
,0x1c,0x03,0xc0,0x03,0xff};
//
static char nt3511_yushun2_cmd_page1_d4[53] = {0xd4,0x00,0x00,0x00,0x20,0x00,0x40,0x00,0x65,0x00,0x80,0x00
,0x90,0x00,0xa0,0x00,0xc0,0x00,0xf3,0x01,0x4c,0x01,0x70,0x01,0xdf,0x02,0x10,0x02,0x12,0x02
,0x40,0x02,0x70,0x02,0x8B,0x02,0xab,0x02,0xbd,0x02,0xd2,0x02,0xe4,0x02,0xf4,0x03,0x05,0x03
,0x1c,0x03,0xc0,0x03,0xff};
//
static char nt3511_yushun2_cmd_page1_d5[53] = {0xd5,0x00,0x00,0x00,0x20,0x00,0x40,0x00,0x65,0x00,0x80,0x00
,0x90,0x00,0xa0,0x00,0xc0,0x00,0xf3,0x01,0x4c,0x01,0x70,0x01,0xdf,0x02,0x10,0x02,0x12,0x02
,0x40,0x02,0x70,0x02,0x8B,0x02,0xab,0x02,0xbd,0x02,0xd2,0x02,0xe4,0x02,0xf4,0x03,0x05,0x03
,0x1c,0x03,0xc0,0x03,0xff};
//
static char nt3511_yushun2_cmd_page1_d6[53] = {0xd6,0x00,0x00,0x00,0x20,0x00,0x40,0x00,0x65,0x00,0x80,0x00
,0x90,0x00,0xa0,0x00,0xc0,0x00,0xf3,0x01,0x4c,0x01,0x70,0x01,0xdf,0x02,0x10,0x02,0x12,0x02
,0x40,0x02,0x70,0x02,0x8B,0x02,0xab,0x02,0xbd,0x02,0xd2,0x02,0xe4,0x02,0xf4,0x03,0x05,0x03
,0x1c,0x03,0xc0,0x03,0xff};
//page 0
static char nt3511_yushun2_cmd_page0_f0[6] = {0xf0, 0x55,0xaa,0x52,0x08,0x00};
static char nt3511_yushun2_cmd_page0_b4[2] = {0xb4, 0x10};
static char nt3511_yushun2_cmd_page0_c9[6] = {0xc9, 0x01,0x02,0x50,0x50,0x50};
//static char nt3511_yushun2_cmd_page0_2a[5] = {0x2a, 0x00,0x00,0x01,0xDF};
//static char nt3511_yushun2_cmd_page0_2b[5] = {0x2a, 0x00,0x00,0x03,0x55};
static char nt3511_yushun2_cmd_page0_b5[2] = {0xb5, 0x6c};//wangminrong
static char nt3511_yushun2_cmd_page0_b6[2] = {0xb6, 0x05};
static char nt3511_yushun2_cmd_page0_bc[4] = {0xbc, 0x00,0x88,0x00};//20120814 //
static char nt3511_yushun2_cmd_page0_b1[4] = {0xb1, 0xFC,0x06,0x00};//20120814

static char nt3511_yushun2_cmd_page0_b7[3] = {0xb7, 0x80,0x80};
static char nt3511_yushun2_cmd_page0_b8[5] = {0xb8, 0x01,0x07,0x07,0x07};

//static char nt3511_yushun2_cmd_page0_f7[16] = {0xf7, 0x63,0x40,0x00,0x00,0x00,0x01,0xc4,0xa2,0x00,0x02,0x64,0x54,0x48,0x00,0xd0};

static struct dsi_cmd_desc nt3511_tianma_display_on_cmds_2[] = {
     
//	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_ff),nt3511_yushun2_cmd_page0_ff},

//	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_f8),nt3511_yushun2_cmd_page0_f8},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_f0),nt3511_yushun2_cmd_page1_f0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b6),nt3511_yushun2_cmd_page1_b6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b0),nt3511_yushun2_cmd_page1_b0},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b7),nt3511_yushun2_cmd_page1_b7},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b1),nt3511_yushun2_cmd_page1_b1},

	

	//{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_ba),nt3511_yushun2_cmd_page1_ba},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b5),nt3511_yushun2_cmd_page1_b5},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_bc),nt3511_yushun2_cmd_page1_bc},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b5),nt3511_yushun2_cmd_page1_b5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_ba),nt3511_yushun2_cmd_page1_ba},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_bc),nt3511_yushun2_cmd_page1_bc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_bd),nt3511_yushun2_cmd_page1_bd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_be),nt3511_yushun2_cmd_page1_be},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_bf),nt3511_yushun2_cmd_page1_bf},
	//
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b3),nt3511_yushun2_cmd_page1_b3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b9),nt3511_yushun2_cmd_page1_b9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b2),nt3511_yushun2_cmd_page1_b2},

		{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_b8),nt3511_yushun2_cmd_page1_b8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_d1), nt3511_yushun2_cmd_page1_d1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_d2), nt3511_yushun2_cmd_page1_d2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_d3), nt3511_yushun2_cmd_page1_d3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_d4), nt3511_yushun2_cmd_page1_d4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_d5), nt3511_yushun2_cmd_page1_d5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page1_d6), nt3511_yushun2_cmd_page1_d6},
	

	//page 0
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_f0),nt3511_yushun2_cmd_page0_f0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_b4),nt3511_yushun2_cmd_page0_b4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_c9),nt3511_yushun2_cmd_page0_c9},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_2a),nt3511_yushun2_cmd_page0_2a},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_2b),nt3511_yushun2_cmd_page0_2b},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_b5),nt3511_yushun2_cmd_page0_b5},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_b6),nt3511_yushun2_cmd_page0_b6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_bc),nt3511_yushun2_cmd_page0_bc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_b1),nt3511_yushun2_cmd_page0_b1},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_b7),nt3511_yushun2_cmd_page0_b7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_b8),nt3511_yushun2_cmd_page0_b8},
//	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt3511_yushun2_cmd_page0_f7),nt3511_yushun2_cmd_page0_f7},
	

	
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}
	
};

/************tianma 20120820 update ****************/
/*
static char tianma0814_page0_0xF0[6]= {0xF0,0x55,0xAA,0x52,0x08,0x00,};
static char tianma0814_page0_0xB4[2]= {0xB4,0x00,};
static char tianma0814_page0_0xC9[6]= {0xC9,0x01,0x02,0x50,0x50,0x50,};
static char tianma0814_page0_0xB5[2]= {0xB5,0x6C,};
static char tianma0814_page0_0xB6[2]= {0xB6,0x05,};
static char tianma0814_page0_0xBC[2]= {0xBC,0x02,};
static char tianma0814_page0_0xB1[4]= {0xB1,0xFC,0x06,0x00,};
static char tianma0814_page0_0xB7[3]= {0xB7,0x75,0x75,};
static char tianma0814_page0_0xB8[5]= {0xB8,0x01,0x07,0x07,0x07,};
static char tianma0814_page1_0xF0[6]= {0xF0,0x55,0xAA,0x52,0x08,0x01,};
static char tianma0814_page1_0xB0[2]= {0xB0,0x0A,};
static char tianma0814_page1_0xB6[2]= {0xB6,0x54,};
static char tianma0814_page1_0xB1[2]= {0xB1,0x0A,};
static char tianma0814_page1_0xB7[2]= {0xB7,0x44,};
static char tianma0814_page1_0xB2[2]= {0xB2,0x03,};
static char tianma0814_page1_0xB8[2]= {0xB8,0x25,};
static char tianma0814_page1_0xBF[2]= {0xBF,0x01,};
static char tianma0814_page1_0xB3[2]= {0xB3,0x05,};
static char tianma0814_page1_0xB9[2]= {0xB9,0x37,};
static char tianma0814_page1_0xBA[2]= {0xBA,0x14,};
static char tianma0814_page1_0xBC[4]= {0xBC,0x00,0x8A,0x00,};
static char tianma0814_page1_0xBD[4]= {0xBD,0x00,0x8A,0x00,};
static char tianma0814_page1_0xBE[3]= {0xBE,0x00,0x86,};
static char tianma0814_page1_0xD1[53]= {0xD1,0x00,0x00,0x00,0x30,0x00,0x33,0x00,0x52,0x00,0x77,0x00,0xB0,0x00,0xD7,0x01,0x10,0x01,0x3A,0x01,0x82,0x01,0xB9,0x01,0xF8,0x02,0x29,0x02,0x29,0x02,0x52,0x02,0x79,0x02,0x90,0x02,0xB0,0x02,0xBD,0x02,0xD2,0x02,0xDD,0x02,0xF2,0x03,0x05,0x03,0x1C,0x03,0xC0,0x03,0xFF,};
static char tianma0814_page1_0xD2[53]= {0xD2,0x00,0x00,0x00,0x30,0x00,0x33,0x00,0x52,0x00,0x77,0x00,0xB0,0x00,0xD7,0x01,0x10,0x01,0x3A,0x01,0x82,0x01,0xB9,0x01,0xF8,0x02,0x29,0x02,0x29,0x02,0x52,0x02,0x79,0x02,0x90,0x02,0xB0,0x02,0xBD,0x02,0xD2,0x02,0xDD,0x02,0xF2,0x03,0x05,0x03,0x1C,0x03,0xC0,0x03,0xFF,};
static char tianma0814_page1_0xD3[53]= {0xD3,0x00,0x00,0x00,0x20,0x00,0x40,0x00,0x65,0x00,0x80,0x00,0x90,0x00,0xA0,0x00,0xC0,0x00,0xF3,0x01,0x7C,0x01,0xA0,0x01,0xDF,0x02,0x10,0x02,0x12,0x02,0x40,0x02,0x70,0x02,0x8B,0x02,0xAB,0x02,0xBD,0x02,0xD2,0x02,0xE4,0x02,0xF4,0x03,0x05,0x03,0x1C,0x03,0xC0,0x03,0xFF,};
static char tianma0814_page1_0xD4[53]= {0xD4,0x00,0x00,0x00,0x30,0x00,0x33,0x00,0x52,0x00,0x77,0x00,0xB0,0x00,0xD7,0x01,0x10,0x01,0x3A,0x01,0x82,0x01,0xB9,0x01,0xF8,0x02,0x29,0x02,0x29,0x02,0x52,0x02,0x79,0x02,0x90,0x02,0xB0,0x02,0xBD,0x02,0xD2,0x02,0xDD,0x02,0xF2,0x03,0x05,0x03,0x1C,0x03,0xC0,0x03,0xFF,};
static char tianma0814_page1_0xD5[53]= {0xD5,0x00,0x00,0x00,0x30,0x00,0x33,0x00,0x52,0x00,0x77,0x00,0xB0,0x00,0xD7,0x01,0x10,0x01,0x3A,0x01,0x82,0x01,0xB9,0x01,0xF8,0x02,0x29,0x02,0x29,0x02,0x52,0x02,0x79,0x02,0x90,0x02,0xB0,0x02,0xBD,0x02,0xD2,0x02,0xDD,0x02,0xF2,0x03,0x05,0x03,0x1C,0x03,0xC0,0x03,0xFF,};
static char tianma0814_page1_0xD6[53]= {0xD6,0x00,0x00,0x00,0x20,0x00,0x40,0x00,0x65,0x00,0x80,0x00,0x90,0x00,0xA0,0x00,0xC0,0x00,0xF3,0x01,0x7C,0x01,0xA0,0x01,0xDF,0x02,0x10,0x02,0x12,0x02,0x40,0x02,0x70,0x02,0x8B,0x02,0xAB,0x02,0xBD,0x02,0xD2,0x02,0xE4,0x02,0xF4,0x03,0x05,0x03,0x1C,0x03,0xC0,0x03,0xFF,};


static struct dsi_cmd_desc nt3512_tianma_display_on_cmds_3[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma0814_page0_0xF0),tianma0814_page0_0xF0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(tianma0814_page0_0xB4),tianma0814_page0_0xB4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma0814_page0_0xC9),tianma0814_page0_0xC9},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(tianma0814_page0_0xB5),tianma0814_page0_0xB5},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(tianma0814_page0_0xB6),tianma0814_page0_0xB6},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(tianma0814_page0_0xBC),tianma0814_page0_0xBC},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma0814_page0_0xB1),tianma0814_page0_0xB1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma0814_page0_0xB7),tianma0814_page0_0xB7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma0814_page0_0xB8),tianma0814_page0_0xB8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma0814_page1_0xF0),tianma0814_page1_0xF0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(tianma0814_page1_0xB0),tianma0814_page1_0xB0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(tianma0814_page1_0xB6),tianma0814_page1_0xB6},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(tianma0814_page1_0xB1),tianma0814_page1_0xB1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(tianma0814_page1_0xB7),tianma0814_page1_0xB7},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(tianma0814_page1_0xB2),tianma0814_page1_0xB2},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(tianma0814_page1_0xB8),tianma0814_page1_0xB8},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(tianma0814_page1_0xBF),tianma0814_page1_0xBF},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(tianma0814_page1_0xB3),tianma0814_page1_0xB3},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(tianma0814_page1_0xB9),tianma0814_page1_0xB9},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(tianma0814_page1_0xBA),tianma0814_page1_0xBA},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma0814_page1_0xBC),tianma0814_page1_0xBC},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma0814_page1_0xBD),tianma0814_page1_0xBD},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma0814_page1_0xBE),tianma0814_page1_0xBE},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma0814_page1_0xD1),tianma0814_page1_0xD1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma0814_page1_0xD2),tianma0814_page1_0xD2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma0814_page1_0xD3),tianma0814_page1_0xD3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma0814_page1_0xD4),tianma0814_page1_0xD4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma0814_page1_0xD5),tianma0814_page1_0xD5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma0814_page1_0xD6),tianma0814_page1_0xD6},
	
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}
	
};
*/

/**************************************
P825F01 lead NT35512 20120814
**************************************/
static char nt35512_lead_para_page0_f0[6]={0xf0,0x55,0xaa,0x52,0x08,0x00}; 
static char nt35512_lead_para_page0_b5[2]={0xb5,0x6b}; 
static char nt35512_lead_para_page0_b1[3]={0xb1,0xfc,0x06}; 
static char nt35512_lead_para_page0_b6[2]={0xb6,0x03}; 
static char nt35512_lead_para_page0_b7[3]={0xb7,0x00,0x00}; 
static char nt35512_lead_para_page0_b8[5]={0xb8,0x01,0x06,0x06,0x06}; 
static char nt35512_lead_para_page0_bc[4]={0xbc,0x02,0x00,0x00};
 

static char nt35512_lead_para_page0_c8[19]={0xc8,0x01,0x00,0x46,0x1e,0x46,0x1e,0x46,0x1e,0x46,0x1e,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64}; 
static char nt35512_lead_para_page1_f0[6]={0xf0,0x55,0xaa,0x52,0x08,0x01}; 
static char nt35512_lead_para_page1_b0[4]={0xb0,0x05,0x05,0x05}; 
static char nt35512_lead_para_page1_b6[4]={0xb6,0x54,0x54,0x54}; 
static char nt35512_lead_para_page1_b1[4]={0xb1,0x05,0x05,0x05};
static char nt35512_lead_para_page1_b7[4]={0xb7,0x44,0x44,0x44};
static char nt35512_lead_para_page1_b2[4]={0xb2,0x03,0x03,0x03};
static char nt35512_lead_para_page1_b8[4]={0xb8,0x25,0x25,0x25};

static char nt35512_lead_para_page1_bf[2]={0xbf,0x01};
static char nt35512_lead_para_page1_b3[2]={0xb3,0x08};
static char nt35512_lead_para_page1_b9[4]={0xb9,0x37,0x37,0x37};

static char nt35512_lead_para_page1_c3[2]={0xc3,0x0a};
static char nt35512_lead_para_page1_ba[4]={0xba,0x24,0x24,0x24};
static char nt35512_lead_para_page1_c0[3]={0xc0,0x04,0x08};
static char nt35512_lead_para_page1_bc[4]={0xbc,0x00,0x90,0x00};
static char nt35512_lead_para_page1_bd[4]={0xbd,0x00,0x90,0x00};

static char nt35512_lead_para_page1_be[3]={0xbe,0x00,0x4b};

static char nt35512_lead_para_page1_d1[53] = {0xd1, 0x00,0x32,0x00,0x4f,0x00,0x77,0x00,0x9f,0x00,0xc0,0x00
,0xf3,0x01,0x22,0x01,0x5a,0x01,0x84,0x01,0xb8,0x01,0xe0,0x02,0x19,0x02,0x43,0x02,0x44,0x02,0x6a,0x02,0x91,
0x02,0xa7,0x02,0xc0,0x02,0xd0,0x02,0xe4,0x02,0xf1,0x03,0x03,0x03,0x10,0x03,0x23,0x03,0x4e,0x03,0xe0};

static char nt35512_lead_para_page1_d2[53] = {0xd2, 0x00,0x32,0x00,0x4f,0x00,0x77,0x00,0x9f,0x00,0xc0,0x00
,0xf3,0x01,0x22,0x01,0x5a,0x01,0x84,0x01,0xb8,0x01,0xe0,0x02,0x19,0x02,0x43,0x02,0x44,0x02,0x6a,0x02,0x91,
0x02,0xa7,0x02,0xc0,0x02,0xd0,0x02,0xe4,0x02,0xf1,0x03,0x03,0x03,0x10,0x03,0x23,0x03,0x4e,0x03,0xe0};
static char nt35512_lead_para_page1_d3[53] = {0xd3, 0x00,0x32,0x00,0x4f,0x00,0x77,0x00,0x9f,0x00,0xc0,0x00
,0xf3,0x01,0x22,0x01,0x5a,0x01,0x84,0x01,0xb8,0x01,0xe0,0x02,0x19,0x02,0x43,0x02,0x44,0x02,0x6a,0x02,0x91,
0x02,0xa7,0x02,0xc0,0x02,0xd0,0x02,0xe4,0x02,0xf1,0x03,0x03,0x03,0x10,0x03,0x23,0x03,0x4e,0x03,0xe0};

static char nt35512_lead_para_page1_d4[53] = {0xd4, 0x00,0x32,0x00,0x4f,0x00,0x77,0x00,0x9f,0x00,0xc0,0x00
,0xf3,0x01,0x22,0x01,0x5a,0x01,0x84,0x01,0xb8,0x01,0xe0,0x02,0x19,0x02,0x43,0x02,0x44,0x02,0x6a,0x02,0x91,
0x02,0xa7,0x02,0xc0,0x02,0xd0,0x02,0xe4,0x02,0xf1,0x03,0x03,0x03,0x10,0x03,0x23,0x03,0x4e,0x03,0xe0};

static char nt35512_lead_para_page1_d5[53] = {0xd5, 0x00,0x32,0x00,0x4f,0x00,0x77,0x00,0x9f,0x00,0xc0,0x00
,0xf3,0x01,0x22,0x01,0x5a,0x01,0x84,0x01,0xb8,0x01,0xe0,0x02,0x19,0x02,0x43,0x02,0x44,0x02,0x6a,0x02,0x91,
0x02,0xa7,0x02,0xc0,0x02,0xd0,0x02,0xe4,0x02,0xf1,0x03,0x03,0x03,0x10,0x03,0x23,0x03,0x4e,0x03,0xe0};

static char nt35512_lead_para_page1_d6[53] = {0xd6, 0x00,0x32,0x00,0x4f,0x00,0x77,0x00,0x9f,0x00,0xc0,0x00
,0xf3,0x01,0x22,0x01,0x5a,0x01,0x84,0x01,0xb8,0x01,0xe0,0x02,0x19,0x02,0x43,0x02,0x44,0x02,0x6a,0x02,0x91,
0x02,0xa7,0x02,0xc0,0x02,0xd0,0x02,0xe4,0x02,0xf1,0x03,0x03,0x03,0x10,0x03,0x23,0x03,0x4e,0x03,0xe0};

static struct dsi_cmd_desc nt3512_lead_display_on_cmds[] = {
     
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page0_f0),nt35512_lead_para_page0_f0},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35512_lead_para_page0_b5),nt35512_lead_para_page0_b5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page0_b1),nt35512_lead_para_page0_b1},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35512_lead_para_page0_b6),nt35512_lead_para_page0_b6},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page0_b7),nt35512_lead_para_page0_b7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page0_b8),nt35512_lead_para_page0_b8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page0_bc),nt35512_lead_para_page0_bc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page0_c8),nt35512_lead_para_page0_c8},
	
//page 1
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_f0),nt35512_lead_para_page1_f0},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_b0),nt35512_lead_para_page1_b0},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_b6),nt35512_lead_para_page1_b6},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_b1),nt35512_lead_para_page1_b1},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_b7),nt35512_lead_para_page1_b7},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_b2),nt35512_lead_para_page1_b2},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_b8),nt35512_lead_para_page1_b8},
  {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_bf),nt35512_lead_para_page1_bf},
  {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_b3),nt35512_lead_para_page1_b3},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_b9),nt35512_lead_para_page1_b9},
  {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_c3),nt35512_lead_para_page1_c3},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_ba),nt35512_lead_para_page1_ba},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_c0),nt35512_lead_para_page1_c0},
  {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_bc),nt35512_lead_para_page1_bc},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_bd),nt35512_lead_para_page1_bd},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_be),nt35512_lead_para_page1_be},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_d1),nt35512_lead_para_page1_d1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_d2),nt35512_lead_para_page1_d2},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_d3),nt35512_lead_para_page1_d3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_d4),nt35512_lead_para_page1_d4},
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_d5),nt35512_lead_para_page1_d5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35512_lead_para_page1_d6),nt35512_lead_para_page1_d6},
	
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}
	
};
/**************************************
8. 461408 truly start 
**************************************/
static char r61408_truly_lg_para_0xb0[2]={0xB0,0x04}; 
static char r61408_truly_lg_para_0xb3[3]={0xB3,0x10,0x00}; 
static char r61408_truly_lg_para_0xbd[2]={0xbd,0x00}; 
static char r61408_truly_lg_para_0xc0[3]={0xc0,0x00,0x66};
static char r61408_truly_lg_para_0xc1[16]={0xc1,0x23,0x31,0x99,0x26,0x25,0x00,
	0x10,0x28,0x0c,0x0c,0x00,0x00,0x00,0x21,0x01};
static char r61408_truly_lg_para_0xc2[7]={0xc2,0x10,0x06,0x06,0x01,0x03,0x00};
static char r61408_truly_lg_para_0xc8[25]={0xc8,0x00,0x0e,0x17,0x20,0x2e,0x4b,
	0x3b,0x28,0x19,0x11,0x0a,0x02,0x00,0x0e,0x15,0x20,0x2e,0x47,0x3b,0x28,0x19,
	0x11,0x0a,0x02};
static char r61408_truly_lg_para_0xc9[25]={0xc9,0x00,0x0e,0x17,0x20,0x2e,0x4b,
	0x3b,0x28,0x19,0x11,0x0a,0x02,0x00,0x0e,0x15,0x20,0x2e,0x47,0x3b,0x28,0x19,
	0x11,0x0a,0x02};
static char r61408_truly_lg_para_0xca[25]={0xca,0x00,0x0e,0x17,0x20,0x2e,0x4b,
	0x3b,0x28,0x19,0x11,0x0a,0x02,0x00,0x0e,0x15,0x20,0x2e,0x47,0x3b,0x28,0x19,
	0x11,0x0a,0x02};
static char r61408_truly_lg_para_0xd0[17]={0xd0,0x29,0x03,0xce,0xa6,0x0c,0x43,
	0x20,0x10,0x01,0x00,0x01,0x01,0x00,0x03,0x01,0x00};
static char r61408_truly_lg_para_0xd1[8]={0xd1,0x18,0x0c,0x23,0x03,0x75,0x02,0x50};
static char r61408_truly_lg_para_0xd3[2]={0xd3,0x33};
static char r61408_truly_lg_para_0xd5[3]={0xd5,0x2a,0x2a};
static char r61408_truly_lg_para_0xde[3]={0xde,0x01,0x51};
static char r61408_truly_lg_para_0xe6[2]={0xe6,0x51};//vcomdc flick
static char r61408_truly_lg_para_0xfa[2]={0xfa,0x03};
static char r61408_truly_lg_para_0xd6[2]={0xd6,0x28};
static char r61408_truly_lg_para_0x2a[5]={0x2a,0x00,0x00,0x01,0xdf};
static char r61408_truly_lg_para_0x2b[5]={0x2b,0x00,0x00,0x03,0x1f};
static char r61408_truly_lg_para_0x36[2]={0x36,0x00};
static char r61408_truly_lg_para_0x3a[2]={0x3a,0x77};


static struct dsi_cmd_desc r61408_truly_lg_display_on_cmds[] = 
{
	
	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xb0), r61408_truly_lg_para_0xb0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xb3), r61408_truly_lg_para_0xb3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xbd), r61408_truly_lg_para_0xbd},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xc0), r61408_truly_lg_para_0xc0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xc1), r61408_truly_lg_para_0xc1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xc2), r61408_truly_lg_para_0xc2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xc8), r61408_truly_lg_para_0xc8},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xc9), r61408_truly_lg_para_0xc9},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xca), r61408_truly_lg_para_0xca},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xd0), r61408_truly_lg_para_0xd0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xd1), r61408_truly_lg_para_0xd1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xd3), r61408_truly_lg_para_0xd3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xd5), r61408_truly_lg_para_0xd5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xde), r61408_truly_lg_para_0xde},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xe6), r61408_truly_lg_para_0xe6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xfa), r61408_truly_lg_para_0xfa},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0xd6), r61408_truly_lg_para_0xd6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0x2a), r61408_truly_lg_para_0x2a},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0x2b), r61408_truly_lg_para_0x2b},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0x36), r61408_truly_lg_para_0x36},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r61408_truly_lg_para_0x3a), r61408_truly_lg_para_0x3a},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};

//9./******************************wangminrong modify for P825F01 OTM8009A 20120719*****/
static char array_0x00[2] = {0x00,0x00};
static char array_0x03[2] = {0x00,0x03};
static char array_0x80[2]= {0x00,0x80};
static char array_0x81[2]= {0x00,0x81};
static char array_0x82[2]= {0x00,0x82};
static char array_0x90[2]= {0x00,0x90};
static char array_0x91[2]= {0x00,0x91};
static char array_0x92[2]= {0x00,0x92};
static char array_0xa6[2] = {0x00,0xa6};
static char array_0xa7[2] = {0x00,0xa7};
static char array_0xa0[2] = {0x00,0xa0};
//static char array_0xaa[2] = {0x00,0xaa};
static char array_0xb0[2] = {0x00,0xb0};
//static char array_0xba[2] = {0x00,0xba};
static char array_0xc0[2] = {0x00,0xc0};
static char array_0xd0[2] = {0x00,0xd0};
static char array_0xe0[2] = {0x00,0xe0};
static char array_0xf0[2] = {0x00,0xf0};
//static char array_0x0a[2] = {0x00,0x0a};
static char array_0xa3[2] = {0x00,0xa3};
static char array_0xb4[2] = {0x00,0xb4};
static char array_0xa1[2] = {0x00,0xa1};
static char array_0xb1[2] = {0x00,0xb1};



static char otm8009a_jdf_para_0xff00[4]={0xff,0x80,0x09,0x01};
static char otm8009a_jdf_para_0xff80[3]={0xff,0x80,0x09};
static char otm8009a_jdf_para_0xff03[2]={0xff,0x01};
static char otm8009a_jdf_para_0xb390[2]={0xb3,0x02};
static char otm8009a_jdf_para_0xb392[2]={0xb3,0x45};
static char otm8009a_jdf_para_0xb3a6[2]={0xb3,0x20};
static char otm8009a_jdf_para_0xb3a7[2]={0xb3,0x01};
//static char otm8009a_jdf_para_0xce80[7]={0xce,0x86,0x01,0x00,0x85,0x01,0x00};
static char otm8009a_jdf_para_0xce80[7]={0xce,0x85,0x01,0x00,0x84,0x01,0x00};//liyeqing change 20121025
//static char otm8009a_jdf_para_0xcea0[15]={0xCE,0x18,0x05,0x03,0x58,0x00,0x00,0x00,0x18,0x04,0x03,0x59,0x00,0x00,0x00};
static char otm8009a_jdf_para_0xcea0[15]={0xCE,0x18,0x04,0x03,0x5B,0x00,0x00,0x00,0x18,0x03,0x03,0x5C,0x00,0x00,0x00};//liyeqing change 20121025
//static char otm8009a_jdf_para_0xceaa[5]={0xce,0x5c,0x00,0x00,0x00};

//static char otm8009a_jdf_para_0xceb0[15]={0xCE,0x18,0x03,0x03,0x56,0x00,0x00,0x00,0x18,0x02,0x03,0x57,0x00,0x00,0x00};
static char otm8009a_jdf_para_0xceb0[15]={0xCE,0x18,0x02,0x03,0x5D,0x00,0x00,0x00,0x18,0x01,0x03,0x5E,0x00,0x00,0x00};//liyeqing change 20121025
//static char otm8009a_jdf_para_0xceba[5]={0xce,0x5e,0x00,0x00,0x00};

static char otm8009a_jdf_para_0xcfc0[11]={0xcf,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00};
static char otm8009a_jdf_para_0xcfd0[2]={0xcf,0x00};

static char otm8009a_jdf_para_0xcb80[11]={0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char otm8009a_jdf_para_0xcb90[16]={0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
 
static char otm8009a_jdf_para_0xcba0[16]={0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char otm8009a_jdf_para_0xcbb0[11]={0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

static char otm8009a_jdf_para_0xcbc0[16]={0xcb,0x00,0x04,0x04,0x04,0x04,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};//
static char otm8009a_jdf_para_0xcbd0[16]={0xCB,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x04,0x04,0x04,0x04,0x00,0x00,0x00,0x00};//

static char otm8009a_jdf_para_0xcbe0[11]={0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char otm8009a_jdf_para_0xcbf0[11]={0xCB,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};//

static char otm8009a_jdf_para_0xcc80[11]={0xCC,0x00,0x26,0x09,0x0b,0x01,0x25,0x00,0x00,0x00,0x00};//
static char otm8009a_jdf_para_0xcc90[16]={0xCC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x26,0x0a,0x0c,0x02};//
static char otm8009a_jdf_para_0xcca0[16]={0xCC,0x25,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};//
static char otm8009a_jdf_para_0xccb0[11]={0xCC,0x00,0x25,0x0a,0x0c,0x02,0x26,0x00,0x00,0x00,0x00};//
static char otm8009a_jdf_para_0xccc0[16]={0xCC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x25,0x09,0x0b,0x01};//
static char otm8009a_jdf_para_0xccd0[11]={0xCC,0x26,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};//

//static char otm8009a_jdf_para_0xe10a[17]={0xE1,0x0a,0x17,0x1c,0x0e,0x07,0x0f,0x0b,0x0a,0x02,0x06,0x0c,0x07,0x0e,0x0f,0x09,0x05};
//static char otm8009a_jdf_para_0xe20a[17]={0xE2,0x0a,0x17,0x1c,0x0e,0x07,0x0f,0x0b,0x0a,0x02,0x06,0x0c,0x07,0x0e,0x0f,0x09,0x05};
static char otm8009a_jdf_para_0xe100[17]={0xE1,0x05,0x0d,0x13,0x0d,0x06,0x10,0x0a,0x09,0x04,0x07,0x09,0x03,0x0b,0x18,0x16,0x0e};
static char otm8009a_jdf_para_0xe200[17]={0xE2,0x05,0x0d,0x13,0x0e,0x06,0x0f,0x0a,0x09,0x04,0x07,0x09,0x03,0x0b,0x17,0x15,0x0e};


static char otm8009a_jdf_para_0xc0a3[2]={0xc0,0x1b};
static char otm8009a_jdf_para_0xc0b4[2]={0xc0,0x50};
static char otm8009a_jdf_para_0xc481[2]={0xc4,0x04};
static char otm8009a_jdf_para_0xc580[2]={0xc5,0x03};
static char otm8009a_jdf_para_0xc582[2]={0xc5,0x03};
static char otm8009a_jdf_para_0xc590[2]={0xc5,0x96};
static char otm8009a_jdf_para_0xc591[5]={0xc5,0x1d,0x01,0x7b,0x33};

static char otm8009a_jdf_para_0xd800[3]={0xd8,0x70,0x70};
static char otm8009a_jdf_para_0xd900[2]={0xd9,0x39};//?
static char otm8009a_jdf_para_0xc5b1[2]={0xc5,0x29};
static char otm8009a_jdf_para_0xc181[2]={0xc1,0x66};//wangminrong modify 20121126 /0x55
static char otm8009a_jdf_para_0xc080[10]={0xc0,0x00,0x58,0x00,0x15,0x15,0x00,0x58,0x15,0x15};

static char otm8009a_jdf_para_0xc1a1[2]={0xc1,0x08};
//static char otm8009a_jdf_para_0xb282[2]={0xb2,0x20};
//static char otm8009a_jdf_para_0xc5b1[2]={0xc5,0x29};//?
#if defined (CONFIG_PROJECT_P865F01)
static char otm8009a_jdf_para_0x3600[2]={0x36,0xd0};   //0xc0
#else
static char otm8009a_jdf_para_0x3600[2]={0x36,0x00};    //zhoufan 20130128
#endif
/*
static struct dsi_cmd_desc otm8009a_setpassword_cmd[] = 
{
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x00), array_0x00},
};
static char otm8009a_icid_rd_para[2] = {0xa1, 0x00};  // zhoufan 20130129 0xd2--0xa1
static struct dsi_cmd_desc otm8009a_icid_rd_cmd = 
{
	DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(otm8009a_icid_rd_para), otm8009a_icid_rd_para
};

static char otm8009a_panleid_rd_para[2] = {0xDA, 0x00};
static struct dsi_cmd_desc otm8009a_panleid_rd_cmd = {
	DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(otm8009a_panleid_rd_para), otm8009a_panleid_rd_para
};
*/

static struct dsi_cmd_desc otm8009a_jdf_display_on_cmds[] = 
{
    //W_REG_OFFSET(0x00),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x00), array_0x00},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xff00), otm8009a_jdf_para_0xff00},
    //W_REG_OFFSET(0x03),
    
    // W_REG_OFFSET(0x80),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x80), array_0x80},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xff80), otm8009a_jdf_para_0xff80},
    
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x03), array_0x03},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xff03), otm8009a_jdf_para_0xff03},
    
    
    //W_REG_OFFSET(0x90),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x90), array_0x90},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xb390), otm8009a_jdf_para_0xb390},
    //W_REG_OFFSET(0x92),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x92), array_0x92},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xb392), otm8009a_jdf_para_0xb392},
    //W_REG_OFFSET(0xa6),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xa6), array_0xa6},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xb3a6), otm8009a_jdf_para_0xb3a6},

    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xa7), array_0xa7},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xb3a7), otm8009a_jdf_para_0xb3a7},

    //W_REG_OFFSET(0x80),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x80), array_0x80},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xce80), otm8009a_jdf_para_0xce80},
    //W_REG_OFFSET(0xa0),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xa0), array_0xa0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xcea0), otm8009a_jdf_para_0xcea0},
    //W_REG_OFFSET(0xaa),
    //{0x15, 1, 0, 0, 0, sizeof(array_0xaa), array_0xaa},
    //{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xceaa), otm8009a_jdf_para_0xceaa},
    //W_REG_OFFSET(0xb0),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xb0), array_0xb0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xceb0), otm8009a_jdf_para_0xceb0},
    //W_REG_OFFSET(0xba),
    //{0x15, 1, 0, 0, 0, sizeof(array_0xba), array_0xba},
    //{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xceba), otm8009a_jdf_para_0xceba},
    //W_REG_OFFSET(0xc0),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xc0), array_0xc0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xcfc0), otm8009a_jdf_para_0xcfc0},
    //W_REG_OFFSET(0xd0),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xd0), array_0xd0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xcfd0), otm8009a_jdf_para_0xcfd0},
    //W_REG_OFFSET(0x80),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x80), array_0x80},
    
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xcb80), otm8009a_jdf_para_0xcb80},
    //W_REG_OFFSET(0x90),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x90), array_0x90},
    
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xcb90), otm8009a_jdf_para_0xcb90},
    //W_REG_OFFSET(0xa0),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xa0), array_0xa0},
    
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xcba0), otm8009a_jdf_para_0xcba0},
    //W_REG_OFFSET(0xb0),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xb0), array_0xb0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xcbb0), otm8009a_jdf_para_0xcbb0},
    //W_REG_OFFSET(0xc0),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xc0), array_0xc0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xcbc0), otm8009a_jdf_para_0xcbc0},
    //W_REG_OFFSET(0xd0),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xd0), array_0xd0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xcbd0), otm8009a_jdf_para_0xcbd0},//
    //W_REG_OFFSET(0xe0),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xe0), array_0xe0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xcbe0), otm8009a_jdf_para_0xcbe0},
    //W_REG_OFFSET(0xf0),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xf0), array_0xf0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xcbf0), otm8009a_jdf_para_0xcbf0},
    //W_REG_OFFSET(0x80),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x80), array_0x80},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xcc80), otm8009a_jdf_para_0xcc80},//
    //W_REG_OFFSET(0x90),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x90), array_0x90},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xcc90), otm8009a_jdf_para_0xcc90},
    //W_REG_OFFSET(0xa0),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xa0), array_0xa0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xcca0), otm8009a_jdf_para_0xcca0},
    //W_REG_OFFSET(0xb0),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xb0), array_0xb0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xccb0), otm8009a_jdf_para_0xccb0},
    //W_REG_OFFSET(0xc0),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xc0), array_0xc0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xccc0), otm8009a_jdf_para_0xccc0},
    //W_REG_OFFSET(0xd0),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xd0), array_0xd0},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xccd0), otm8009a_jdf_para_0xccd0},
    //W_REG_OFFSET(0x0a),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x00), array_0x00},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xe100), otm8009a_jdf_para_0xe100},
    //W_REG_OFFSET(0x0a),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x00), array_0x00},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xe200), otm8009a_jdf_para_0xe200},
    //W_REG_OFFSET(0xa3),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xa3), array_0xa3},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xc0a3), otm8009a_jdf_para_0xc0a3},
    //W_REG_OFFSET(0xb4),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xb4), array_0xb4},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xc0b4), otm8009a_jdf_para_0xc0b4},
    //W_REG_OFFSET(0x81),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x81), array_0x81},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xc481), otm8009a_jdf_para_0xc481},
    //W_REG_OFFSET(0x80),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x80), array_0x80},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xc580), otm8009a_jdf_para_0xc580},
    //W_REG_OFFSET(0x82),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x82), array_0x82},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xc582), otm8009a_jdf_para_0xc582},
    //W_REG_OFFSET(0x90),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x90), array_0x90},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xc590), otm8009a_jdf_para_0xc590},
    //W_REG_OFFSET(0x91),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x91), array_0x91},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xc591), otm8009a_jdf_para_0xc591},
    //W_REG_OFFSET(0x00),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x00), array_0x00},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xd800), otm8009a_jdf_para_0xd800},
    //W_REG_OFFSET(0x00),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x00), array_0x00},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xd900), otm8009a_jdf_para_0xd900},
    //W_REG_OFFSET(0xb1),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xb1), array_0xb1},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xc5b1), otm8009a_jdf_para_0xc5b1},
    //W_REG_OFFSET(0x81),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x81), array_0x81},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xc181), otm8009a_jdf_para_0xc181},
    //W_REG_OFFSET(0x80),
    {0x15, 1, 0, 0, 0, sizeof(array_0x80), array_0x80},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xc080), otm8009a_jdf_para_0xc080},
    //W_REG_OFFSET(0xa1),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0xa1), array_0xa1},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xc1a1), otm8009a_jdf_para_0xc1a1},
    //W_REG_OFFSET(0x82),
    //{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x82), array_0x82},
    //{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xb282), otm8009a_jdf_para_0xb282},
    //W_REG_OFFSET(0x00),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x00), array_0x00},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0x3600), otm8009a_jdf_para_0x3600},
    //W_REG_OFFSET(0x80),
    {DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(array_0x80), array_0x80},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm8009a_jdf_para_0xc580), otm8009a_jdf_para_0xc580},
    {DTYPE_DCS_WRITE, 1, 0, 0, 200, sizeof(exit_sleep), exit_sleep},
    {DTYPE_DCS_WRITE, 1, 0, 0, 100, sizeof(display_on), display_on},
};	
///////wangminrong add tianma hx8369a02 ips 20120921 
static char tianma_hx8369a02_ips_para_0xB9[4]= {0xB9,0xFF,0x83,0x69,};

static char tianma_hx8369a02_ips_para_0xB1[20]= {0xB1,0x01,0x00,0x44,0x0A,0x00,0x0f,0x0f,0x2c,0x34,0x3F,0x3F,0x01,0x3A,0x01,0xE6,0xE6,0xE6,0xE6,0xE6,};

static char tianma_hx8369a02_ips_para_0xB2[3]= {0xB2,0x00,0x13};//liyeqing change 20121022

static char tianma_hx8369a02_ips_para_0xB4[6]= {0xB4,0x00,0x1d,0x80,0x00,0x00,};

static char tianma_hx8369a02_ips_para_0xB6[3]= {0xB6,0x46,0x46,};//liyeqing change 20121024

#if defined(CONFIG_PROJECT_P865F01)
static char tianma_hx8369a02_ips_para_0x36[2]= {0x36,0x10,};
#else
static char tianma_hx8369a02_ips_para_0x36[2]= {0x36,0x00,};   //zhoufan
#endif
static char tianma_hx8369a02_ips_para_0xD4[3]= {0xD4,0x12,0x04,};//liyeqing add 20121024
// zhoufan  changged for low  temperature huaping 20130222
//static char tianma_hx8369a02_ips_para_0xD5[27]= {0xD5,0x00,0x01,0x03,0x7D,0x01,0x0a,0x28,0x5a,0x11,0x13,0x00,0x00,0x60,0x04,0x71,0x05,0x00,0x00,0x71,0x05,0x60,0x04,0x07,0x0F,0x04,0x04,};//liyeqing change 20121024
static char tianma_hx8369a02_ips_para_0xD5[27]= {0xD5,0x00,0x01,0x03,0x7D,0x01,0x0a,0x05,0x70,0x11,0x13,0x00,0x00,0x60,0x04,0x71,0x05,0x00,0x00,0x71,0x05,0x60,0x04,0x07,0x0F,0x04,0x04,};//liyeqing change 20121024

static char tianma_hx8369a02_ips_para_0xE0[35]= {0xE0,0x00,0x20,0x26,0x34,0x38,0x3F,0x33,0x4B,0x09,0x13,0x0E,0x15,0x16,0x14,0x15,0x11,0x17,0x00,0x20,0x26,0x34,0x38,0x3F,0x33,0x4B,0x09,0x13,0x0E,0x15,0x16,0x14,0x15,0x11,0x17,};
#if defined(CONFIG_PROJECT_P865F01)
static char tianma_hx8369a02_ips_para_0xCC[2]= {0xCC,0x0a,};
#else
static char tianma_hx8369a02_ips_para_0xCC[2]= {0xCC,0x02,};  //zhoufan
#endif
static char tianma_hx8369a02_ips_para_0xBA[14]= {0xBA,0x00,0xA0,0xC6,0x80,0x0A,0x00,0x10,0x30,0x6F,0x02,0x11,0x18,0x40,};

static struct dsi_cmd_desc tianma_hx8369a02_ips[] = {

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma_hx8369a02_ips_para_0xB9),tianma_hx8369a02_ips_para_0xB9},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma_hx8369a02_ips_para_0xB1),tianma_hx8369a02_ips_para_0xB1},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma_hx8369a02_ips_para_0xB2),tianma_hx8369a02_ips_para_0xB2},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma_hx8369a02_ips_para_0xB4),tianma_hx8369a02_ips_para_0xB4},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma_hx8369a02_ips_para_0xB6),tianma_hx8369a02_ips_para_0xB6},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(tianma_hx8369a02_ips_para_0x36),tianma_hx8369a02_ips_para_0x36},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma_hx8369a02_ips_para_0xD4),tianma_hx8369a02_ips_para_0xD4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma_hx8369a02_ips_para_0xD5),tianma_hx8369a02_ips_para_0xD5},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma_hx8369a02_ips_para_0xE0),tianma_hx8369a02_ips_para_0xE0},

	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,sizeof(tianma_hx8369a02_ips_para_0xCC),tianma_hx8369a02_ips_para_0xCC},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,sizeof(tianma_hx8369a02_ips_para_0xBA),tianma_hx8369a02_ips_para_0xBA},
	// [ECID:0000] ZTEBSP liyeqing add for CABC of tianma hx8369a02 , begin --20121017	
	#ifdef CONFIG_BACKLIGHT_CABC
	{DTYPE_DCS_LWRITE, 1, 0, 0, 5, sizeof(tianma_hx8369a02_para_CABC_0xc9),tianma_hx8369a02_para_CABC_0xc9},
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_hx8369a02_para_CABC_0x51),tianma_hx8369a02_para_CABC_0x51},
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(tianma_hx8369a02_para_CABC_0x53),tianma_hx8369a02_para_CABC_0x53},
	//{DTYPE_DCS_WRITE1, 1, 0, 0, 5, sizeof(tianma_hx8369a02_para_CABC_0x55),tianma_hx8369a02_para_CABC_0x55},//liyeqing change 20121019
	#endif
	// [ECID:0000] ZTEBSP liyeqing add for CABC of tianma hx8369a02 , end --20121017

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_on), display_on}

};

static void lcd_panle_reset(void)
{
	gpio_direction_output(GPIO_LCD_RESET,1);
	msleep(10);
	gpio_direction_output(GPIO_LCD_RESET,0);
	msleep(10);
	gpio_direction_output(GPIO_LCD_RESET,1);
	msleep(10);
}

struct mipi_manufacture_ic {
	struct dsi_cmd_desc *readid_tx;
	int readid_len_tx;
	struct dsi_cmd_desc *readid_rx;
	int readid_len_rx;
	int mode;
};

#if 0
static int mipi_get_manufacture_icid(struct msm_fb_data_type *mfd)
{
	uint32 icid = 0;
	int i ;
	

	 struct mipi_manufacture_ic mipi_manufacture_icid[] = {
		{hx8363_setpassword_cmd,ARRAY_SIZE(hx8363_setpassword_cmd),&hx8363_icid_rd_cmd,3,1},
		{nt3511_setpassword_cmd,ARRAY_SIZE(nt3511_setpassword_cmd),&nt3511_icid_rd_cmd,3,0},
		{hx8369_setpassword_cmd,ARRAY_SIZE(hx8369_setpassword_cmd),&hx8369_icid_rd_cmd,3,1},
		{r61408_setpassword_cmd,ARRAY_SIZE(r61408_setpassword_cmd),&r61408_icid_rd_cmd,4,1},
		{otm8009a_setpassword_cmd,ARRAY_SIZE(otm8009a_setpassword_cmd),&otm8009a_icid_rd_cmd,4,0},
	 };

	for(i = 0; i < ARRAY_SIZE(mipi_manufacture_icid) ; i++)
	{	lcd_panle_reset();	
		mipi_dsi_buf_init(&fwvga_tx_buf);
		mipi_dsi_buf_init(&fwvga_rx_buf);
		mipi_set_tx_power_mode(1);	
		
		mipi_dsi_cmds_tx(&fwvga_tx_buf, mipi_manufacture_icid[i].readid_tx,mipi_manufacture_icid[i].readid_len_tx);
		mipi_dsi_cmd_bta_sw_trigger(); 
		
		if(!mipi_manufacture_icid[i].mode)
			mipi_set_tx_power_mode(0);	
		
		mipi_dsi_cmds_rx(&fwvga_tx_buf, &fwvga_rx_buf, mipi_manufacture_icid[i].readid_rx,mipi_manufacture_icid[i].readid_len_rx);

		if(mipi_manufacture_icid[i].mode)
			mipi_set_tx_power_mode(0);
		
		icid = *(uint32 *)(fwvga_rx_buf.data);
		
		printk("wangjianping debug read icid i[%d] id[0x%x]\n",i, icid & 0xffffffff);

		switch(icid & 0xffffff){
			case 0x1055:
						return NOVATEK_35510;
			case 0x6383ff:
						return HIMAX_8363;
						
			case 0x6983ff:
						return HIMAX_8369;
			case 0x142201:
						return RENESAS_R61408;
			case 0x808b01:
						return ORISE_OTM8009A;
			default:
						break;			
		}

	}
	return 0;
}

static uint32 mipi_get_commic_panleid(struct msm_fb_data_type *mfd,struct dsi_cmd_desc *para,uint32 len,int mode)
{
	uint32 panelid = 0;
	mipi_dsi_buf_init(&fwvga_tx_buf);
	mipi_dsi_buf_init(&fwvga_rx_buf);
	mipi_dsi_cmd_bta_sw_trigger(); 
	if(mode)
		mipi_set_tx_power_mode(1);
	else 
		mipi_set_tx_power_mode(0);
	mipi_dsi_cmds_rx(&fwvga_tx_buf, &fwvga_rx_buf, para,len);
	if(mode)
		mipi_set_tx_power_mode(0);
	panelid = *(uint32 *)(fwvga_rx_buf.data);
	printk("debug read panelid is %x\n",panelid & 0xffffffff);
	return panelid;
}

static uint32 mipi_get_himax8369_panleid(struct msm_fb_data_type *mfd)
{
	uint32 panleid;
	
	panleid =  mipi_get_commic_panleid(mfd,&hx8369_panleid_rd_cmd,1,1);
/*	switch((panleid>>8) & 0xff){
		case HIMAX8369_TIANMA_TN_ID:
				return (u32)LCD_PANEL_4P0_HIMAX8369_TIANMA_TN;
		case HIMAX8369_TIANMA_IPS_ID:
				return (u32)LCD_PANEL_4P0_HIMAX8369_TIANMA_IPS;
		case HIMAX8369_LEAD_ID:
				return (u32)LCD_PANEL_4P0_HIMAX8369_LEAD;
		case HIMAX8369_LEAD_HANNSTAR_ID:
				return (u32)LCD_PANEL_4P0_HIMAX8369_LEAD_HANNSTAR;
		default:
*/				
    panleid = lcd_gpio_config();
	printk("the lcd is hixmax8369  gpio is %d\r\n",panleid);
	switch(panleid)
	{
							
		case 16:	
			return (u32)LCD_PANEL_4P0_HIMAX8369_TIANMA_TN;
			break;
		default:
			return (u32)LCD_PANEL_NOPANEL;
			break;
			
	}
				
	
}
//ZHANGQI ADD for test whether OTM lcd connect start 
/*
static uint32 mipi_get_test_panleid(void)
{
		panleid = lcd_gpio_config();//wangminrong add for lcd a10
		printk("the lcd is test  gpio is %d\r\n",panleid);

		return panleid;
	
}*/
//ZHANGQI ADD for test whether OTM lcd connect end

static uint32 mipi_get_nt35510_panleid(struct msm_fb_data_type *mfd)
{
	uint32 panleid =  mipi_get_commic_panleid(mfd,&nt3511_panleid_rd_cmd,1,0);
/*	switch(panleid & 0xff){
		case NT35510_YUSHUN_ID:
				return  (u32)LCD_PANEL_4P0_NT35510_HYDIS_YUSHUN	;
		case NT35510_LEAD_ID:
				return (u32)LCD_PANEL_4P0_NT35510_LEAD;
		default:
*/		//	panleid = lcd_gpio_config();//wangminrong add for lcd a10
				printk("the lcd is nt35510  gpio is 0x%x\r\n",panleid);
					switch(panleid)
					{
						case 0x120000:
							return  (u32)LCD_PANEL_4P5_NT35512_LEAD	;//lead xinli tianma yushun 
							break;
					//	case 1:
						//	return  (u32)LCD_PANEL_4P0_NT35510_HYDIS_YUSHUN	;
					//		break;
					//	case 17:
					//		return (u32)LCD_PANEL_4P0_NT35510_HYDIS_YUSHUN;//wangminrong
					//		break;
						
						default:
                            LcdPanleID = LCD_PANEL_NOPANEL;
                            return (u32)LcdPanleID;
                            //	return (u32)LCD_PANEL_NOPANEL;
                            break;				
					}
			
	
}

/* [ECID:000000] ZTEBSP wangjianping, 20120904 add OTM8009A LCD IC driver, start */
static uint32 mipi_get_otm8009a_panleid(struct msm_fb_data_type *mfd)
{
    uint32 panleid = 0; 
    
    panleid = mipi_get_commic_panleid(mfd,&otm8009a_panleid_rd_cmd,1,0);
    
    printk("wangjianping the lcd is otm8009a  panelid is 0x%x\r\n",panleid);
    switch(panleid & 0xff)
    {
        case 0x40:
            return  (u32)LCD_PANEL_4P5_OTM8009A_JDF	;
        default:
            return (u32)LCD_PANEL_NOPANEL;
    }
}
/* [ECID:000000] ZTEBSP wangjianping, 20120904 add OTM8009A LCD IC driver, end */

static uint32 mipi_get_icpanleid(struct msm_fb_data_type *mfd )
{
	int icid = 0;
	//unsigned int panleid = -1;
	lcd_panle_reset();
	icid = mipi_get_manufacture_icid(mfd);
	printk("wangmirnong icid is 0x%x-------\r\n",icid);
  //panleid = lcd_gpio_config();//wangminrong
  //printk("wangmirnong P825F01 panleid is %d---------------\r\n",panleid);
	switch(icid){
		case HIMAX_8363:		
					LcdPanleID = LCD_PANEL_4P0_HX8363_CMI_YASSY;
					break;
		case HIMAX_8369:
					LcdPanleID = mipi_get_himax8369_panleid(mfd);
					break;
		case NOVATEK_35510:
					LcdPanleID = mipi_get_nt35510_panleid(mfd);
					break;
		case ORISE_OTM8009A:
					LcdPanleID = mipi_get_otm8009a_panleid(mfd);
					break;					
		case RENESAS_R61408:
					LcdPanleID = LCD_PANEL_4P0_R61408_TRULY_LG;
			break;
		default:
					//LcdPanleID = (u32)LCD_PANEL_4P5_NT35512_LEAD;
					//LcdPanleID =  (u32)LCD_PANEL_4P5_NT35512_TIANMA;
					//LcdPanleID = mipi_get_nt35510_panleid(mfd);
					printk("the lcd LcdPanleID is 0x%x\r\n",LcdPanleID);
					break;
	}
		
	return LcdPanleID;
}
#endif

static u32 __init get_lcdpanleid_from_bootloader(void)
{
	/*smem_global*	msm_lcd_global = (smem_global*) ioremap(SMEM_LOG_GLOBAL_BASE, sizeof(smem_global));
	
	printk("debug chip id 0x%x\n",msm_lcd_global->lcd_id);
	
	if (((msm_lcd_global->lcd_id) & 0xffff0000) == 0x09830000) 
	{ */
		switch(lcd_id_type)
		{	
			case 0x0001:
				return (u32)LCD_PANEL_4P0_HX8363_CMI_YASSY;
			case 0x0002:
				return (u32)LCD_PANEL_4P0_HIMAX8369_LEAD;
			case 0x0003:
				return (u32)LCD_PANEL_4P0_HIMAX8369_LEAD_HANNSTAR;
			case 0x0004:
				return (u32)LCD_PANEL_4P0_HIMAX8369_TIANMA_TN;
			case 0x0005:
				return (u32)LCD_PANEL_4P0_HIMAX8369_TIANMA_IPS;
			case 0x0006:
				return (u32)LCD_PANEL_4P0_NT35510_LEAD;
			case 0x0007:
				return (u32)LCD_PANEL_4P0_NT35510_HYDIS_YUSHUN;
			case 0x0008:
				return (u32)LCD_PANEL_4P0_R61408_TRULY_LG;
			case 0x000a:
				return (u32)LCD_PANEL_4P5_NT35512_TIANMA;
			case 0x000b:
				return (u32)LCD_PANEL_4P5_NT35512_LEAD;				
			case 0x000c:
				return (u32)LCD_PANEL_4P5_OTM8009A_JDF;	
			case 0x000d:
				return (u32)LCD_PANNEL_4P5_HX8369_TIANMA_IPS;						
			/** ZTEBSP zhoufan modify for NT35110B LEAD IPS LCD++ 20130226 **/		
			case 0x000e:
				return (u32)LCD_PANNEL_4P5_NT35110B_LEAD_IPS;
			/** ZTEBSP zhoufan modify for NT35110B LEAD IPS LCD-- 20130226 **/		
			default:
				break;
		}		
	//}
	return (u32)LCD_PANEL_NOPANEL;
}


static int mipi_4p3_video_fwvga_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	mipi_set_tx_power_mode(1);

/* [ECID:0000] ZTEBSP wangbing start 201205281 for lcd, start*/
if (0) {
	mipi_dsi_cmds_tx(&fwvga_tx_buf, display_off_cmds,  //yaotierui for JB
			ARRAY_SIZE(display_off_cmds));
	gpio_direction_output(GPIO_LCD_RESET,0);
	msleep(5);
	gpio_direction_output(GPIO_LCD_RESET,1);
	msleep(10);
	gpio_direction_output(121,0);
}else {
/*	mipi_dsi_cmds_tx(&fwvga_tx_buf, display_off_deep_cmds,
			ARRAY_SIZE(display_off_deep_cmds)); */
	mipi_dsi_cmds_tx(&fwvga_tx_buf, display_off_cmds, //yaotierui for JB
			ARRAY_SIZE(display_off_cmds));			
		gpio_direction_output(GPIO_LCD_RESET,0);//wangminrong modify the current big when off the lcd should be on,not ret = 0(off)
		msleep(5);
		gpio_direction_output(GPIO_LCD_RESET,1);
		msleep(10);	
}
/* [ECID:0000] ZTEBSP wangbing start 201205281 for lcd, end*/

	return 0;
}
//[ECID:617001723914]ZTEBSP wangminrong modify the white screen off the backlight and reset 20120917 start for A20 taiwan
#ifdef CONFIG_PROJECT_P825A20_S_TW
void set_lcd_backlight_off(struct msm_fb_data_type *mfd,int bl)
{

	struct msm_fb_panel_data *pdata;
	printk("wangminrong -----\r\n");
	pdata = (struct msm_fb_panel_data *)mfd->pdev->dev.platform_data;
	mfd->bl_level = bl;
	printk("wangminrong 11\r\n");
	pdata->set_backlight(mfd);
	printk("wangminrong 22\r\n");

}
#endif
static int first_time_panel_on = 1;
//[ECID:0000] ZTEBSP wangminrong start  20120618 for logo first is return 
static int first_init = 1;
//[ECID:617001723914]ZTEBSP wangminrong modify the white screen off the backlight and reset 20120917 start for A20 taiwan
#ifdef CONFIG_PROJECT_P825A20_S_TW
static int first_not_reset = 1;
#endif
static int mipi_4p3_video_fwvga_lcd_on(struct platform_device *pdev)
{
	
	struct msm_fb_data_type *mfd = platform_get_drvdata(pdev);
		//[ECID:000000] ZTEBSP wangminrong  for qualcomm continus patch 20120822 start
	 #ifdef CONFIG_CONTINOUS_LOGO
	 if(!mfd->cont_splash_done)
	 {
	 
	 	mfd->cont_splash_done = 1;
		first_init = 0;
		first_time_panel_on = 0;
		#ifndef CONFIG_PROJECT_P825A20_S_TW
		return 0;
		#endif
	 }
	 #endif
	 
	//[ECID:000000] ZTEBSP wangminrong  for qualcomm continus patch 20120822 end
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	
	if(first_time_panel_on){
		first_time_panel_on = 0;
		if(first_init == 1)
		{
			first_init = 0;
			return 0;

		}
		
	//	if(LcdPanleID != (u32)LCD_PANEL_NOPANEL)
		//	return 0;
	//	;
	/*	else
			{
				printk("wangminrong P825F01 read id-----\r\n");
				LcdPanleID = mipi_get_icpanleid(mfd);
			} */
	}

#ifdef CONFIG_PROJECT_P825A20_S_TW
if(first_not_reset == 1)
{
	first_not_reset = 0;
 printk("wangminrong lcd off 1111\r\n");  
	set_lcd_backlight_off(mfd,0);
  printk("wangminrong lcd off\r\n");  
}
#endif
	lcd_panle_reset();
	printk("mipi init start\n");
	mipi_set_tx_power_mode(1);
	switch(LcdPanleID){
		case (u32)LCD_PANEL_4P0_HX8363_CMI_YASSY:
				mipi_dsi_cmds_tx(&fwvga_tx_buf, hx8363_yassy_display_on_cmds,ARRAY_SIZE(hx8363_yassy_display_on_cmds));
				printk("HIMAX8363_YASS init ok !!\n");
				break;
				
		case (u32)LCD_PANEL_4P0_HIMAX8369_LEAD:
				mipi_dsi_cmds_tx(&fwvga_tx_buf, hx8369_lead_display_on_cmds,ARRAY_SIZE(hx8369_lead_display_on_cmds));
				printk("HIMAX8369_LEAD init ok !!\n");
				break;
				
		case (u32)LCD_PANEL_4P0_HIMAX8369_LEAD_HANNSTAR:
				mipi_dsi_cmds_tx(&fwvga_tx_buf, hx8369_lead_hannstar_display_on_cmds,ARRAY_SIZE(hx8369_lead_hannstar_display_on_cmds));
				printk("HIMAX8369_LEAD_HANNSTAR init ok !!\n");
				break;
				
		case (u32)LCD_PANEL_4P0_HIMAX8369_TIANMA_TN:
				mipi_dsi_cmds_tx(&fwvga_tx_buf, hx8369_tianma_tn_display_on_cmds,ARRAY_SIZE(hx8369_tianma_tn_display_on_cmds));
				printk("HIMAX8369_TIANMA_TN init ok !!\n");
				break;
				
		case (u32)LCD_PANEL_4P0_HIMAX8369_TIANMA_IPS:
				mipi_dsi_cmds_tx(&fwvga_tx_buf, hx8369_tianma_ips_display_on_cmds,ARRAY_SIZE(hx8369_tianma_ips_display_on_cmds));
				printk("HIMAX8369_TIANMA_IPS init ok !!\n");
				break;
				
		case (u32)LCD_PANEL_4P0_NT35510_LEAD: //wangminrong for P825A10 yushun lead
				mipi_dsi_cmds_tx(&fwvga_tx_buf, nt35510_lead_display_on_cmds,ARRAY_SIZE(nt35510_lead_display_on_cmds));
				printk("NT35510_LEAD init ok !!\n");
				break;
				
		case (u32)LCD_PANEL_4P0_NT35510_HYDIS_YUSHUN:
				mipi_dsi_cmds_tx(&fwvga_tx_buf, nt3511_yushun_display_on_cmds,ARRAY_SIZE(nt3511_yushun_display_on_cmds));
				printk("NT35510_HYDIS_YUSHUN init ok !!\n");
				break;
		case (u32)LCD_PANEL_4P0_R61408_TRULY_LG:
				mipi_dsi_cmds_tx(&fwvga_tx_buf, r61408_truly_lg_display_on_cmds,ARRAY_SIZE(r61408_truly_lg_display_on_cmds));
				printk("R61408 TRULY LG  init ok !!\n");
			break;
		//wangjianping add for otm8009A	
		case (u32)LCD_PANEL_4P5_OTM8009A_JDF:
				mipi_dsi_cmds_tx(&fwvga_tx_buf, otm8009a_jdf_display_on_cmds,ARRAY_SIZE(otm8009a_jdf_display_on_cmds));
				printk("otm8009a jingdongfang lcd  init ok !!\n");
			break;
		//wangminrong add tianma nt35512
		case (u32)LCD_PANEL_4P5_NT35512_TIANMA:
				mipi_dsi_cmds_tx(&fwvga_tx_buf, nt3511_tianma_display_on_cmds_2,ARRAY_SIZE(nt3511_tianma_display_on_cmds_2));
				printk("TIANMA NT35512 LCD init ok !!\n");
			break;
			//wangminrong add lead nt35512
		case (u32)LCD_PANEL_4P5_NT35512_LEAD:
				mipi_dsi_cmds_tx(&fwvga_tx_buf, nt3512_lead_display_on_cmds,ARRAY_SIZE(nt3512_lead_display_on_cmds));
				printk("LEAD NT35512 LCD init ok !!\n");
			break;
			//wangminrong add tianma ips
		case (u32)LCD_PANNEL_4P5_HX8369_TIANMA_IPS:
				mipi_dsi_cmds_tx(&fwvga_tx_buf, tianma_hx8369a02_ips,ARRAY_SIZE(tianma_hx8369a02_ips));
				printk("TIANMA HX8369A02 IPS LCD init ok !!\n");
			break;
		/** ZTEBSP zhoufan modify for NT35110B LEAD IPS LCD ++ 20130226 **/	
		case (u32)LCD_PANNEL_4P5_NT35110B_LEAD_IPS:
				mipi_dsi_cmds_tx(&fwvga_tx_buf, nt35510_lead_display_on_cmds,ARRAY_SIZE(nt35510_lead_display_on_cmds));
				printk("LEAD NT35110B IPS LCD init ok !!\n");
			break;
		/** ZTEBSP zhoufan modify for NT35110B LEAD IPS LCD -- 20130226 **/	
		default:
				printk("can't get icpanelid value\n");
				break;
				
	}	
	mipi_set_tx_power_mode(0);
	return 0;
}



static struct msm_fb_panel_data mipi_4p3_video_fwvga_panel_data = {
	.on		= mipi_4p3_video_fwvga_lcd_on,
	.off		= mipi_4p3_video_fwvga_lcd_off,
//[ECID 000000] zhangqi add for CABC begin
#ifdef CONFIG_BACKLIGHT_CABC
	.set_backlight = mipi_set_backlight_4p3,
#else
	.set_backlight = mipi_zte_set_backlight,
#endif
};

//[ECID 000000]zhangqi add for CABC begin
#ifdef CONFIG_BACKLIGHT_CABC
void mipi_set_backlight_4p3(struct msm_fb_data_type *mfd)
{
         /*value range is 1--32*/
	 int current_lel = mfd->bl_level;
	 //unsigned long flags;
	 
	 //[ECID:0000]ZTE_BSP maxiaoping 20120803 disable print logs,start.
	 pr_debug("zhangqi add for CABC level=%d lcd_type=%d in %s func \n ",current_lel,lcd_id_type,__func__);
	 //[ECID:0000]ZTE_BSP maxiaoping 20120803 disable print logs,end.
	 
	 if (current_lel >32)
	 	{
	 			printk("Backlight level >32 ? return error. CABC level=%d in %s func \n ",current_lel,__func__);
	 			return;
	 	}
	 	
	 //mipi_zte_set_backlight(mfd);//maxiaoping 20130201


	 if (lcd_id_type == 12 )
	 {
	   //[ECID:0000]ZTE_BSP maxiaoping 20120803 disable print logs,start.
	   pr_debug("wangminrong ----- add for CABC it is a OTM8009A IC \n");
	   //[ECID:0000]ZTE_BSP maxiaoping 20120803 disable print logs,end.
	   if(current_lel==0)
	   {
	    mipi_set_tx_power_mode(0);
		  mipi_dsi_cmds_tx(&fwvga_tx_buf, otm8009a_display_off_CABC_backlight_cmds,ARRAY_SIZE(otm8009a_display_off_CABC_backlight_cmds));
			mipi_set_tx_power_mode(1);
			//msleep(500);//maxiaoping 20130201

	   }
	   else
	   {
		//[ECID:0000]ZTE_BSP liyeqing add for derease the level of LCD's brightness 20121015, begin
		if (current_lel>23)
		{
			current_lel=23;
		}
		//[ECID:0000]ZTE_BSP liyeqing add for derease the level of LCD's brightness 20121015, end		
		otm8009a_para_CABC_0x51[1]=(8*current_lel-1);     
		mipi_set_tx_power_mode(0);
		  mipi_dsi_cmds_tx(&fwvga_tx_buf, otm8009a_display_on_CABC_backlight_cmds,ARRAY_SIZE(otm8009a_display_on_CABC_backlight_cmds));
 			mipi_set_tx_power_mode(1);
	   }

	 }
	//[ECID:0000]ZTE_BSP liyeqing add for CABC of tianma hx8369a02 , begin --20121012	
	else if (lcd_id_type == 13 )
	 {
	   //[ECID:0000]ZTE_BSP maxiaoping 20120803 disable print logs,start.
	   pr_debug("liyeqing ----- add for CABC it is a HX8369A02 IC \n");
	   //[ECID:0000]ZTE_BSP maxiaoping 20120803 disable print logs,end.
	   if(current_lel==0)
	   {
	    mipi_set_tx_power_mode(0);//20121026
		  mipi_dsi_cmds_tx(&fwvga_tx_buf, tianma_hx8369a02_display_off_CABC_backlight_cmds,ARRAY_SIZE(tianma_hx8369a02_display_off_CABC_backlight_cmds));
			mipi_set_tx_power_mode(1);
			//msleep(500);//maxiaoping 20130201
			//msleep(50);

	   }
	   else
	   {
		//[ECID:0000]ZTE_BSP liyeqing add for decrease the level of LCD's brightness 20121015, begin
		if (current_lel>23)
		{
			current_lel=23;
		}
		//[ECID:0000]ZTE_BSP liyeqing add for decrease the level of LCd's brightness 20121015, end		
	    tianma_hx8369a02_para_CABC_0x51[1]=(8*current_lel-1);	    
	    mipi_set_tx_power_mode(0);
		  mipi_dsi_cmds_tx(&fwvga_tx_buf, tianma_hx8369a02_display_on_CABC_backlight_cmds,ARRAY_SIZE(tianma_hx8369a02_display_on_CABC_backlight_cmds));
 			mipi_set_tx_power_mode(1);
	   }

	 }
	//[ECID:0000]ZTE_BSP liyeqing add for CABC of tianma hx8369a02 , end --20121012
	/** ZTEBSP zhoufan modify for NT35110B LEAD IPS LCD++ 20130226 **/	
	else if (lcd_id_type == 14 )
	{
	   if(current_lel==0)
	   {
	    mipi_set_tx_power_mode(0);
		  mipi_dsi_cmds_tx(&fwvga_tx_buf, nt35110_display_off_CABC_backlight_cmds,ARRAY_SIZE(nt35110_display_off_CABC_backlight_cmds));
			mipi_set_tx_power_mode(1);
			msleep(500);
	   }
	   else
	   {
		nt35110_para_CABC_0x51[1]=(8*current_lel-1);     
		mipi_set_tx_power_mode(0);
		  mipi_dsi_cmds_tx(&fwvga_tx_buf, nt35110_display_on_CABC_backlight_cmds,ARRAY_SIZE(nt35110_display_on_CABC_backlight_cmds));
 			mipi_set_tx_power_mode(1);
	   }
	 }
	/** ZTEBSP zhoufan modify for NT35110B LEAD IPS LCD-- 20130226 **/	
	 else
	 	 printk("zhangqi add for CABC it is ?? IC \n");
	 return;
}
#endif
//[ECID 000000]zhangqi add for CABC end

static int ch_used[3];

int mipi_4p3_video_fwvga_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_4p3_video_fwvga", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	mipi_4p3_video_fwvga_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &mipi_4p3_video_fwvga_panel_data,
		sizeof(mipi_4p3_video_fwvga_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}


static int __devinit mipi_4p3_video_fwvga_lcd_probe(struct platform_device *pdev)
{	
//	struct msm_panel_info   *pinfo =&( ((struct msm_fb_panel_data  *)(pdev->dev.platform_data))->panel_info);

	if (pdev->id == 0) return 0;

	mipi_dsi_buf_alloc(&fwvga_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&fwvga_rx_buf, DSI_BUF_SIZE);
	
	if((LcdPanleID = get_lcdpanleid_from_bootloader() )==(u32)LCD_PANEL_NOPANEL)
		printk("cann't get get_lcdpanelid from bootloader\n");

    //printk("mipi_lead_lcd_probe LcdPanleID %d\n", LcdPanleID);

/*	
	if (LcdPanleID ==LCD_PANEL_4P0_R61408_TRULY_LG)//this panel is different from others
	{
		pinfo->lcdc.h_back_porch = 80;
		pinfo->lcdc.h_front_porch = 180;	
		pinfo->lcdc.v_back_porch = 12;	
	}
*/

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_4p3_video_fwvga_lcd_probe,
	.driver = {
		.name   = "mipi_4p3_video_fwvga",
	},
};

static int __init mipi_4p3_video_fwvga_lcd_init(void)
{
	return platform_driver_register(&this_driver);
}

module_init(mipi_4p3_video_fwvga_lcd_init);
//[ECID:0000] ZTEBSP wangminrong end 20120411 for lcd jianrong 
