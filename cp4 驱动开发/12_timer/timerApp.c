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
* eg: ./timerApp /dev/mytimer
*/
int main(int argc , char *argv[]){
    int fd , ret;
    int cmd , arg;
    char *filename;
    int cnt = 5;
    unsigned char str[10];

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

   /*主循环*/
   while(1){
    printf("INPUT CMD:  ");
    ret = scanf("%d" , &cmd);
    // if(ret != 1){
    //     gets(str);/*防止卡死*/
    // }

    if(cmd == 1){ //关闭
        ioctl(fd , CLOSE_CMD , &arg);
    }else if(cmd == 2){ //打开
        ioctl(fd , OPEN_CMD , &arg);
    }else if(cmd == 3){ //设置周期
        printf("Input period for timer\r\n");
        ret  = scanf("%d" , &arg); // 设置周期
        ioctl(fd , SETPER_CMD , &arg);
    }else if(cmd == 4){ //退出
        break;
    }

   }

    printf("App shutdown\r\n");
    
    close(fd);

    return 0;
}