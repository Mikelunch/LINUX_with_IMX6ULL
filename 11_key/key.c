/*
 * 11 按键输入操作试验
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

#define GPIOKEY_CNT 1
#define GPIOKEY_NAME "gpiokey"

#define LEDON  1
#define LEDOFF 0

#define KEY0VAL 0XF0
#define INVKEYVAL 0X0

/*gpioled设备结构体*/
struct gpiokey_dev
{
    dev_t devid;
    int major;
    int minor;
    struct cdev mycdev;
    struct class *myclass;
    struct device *mydevice;
    struct device_node *nd;
    int key_gpio;
    atomic_t keyval; //按键值
}gpiokey;


/*实现具体驱动操作函数*/
static int key_open(struct inode *inode , struct file *filp){
    filp->private_data = &gpiokey;//设置为私有数据
    return 0;
}

static int key_release(struct inode *inode , struct file *filp){
    struct gpiokey_dev *dev = (struct gpiokey_dev*)filp->private_data;
   
    return 0;
}

static ssize_t key_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){
    struct gpiokey_dev *dev = (struct gpiokey_dev*)filp->private_data;
    return 0;
}

static ssize_t key_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos){

    int value;
    struct gpiokey_dev *dev = (struct gpiokey_dev*)filp->private_data;
    int ret = 0;
    //读取电平值
    if(gpio_get_value(dev->key_gpio) == 0)/*按下*/{
        while(!gpio_get_value(dev->key_gpio));//等待释放
        atomic_set(&dev->keyval , KEY0VAL);
    }
    else{
        atomic_set(&dev->keyval , INVKEYVAL);
    }
    value = atomic_read(&dev->keyval);
    ret = copy_to_user(buf ,  &value , sizeof(value));
    return ret;
}

static const struct file_operations key_fops = {
	.owner	= THIS_MODULE,
	.open	= key_open,
	.write	= key_write,
	.read	= key_read,
    .release = key_release,
};

static int __init key_init(void){
    int ret;
    printk("start to init gpiokey\r\n");

    //注册字符设备驱动
    gpiokey.major = 0;
    if(gpiokey.major){
        gpiokey.devid = MKDEV(gpiokey.major , 0);
        ret = register_chrdev_region(gpiokey.devid , GPIOKEY_CNT , GPIOKEY_NAME);
    }
    else{ /*自动分配设备号*/
        ret = alloc_chrdev_region(&gpiokey.devid , 0 ,GPIOKEY_CNT ,  GPIOKEY_NAME);
    }
    if(ret < 0){
        printk("fail to aceesee device id\r\n");
        goto fail_devid;
    }
    gpiokey.major = MAJOR(gpiokey.devid);
    gpiokey.minor = MINOR(gpiokey.devid);
    printk("gpiokey major = %d minor = %d\r\n",gpiokey.major , gpiokey.minor);

    //初始化字符cdev
    gpiokey.mycdev.owner =  THIS_MODULE;
    cdev_init(&gpiokey.mycdev , &key_fops);
    ret = cdev_add(&gpiokey.mycdev , gpiokey.devid , GPIOKEY_CNT);
    if(ret < 0){
        printk("fail to access cdev\r\n");
        goto fail_cdev;
    }

    //创建类
    gpiokey.myclass = class_create(THIS_MODULE, GPIOKEY_NAME);
    if(IS_ERR(gpiokey.myclass)){
        printk("fail to create class\r\n");
        ret = PTR_ERR(gpiokey.myclass);
        goto fail_class;
    }

    //创建设备
    gpiokey.mydevice = device_create(gpiokey.myclass, NULL, gpiokey.devid, NULL,GPIOKEY_NAME);
    if(IS_ERR(gpiokey.mydevice )){
        printk("fail to create device\r\n");
        ret =  PTR_ERR(gpiokey.mydevice );
        goto fail_device;
    }
 
    //gpio相关操作
    //初始化原子变量
    atomic_set(&gpiokey.keyval , INVKEYVAL);
    //获取设备节点
    gpiokey.nd = of_find_node_by_path("/alientekkey");
    if(gpiokey.nd == NULL){
        printk("fail to access device node\r\n");
        ret = -EINVAL;
        goto fail_fdnd;
    }
    //获取LED所对应的gpio
    gpiokey.key_gpio = of_get_named_gpio(gpiokey.nd , "key-gpios" , 0);//注意 0是索引
    if(gpiokey.key_gpio < 0){
        printk("fail to access property name\r\n");
        ret = -EINVAL;
        goto  fail_fnnm;
    }
    printk("key gpio num = %d\r\n",gpiokey.key_gpio);
    //申请GPIO
    ret = gpio_request(gpiokey.key_gpio , "gpiokey.key_gpio");
    if(ret){
        printk("fail to request gpio num\r\n");
        ret = -EBUSY;
        goto fail_getnm;
    }
    //使用IO
    ret = gpio_direction_input(gpiokey.key_gpio);//设置为输入
    if(ret){
        printk("fail to set gpio input\r\n");
        ret = -EINVAL;
        goto fail_stip;
    }
    
    return 0;

fail_stip:
    gpio_free(gpiokey.key_gpio);
fail_getnm:
fail_fnnm:
fail_fdnd:
    device_destroy(gpiokey.myclass , gpiokey.devid);
fail_device:
    class_destroy(gpiokey.myclass);
fail_class:
    cdev_del(&gpiokey.mycdev);
fail_cdev:
    unregister_chrdev_region(gpiokey.devid , GPIOKEY_CNT);
fail_devid:
    return ret;
}

static void __exit key_exit(void){
    //释放GPIO
    gpio_free(gpiokey.key_gpio);
    //摧毁设备
    device_destroy(gpiokey.myclass , gpiokey.devid);
    //摧毁类
    class_destroy(gpiokey.myclass); 
    //摧毁cdev
    cdev_del(&gpiokey.mycdev);
    //注销字符设备
    unregister_chrdev_region(gpiokey.devid , GPIOKEY_CNT);
}


module_init(key_init);
module_exit(key_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("constan_z");
