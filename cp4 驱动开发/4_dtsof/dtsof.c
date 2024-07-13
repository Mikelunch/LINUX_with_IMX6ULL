/*
 * 4 设备树驱动试验
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

// backlight {
// 		compatible = "pwm-backlight";
// 		pwms = <&pwm1 0 5000000>;
// 		brightness-levels = <0 4 8 16 32 64 128 255>;
// 		default-brightness-level = <7>;
// 		status = "okay";
// };


static int __init dtsof_init(void){
    int ret = 0;
    struct device_node *bl_nd = NULL; /*背光设备节点*/
    struct property *comppro = NULL;
    const char* name;
    int default_val;
    u32 *bl_val;
    u32 elemsize;
    printk("regiter dtsof_init\r\n");
    
    /*编写设备树驱动*/
    //1找到 backlight节点
    bl_nd = of_find_node_by_path("/backlight");
    if(bl_nd == NULL){
        ret = -EINVAL;
        goto fail_findnd;
    }
    //2获取属性
    comppro = of_find_property(bl_nd,"compatible",NULL);
    if(comppro == NULL){
        ret = -EINVAL;
        goto fail_findpro;
    }
    else{
        printk("get property = %s\r\n" , (char*)comppro->value);
    }

    ret = of_property_read_string(bl_nd,"status",&name);
    if(ret < 0){
        goto fail_rs;
    }
    else{
        printk("get status = %s\r\n" , name);
    }

    ret = of_property_read_u32(bl_nd,"default-brightness-level",&default_val);
    if(ret < 0){
        goto fail_rd32;
    }
    else{
        printk("get default-brightness-level = %d\r\n" , default_val);
    }

    //获取属性值是数组的数组个数
    elemsize =  of_property_count_elems_of_size(bl_nd,"brightness-levels", sizeof(u32));
    if(elemsize < 0){
        ret = -elemsize;
        goto fail_rd32arraysize;
    }
    else{
        printk("get brightness-levels size is = %d\r\n" , elemsize);
    }
    //申请内存
    bl_val = kmalloc(elemsize * sizeof(u32) , GFP_KERNEL);
    if(!bl_val){//申请内存失败
        ret = -ENOMEM;
        goto fail_mem;
    }
    //获取属性值是数组的具体数组值
    ret = of_property_read_u32_array(bl_nd,"brightness-levels",bl_val, elemsize);
    if(ret < 0){
        ret = -EINVAL;
        goto fail_rd32array;
    }
    else{
        int i = 0;
        printk("get brightness-levels = ");
        for( ;i < 8;i++){
            printk("%d " , bl_val[i]);
        }
        printk("\r\n");
    }
    kfree(bl_val);

    return 0;
    
fail_mem:
    kfree(bl_val); /*释放内存*/
fail_rd32array:
fail_rd32arraysize:
fail_rd32:
fail_rs:
fail_findpro:
fail_findnd:
    return ret;

}

static void __exit dtsof_exit(void){

}


module_init(dtsof_init);
module_exit(dtsof_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("constant_z");