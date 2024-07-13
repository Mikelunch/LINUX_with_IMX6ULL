/*
 * 5 LED设备树驱动试验
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

#define DTSLED_COUNT 1 //设备号个数
#define DTSLED_NAME "dtsled" //设备号名称

#define LEDON 1
#define LEDOFF 0

/*led虚拟地址指针*/
static void __iomem *IMX6ULL_CCM_CCGR1_BASE;
static void __iomem *IMX6ULL_SW_MUX_GPIO1_IO03;
static void __iomem *IMX6ULL_SW_PAD_GPIO1_IO03;
static void __iomem *IMX6ULL_GPIO1_DR_BASE;
static void __iomem *IMX6ULL_GPIO1_GDIR_BASE;


struct dtsled_dev
{
    struct cdev mycdev;
    dev_t devid;
    int major;
    int minor;
    struct device *mydevice;
    struct class *myclass;
    struct device_node *nd;//设备节点

}dtsled; /*led设备*/

/*实现具体驱动操作函数*/
static int devled_open(struct inode *inode , struct file *filp){
    filp->private_data = &dtsled;//设置为私有数据
    return 0;
}

static int devled_release(struct inode *inode , struct file *filp){
    struct dtsled_dev *dev = (struct dtsled_dev*)filp->private_data;
    return 0;
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

static ssize_t devled_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){
    
    struct dtsled_dev *dev = (struct dtsled_dev*)filp->private_data;
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

static ssize_t devled_read(struct file *file, char __user *buf, size_t size, loff_t *ppos){
    return 0;
}

/*设备操作函数集合*/
static const struct file_operations ops = {
    .owner = THIS_MODULE,
    .write = devled_write,
    .read  = devled_read,
    .open  = devled_open,
    .release = devled_release
};

static int __init dtsled_init(void){
    int ret = 0;
    const char *str;
    u32 reg_data[10];
    /*1注册字符设备*/
    /*1.1申请设备号*/
    dtsled.major = 0;
    dtsled.minor = 0;
    if(dtsled.major){//手动分配设备号
        dtsled.devid = MKDEV(dtsled.major , 0);//默认此设备号为0
        /*1.2注册设备号*/
        ret = register_chrdev_region(dtsled.devid , DTSLED_COUNT , DTSLED_NAME);
    }
    else{//自动分配设备号
        ret = alloc_chrdev_region(&dtsled.devid , 0 , 1 , DTSLED_NAME);
    }
    if(ret < 0){ //分配失败
        printk("alloc device num fail\r\n");
        goto fail_devid;
    }
    dtsled.major = MAJOR(dtsled.devid);
    dtsled.minor = MINOR(dtsled.devid);
    printk("dtsled :major = %d , minor = %d\r\n",dtsled.major , dtsled.minor);
    /*2添加字符设备*/
    dtsled.mycdev.owner = THIS_MODULE;
    cdev_init(&dtsled.mycdev , &ops);
    ret = cdev_add(&dtsled.mycdev , dtsled.devid , 1);
    if(ret < 0){
        printk("fail to add cdev\r\n");
        goto fail_cdev;
    }

    /*3自动创建设备节点*/
    dtsled.myclass = class_create(THIS_MODULE , DTSLED_NAME);
    if(IS_ERR(dtsled.myclass)){
        ret = PTR_ERR(dtsled.myclass);
        printk("fail to create class\r\n");
        goto fail_class;
    }
    dtsled.mydevice = device_create(dtsled.myclass , NULL , dtsled.devid , NULL , DTSLED_NAME);
    if(IS_ERR(dtsled.mydevice)){
        ret = PTR_ERR(dtsled.mydevice);
        printk("fail to create device\r\n");
        goto fail_device;
    }

    /*4 获取设备树节点信息*/
    dtsled.nd = of_find_node_by_path("/alphaled");//注意是全路径
    if(dtsled.nd == NULL){ //获取失败
        printk("fail to access device nod info\r\n");
        ret = -EINVAL;
        goto fail_fdnd;
    }

    //4.2获取属性信息
    ret = of_property_read_string(dtsled.nd , "status" , &str);
    if(ret < 0){//获取失败
        printk("fail to access device status\r\n");
        goto fail_rs;
    }
    else{
        printk("device status = %s \r\n" , str);
    }
    ret = of_property_read_string(dtsled.nd , "compatible" , &str);
    if(ret < 0){//获取失败
        printk("fail to access device compatible\r\n");
        goto fail_rs;
    }
    else{
        printk("device compatible = %s \r\n" , str);
    }


    //4.3 获取reg属性 最关键的一点
    ret = of_property_read_u32_array(dtsled.nd , "reg" ,reg_data , 10);
    if(ret < 0){
        ret = -EINVAL;
        printk("fail to access reg info\r\n");
        goto fail_rs;
    }
    else{
        u32 i = 0;
        printk("reg = ");
        for(; i < 10; i++){
            printk("%#x ", reg_data[i]);
        }
        printk("\r\n");
    }

    //5 获取到属性值后，便是正常的LED初始化//
#if 0
    /*LED初始化*/
    IMX6ULL_CCM_CCGR1_BASE = ioremap(reg_data[0], reg_data[1]);//CCGR1 4字节
    IMX6ULL_SW_MUX_GPIO1_IO03 = ioremap(reg_data[2], reg_data[3]);
    IMX6ULL_SW_PAD_GPIO1_IO03 = ioremap(reg_data[4] , reg_data[5]);
    IMX6ULL_GPIO1_DR_BASE = ioremap(reg_data[6] , reg_data[7]);
    IMX6ULL_GPIO1_GDIR_BASE = ioremap(reg_data[8] , reg_data[9]);
#endif

    IMX6ULL_CCM_CCGR1_BASE = of_iomap(dtsled.nd , 0);//特别注意第二参数是按对来算的
    IMX6ULL_SW_MUX_GPIO1_IO03 = of_iomap(dtsled.nd , 1);
    IMX6ULL_SW_PAD_GPIO1_IO03 = of_iomap(dtsled.nd , 2);
    IMX6ULL_GPIO1_DR_BASE = of_iomap(dtsled.nd , 3);
    IMX6ULL_GPIO1_GDIR_BASE = of_iomap(dtsled.nd , 4);
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

    return 0;

fail_rs:
fail_fdnd:
    device_destroy(dtsled.myclass , dtsled.devid);
fail_device:
    class_destroy(dtsled.myclass);
fail_class:
    cdev_del(&dtsled.mycdev);
fail_cdev:
    unregister_chrdev_region(dtsled.devid , 1);
fail_devid:
    return ret;
}

static void __exit dtsled_exit(void){

    /*关闭led灯*/
    writel(1 << 3 , IMX6ULL_GPIO1_DR_BASE);
    /*取消地址映射*/
    iounmap(IMX6ULL_CCM_CCGR1_BASE);
    iounmap(IMX6ULL_SW_MUX_GPIO1_IO03);
    iounmap(IMX6ULL_SW_PAD_GPIO1_IO03);
    iounmap(IMX6ULL_GPIO1_DR_BASE);
    iounmap(IMX6ULL_GPIO1_GDIR_BASE);
    /*注销字符设备*/
    cdev_del(&dtsled.mycdev);
    /*释放设备号*/
    unregister_chrdev_region(dtsled.devid , 1);
    /*摧毁设备*/
    device_destroy(dtsled.myclass , dtsled.devid);
    /*摧毁类*/
    class_destroy(dtsled.myclass);
}


module_init(dtsled_init);
module_exit(dtsled_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("constant_z");