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
#include <linux/interrupt.h> //request_irq时使用
#include <linux/timer.h> //Linux内核定时器
#include <linux/jiffies.h>  //Linux定时器计数值

/*设备数量和设备名称定义*/
#define MOD_CNT 1
#define MOD_NAME "imxirq"

/*LEDk开关宏定义*/
#define LEDON  1
#define LEDOFF 0

/*按键值宏定义*/
#define KEY0VAL   0X01
#define INVKEYVAL 0Xff
#define KEYNUM    0x01  

/*按键结构体*/
struct irq_keydev
{
    int gpio; //io编号
    int irqNum; //中断号
    int keyval; //键值
    char name[10]; //按键名字
    irqreturn_t (*handler)(int , void*);                //中断服务函数
    struct tasklet_struct tasklet; //中断下半部结构体
};

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
    struct irq_keydev irq_key[1]; //一个按键对应一个结构体
    struct timer_list mytimer; //内核定时器结构体
    atomic_t key0val; //按键按下的值
    atomic_t releasekey; //按键是否释放
    

}mod;


/*实现具体驱动操作函数*/
static int mod_open(struct inode *inode , struct file *filp){
    filp->private_data = &mod;//设置为私有数据
    return 0;
}

static int mod_release(struct inode *inode , struct file *filp){
    //struct mod_dev *dev = (struct mod_dev*)filp->private_data;
   
    return 0;
}

static ssize_t mod_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){
    //struct mod_dev *dev = (struct mod_dev*)filp->private_data;
    return 0;
}

static ssize_t mod_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos){
    int ret = 0 ;
    int keyval , releasekey;
    struct mod_dev *dev = (struct mod_dev*)filp->private_data;
    keyval = atomic_read(&dev->key0val);
    releasekey = atomic_read(&dev->releasekey);

    if(releasekey){ //按键释放
        if(keyval & 0x80){
            keyval &= ~0x80;
            ret = copy_to_user(buf , &keyval , sizeof(keyval)); //发送给应用端
        }else{
            printk("error keyvla\r\n");
            ret = -EINVAL;
            goto data_err;
        }
        atomic_set(&dev->releasekey , 0); //清零 以便下一次使用
    }else{
        ret = -EINVAL;
        goto data_err;
    }

    return ret;
data_err:
    return ret;
}

static const struct file_operations mod_fops = {
	.owner	= THIS_MODULE,
	.open	= mod_open,
	.write	= mod_write,
	.read	= mod_read,
    .release = mod_release,
};

/*中断处理函数*/
static irqreturn_t key0_handler(int irq, void *parm){
    //int val = 0;
    struct mod_dev *dev = (struct mod_dev*)parm;
#if 0
    val = gpio_get_value(dev->irq_key[0].gpio);
    if(val == 0){ /*按下*/
        printk("key0 push\r\n");
    }else if(val == 1){ /*释放*/
        printk("key0 realeas\r\n");
    }
#endif
#if 0
    /*开启定时器*/
    dev->mytimer.data = (volatile unsigned long)parm;
    mod_timer(&dev->mytimer , jiffies + msecs_to_jiffies(10));//10ms定时
    return IRQ_HANDLED;
#endif
    /*使用中断上下半部的方式开启定时器*/
    tasklet_schedule(&dev->irq_key[0].tasklet);//调度tasklet 按键按下就会执行下部的处理函数
    return IRQ_HANDLED;
}

/*中断下半部处理函数*/
static void tasklet_hanlder(unsigned long parm){
    printk("tasklet_hanlder\r\n");
    /*开启定时器*/
    struct mod_dev *dev = (struct mod_dev*)parm;
    dev->mytimer.data = (volatile unsigned long)parm;
    mod_timer(&dev->mytimer , jiffies + msecs_to_jiffies(10));//10ms定时
}

