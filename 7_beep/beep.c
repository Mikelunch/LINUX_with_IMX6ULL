/*
 * 6 GPIO系统驱动试验
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
#include <linux/delay.h> //ssleep()使用


#define GPIOBEEP_CNT 1
#define GPIOBEEP_NAME "gpiobeep"

#define BEEPON  1
#define BEEPOFF 0

struct dtsbeep_dev
{
    int major;
    int minor;
    dev_t devid;
    struct cdev mycdev;
    struct device *mydevice;
    struct device_node *nd;
    struct class *myclass;
    int beep_gpio;

}dtsbeep;

/*实现具体驱动操作函数*/
static int devbeep_open(struct inode *inode , struct file *filp){
    filp->private_data = &dtsbeep;//设置为私有数据
    return 0;
}

static int devbeep_release(struct inode *inode , struct file *filp){
    struct dtsbeep_dev *dev = (struct dtsbeep_dev*)filp->private_data;
    return 0;
}

/*beep打开或者关闭*/
void beep_switch(unsigned char flag){
    if(flag == BEEPON){
        gpio_set_value(dtsbeep.beep_gpio , 0); //开启BEEP
    }
    else if(flag == BEEPOFF){
        gpio_set_value(dtsbeep.beep_gpio , 1); //关闭BEEP
    }
}

static ssize_t devbeep_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){

    int ret;
    char databuf[1];
    struct dtsbeep_dev *dev = (struct dtsbeep_dev*)filp->private_data;
    ret = copy_from_user(databuf , buf , count);
    if(ret < 0){
        printk("invaild value\r\n");
        return -EINVAL;
    }
    beep_switch(databuf[0]);

    return 0;
}

static ssize_t devbeep_read(struct file *file, char __user *buf, size_t size, loff_t *ppos){
    return 0;
}

/*设备操作函数集合*/
static const struct file_operations ops = {
    .owner = THIS_MODULE,
    .write = devbeep_write,
    .read  = devbeep_read,
    .open  = devbeep_open,
    .release = devbeep_release
};


static int __init dtsbepp_init(void){
    int ret = 0;

    //1 申请设备号
    dtsbeep.major = 0;
    dtsbeep.minor = 0;
    if(dtsbeep.major){
        printk("手动分配设备号\r\n");
        ret  = register_chrdev_region(dtsbeep.devid , GPIOBEEP_CNT , GPIOBEEP_NAME);
    }
    else{
        printk("自动分配设备号\r\n");
        ret = alloc_chrdev_region(&dtsbeep.devid , 0 ,GPIOBEEP_CNT , GPIOBEEP_NAME);
    }
    if(ret < 0){
        printk("申请设备号失败\r\n");
        goto fail_devid;
    }
    dtsbeep.major = MAJOR(dtsbeep.devid);
    dtsbeep.minor = MINOR(dtsbeep.devid);
    printk("申请设备号成功 major = %d minor = %d\r\n" , dtsbeep.major , dtsbeep.minor);

    //2 添加字符设备
    dtsbeep.mycdev.owner = THIS_MODULE;
    cdev_init(&dtsbeep.mycdev , &ops);
    ret = cdev_add(&dtsbeep.mycdev , dtsbeep.devid , GPIOBEEP_CNT);
    if(ret < 0){
        printk("添加字符设备失败\r\n");
        goto fail_cdev;
    }

    //3 自动创建按节点
    dtsbeep.myclass = class_create(THIS_MODULE , GPIOBEEP_NAME);
    if(IS_ERR(dtsbeep.myclass)){
        ret = PTR_ERR(dtsbeep.myclass);
        printk("创建类失败\r\n");
        goto fail_class;
    }
    dtsbeep.mydevice = device_create(dtsbeep.myclass , NULL , dtsbeep.devid , NULL , GPIOBEEP_NAME);
    if(IS_ERR(dtsbeep.mydevice)){
        ret = PTR_ERR(dtsbeep.mydevice);
        printk("创建设备节点失败\r\n");
        goto fail_device;
    }

    //4 获取设备树节点信息
    dtsbeep.nd = of_find_node_by_path("/alientekbeep:");
    if(dtsbeep.nd == NULL){
        printk("获取节点信息失败\r\n");
        goto fail_fdnd;
    }
    //获取对应GPIO号
    dtsbeep.beep_gpio = of_get_named_gpio(dtsbeep.nd , "beep-gpios" , 0);
    if(dtsbeep.beep_gpio < 0){
        printk("获取GPIO信息失败\r\n");
        ret = -EINVAL;
        goto fail_getgpio;
    }
    printk("beep gpio num = %d\r\n",dtsbeep.beep_gpio);
    //申请使用GPIO
    ret = gpio_request(dtsbeep.beep_gpio , "(dtsbeep.beep_gpio");
    if(ret){
       printk("申请使用GPIO失败\r\n"); 
       ret = -EINVAL;
       goto fail_usgpio;
    }
    //使用io
    ret = gpio_direction_output(dtsbeep.beep_gpio , 1); //默认关闭
    if(ret){
        printk("设置GPIO电平失败\r\n");
        ret = -EINVAL;
        goto fail_stout;
    }
    printk("蜂鸣器初始化正常\r\n");
    return 0;

fail_stout:
    gpio_free(dtsbeep.beep_gpio);
fail_usgpio:
fail_getgpio:
fail_fdnd:
    device_destroy(dtsbeep.myclass , dtsbeep.devid);
fail_device:
    class_destroy(dtsbeep.myclass);
fail_class:
    cdev_del(&dtsbeep.mycdev);
fail_cdev:
    unregister_chrdev_region(dtsbeep.devid , GPIOBEEP_CNT);
fail_devid:
    return ret;
}

static void __exit dtsbepp_exit(void){

    gpio_direction_output(dtsbeep.beep_gpio , 1);
    gpio_free(dtsbeep.beep_gpio);
    device_destroy(dtsbeep.myclass , dtsbeep.devid);
    class_destroy(dtsbeep.myclass);
    cdev_del(&dtsbeep.mycdev);
    unregister_chrdev_region(dtsbeep.devid , GPIOBEEP_CNT);
    
}

module_init(dtsbepp_init);
module_exit(dtsbepp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("constant_z");
