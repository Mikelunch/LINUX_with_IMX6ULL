/*
 *驱动测试APP
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define KEY0VAL 0XF0
#define INVKEYVAL 0X0

/*
* argc:应用程序参数个数
* argv[]:传递给应用程序的参数内容
    -> 1    <filename> 驱动在Linux下的文件路径
* eg: ./keyApp /dev/gpiokey 
*/
int main(int argc , char *argv[]){
    int fd , ret , val;
    char *filename;
    int cnt = 5;

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

    /*循环读取按键值*/
    while(1){
        read(fd , &val , sizeof val);
        if(val == KEY0VAL){
            printf("key0 is pressed , val = %d \r\n" , val);
        }
    }

    printf("App shutdown\r\n");
    
    close(fd);

    return 0;
}