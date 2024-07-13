/*
 * 9 自旋锁操作试验
 * 驱动端程序
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

#define GPIOLED_CNT 1
#define GPIOLED_NAME "gpioled"

#define LEDON  1
#define LEDOFF 0

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

    int dev_status; /*0:设备未有程序占用*/
    spinlock_t lock;

}gpioled;


/*实现具体驱动操作函数*/
static int led_open(struct inode *inode , struct file *filp){
    unsigned long irqflag;
    filp->private_data = &gpioled;//设置为私有数据
    //spin_lock(&gpioled.lock); /*数据临界区*/
    spin_lock_irqsave(&gpioled.lock , irqflag);/*保存中断、禁止中断并加锁*/
    if(gpioled.dev_status){
        printk("设备被占用\r\n");
        //spin_unlock(&gpioled.lock);
        spin_unlock_irqrestore(&gpioled.lock , irqflag);
        return -EBUSY;
    }
    printk("设备未被使用\r\n");
    gpioled.dev_status++;//占用设备
    spin_unlock_irqrestore(&gpioled.lock , irqflag);
   // spin_unlock(&gpioled.lock);
    return 0;
}

static int led_release(struct inode *inode , struct file *filp){
    struct gpioled_dev *dev = (struct gpioled_dev*)filp->private_data;
    unsigned long irqflag;
    //spin_lock(&dev->lock);/*数据临界区*/
     spin_lock_irqsave(&dev->lock , irqflag);/*保存中断、禁止中断并加锁*/
    if(dev->dev_status)
        dev->dev_status--;//解除占用
    spin_unlock_irqrestore(&dev->lock , irqflag);
    //spin_unlock(&dev->lock);
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
    struct gpioled_dev *dev = (struct gpioled_dev*)filp->private_data;
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

static int __init led_init(void){
    int ret;
    printk("start to init gpioled\r\n");
    //初始化自旋锁
    spin_lock_init(&gpioled.lock);
    gpioled.dev_status = 0;
    //注册字符设备驱动
    gpioled.major = 0;
    if(gpioled.major){
        gpioled.devid = MKDEV(gpioled.major , 0);
        register_chrdev_region(gpioled.devid , GPIOLED_CNT , GPIOLED_NAME);
    }
    else{ /*自动分配设备号*/
        alloc_chrdev_region(&gpioled.devid , 0 ,GPIOLED_CNT ,  GPIOLED_NAME);
    }
    gpioled.major = MAJOR(gpioled.devid);
    gpioled.minor = MINOR(gpioled.devid);
    printk("gpioled major = %d minor = %d\r\n",gpioled.major , gpioled.minor);

    //初始化字符cdev
    gpioled.mycdev.owner =  THIS_MODULE;
    cdev_init(&gpioled.mycdev , &led_fops);
    ret = cdev_add(&gpioled.mycdev , gpioled.devid , GPIOLED_CNT);
    if(ret < 0){
        printk("fail to access cdev\r\n");
    }

    //创建类
    gpioled.myclass = class_create(THIS_MODULE, GPIOLED_NAME);
    if(IS_ERR(gpioled.myclass)){
        printk("fail to create class\r\n");
        return PTR_ERR(gpioled.myclass);
    }

    //创建设备
    gpioled.mydevice = device_create(gpioled.myclass, NULL, gpioled.devid, NULL,GPIOLED_NAME);
    if(IS_ERR(gpioled.mydevice )){
        printk("fail to create device\r\n");
        return PTR_ERR(gpioled.mydevice );
    }

    //gpio相关操作
    //获取设备节点
    gpioled.nd = of_find_node_by_path("/gpioled");
    if(gpioled.nd == NULL){
        printk("fail to access device node\r\n");
        ret = -EINVAL;
        goto fail_fdnd;
    }
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
fail_fdnd:
    return ret;
}

static void __exit led_exit(void){
    //关闭LED
    gpio_set_value(gpioled.led_gpio , 1); //关闭LED
    //摧毁GPIO
    gpio_free(gpioled.led_gpio);
    //摧毁cdev
    cdev_del(&gpioled.mycdev);
    //注销字符设备
    unregister_chrdev_region(gpioled.devid , GPIOLED_CNT);
    //摧毁设备
    device_destroy(gpioled.myclass , gpioled.devid);
    //摧毁类
    class_destroy(gpioled.myclass);
}


module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("constan_z");