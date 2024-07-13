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
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>


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
int main(int argc , char *argv[]){
    int fd , ret = 0;
    int data;
    char *filename;
    //fd_set readfds;
    //struct timeval timeout;

    struct pollfd fds;
    int timeout = 2000;

    int cnt = 5;

    if(argc != 2){
        printf("Error Usage\r\n");
        return -1;
    }

    filename = argv[1];
    fd = open(filename , O_RDWR | O_NONBLOCK); /*非阻塞方式打开*/
    if(fd < 0){
        printf("failed to open file\r\n");
        return -1;
    }

/*使用select 监视非阻塞IO访问*/
#if 0
   /*主循环*/
   while(1){
        FD_ZERO(&readfds); //初始化fds
        FD_SET(fd , &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000000; //超时时间设置为1s

        ret = select(fd + 1 , &readfds , NULL , NULL , &timeout);

        switch (ret)
        {
            case 0:
                //printf("app time out \r\n");
                /*超时处理*/
                break;
            case -1:
                printf("app read file err\r\n");
                /*访问错误处理*/
        default:
            /*可以读取数据*/
            if(FD_ISSET(fd , &readfds)){
                printf("start to read file\r\n");
                ret = read(fd , &data , sizeof(data));
                if(ret < 0){
                   /*读文件失败*/ 
                }else{
                    if(data){
                        printf("key val = %#x\r\n" , data);
                    }
                }
            }
            break;
        }
   }
#endif

/*使用poll监视*/
    while(1){
        fds.fd = fd;
        fds.events = POLLIN; //监视的事件类型是读入
       
        ret = poll(&fds ,  1 , timeout); //超时2000ms

        if(ret == 0){
            printf("app time out \r\n");
            /*超时处理*/
            cnt--;
        }
        else if(ret == -1){
            printf("app read file err\r\n");
            /*访问错误处理*/
        }
        else{
            /*可以读取数据*/
            if(fds.revents | POLLIN){
                printf("start to read file\r\n");
                ret = read(fd , &data , sizeof(data));
                if(ret < 0){
                   /*读文件失败*/ 
                }else{
                    if(data){
                        printf("key val = %#x\r\n" , data);
                    }
                }
            }
        }
        if(!cnt){
            break;
        }
   }

    printf("App shutdown\r\n");
    
    close(fd);

    return 0;
}