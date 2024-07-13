/*
 * 12 定时器试验
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
#include <linux/timer.h> //Linux内核定时器
#include <linux/jiffies.h> 

/*设备数量和设备名称定义*/
#define MOD_CNT 1
#define MOD_NAME "mytimer"

/*LEDk开关宏定义*/
#define LEDON  1
#define LEDOFF 0

/*按键值宏定义*/
#define KEY0VAL 0XF0
#define INVKEYVAL 0X0


/*命令定义*/
#define CLOSE_CMD   _IO(0XEF , 1) //关闭命令
#define OPEN_CMD    _IO(0xef , 2) 
#define SETPER_CMD  _IOW(0xef , 3 , int) //设置定时器周期值


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
    /*客制化区*/
    struct device_node *nd;  //设备树节点
    int gpionum; //gpio编号
    struct timer_list mytimer; //内核定时器结构体
    int timer_per; //定时器周期 单位ms 使用自旋锁处理
    spinlock_t lock; //自旋锁处理定时器周期赋值

}timer;


/*实现具体驱动操作函数*/
static int mod_open(struct inode *inode , struct file *filp){
    filp->private_data = &timer;//设置为私有数据
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

static long mod_unlocked_ioctl (struct file *filp, unsigned cmd, unsigned long arg){
    
    struct mod_dev *dev = (struct mod_dev*)filp->private_data;
    int ret = 0;
    int val = 0;
    unsigned long flags;
    switch (cmd)
    {
        case CLOSE_CMD:
            del_timer_sync(&dev->mytimer);
            break;
        case OPEN_CMD:
            mod_timer(&dev->mytimer , jiffies + msecs_to_jiffies(dev->timer_per)); //重启定时器
            break;
        case SETPER_CMD:
            spin_lock_irqsave(&dev->lock , flags);
            ret = copy_from_user(&val , (int *)arg , sizeof(int));
            if(ret < 0){
                printk("fail to set period for timer\r\n");
                ret = -EFAULT;
            }
            dev->timer_per = val; //设置周期值
            spin_unlock_irqrestore(&dev->lock , flags);
            mod_timer(&dev->mytimer , jiffies + msecs_to_jiffies(dev->timer_per)); //重启定时器
            break;
    default:
        break;
    }
    return ret;
}

static const struct file_operations mod_fops = {
	.owner	= THIS_MODULE,
	.open	= mod_open,
	.write	= mod_write,
	.read	= mod_read,
    .release = mod_release,
    .unlocked_ioctl = mod_unlocked_ioctl,
};

/*定时器处理函数*/
static void mytimerFunction(unsigned long arg){
    struct mod_dev *dev = (struct mod_dev*)arg;
    static int led_stat = 1; //led状态
    
    led_stat = !led_stat;

    gpio_set_value(dev->gpionum , led_stat);

    mod_timer(&dev->mytimer , jiffies + msecs_to_jiffies(dev->timer_per)); //重启定时器
    
}

/*初始化led灯*/
static int led_init(struct mod_dev * dev){
    int ret;

    /*获取设备节点*/
    dev->nd = of_find_node_by_path("/gpioled");
    if(dev->nd == NULL){
        printk("fail to find node\r\n");
        ret = -EINVAL;
        goto fail_fdnd;
    }
    /*查找对应IO*/
    dev->gpionum = of_get_named_gpio(dev->nd , "led-gpios" , 0);
    if(dev->gpionum < 0){
        printk("fail to find gpio num\r\n");
        ret = -EINVAL;
        goto fail_fngn;
    }
    /*申请使用IO*/
    ret = gpio_request(dev->gpionum , "led");
    if(ret){
        printk("fail to use gpio num\r\n");
        ret = -EBUSY;
        goto fail_allocgp;
    }

    /*设置IO电平*/
    ret = gpio_direction_output(dev->gpionum , 1); /*默认关灯*/
    if(ret < 0){
        printk("fail to set level\r\n");
        ret = -EINVAL;
        goto fail_usegp;
    }

    return 0;

fail_usegp:
fail_allocgp:
    gpio_free(dev->gpionum);
fail_fngn:
fail_fdnd:
    return ret;
}

static int __init mod_init(void){
    int ret;
    printk("start to init modproject\r\n"); /*prinrk可以支持中文输出，但为标准化，还是使用英文*/

    //注册字符设备驱动
    timer.major = 0;
    if(timer.major){
        timer.devid = MKDEV(timer.major , 0);
        ret = register_chrdev_region(timer.devid , MOD_CNT , MOD_NAME);
    }
    else{ /*自动分配设备号*/
        ret = alloc_chrdev_region(&timer.devid , 0 ,MOD_CNT ,  MOD_NAME);
    }
    if(ret < 0){
        printk("fail to aceesee device id\r\n");
        goto fail_devid;
    }

    timer.major = MAJOR(timer.devid);
    timer.minor = MINOR(timer.devid);
    printk("timer major = %d minor = %d\r\n",timer.major , timer.minor);

    //初始化字符cdev
    timer.mycdev.owner =  THIS_MODULE;
    cdev_init(&timer.mycdev , &mod_fops);
    ret = cdev_add(&timer.mycdev , timer.devid , MOD_CNT);
    if(ret < 0){
        printk("fail to access cdev\r\n");
        goto fail_cdev;
    }

    //创建类
    timer.myclass = class_create(THIS_MODULE, MOD_NAME);
    if(IS_ERR(timer.myclass)){
        printk("fail to create class\r\n");
        ret = PTR_ERR(timer.myclass);
        goto fail_class;
    }

    //创建设备
    timer.mydevice = device_create(timer.myclass, NULL, timer.devid, NULL,MOD_NAME);
    if(IS_ERR(timer.mydevice )){
        printk("fail to create device\r\n");
        ret =  PTR_ERR(timer.mydevice );
        goto fail_device;
    }

    //初始化自旋锁
    spin_lock_init(&timer.lock);
    //初始化led
    ret = led_init(&timer);
    if(ret < 0){
        printk("fail to init led\r\n");
        goto fail_useled;
    }
    //初始化定时器
    init_timer(&timer.mytimer);
    timer.timer_per = 500; //500ms周期
    //处理定时器函数内容
    timer.mytimer.function = mytimerFunction; //定时器处理函数入口 
    timer.mytimer.expires = jiffies + msecs_to_jiffies(timer.timer_per);        //设置终止时间点 500ms
    timer.mytimer.data = (unsigned long)&timer;
    //向linux内核注册一个定时器
    add_timer(&timer.mytimer);


    return 0;

fail_useled:
    device_destroy(timer.myclass , timer.devid);
fail_device:
    class_destroy(timer.myclass);
fail_class:
    cdev_del(&timer.mycdev);
fail_cdev:
    unregister_chrdev_region(timer.devid , MOD_CNT);
fail_devid:
    return ret;
}

static void __exit mod_exit(void){
    //关闭led
    gpio_direction_output(timer.gpionum, 1);
    //删除定时器
    del_timer(&timer.mytimer);
    //删除GPIO号
    gpio_free(timer.gpionum);
    //摧毁设备
    device_destroy(timer.myclass , timer.devid);
    //摧毁类
    class_destroy(timer.myclass); 
    //摧毁cdev
    cdev_del(&timer.mycdev);
    //注销字符设备
    unregister_chrdev_region(timer.devid , MOD_CNT);
}


module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("constan_z");
