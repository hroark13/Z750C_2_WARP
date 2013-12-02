/*
 *  kernel/drivers/misc/zte_board.c
 *
 *  Copyright (C) 2012 ZTE
 *
 * Maxiaoping 20120521 Modified for MSM8X25 zte hardware board ID control.
 */
 
/* modified history
 
         when             who   what  where
     12/05/21         maxiaoping  add file and code,add a macro to distinguish 2 gpios and 3 gpios method.
 */
 
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/console.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/nmi.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>			/* For in_interrupt() */
#include <linux/delay.h>
#include <linux/smp.h>
#include <linux/security.h>
#include <linux/bootmem.h>
#include <linux/syscalls.h>
#include <linux/kexec.h>
#include <linux/kdb.h>
#include <linux/ratelimit.h>
#include <linux/kmsg_dump.h>
#include <linux/syslog.h>
#include <linux/cpu.h>
#include <linux/notifier.h>
#include <asm/uaccess.h>

//[ECID:0000]ZTE_BSP maxiaoping 20120726 disable debug logs,start.
//[ECID:0000] ZTEBSP maxiaoping 20120521  for MSM8X25  hardware board it acquisition,start.
#include <linux/gpio.h>

/*[ECID:000000] ZTEBSP yaotierui  for 8x25 FOUR GPIOS VERSION start 20130108*/
#ifdef CONFIG_HW_BOARD_ID_USE_FOUR_GPIOS
#define HARDWARE_VERSION0  6 //GPIO_6
#define HARDWARE_VERSION1  7 //GPIO_7
#define HARDWARE_VERSION2  8 //GPIO_8
#define HARDWARE_VERSION3  33
#else
//add gpio input detction.
#ifdef CONFIG_HW_BOARD_ID_USE_THREE_GPIOS
#if defined(CONFIG_PROJECT_V72A)
#define HARDWARE_VERSION0  124 //GPIO_124
#define HARDWARE_VERSION1  123//GPIO_123
#define HARDWARE_VERSION2  122 //GPIO_122
#else
#define HARDWARE_VERSION0  6 //GPIO_6
#define HARDWARE_VERSION1  7 //GPIO_7
#define HARDWARE_VERSION2  8 //GPIO_8
#endif
#else
#define HARDWARE_VERSION0  8 //GPIO_8
#define HARDWARE_VERSION1  6 //GPIO_6
#endif
#endif


