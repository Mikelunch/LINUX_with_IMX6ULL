/*
 * 1.模块加载、卸载试验 
 * 驱动端程序
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h> //调用copy_to_user要用到

#define CHRDEVBASE_MAJOR 200 //主设备号
#define CHRDEVBASE_NAME "chrdevbase" //设备名

//读写缓冲
static char readbuf[100];
static char writebuf[100];
static char kerneldata[] = {"kernel data"};

static int chrdevbase_open(struct inode *inodes, struct file *file){
    printk("chrdevbase_open\r\n");
    /*想实现的内容*/
    return 0;
}

static int chrdevbase_release(struct inode *inodes, struct file *filp){
    //printk("chrdevbase_release\r\n");
    /*想实现的内容*/
    return 0;
}

/*应用程序读取驱动端数据*/
static ssize_t chrdevbase_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos){
    int ret = 0;
    //先拷贝数据，不要直接发送原始数据
    memcpy(readbuf,kerneldata,sizeof kerneldata);
    //向应用程序发送数据
    ret = copy_to_user(buf,readbuf,count);
    if(ret != 0){
        printk("copy failed!\r\n");
    }
    return 0;
}

/*应用程序向驱动端写数据*/
static ssize_t chrdevbase_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){
    int ret = 0;
    ret = copy_from_user(writebuf , buf , count);
    if(ret != 0 ){
        printk("write to kernel failed\r\n");
    }
    else{
        //printk("write to kernel %s\r\n" , writebuf);
    }
    return 0;
}

static struct file_operations chrdevbase_fops = {
    .owner = THIS_MODULE,
    .open = chrdevbase_open,
    .release = chrdevbase_release,
    .read = chrdevbase_read,
    .write = chrdevbase_write, 
};

/*
 * 要注册的驱动
*/
static int __init chrdevbase_init(void){
    int ret = 0;
    printk("chrdevbase_init\r\n");
    /*注册字符设备*/
    ret = register_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME,&chrdevbase_fops);
    
    if(ret < 0){//出错
        printk("chrdevbase init failed\r\n");
    }

    return 0;
}

static void __exit chrdevbase_exit(void){
    printk("exit chrdevbase\r\n");
    /*注销字符设备*/
    unregister_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME);  
}

/*
 * 模块入口出口
*/
module_init(chrdevbase_init);
module_exit(chrdevbase_exit);

