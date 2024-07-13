/*
 * 3 新字符驱动试验
 * 驱动端程序
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h> //调用copy_to_user要用到
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>


#define LED_MAJOR 200
#define LED_NAME "LED"

#define LEDOFF 0
#define LEDON  1

/*led物理地址寄存器*/
#define CCM_CCGR1_BASE      (0X020C406C)
#define SW_MUX_GPIO1_IO03  (0X020E0068)
#define SW_PAD_GPIO1_IO03  (0X020E02F4)
#define GPIO1_DR_BASE      (0X0209C000)
#define GPIO1_GDIR_BASE    (0X0209C004)

#if 1
/*led虚拟地址指针*/
static void __iomem *IMX6ULL_CCM_CCGR1_BASE;
static void __iomem *IMX6ULL_SW_MUX_GPIO1_IO03;
static void __iomem *IMX6ULL_SW_PAD_GPIO1_IO03;
static void __iomem *IMX6ULL_GPIO1_DR_BASE;
static void __iomem *IMX6ULL_GPIO1_GDIR_BASE;
#endif

/*新的设备结构体*/
struct newchrbase_dev
{
    struct cdev mycdev;
    dev_t deviceNum;//设备号
    int major; //主设备号
    int minor; //次设备号
    struct class *myclass; //类
    struct device *mydevice; //自动挂载设备
}mynewchrbase;

/*led初始化函数*/
static void led_init(void){
     /*LED初始化*/
    IMX6ULL_CCM_CCGR1_BASE = ioremap(CCM_CCGR1_BASE, 4);//CCGR1 4字节
    IMX6ULL_SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03 , 4);
    IMX6ULL_SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03 , 4);
    IMX6ULL_GPIO1_DR_BASE = ioremap(GPIO1_DR_BASE , 4);
    IMX6ULL_GPIO1_GDIR_BASE = ioremap(GPIO1_GDIR_BASE , 4);
     //使能时钟
    writel(0xffffffff , IMX6ULL_CCM_CCGR1_BASE);

    //配置复用属性
    writel(0x5 , IMX6ULL_SW_MUX_GPIO1_IO03);

    //配置电气属性
    writel(0x10b0,IMX6ULL_SW_PAD_GPIO1_IO03);

    //配置IO输出
    writel(1 << 3,IMX6ULL_GPIO1_GDIR_BASE);

    //输出低电平 默认点亮
    writel(0 << 3 , IMX6ULL_GPIO1_DR_BASE);
}

/*led打开或者关闭*/
void led_switch(unsigned char flag){
    if(flag == LEDON){
        writel(0 << 3 , IMX6ULL_GPIO1_DR_BASE);
    }
    else if(flag == LEDOFF){
        writel(1 << 3 , IMX6ULL_GPIO1_DR_BASE);
    }
}

/*实现具体驱动操作函数*/
static int newchrdevbase_open(struct inode *inode , struct file *filp){
    filp->private_data = &mynewchrbase;//设置为私有数据
    return 0;
} 


static int newchrdevbase_release(struct inode *inode , struct file *filp){
    struct newchrbase_dev *dev = (struct newchrbase_dev*)filp->private_data;
    return 0;
}

static ssize_t newchrdevbase_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos){
    int retval;
    unsigned char databuf[1];
    /*led控制*/
    retval = copy_from_user(databuf , buf , count);
    if(retval < 0){
        printk("kernel write failed\r\n");
        return -EFAULT;
    }

    led_switch(databuf[0]);

    return 0;
}

static ssize_t newchrdevbase_read(struct file *file, char __user *buf, size_t size, loff_t *ppos){
    return 0;
}

static const struct file_operations ops = {
    .owner   = THIS_MODULE,
    .write   = newchrdevbase_write,
    .read    = newchrdevbase_read,
    .open    = newchrdevbase_open,
    .release = newchrdevbase_release 
};

/*驱动入口*/
static int __init newchrbase_init(void){
    int ret;
    printk("init newchrbase_init\r\n");
    led_init();
    /*申请设备号*/
    if(mynewchrbase.major){
        printk("already give major num\r\n");
        mynewchrbase.deviceNum = MKDEV(mynewchrbase.major , 0);//自动分配 主设备号 ： 0为设备号
        ret = register_chrdev_region(mynewchrbase.deviceNum , 1 , "newchrdevbase");
        if(ret < 0){
            printk("give newchrdevbase device num err!\r\n");
            goto fail_devid;
        }
    }
    else{
        printk("automatically give device num\r\n");
        ret = alloc_chrdev_region(&mynewchrbase.deviceNum , 0 , 1 , "newchrdevbase");
        if(ret < 0){
            printk("give newchrdevbase device num err!\r\n");
            goto fail_devid;
        }
    }

    mynewchrbase.major = MAJOR(mynewchrbase.deviceNum);
    mynewchrbase.minor = MINOR(mynewchrbase.deviceNum);
    printk("give device num successfully major = %d minor = %d\r\n" , mynewchrbase.major,mynewchrbase.minor);
     
    /*注册具体驱动操作函数*/
    mynewchrbase.mycdev.owner = THIS_MODULE;
    cdev_init(&mynewchrbase.mycdev , &ops);
    ret = cdev_add(&mynewchrbase.mycdev, mynewchrbase.deviceNum, 1);
    if(ret < 0){
        printk("register newchrdevbase device err!\r\n");
        goto fail_register;
    }

    /*创建类*/
    mynewchrbase.myclass = class_create(THIS_MODULE,"newchrdevbase");
    if(IS_ERR(mynewchrbase.myclass)){//检查是否创建检查成功
        ret = IS_ERR(mynewchrbase.myclass);
        printk("create class for newchrdevbase device err!\r\n");
        goto fail_class;
    }
    /*自动挂载节点*/
    mynewchrbase.mydevice = device_create(mynewchrbase.myclass, NULL,
			     mynewchrbase.deviceNum,NULL, "newchrdevbase");
    return 0;

fail_devid:
    return -1;
fail_register:
    unregister_chrdev_region(mynewchrbase.deviceNum , 1);
    return -2;
fail_class:
    cdev_del(&mynewchrbase.mycdev);
    unregister_chrdev_region(mynewchrbase.deviceNum , 1);
    return ret;
}

/*驱动出口*/
static void __exit newchrbase_exit(void){
    /*关闭led灯*/
    writel(1 << 3 , IMX6ULL_GPIO1_DR_BASE);
    /*取消地址映射*/
    iounmap(IMX6ULL_CCM_CCGR1_BASE);
    iounmap(IMX6ULL_SW_MUX_GPIO1_IO03);
    iounmap(IMX6ULL_SW_PAD_GPIO1_IO03);
    iounmap(IMX6ULL_GPIO1_DR_BASE);
    iounmap(IMX6ULL_GPIO1_GDIR_BASE);
    /*注销字符设备*/
    cdev_del(&mynewchrbase.mycdev);
    /*注销设备号*/
    unregister_chrdev_region(mynewchrbase.deviceNum , 1);
    /*摧毁设备*/
    device_destroy(mynewchrbase.myclass , mynewchrbase.deviceNum);//先摧毁设备再注销类和加载时操作相反
    /*注销类*/
    class_destroy(mynewchrbase.myclass);
}

/*1 注册驱动*/
module_init(newchrbase_init);
module_exit(newchrbase_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("constant_z");