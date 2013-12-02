/* linux/arch/arm/mach-msm/board-zte-wifi.c
*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <asm/mach-types.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/skbuff.h>
#include <linux/wlan_plat.h>
#include <linux/if.h> /*For IFHWADDRLEN */

#include "devices.h"
#include <mach/proc_comm.h>  /* For msm_proc_cmd */   //yaotierui for JB 2020 V2
#include <linux/gpio.h>

#include <linux/random.h>
#include <linux/jiffies.h>
#include <linux/board-zte-wifi.h>

#include <mach/pmic.h>
#include <mach/vreg.h>
#include <linux/proc_fs.h> //added by fanjiankang for write proc info

#include <linux/export.h>  //yaotierui for JB 2020 V2


#if defined ( CONFIG_PROJECT_P825A10 ) || defined ( CONFIG_PROJECT_P825F01 ) || defined ( CONFIG_PROJECT_P865F01 ) || defined ( CONFIG_PROJECT_P865E02 )||defined ( CONFIG_PROJECT_P865F03 )|| defined ( CONFIG_PROJECT_P865V30 ) ||defined ( CONFIG_PROJECT_P865V20)
#define WLAN_CHIP_PWD_PIN  124
#define WLAN_CHIP_WOW_PIN     39
#elif defined(CONFIG_PROJECT_V72A)
#define WLAN_CHIP_PWD_PIN  33
#define WLAN_CHIP_WOW_PIN  40
#else
#define WLAN_CHIP_PWD_PIN  113
#define WLAN_CHIP_WOW_PIN     114
#define ZTE_PROC_COMM_CMD3_PMIC_GPIO 16
#define WLAN_1V8_EN             117
#endif

static calData ncal[3];
static int nv_tag;

//[ECID 000000] fanjiankang add for ic information add 20120314 begin
static char wlan_info[50] = "Device manufacturing:Atheros Model Number:AR6005";
static ssize_t wlan_info_read_proc(char *page, char **start, off_t off,
				int count, int *eof, void *data)
{
	int len = strlen(wlan_info);
	sprintf(page, "%s\n", wlan_info);
	return len + 1;
}
static void create_wlan_info_proc_file(void)
{
  struct proc_dir_entry *wlan_info_proc_file = create_proc_entry("driver/wlan_info", 0644, NULL);
  printk("goes to create_wlan_info_proc_file\n");
  if (wlan_info_proc_file) {
			wlan_info_proc_file->read_proc = wlan_info_read_proc;
   } else
	printk(KERN_INFO "proc file create failed!\n");
}

//[ECID 000000] fanjiankang add for ic information20120314 end

static int ath6kl_readCalDataFromNV(void)
{
    int ret = -1;
    int i;
    u32 rf_data1, rf_data2;   

    for(i = 0; i < 3; i++) {
                rf_data2 = ((i+1)<<24);
		  pr_info("fanjiankang before msm_proc_comm\n ");
                ret  = msm_proc_comm(PCOM_CUSTOMER_CMD1, &rf_data1, &rf_data2);
		  pr_info("ath6kl_readCalDataFromNV ret value is %d\n",ret);
                if(ret!=0) {
                        pr_info("[WIFI]_read RF NV item 3757 failed i = %d\n", i);
			   nv_tag=0;
                        break;
                } else {
                        /* copy bytes */
			   nv_tag=1;
                        if( i != (rf_data1 & 0xff) ) {
                                pr_info("Invalid RF data from NV. i=%d\n", i);
                                break;
                        }
                        ncal[i].olpcGainDelta = (rf_data1>>8)&0xff;
                        ncal[i].desiredScaleCck_t10 = (rf_data1>>16)&0xff;
                        ncal[i].desiredScale6M_t10 = (rf_data1>>24)&0xff;
                        ncal[i].desiredScale36M_t10 = rf_data2&0xff;
                        ncal[i].desiredScale54M_t10 = (rf_data2>>8)&0xff;
                        ncal[i].desiredScaleMCS0HT20_t10 = (rf_data2>>16)&0xff;
                        ncal[i].desiredScaleMCS7HT20_t10 = (rf_data2>>24)&0xff;
                        pr_info("NV RF data  i=%d\n",i);
                        pr_info("NV RF data  from %02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
                                ncal[i].olpcGainDelta, ncal[i].desiredScaleCck_t10, ncal[i].desiredScale6M_t10,
                                ncal[i].desiredScale36M_t10, ncal[i].desiredScale54M_t10,
                                ncal[i].desiredScaleMCS0HT20_t10, ncal[i].desiredScaleMCS7HT20_t10 );
               }
    }	

    return ret;
}

 int ath6kl_nv_states_get(void)
{
    return nv_tag;
}

 void ath6kl_getCaldata(calData **ncal_init)
{
    *ncal_init  = ncal;
}


 int zte_wifi_power(int on)
 {
     if(on)
     {
          pr_info("================ar6003 chip back to live=============\n");
          gpio_request(WLAN_CHIP_PWD_PIN,"WLAN_CHIP_PWD");
          gpio_direction_output(WLAN_CHIP_PWD_PIN,1);
          gpio_free(WLAN_CHIP_PWD_PIN);
     }
     else
      {
            pr_info("================ar6003 chip power cut=============\n");
             gpio_request(WLAN_CHIP_PWD_PIN,"WLAN_CHIP_PWD");
             gpio_direction_output(WLAN_CHIP_PWD_PIN,0);
             gpio_free(WLAN_CHIP_PWD_PIN);
      }
      mdelay(200);
    return 0;
}


