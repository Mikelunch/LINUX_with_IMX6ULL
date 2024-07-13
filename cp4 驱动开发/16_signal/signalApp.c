/*
 *驱动测试APP
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "linux/ioctl.h"
#include <fcntl.h>
#include <signal.h> //信号

#define KEY0VAL 0XF0
#define INVKEYVAL 0X0

/*命令定义*/
#define CLOSE_CMD   _IO(0XEF , 1) //关闭命令
#define OPEN_CMD    _IO(0xef , 2) 
#define SETPER_CMD  _IOW(0xef , 3 , int) //设置定时器周期值

/*
* argc:应用程序参数个数
* argv[]:传递给应用程序的参数内容
    -> 1    <filename> 驱动在Linux下的文件路径
* eg: ./irqApp /dev/imxirq
*/
int fd;

static void signal_hanlder(int arg){
    int err , data;
    err = read(fd, &data , sizeof data); //读取按键值
    if(err < 0){
        /*失败*/
    }else{
        printf("sigio signal! key val = %d\r\n" ,data);
    }
}

int main(int argc , char *argv[]){
    int ret;
    int data;
    char *filename;
    int flags = 0;
    int cnt = 2;


    if(argc != 2){
        printf("Error Usage\r\n");
        return -1;
    }

    filename = argv[1];
    fd = open(filename , O_RDWR);
    if(fd < 0){
        printf("failed to open file\r\n");
        return -1;
    }

    /*设置信号处理函数*/
    signal(SIGIO , signal_hanlder);

    /*在应用中对通知进行处理*/
    fcntl(fd , F_SETOWN , getpid()); /*设置当前进程接受SIGIO信号*/
    
    flags = fcntl(fd , F_GETFL); /*当前进程开启异步通知*/

    fcntl(fd , F_SETFL , flags | FASYNC); /*异步通知*/

    while(1){
        sleep(2);
    }
    printf("App shutdown\r\n");
    
    close(fd);

    return 0;
}