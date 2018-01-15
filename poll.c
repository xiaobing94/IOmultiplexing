#include <string.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <sys/select.h>  
#include <errno.h>
#include <sys/time.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>  


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
    int sock_fd = 0;
    int conn_fd = 0;
    int ret = 0;
    struct pollfd fds[MAX_SIZE] = {0};
    sock_fd = create_socket();
    fds[0].fd = sock_fd;
    fds[0].events = POLLIN;  // 普通或优先级带数据可读
    while(1)
    {
        ret = poll(fds, 4, -1);
        if(ret == -1){ // 出错    
            perror("poll()");    
        }
        else if(ret > 0)
        {
            char buf[MAX_SIZE] = {0};
            int i=1;
            if((fds[0].revents & POLLIN) == POLLIN)  // sock_fd发生变化，有新的连接进来
            {
                conn_fd = accept(sock_fd, (struct sockaddr*)NULL, NULL);
                
                for(; i<MAX_SIZE; i ++)
                {
                    if(fds[i].fd == 0)
                    {
                        fds[i].fd = conn_fd;
                        fds[i].events = POLLIN;
                        break;
                    }
                }
            }
            else{
                for(; i<MAX_SIZE; i ++)
                {
                    if(fds[i].fd == 0)
                    {
                        continue;
                    }
                    if((fds[i].revents & POLLIN) == POLLIN)
                    {
                        int n = recv(fds[i].fd, buf, MAX_SIZE, MSG_DONTWAIT);
                        if(n == 0)  // 数据读取完毕, 关闭套接字
                        {
                            close(fds[i].fd);
                            printf("close conn :%d", fds[i].fd);
                        }
                        printf("connfd:%d,recv_data:%s\n", fds[i].fd, buf);
                        send(fds[i].fd, buf, strlen(buf), MSG_DONTWAIT);
                    }
                }
            }
        }
    }
    return 0;
}