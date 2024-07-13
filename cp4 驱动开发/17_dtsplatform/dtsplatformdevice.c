/*
 * 0 设备模板
 * 
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h> //调用copy_to_user要用到
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h> //调用设备驱动用到
#include <linux/of.h> //调用of函数会用到
#include <linux/slab.h> //调用kmalloc kfree要用到
#include <linux/of_device.h>
#include <linux/of_address.h> //调用of_iomap时使用
#include <linux/of_gpio.h>  //of_get_named_gpio使用
#include <linux/atomic.h> //原子操作
#include <linux/interrupt.h> //request_irq时使用
#include <linux/timer.h> //Linux内核定时器
#include <linux/jiffies.h>  //Linux定时器计数值
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/ide.h>
#include <linux/platform_device.h> //平台驱动需要的文件
/* 
 * 寄存器地址定义
*/
#define CCM_CCGR1_BASE (0X020C406C) 
#define SW_MUX_GPIO1_IO03_BASE (0X020E0068)
#define SW_PAD_GPIO1_IO03_BASE (0X020E02F4)
#define GPIO1_DR_BASE (0X0209C000)
#define GPIO1_GDIR_BASE (0X0209C004)
#define REGISTER_LENGTH 4 /*地址长度 4字节*/

static struct resource led_resource[] = {
    {
		.start	= CCM_CCGR1_BASE, /*启动地址*/
		.end	= CCM_CCGR1_BASE + REGISTER_LENGTH - 1, /*注意要-1*/
		.flags	= IORESOURCE_MEM, /*资源类型：内存*/
	},
    {
		.start	= SW_MUX_GPIO1_IO03_BASE,
		.end	= SW_MUX_GPIO1_IO03_BASE + REGISTER_LENGTH - 1, /*注意要-1*/
		.flags	= IORESOURCE_MEM,
	},
    {
		.start	= SW_PAD_GPIO1_IO03_BASE,
		.end	= SW_PAD_GPIO1_IO03_BASE + REGISTER_LENGTH - 1, /*注意要-1*/
		.flags	= IORESOURCE_MEM,
	},
    {
		.start	= GPIO1_DR_BASE,
		.end	= GPIO1_DR_BASE + REGISTER_LENGTH - 1, /*注意要-1*/
		.flags	= IORESOURCE_MEM,
	},
    {
		.start	= GPIO1_GDIR_BASE,
		.end	= GPIO1_GDIR_BASE + REGISTER_LENGTH - 1, /*注意要-1*/
		.flags	= IORESOURCE_MEM,
	},
};

void leddevic_release(struct device *dev){
    printk("release leddevic\r\n");
}

static struct platform_device leddevic = {
	.name = "imx6u-led",/*一定要和设备中的结构name一样*/
    .id = -1,/*该设备无ID号*/
    .dev = {
        .release = &leddevic_release, /*释放平台驱动时调用的函数*/
    },
    .num_resources = ARRAY_SIZE(led_resource), /*resource数组个数*/
    .resource =  led_resource/*描述平台设备的内存资源*/
};

static int __init leddevic_init(void){
    int ret = 0;

    //注册设备
    ret = platform_device_register(&leddevic);
    if(ret){
        printk("cant register platform_device\r\n");
        
    }

    return ret;
}

static void __exit leddevic_exit(void){
    //卸载设备
    platform_device_unregister(&leddevic);
}

module_init(leddevic_init);
module_exit(leddevic_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("constant_z");