static struct platform_device zte_wifi_device = {
	.name           = "wlan_ar6000_pm_dev",
	.id             = -1,
       .dev            = {
                .platform_data = &zte_wifi_power,
        },

};
//zte-modify quguotao197012 read mac address form NV start
static u8 softmac[6]={0};

static int pc_wifi_mac_nvread(void)
{
    int rc;
    u32 data1, data2 = (1<<31);//for wifi mac
    rc = msm_proc_comm(PCOM_CUSTOMER_CMD1, &data1, &data2);
	if (rc){
		printk("Read wifi mac address form nv failed! rc=%d\n",rc);
		return rc;
		}
	else{
		softmac[5] = (u8)((data2>>8)&0xff);
              softmac[4] = (u8)(data2&0xff);
              softmac[3] = (u8)((data1>>24)&0xff);
              softmac[2] = (u8)((data1>>16)&0xff);
              softmac[1] = (u8)((data1>>8)&0xff);
              softmac[0] = (u8)(data1&0xff);
		printk("Read wifi mac address form nv successful!\n");
		printk("data1=%x,data2=%x\n",data1,data2);
		printk("quguotao MAC = %02X:%02X:%02X:%02X:%02X:%02X\n",
            softmac[0], softmac[1], softmac[2], 
            softmac[3], softmac[4], softmac[5]); 
		return 0;//success
		}
}

