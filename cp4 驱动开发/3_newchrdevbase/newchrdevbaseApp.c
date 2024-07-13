/*
 *驱动测试APP
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/*
* argc:应用程序参数个数
* argv[]:传递给应用程序的参数内容
    -> 1    <filename> 驱动在Linux下的文件路径
    -> 2    <rwcmd>    向驱动写数据 0:LED关灯 1:LED开灯
* eg: ./ledApp /dev/led 0 :led关灯
*/
int main(int argc , char *argv[]){
    int fd , ret;
    char *filename;
    unsigned char databuf[1];

    if(argc != 3){
        printf("Error Usage\r\n");
        return -1;
    }

    filename = argv[1];
    fd = open(filename , O_RDWR);
    if(fd < 0){
        printf("failed to open file\r\n");
        return -1;
    }

    databuf[0] = atoi(argv[2]);//字符转int
    ret = write(fd ,databuf , sizeof databuf);
    if(ret < 0){
        printf("Write to %s failed\r\n" , argv[1]);
        close(fd);
        return -1;
    }

    close(fd);

    return 0;
}