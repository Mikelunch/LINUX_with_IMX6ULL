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
#include <linux/miscdevice.h>

#define DEV_NAME "beep" 
#define DEV_MINOR 255 /*自动分配*/

#define BEEP_OFF 0
#define BEEP_ON  1

/*平台设备驱动匹配表*/
static const struct of_device_id beep_of_match[] = {
    {
        .compatible = "alientek,beep"
    },
    {
        /*Sentinel*/
    },
};

/*蜂鸣器设备结构体*/
struct miscbeep_dev{
    int gpio;
    struct device_node *nd;
}miscbeep;

static int miscbeep_open(struct inode *inode , struct file *filp){
    filp->private_data = &miscbeep;//设置为私有数据
    return 0;
}

static int miscbeep_release(struct inode *inode , struct file *filp){
    //struct miscbeep *dev = (struct miscbeep*)filp->private_data;
    return 0;
}

static ssize_t miscbeep_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){
    int ret;
    //struct miscbeep_dev *dev = (struct miscbeep_dev*)filp->private_data;
    
    int databuf[1];

    ret = copy_from_user(databuf , buf , count);
    if(ret < 0){
        printk("fail to recieve data from user\r\n");
        ret =  -EINVAL;
        goto fail_reus;
    }

    if(databuf[0] == BEEP_OFF){
        printk("beep off\r\n");
        //gpio_set_value(dev->gpio , 1);
    }
    else if(databuf[0] == BEEP_ON){
        printk("beep on\r\n");
        //gpio_set_value(dev->gpio , 0);
    }

    return 0;

fail_reus:
    return ret;
}

static ssize_t miscbeep_read(struct file *file, char __user *buf, size_t size, loff_t *ppos){
    return 0;
}



/*字符设备操作结构集合*/
static struct file_operations miscbeep_fops = {
    .owner   = THIS_MODULE,
    .read    = miscbeep_read,
    .open    = miscbeep_open,
    .write   = miscbeep_write,
    .release = miscbeep_release,
};

/*MISC设备结构体*/
static struct miscdevice beep_miscdev = {
    .minor  = DEV_MINOR,
    .name   = DEV_NAME,
    .fops   = &miscbeep_fops,
};

static int miscbeep_probe(struct platform_device *pdev){
    int ret;

    /*MISC驱动设备注册*/
    ret = misc_register(&beep_miscdev);
    if(ret < 0){
        printk("fail to resgister misc dev\r\n");
        ret = -EINVAL;
        goto fail_rgmisc;
    }
    printk("minor : %d\r\n" , beep_miscdev.minor);

    /*初始化蜂鸣器*/
    miscbeep.nd = pdev->dev.of_node; //
    miscbeep.gpio = of_get_named_gpio(miscbeep.nd , "beep-gpios" , 0);
    if(miscbeep.gpio < 0){
        printk("fail to find gpio num\r\n");
        ret = -EINVAL;
        goto fail_fdgpio;
    }
    ret = gpio_request(miscbeep.gpio , "beep-gpio");
    if(ret){
        printk("fail to request gpio %d\r\n" , miscbeep.gpio);
        ret = -EINVAL;
        goto fail_rqgpio;
    }
    ret = gpio_direction_output(miscbeep.gpio , 1);/*默认高电平*/
    if(ret < 0){
        printk("fail to set gpio\r\n");
        ret = -EINVAL;
        goto fail_stop;
    }

    return 0;


fail_stop:
    gpio_free(miscbeep.gpio);
fail_rqgpio:
fail_fdgpio:
fail_rgmisc:
    return ret;
}

static int miscbeep_remove(struct platform_device *pdev){
    //关闭蜂鸣器
    gpio_direction_output(miscbeep.gpio , 1);
    //释放GPIO
    gpio_free(miscbeep.gpio);
    //注销MISC设备
    misc_deregister(&beep_miscdev);
    return 0;
}

/*平台驱动*/
static struct platform_driver misc_driver = 
{
    .driver = {
        .name = "beep_driver",
        .of_match_table = beep_of_match , /*设备树匹配表*/

    }, 
    .probe = miscbeep_probe , 
    .remove = miscbeep_remove,  
};


module_platform_driver(misc_driver); //使用宏定义代替下面注释的代码

/*驱动入口函数*/
// static int __init miscinit(void){
    
//     return platform_driver_register(&misc_driver);/*注册平台驱动*/
// }

// static void __exit miscexit(void){
//     platform_driver_unregister(&misc_driver); /*卸载平台驱动*/
// }

// module_init(miscinit);
// module_exit(miscexit);



MODULE_LICENSE("GPL");
MODULE_AUTHOR("constant_z");