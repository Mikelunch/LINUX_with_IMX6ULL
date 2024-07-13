/*
 * 0 驱动模板
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

/*设备数量和设备名称定义*/
#define MOD_CNT 1
#define MOD_NAME "modproject"

/*LEDk开关宏定义*/
#define LEDON  1
#define LEDOFF 0

/*按键值宏定义*/
#define KEY0VAL 0XF0
#define INVKEYVAL 0X0

/*设备结构体*/
struct mod_dev
{
    /*必要变量区*/
    dev_t devid;
    int major;
    int minor;
    struct cdev mycdev;
    struct class *myclass;
    struct device *mydevice;
    struct device_node *nd;
    /*客制化区*/


}mod;


/*实现具体驱动操作函数*/
static int mod_open(struct inode *inode , struct file *filp){
    filp->private_data = &mod;//设置为私有数据
    return 0;
}

static int mod_release(struct inode *inode , struct file *filp){
    struct mod_dev *dev = (struct mod_dev*)filp->private_data;
   
    return 0;
}

static ssize_t mod_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){
    struct mod_dev *dev = (struct mod_dev*)filp->private_data;
    return 0;
}

static ssize_t mod_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos){

    return 0;
}

static const struct file_operations mod_fops = {
	.owner	= THIS_MODULE,
	.open	= mod_open,
	.write	= mod_write,
	.read	= mod_read,
    .release = mod_release,
};

static int __init mod_init(void){
    int ret;
    printk("start to init modproject\r\n"); /*prinrk可以支持中文输出，但为标准化，还是使用英文*/

    //注册字符设备驱动
    mod.major = 0;
    if(mod.major){
        mod.devid = MKDEV(mod.major , 0);
        ret = register_chrdev_region(mod.devid , MOD_CNT , MOD_NAME);
    }
    else{ /*自动分配设备号*/
        ret = alloc_chrdev_region(&mod.devid , 0 ,MOD_CNT ,  MOD_NAME);
    }
    if(ret < 0){
        printk("fail to aceesee device id\r\n");
        goto fail_devid;
    }

    mod.major = MAJOR(mod.devid);
    mod.minor = MINOR(mod.devid);
    printk("mod major = %d minor = %d\r\n",mod.major , mod.minor);

    //初始化字符cdev
    mod.mycdev.owner =  THIS_MODULE;
    cdev_init(&mod.mycdev , &mod_fops);
    ret = cdev_add(&mod.mycdev , mod.devid , MOD_CNT);
    if(ret < 0){
        printk("fail to access cdev\r\n");
        goto fail_cdev;
    }

    //创建类
    mod.myclass = class_create(THIS_MODULE, MOD_NAME);
    if(IS_ERR(mod.myclass)){
        printk("fail to create class\r\n");
        ret = PTR_ERR(mod.myclass);
        goto fail_class;
    }

    //创建设备
    mod.mydevice = device_create(mod.myclass, NULL, mod.devid, NULL,MOD_NAME);
    if(IS_ERR(mod.mydevice )){
        printk("fail to create device\r\n");
        ret =  PTR_ERR(mod.mydevice );
        goto fail_device;
    }
 
      return 0;


fail_device:
    class_destroy(mod.myclass);
fail_class:
    cdev_del(&mod.mycdev);
fail_cdev:
    unregister_chrdev_region(mod.devid , MOD_CNT);
fail_devid:
    return ret;
}

static void __exit mod_exit(void){
    //摧毁设备
    device_destroy(mod.myclass , mod.devid);
    //摧毁类
    class_destroy(mod.myclass); 
    //摧毁cdev
    cdev_del(&mod.mycdev);
    //注销字符设备
    unregister_chrdev_region(mod.devid , MOD_CNT);
}


module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("constan_z");
