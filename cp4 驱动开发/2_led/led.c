/*
 * 2 LED驱动试验
 * 驱动端程序
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h> //调用copy_to_user要用到
#include <linux/io.h>

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

/*led虚拟地址指针*/
static void __iomem *IMX6ULL_CCM_CCGR1_BASE;
static void __iomem *IMX6ULL_SW_MUX_GPIO1_IO03;
static void __iomem *IMX6ULL_SW_PAD_GPIO1_IO03;
static void __iomem *IMX6ULL_GPIO1_DR_BASE;
static void __iomem *IMX6ULL_GPIO1_GDIR_BASE;

/*led打开或者关闭*/
void led_switch(unsigned char flag){
    if(flag == LEDON){
        writel(0 << 3 , IMX6ULL_GPIO1_DR_BASE);
    }
    else if(flag == LEDOFF){
        writel(1 << 3 , IMX6ULL_GPIO1_DR_BASE);
    }
}

static int led_open(struct inode *inodes, struct file *filp){
    return 0;
}

static int led_release(struct inode *inode, struct file *filp){
    return 0;
}

static ssize_t led_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos){
    int retval;
    unsigned char databuf[1];

    retval = copy_from_user(databuf , buf , count);

    if(retval < 0){
        printk("kernel write failed\r\n");
        return -EFAULT;
    }

    led_switch(databuf[0]);
    
    return 0;
}
/*3驱动函数编写*/
static const struct file_operations led_operartion ={
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    //.read = led_read,
    .write = led_write
};

static int __init led_init(void){
    int ret = 0;
    printk("led_init is on\r\n");
    /*2注册字符设备*/
    ret = register_chrdev(LED_MAJOR, LED_NAME,&led_operartion);
    if(ret < 0){
        printk("register led falied\r\n");
        return -EIO;//返回IO错误类型
    }
    /*LED初始化*/
    //内存映射
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

    return 0;
}

static void __exit led_exit(void){
    /*关闭led灯*/
    writel(1 << 3 , IMX6ULL_GPIO1_DR_BASE);
    /*取消地址映射*/
    iounmap(IMX6ULL_CCM_CCGR1_BASE);
    iounmap(IMX6ULL_SW_MUX_GPIO1_IO03);
    iounmap(IMX6ULL_SW_PAD_GPIO1_IO03);
    iounmap(IMX6ULL_GPIO1_DR_BASE);
    iounmap(IMX6ULL_GPIO1_GDIR_BASE);
    /*注销设备*/
    unregister_chrdev(LED_MAJOR , LED_NAME);
    printk("led_init is off\r\n");
}

/*1注册/卸载驱动*/
module_init(led_init);
module_exit(led_exit);

/*完善驱动信息*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("constant_z");