u8*  get_wifi_mac_address(void)
{
      return softmac;
}
EXPORT_SYMBOL(get_wifi_mac_address);
//zte-modify quguotao197012 read mac address form NV end
 static int __init zte_wifi_init(void)
{

      int ret = 0;
#if defined ( CONFIG_PROJECT_P825A10 ) || defined ( CONFIG_PROJECT_P825F01 ) || defined ( CONFIG_PROJECT_P865F01 ) || defined ( CONFIG_PROJECT_P865E02 ) || defined(CONFIG_PROJECT_V72A)||defined ( CONFIG_PROJECT_P865F03 )|| defined ( CONFIG_PROJECT_P865V30 ) ||defined ( CONFIG_PROJECT_P865V20)
       struct vreg *wlan_3p3; 
	struct vreg *wlan_1p8;
  #else
	unsigned int pmic_gpio_config = (PMIC_GPIO_9 << 28)|	\
									(CONFIG_CMOS << 24)|	\
									(PMIC_GPIO_VIN0 << 20)|	\
									(SOURCE_GND << 16)|		\
									(BUFFER_MEDIUM << 12)|	\
									(0 << 8)|				\
									(1 << 4)|				\
									0x0;
	unsigned int pmic_gpio_cmd = ZTE_PROC_COMM_CMD3_PMIC_GPIO;
 #endif

	do {
		pr_info("%s() enter\n", __func__);
		ret = gpio_tlmm_config(GPIO_CFG(WLAN_CHIP_WOW_PIN, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

       	if(ret) {
            		printk(KERN_ERR"WLAN_CHIP_WOW_PIN config failed\n");
	    		break;
        	} 
		gpio_request(WLAN_CHIP_WOW_PIN, "WLAN_WOW");
		gpio_free(WLAN_CHIP_WOW_PIN);        

#if defined ( CONFIG_PROJECT_P825A10 ) || defined ( CONFIG_PROJECT_P825F01 ) || defined ( CONFIG_PROJECT_P865F01 ) || defined ( CONFIG_PROJECT_P865E02 ) || defined(CONFIG_PROJECT_V72A)||defined ( CONFIG_PROJECT_P865F03 )|| defined ( CONFIG_PROJECT_P865V30 ) ||defined ( CONFIG_PROJECT_P865V20)
	wlan_3p3 = vreg_get(NULL, "bt");
	if(!wlan_3p3)
	{
	      pr_err("%s: VREG_L17 get failed\n",__func__);
	}

       ret = vreg_set_level(wlan_3p3, 3300);
	if(ret<0)
	{
	      pr_err("%s: VREG_L17 set failed\n",__func__);
	}
		ret=vreg_enable(wlan_3p3);
	if (ret) {
		pr_err("%s: vreg_enable failed \n", __func__);
	}
	#else
		printk("PCOM_CUSTOMER_CMD3 send/n");
		ret = msm_proc_comm(PCOM_CUSTOMER_CMD3, &pmic_gpio_config, &pmic_gpio_cmd);
		if(ret) {
            		printk(KERN_ERR"Failed to turn on LDO 3.3v\n");
        	}
       #endif
		mdelay(50);


#if defined ( CONFIG_PROJECT_P825A10 ) || defined ( CONFIG_PROJECT_P825F01 ) || defined ( CONFIG_PROJECT_P865F01 ) || defined ( CONFIG_PROJECT_P865E02 ) || defined(CONFIG_PROJECT_V72A)||defined ( CONFIG_PROJECT_P865F03 )|| defined ( CONFIG_PROJECT_P865V30 ) ||defined ( CONFIG_PROJECT_P865V20)
	wlan_1p8 = vreg_get(NULL,"wlan4");
	       if(!wlan_1p8)
	{
	      pr_err("%s: VREG_L19 get failed\n",__func__);
	}
	  ret = vreg_set_level(wlan_1p8, 1800);
	if(ret<0)
	{
	      pr_err("%s: VREG_L19 set failed\n",__func__);
	}
		ret=vreg_enable(wlan_1p8);
	if (ret) {
		pr_err("%s: vreg_enable failed \n", __func__);
	}
	#else
        ret = gpio_tlmm_config(GPIO_CFG(WLAN_1V8_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
        if(ret) {
        	printk(KERN_ERR"WLAN_1V8_EN config failed\n");
	 	break;
        } 
		gpio_request(WLAN_1V8_EN, "WLAN_1V8_EN");
		gpio_direction_output(WLAN_1V8_EN, 1);
        	gpio_free(WLAN_1V8_EN); 
	#endif

        pr_info("%s() VREG 1.8v On\n", __func__);
        mdelay(100);
        pr_info("%s() Pull low CHIP PWD\n", __func__);
        /*
         * Pull low Chip power down pin
         */		
		ret = gpio_tlmm_config(GPIO_CFG(WLAN_CHIP_PWD_PIN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
        	if(ret) {
            		printk(KERN_ERR"WLAN_CHIP_PWD_PIN config failed\n");
	    		break;
        	} 
		gpio_request(WLAN_CHIP_PWD_PIN, "WLAN_CHIP_PWD");
		gpio_direction_output(WLAN_CHIP_PWD_PIN, 0);
		gpio_free(WLAN_CHIP_PWD_PIN);

		platform_device_register(&zte_wifi_device);	//ZTE_JHT_20120201
		//zte-modify quguotao197012 read mac address form NV start
		pc_wifi_mac_nvread();
		//zte-modify quguotao197012 read mac address form NV end
              return 0;
		}while(0);
        return ret;
}
 
 int wlan_init_power(void)
{
       pr_info("fanjiankang before ath6kl_readCalDataFromNV\n ");
	ath6kl_readCalDataFromNV();
       pr_info("fanjiankang after ath6kl_readCalDataFromNV\n ");
	zte_wifi_init();
	create_wlan_info_proc_file();
	return 0;
}
EXPORT_SYMBOL(ath6kl_nv_states_get);
EXPORT_SYMBOL(ath6kl_getCaldata);

