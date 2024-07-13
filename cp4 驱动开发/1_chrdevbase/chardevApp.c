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
    -> 2    <rwcmd>    1:表示读驱动 2：表示写驱动
*/

int main(int argc , char *argv[]){

    int ret = 0;
    int fd = 0; //文件描述符
    char *filename;
    char readbuf[100] , writebuf[100] = {"hello Linux!"};

    if(argc != 3){
        printf("参数传递错误\r\n , 主程序结束返回-1\r\n");
        return -1;
    }

    filename = argv[1];
    /*打开文件*/
    fd = open(filename, O_RDWR);
    if(fd < 0){
        printf("Cant open file %s\r\n" , filename);
        return -1;
    }

    if(*argv[2] == '1'){ //应用程序读数据
        /*读写文件*/
        ret = read(fd , readbuf , 50);
        if(ret < 0){//读失败
            printf("read %s failed \r\n" , filename);
            //return -1;
        }
        else{ //读成功
            printf("APP read data from kernel: %s\r\n" ,readbuf);
        }
    }

     if(*argv[2] == '2'){ //应用程序写数据
        /*读写文件*/
        ret = write(fd,writebuf,50);
        if(ret < 0){//写失败
            printf("write %s failed \r\n" , filename);
            //return -1;
        }
        else{ //写成功
            printf("write to kernel data : %s\r\n" , writebuf);
        }
    }

    /*关闭文件*/
    ret = close(fd);
    if(ret < 0){//关闭文件失败
        printf("close %s failed \r\n" , filename);
        //return -1;
    }
    else{ //关闭文件成功

    }

    return 0;
}