/*[ECID:000000] ZTEBSP yaotierui  for 8x25 FOUR GPIOS VERSION start 20130108*/
#ifdef CONFIG_HW_BOARD_ID_USE_FOUR_GPIOS
static unsigned hardware_id_gpio[4]=
{
	GPIO_CFG(HARDWARE_VERSION0, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(HARDWARE_VERSION1, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(HARDWARE_VERSION2, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(HARDWARE_VERSION3, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};
#else
#ifdef CONFIG_HW_BOARD_ID_USE_THREE_GPIOS
static unsigned hardware_id_gpio[3]=
{
	GPIO_CFG(HARDWARE_VERSION0, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(HARDWARE_VERSION1, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(HARDWARE_VERSION2, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};
#else
static unsigned hardware_id_gpio[2]=
{
	GPIO_CFG(HARDWARE_VERSION0, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(HARDWARE_VERSION1, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};
#endif
#endif

typedef struct 
{
    unsigned long voltage_mv;
    //char id[5];
    char id[13];
} zte_hardware_version_type;

//按照电压从小到大排列硬件版本号，版本号最长4个字符
static zte_hardware_version_type zte_hardware_version[]=
{
	//{电压(mV)，版本号}
	
	#ifdef CONFIG_PROJECT_P825A20
    	{900,"p8kB"},
	{900,"p8kB"},
	{900,"p8kB"}
	#elif defined(CONFIG_PROJECT_P865E01)
	{900,"cx9A"},
	{900,"cx9A"},
	{900,"cx9A"}
	#elif defined ( CONFIG_PROJECT_P825A10 ) 
	{900,"dt5A"},
	{900,"dt5B"},
	{900,"dt5C"}
	#elif defined ( CONFIG_PROJECT_P825F01 )
	{900,"wfbA"},
	{900,"wfbB"},
	{900,"wfbC"}
	#elif defined ( CONFIG_PROJECT_P865F01 ) 
	{900,"cy1B"},
	{900,"cy1B"},
	{900,"cy1B"}
	#elif defined(CONFIG_PROJECT_P865F02)
	{900,"wesA"},
	{900,"cy8A"},
	{900,"cx8B"}	
	#elif defined (CONFIG_PROJECT_P865E02) 
	{900,"ct6A"},
	{900,"ct6B"},
	{900,"ct6C"}	
	#elif defined (CONFIG_PROJECT_V72A) 
	{900,"dcuA"},
	{900,"dcuB"},
	{900,"dcuC"}	
	#elif defined ( CONFIG_PROJECT_P865V30 )    //need check
	{900,"Z796CHWV1.0"},
	{900,"Z796CHWV2.0"},  
	{900,"Z796CHWV3.0"}, 
	{900,"Z796CHWV4.0"},
	{900,"Z796CHWV5.0"},  
	{900,"Z796CHWV6.0"},  
	{900,"Z796CHWV7.0"},
	{900,"Z796CHWV8.0"},  
	{900,"Z796CHWV9.0"},  
	{900,"Z796CHWV10.0"},
	{900,"Z796CHWV11.0"},  
	{900,"Z796CHWV12.0"},  
	{900,"Z796CHWV13.0"},  
	{900,"Z796CHWV14.0"},
	{900,"Z796CHWV15.0"},  
	{900,"Z796CHWV16.0"}, 	
	#elif defined ( CONFIG_PROJECT_P865V20 )    //need check
	{900,"cccA"},
	{900,"cccB"},  
	{900,"cccC"},   
	#else
	{900,"dt3A"},
	{900,"dt3B"},
	{900,"dt3C"}
	#endif
};

static struct kobject *zte_board_id_kobj;

static int zte_hw_board_initialize = 0;

static void zte_hw_board_id_gpio_init(void)
{
	int i,ret = 0;

	/*[ECID:000000] ZTEBSP yaotierui  for 8x25 FOUR GPIOS VERSION start 20130108*/
    #ifdef CONFIG_HW_BOARD_ID_USE_FOUR_GPIOS
	for(i=0;i<4;i++)
    #else
	//config 3 detect gpios as input down
	#ifdef CONFIG_HW_BOARD_ID_USE_THREE_GPIOS
	for(i=0;i<3;i++)
	#else
	for(i=0;i<2;i++)
	#endif
    #endif
	{
		ret = gpio_tlmm_config(hardware_id_gpio[i],GPIO_CFG_ENABLE);
		if(ret)
		{
			printk(KERN_NOTICE "PM_DEBUG_MXP:config HARDWARE_VERSION0 gpio error:%d,ret=%d.\r\n",i,ret);
		}
	}
	//request gpios
	gpio_request(HARDWARE_VERSION0, "hw_board_id0");
	gpio_request(HARDWARE_VERSION1, "hw_board_id1");

	/*[ECID:000000] ZTEBSP yaotierui  for 8x25 FOUR GPIOS VERSION start 20130108*/
       #ifdef CONFIG_HW_BOARD_ID_USE_FOUR_GPIOS
	   gpio_request(HARDWARE_VERSION2, "hw_board_id2");
	   gpio_request(HARDWARE_VERSION3, "hw_board_id3");
	#else
	    #ifdef CONFIG_HW_BOARD_ID_USE_THREE_GPIOS
	    gpio_request(HARDWARE_VERSION2, "hw_board_id2");
	    #endif
	#endif

	//change the var,init only once.
	zte_hw_board_initialize = 1;
}

int zte_get_hw_board_id(void)
{
	int result=0;
	int rtl = 0;
	
	pr_debug( "PM_DEBUG_MXP:Enter zte_get_hw_board_id.\r\n");
	//printk(KERN_NOTICE "PM_DEBUG_MXP:zte_hw_board_initialize =%d.\r\n",zte_hw_board_initialize);
	if(zte_hw_board_initialize == 0)
	{
		zte_hw_board_id_gpio_init();
	}
	
	//set gpios as input
	gpio_direction_input(HARDWARE_VERSION0);
	gpio_direction_input(HARDWARE_VERSION1);

		/*[ECID:000000] ZTEBSP yaotierui  for 8x25 FOUR GPIOS VERSION start 20130108*/
       #ifdef CONFIG_HW_BOARD_ID_USE_FOUR_GPIOS
	    gpio_direction_input(HARDWARE_VERSION2);
	    gpio_direction_input(HARDWARE_VERSION3);
	#else
	    #ifdef CONFIG_HW_BOARD_ID_USE_THREE_GPIOS
	    gpio_direction_input(HARDWARE_VERSION2);
	    #endif
	#endif
	
	//get gpios input value
	result = gpio_get_value(HARDWARE_VERSION0);
	pr_debug( "PM_DEBUG_MXP:read HARDWARE_VERSION0 value =%d.\r\n",result);
	rtl =result;
	
	result = gpio_get_value(HARDWARE_VERSION1);
	pr_debug( "PM_DEBUG_MXP:read HARDWARE_VERSION1 value =%d.\r\n",result);
	rtl |=(result<<1);

	/*[ECID:000000] ZTEBSP yaotierui  for 8x25 FOUR GPIOS VERSION start 20130108*/
       #ifdef CONFIG_HW_BOARD_ID_USE_FOUR_GPIOS
	    result = gpio_get_value(HARDWARE_VERSION2);
	    pr_debug( "PM_DEBUG_MXP:read HARDWARE_VERSION2 value =%d.\r\n",result);
	    rtl |=(result<<2);

	    result = gpio_get_value(HARDWARE_VERSION3);
	    pr_debug( "PM_DEBUG_MXP:read HARDWARE_VERSION3 value =%d.\r\n",result);
	    rtl |=(result<<3);
	#else
	    #ifdef CONFIG_HW_BOARD_ID_USE_THREE_GPIOS
	    result = gpio_get_value(HARDWARE_VERSION2);
	    pr_debug( "PM_DEBUG_MXP:read HARDWARE_VERSION2 value =%d.\r\n",result);
	    rtl |=(result<<2);
	#endif
	#endif
	
	printk(KERN_NOTICE "PM_DEBUG_MXP:read finally board_id value =%d.\r\n",rtl);
	return rtl;
}
static ssize_t zte_board_id_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *buf)
{	
	int hw_board_id;
	int return_value = 0;
	
	hw_board_id = zte_get_hw_board_id();

#if 1
	if(hw_board_id<(sizeof(zte_hardware_version)/sizeof(zte_hardware_version[0])))
		return_value = sprintf(buf, "%s\n", zte_hardware_version[hw_board_id].id);
	else
		return_value = sprintf(buf, "%s\n", zte_hardware_version[0].id);
#else
	switch(hw_board_id)
	{
		case 0:
		return_value = sprintf(buf, "%s\n", zte_hardware_version[0].id);
		break;

		case 1:
		return_value = sprintf(buf, "%s\n", zte_hardware_version[1].id);
		break;
		
		case 2:
		return_value = sprintf(buf, "%s\n", zte_hardware_version[2].id);
		break;
		
		default:
		return_value = sprintf(buf, "%s\n", zte_hardware_version[0].id);
		break;	
	}
#endif
         return return_value;
}

static ssize_t zte_board_id_store(struct kobject *kobj,
				  struct kobj_attribute *attr,
				  const char *buf, size_t n)
{

         // this is a test code.
         #if 0
	if(n !=4)
	{
		return -EINVAL;
	}
	if(strcpy( zte_hardware_version[0].id,buf)!=&(zte_hardware_version[0].id))
	//if(!strict_strtoul(buf+2,16,&print_control_flag))
	{
		return -EINVAL;
	}
	#endif
		return n;
}

static struct kobj_attribute zte_board_id_attribute =
	__ATTR(board_id, 0755, zte_board_id_show, zte_board_id_store);

static struct attribute *attrs[] = {
	&zte_board_id_attribute.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static int __init zte_board_id_init(void)
{
	int retval;

	if(zte_hw_board_initialize == 0)
	{zte_hw_board_id_gpio_init();}
	
	/*
	 * Create  /sys/zte_board_id/
	 */
	 
	zte_board_id_kobj = kobject_create_and_add("zte_board_id", NULL);
	if (!zte_board_id_kobj)
		return -ENOMEM;

	/* Create the files associated with this kobject */
	retval = sysfs_create_group(zte_board_id_kobj, &attr_group);
	if (retval)
		kobject_put(zte_board_id_kobj);

	return retval;	
	
}

static void __exit zte_board_id_exit(void)
{	
	//free gpios
	gpio_free(HARDWARE_VERSION0);
	gpio_free(HARDWARE_VERSION1);

	/*[ECID:000000] ZTEBSP yaotierui  for 8x25 FOUR GPIOS VERSION start 20130108*/
       #ifdef CONFIG_HW_BOARD_ID_USE_FOUR_GPIOS
	    gpio_free(HARDWARE_VERSION2);
	    gpio_free(HARDWARE_VERSION3);
	#else
	    #ifdef CONFIG_HW_BOARD_ID_USE_THREE_GPIOS
	    gpio_free(HARDWARE_VERSION2);
	    #endif
	#endif
	
	kobject_put(zte_board_id_kobj);
}

module_init(zte_board_id_init);
module_exit(zte_board_id_exit);

//[ECID:0000] ZTEBSP maxiaoping 20120521  for V70  hardware board it acquisition,end.
//[ECID:0000]ZTE_BSP maxiaoping 20120726 disable debug logs,end.