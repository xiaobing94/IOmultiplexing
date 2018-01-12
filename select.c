#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>

#define MAX_SIZE 1024
#define PORT 8888

int create_socket()
{
    int sock_fd;
    if( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){  
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);  
        return -1;
    }  
    int flags = fcntl(sock_fd, F_GETFL, 0);  
    fcntl(sock_fd, F_SETFL, flags|O_NONBLOCK);
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));  
    servaddr.sin_family = AF_INET;  
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//IP地址设置成INADDR_ANY,让系统自动获取本机的IP地址。  
    servaddr.sin_port = htons(PORT);//设置的端口为DEFAULT_PORT

    if( bind(sock_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){  
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    }  
    //开始监听是否有客户端连接  
    if( listen(sock_fd, 1024) == -1){  
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
    } 
    return sock_fd;
}

int main(int argc, char *argv[])
{
    int sock_fd;
    fd_set fds, cpy_fds;
    int conn_fd;
    int ret;
    int fd_max, fd_num;
    int count = 0, i=0;
    char buf[MAX_SIZE] = {0};
    sock_fd = create_socket();
    fd_max = sock_fd;
    FD_ZERO(&fds); 
    FD_SET(sock_fd,&fds); //添加描述符
    while(1)
    {
        cpy_fds = fds;
        if((fd_num = select(fd_max+1, &cpy_fds, 0, 0, NULL)) == -1)
        {
            perror("select error");break;
        }
        if(fd_num == 0)
            continue;
        for(i=0; i<fd_max+1; i++)  // 可以从3开始监听  0 -- 标准输入设备   1 -- 标准输出设备  2 -- 标准错误
        {
            if(FD_ISSET(i, &cpy_fds))
            {
                if(i == sock_fd)  // 如果获取到sock_fd有数据，则表示有新的请求到来
                {
                    conn_fd = accept(sock_fd, (struct sockaddr*)NULL, NULL);
                    FD_SET(conn_fd, &fds);
                    if(conn_fd > fd_max)
                    {
                        fd_max = conn_fd;
                    }
                    printf("connected conn_fd:%d", conn_fd);
                }
                else
                {
                    int n = recv(i, buf, MAX_SIZE, MSG_DONTWAIT);
                    if(n == 0)  // 数据读取完毕, 关闭套接字
                    {
                        FD_CLR(i, &fds);  // 移除套接字
                        close(i);
                        printf("close conn :%d", i);
                    }
                    printf("connfd:%d,recv_data:%s\n", i, buf);
                    send(i, buf, strlen(buf), MSG_DONTWAIT);
                }
            }
        }
    }
    return 0;
}