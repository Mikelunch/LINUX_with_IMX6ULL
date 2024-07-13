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
#define LEDDEV_NAME		"platled"	/* 设备名字 	*/
#define LEDOFF 			0
#define LEDON 			1


/* 寄存器名 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

/* leddev设备结构体 */
struct leddev_dev{
	dev_t devid;			/* 设备号	*/
	struct cdev cdev;		/* cdev		*/
	struct class *class;	/* 类 		*/
	struct device *device;	/* 设备		*/
	int major;				/* 主设备号	*/		
}leddev;

void led0_switch(u8 sta)
{
	u32 val = 0;
	if(sta == LEDON){
		val = readl(GPIO1_DR);
		val &= ~(1 << 3);	
		writel(val, GPIO1_DR);
	}else if(sta == LEDOFF){
		val = readl(GPIO1_DR);
		val|= (1 << 3);	
		writel(val, GPIO1_DR);
	}	
}

static int led_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &leddev; /* 设置私有数据  */
	return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue;
	unsigned char databuf[1];
	unsigned char ledstat;

	retvalue = copy_from_user(databuf, buf, cnt);
	if(retvalue < 0) {
		return -EFAULT;
	}

	ledstat = databuf[0];		/* 获取状态值 */
	if(ledstat == LEDON) {
		led0_switch(LEDON);		/* 打开LED灯 */
	}else if(ledstat == LEDOFF) {
		led0_switch(LEDOFF);	/* 关闭LED灯 */
	}
	return 0;
}

/*驱动操作函数*/
static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.write = led_write,
};



static int led_probe(struct platform_device *pdev){  
   int i = 0;
	int ressize[5];
	u32 val = 0;
	struct resource *ledsource[5];

	printk("led driver and device has matched!\r\n");
	/* 1、获取资源 */
	for (i = 0; i < 5; i++) {
		ledsource[i] = platform_get_resource(pdev, IORESOURCE_MEM, i); /* 依次MEM类型资源 */
		if (!ledsource[i]) {
			dev_err(&pdev->dev, "No MEM resource for always on\n");
			return -ENXIO;
		}
		ressize[i] = resource_size(ledsource[i]);	
	}	

	/* 2、初始化LED */
	/* 寄存器地址映射 */
 	IMX6U_CCM_CCGR1 = ioremap(ledsource[0]->start, ressize[0]);
	SW_MUX_GPIO1_IO03 = ioremap(ledsource[1]->start, ressize[1]);
  	SW_PAD_GPIO1_IO03 = ioremap(ledsource[2]->start, ressize[2]);
	GPIO1_DR = ioremap(ledsource[3]->start, ressize[3]);
	GPIO1_GDIR = ioremap(ledsource[4]->start, ressize[4]);
	
	val = readl(IMX6U_CCM_CCGR1);
	val &= ~(3 << 26);				/* 清除以前的设置 */
	val |= (3 << 26);				/* 设置新值 */
	writel(val, IMX6U_CCM_CCGR1);

	/* 设置GPIO1_IO03复用功能，将其复用为GPIO1_IO03 */
	writel(5, SW_MUX_GPIO1_IO03);
	writel(0x10B0, SW_PAD_GPIO1_IO03);

	/* 设置GPIO1_IO03为输出功能 */
	val = readl(GPIO1_GDIR);
	val &= ~(1 << 3);			/* 清除以前的设置 */
	val |= (1 << 3);			/* 设置为输出 */
	writel(val, GPIO1_GDIR);

	/* 默认关闭LED1 */
	val = readl(GPIO1_DR);
	val |= (1 << 3) ;	
	writel(val, GPIO1_DR);
	
	/* 注册字符设备驱动 */
	/*1、创建设备号 */
	if (leddev.major) {		/*  定义了设备号 */
		leddev.devid = MKDEV(leddev.major, 0);
		register_chrdev_region(leddev.devid, LEDDEV_CNT, LEDDEV_NAME);
	} else {						/* 没有定义设备号 */
		alloc_chrdev_region(&leddev.devid, 0, LEDDEV_CNT, LEDDEV_NAME);	/* 申请设备号 */
		leddev.major = MAJOR(leddev.devid);	/* 获取分配号的主设备号 */
	}
	
	/* 2、初始化cdev */
	leddev.cdev.owner = THIS_MODULE;
	cdev_init(&leddev.cdev, &led_fops);
	
	/* 3、添加一个cdev */
	cdev_add(&leddev.cdev, leddev.devid, LEDDEV_CNT);

	/* 4、创建类 */
	leddev.class = class_create(THIS_MODULE, LEDDEV_NAME);
	if (IS_ERR(leddev.class)) {
		return PTR_ERR(leddev.class);
	}

	/* 5、创建设备 */
	leddev.device = device_create(leddev.class, NULL, leddev.devid, NULL, LEDDEV_NAME);
	if (IS_ERR(leddev.device)) {
		return PTR_ERR(leddev.device);
	}

	return 0;
}

static int led_remove(struct platform_device *pdev){
    printk("imx6u-led driver remove\r\n");

    /*卸载平台驱动*/
    iounmap(IMX6U_CCM_CCGR1);
	iounmap(SW_MUX_GPIO1_IO03);
	iounmap(SW_PAD_GPIO1_IO03);
	iounmap(GPIO1_DR);
	iounmap(GPIO1_GDIR);

	cdev_del(&leddev.cdev);/*  删除cdev */
	unregister_chrdev_region(leddev.devid, LEDDEV_CNT); /* 注销设备号 */
	device_destroy(leddev.class, leddev.devid);
	class_destroy(leddev.class);

    return 0;
}

/*平台驱动结构体*/
static struct platform_driver led_driver = {
    .driver		= {
		.name	= "imx6u-led", /*一定要和设备端的name相同 因为这是无设备匹配*/
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