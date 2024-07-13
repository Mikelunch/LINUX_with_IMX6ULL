/*
 * 0 驱动模板
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

#define LEDDEV_CNT		1			/* 设备号长度 	*/
#define LEDDEV_NAME		"dtsplatled"	/* 设备名字 	*/
#define LEDOFF 			0
#define LEDON 			1


/*gpioled设备结构体*/
struct gpioled_dev
{
    dev_t devid;
    int major;
    int minor;
    struct cdev mycdev;
    struct class *myclass;
    struct device *mydevice;
    struct device_node *nd;
    int led_gpio; //led 的 io编号

}gpioled;

/*实现具体驱动操作函数*/
static int led_open(struct inode *inode , struct file *filp){
    //filp->private_data = &gpioled;//设置为私有数据
    return 0;
}

static int led_release(struct inode *inode , struct file *filp){
    //struct gpioled *dev = (struct gpioled*)filp->private_data;
    return 0;
}

/*led打开或者关闭*/
void led_switch(unsigned char flag){
    if(flag == LEDON){
        gpio_set_value(gpioled.led_gpio , 0); //开启led
    }
    else{
        gpio_set_value(gpioled.led_gpio , 1); //关闭led
    }
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){

    int ret;
    char databuf[1];
    //struct gpioled *dev = (struct gpioled*)filp->private_data;
    ret = copy_from_user(databuf , buf , count);
    if(ret < 0){
        printk("invaild value\r\n");
        return -EINVAL;
    }
   if(databuf[0] == LEDON){
        gpio_set_value(gpioled.led_gpio , 0); //开启led
    }
    else if(databuf[0]  == LEDOFF){
        gpio_set_value(gpioled.led_gpio , 1); //关闭led
    }

    return 0;
}

static ssize_t led_read(struct file *file, char __user *buf, size_t size, loff_t *ppos){
    return 0;
}

static const struct file_operations led_fops = {
	.owner	= THIS_MODULE,
	.open	= led_open,
	.write	= led_write,
	.read	= led_read,
    .release = led_release,
};

static int led_probe(struct platform_device *pdev){  
	int ret = 0;

	printk("led probe\r\n");
	
    printk("start to init gpioled\r\n");
    //注册字符设备驱动
    gpioled.major = 0;
    if(gpioled.major){
        gpioled.devid = MKDEV(gpioled.major , 0);
        register_chrdev_region(gpioled.devid , LEDDEV_CNT , LEDDEV_NAME);
    }
    else{ /*自动分配设备号*/
        alloc_chrdev_region(&gpioled.devid , 0 ,LEDDEV_CNT ,  LEDDEV_NAME);
    }
    gpioled.major = MAJOR(gpioled.devid);
    gpioled.minor = MINOR(gpioled.devid);
    printk("gpioled major = %d minor = %d\r\n",gpioled.major , gpioled.minor);

    //初始化字符cdev
    gpioled.mycdev.owner =  THIS_MODULE;
    cdev_init(&gpioled.mycdev , &led_fops);
    ret = cdev_add(&gpioled.mycdev , gpioled.devid , LEDDEV_CNT);
    if(ret < 0){
        printk("fail to access cdev\r\n");
    }

    //创建类
    gpioled.myclass = class_create(THIS_MODULE, LEDDEV_NAME);
    if(IS_ERR(gpioled.myclass)){
        printk("fail to create class\r\n");
        return PTR_ERR(gpioled.myclass);
    }

    //创建设备
    gpioled.mydevice = device_create(gpioled.myclass, NULL, gpioled.devid, NULL,LEDDEV_NAME);
    if(IS_ERR(gpioled.mydevice )){
        printk("fail to create device\r\n");
        return PTR_ERR(gpioled.mydevice );
    }

    //gpio相关操作
    //获取设备节点
    
	gpioled.nd = pdev->dev.of_node; //已经和设备树匹配情况下

    //获取LED所对应的gpio
    gpioled.led_gpio = of_get_named_gpio(gpioled.nd , "led-gpios" , 0);
    if(gpioled.led_gpio < 0){
        printk("fail to access property name\r\n");
        ret = -EINVAL;
        goto  fail_fnnm;
    }
    printk("led gpio num = %d\r\n",gpioled.led_gpio);
    //申请GPIO
    ret = gpio_request(gpioled.led_gpio , "gpioled.led_gpio");
    if(ret){
        printk("fail to request gpio num\r\n");
        ret = -EINVAL;
        goto fail_getnm;
    }
    //使用IO
    ret = gpio_direction_output(gpioled.led_gpio , 1);//默认高电平 不点亮
    if(ret){
        printk("fail to set gpio output\r\n");
        ret = -EINVAL;
        goto fail_stout;
    }

    gpio_set_value(gpioled.led_gpio , 0); //开启led

    return 0;

fail_stout:
    gpio_free(gpioled.led_gpio);
fail_getnm:
fail_fnnm:
//fail_fdnd:
    return ret;
}

static int led_remove(struct platform_device *pdev){
  
	printk("about to remove led driver\r\n");
     //关闭LED
    gpio_set_value(gpioled.led_gpio , 1); //关闭LED
    //摧毁GPIO
    gpio_free(gpioled.led_gpio);
    //摧毁cdev
    cdev_del(&gpioled.mycdev);
    //注销字符设备
    unregister_chrdev_region(gpioled.devid , LEDDEV_CNT);
    //摧毁设备
    device_destroy(gpioled.myclass , gpioled.devid);
    //摧毁类
    class_destroy(gpioled.myclass);

	return 0;
}

struct of_device_id led_of_match[] = {
	{
		.compatible = "alientek,gpioled" , 

	},
	{/*sentinel*/},
};

/*平台驱动结构体*/
static struct platform_driver led_driver = {
    .driver		= {
		.name	= "imx6u-dts-led", /*有设备树情况*/
		.of_match_table = led_of_match, /*设备树匹配表*/
	},
	.probe		= led_probe, /*匹配成功后执行的函数*/
	.remove		= led_remove, /*卸载驱动时执行*/
};

static int __init leddriver_init(void){
    int ret = 0;

   //注册平台驱动
   ret = platform_driver_register(&led_driver);
   if(ret){
    printk("cant register driver\r\n");
   }

    return ret;
}

static void __exit leddriver_exit(void){
    //卸载设备
    platform_driver_unregister(&led_driver);
}

module_init(leddriver_init);
module_exit(leddriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("constant_z");