/*定时器处理函数*/
static void mytimerFunction(unsigned long arg){
    int val = 0;
    struct mod_dev *dev = (struct mod_dev*)arg;
    val = gpio_get_value(dev->irq_key[0].gpio);
    if(val == 0){ /*按下*/
        printk("key0 push\r\n");
        atomic_set(&dev->key0val , dev->irq_key[0].keyval);
    }else if(val == 1){ /*释放*/
        printk("key0 realeas\r\n");
        atomic_set(&dev->key0val , dev->irq_key[0].keyval | 0x80); //代表释放的键值
        atomic_set(&dev->releasekey , 1);//代表释放
    }
    
}

static int __init mod_init(void){
    int ret;
    int i , j , cnt1 , cnt2;
    printk("start to init modproject\r\n"); /*printk可以支持中文输出，但为标准化，还是使用英文*/

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

    //原子变量初始化
    atomic_set(&mod.key0val , INVKEYVAL);//默认无效按键值
    atomic_set(&mod.releasekey , 0); //默认没释放

    //按键初始化
    //获取设备节点
    mod.nd = of_find_node_by_path("/alientekkey");
    if(mod.nd == NULL){
        printk("fail to find device node\r\n");
        ret = -EINVAL;
        goto fail_fdnd;
    }
    //获取GPIO编号
    
    for(i = 0; i < KEYNUM; i++){
        mod.irq_key[i].gpio = of_get_named_gpio(mod.nd , "key-gpios" , i);
        if(mod.irq_key[i].gpio < 0){
            printk("fail to find gpio num\r\n");
            ret = -EINVAL;
            goto fail_fdgn;
        }
    }
    //申请使用IO
    for(i = 0; i < KEYNUM; i++){
        memset(mod.irq_key[i].name , 0 , sizeof(mod.irq_key[i].name));
        sprintf(mod.irq_key[i].name , "KEY%d" , i);
        ret = gpio_request(mod.irq_key[i].gpio , mod.irq_key[i].name);
        if(ret){
            printk("fail to use gpio\r\n");
            ret = -EINVAL;
            goto fail_usgpio;
        }
        ret = gpio_direction_input(mod.irq_key[i].gpio);
        if(ret){
            printk("fail to set gpio as input mode\r\n");
            ret = -EINVAL;
            cnt1 = i;
            goto fail_stit;
        }

        //获取中断号
        mod.irq_key[i].irqNum = gpio_to_irq(mod.irq_key[i].gpio);
        //mod.irq_key[i].irqNum = irq_of_parse_and_map(mod.nd , i);
        printk("interrput num is %d\r\n" , mod.irq_key[i].irqNum);
    }
    mod.irq_key[0].handler =  key0_handler;
    mod.irq_key[0].keyval  =  KEY0VAL;

    //按键中断初始化
    for(i = 0; i < KEYNUM; i++){
        ret = request_irq(mod.irq_key[i].irqNum , key0_handler , IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING ,
        mod.irq_key[i].name , &mod);

        if(ret){
            printk("fail to request  %d interrput\r\n" , i);
            cnt2 = i;
            cnt1 = KEYNUM;
            goto fail_rqirq;
        }

        //初始化中断下半部
        tasklet_init(&mod.irq_key[i].tasklet , tasklet_hanlder , (volatile unsigned long)&mod);
    }

    //定时器初始化
    init_timer(&mod.mytimer);
    mod.mytimer.function = mytimerFunction; //定时器处理函数入口 

    return 0;

fail_rqirq:
    for(j = 0; j <= cnt2; j++){
        free_irq(mod.irq_key[j].irqNum , &mod);
    }
fail_stit:
    for(j = 0; j <= cnt1; j++){
        gpio_free(mod.irq_key[j].gpio);
    }
fail_usgpio:
fail_fdgn:
fail_fdnd:
    device_destroy(mod.myclass , mod.devid);
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
    int i;
    //释放中断
    for(i = 0; i < KEYNUM; i++){
        free_irq(mod.irq_key[i].irqNum , &mod);
    }   
    //删除定时器
    del_timer(&mod.mytimer);
    //注销GPIO号
    for(i = 0; i < KEYNUM; i++){
        gpio_free(mod.irq_key[i].gpio);
    }
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
