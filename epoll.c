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
#include <sys/epoll.h>  


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
    struct epoll_event event;   // 告诉内核要监听什么事件    
    struct epoll_event wait_event; //内核监听完的结果  
    int i = 0;
    int event_count = 0;

    int fds[MAX_SIZE]; 
    int max_index = 0;
    memset(fds,-1, sizeof(fds));
    sock_fd = create_socket();
    fds[0] = sock_fd;
    int epfd = epoll_create(10);
    if(epfd == -1)
    {
        perror ("epoll_create");    
        return -1;    
    }
    event.data.fd = sock_fd;
    event.events = EPOLLIN;

    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sock_fd, &event);
    if(ret == -1)
    {
        perror("epoll_ctl");
        return -1;
    }

    while(1)
    {
        ret = epoll_wait(epfd, &wait_event, max_index+1, -1);
        if(sock_fd == wait_event.data.fd && (EPOLLIN == wait_event.events & EPOLLIN))
        {
            conn_fd = accept(sock_fd, (struct sockaddr*)NULL, NULL);
            // printf("socket_:%d,%d\n", sock_fd, errno);
            if(conn_fd > 0)
            {
                for(i=1; i<MAX_SIZE; i++)
                {
                    if(fds[i] < 0)
                    {
                        fds[i] = conn_fd;
                        event.data.fd = conn_fd;
                        event.events = EPOLLIN;
                        ret = epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &event); // 将conn_fd加入监听事件
                        if(ret == -1){    
                            printf("conn_fd:%d\n", conn_fd);
                            perror("epoll_ctl");    
                            return -1;    
                        }   
                        
                        break;
                    }
                }
                if(i > max_index)
                {
                    max_index = i;
                }
                if(--ret <= 0)  // 没有就绪的描述符，继续监听
                {
                    continue;
                }
            }
        }
        for(i=1; i<=max_index; i++)
        {
            
            char buf[MAX_SIZE] = {0};
            if(fds[i] < 0)  
                continue; 
            if(( fds[i] == wait_event.data.fd )     
                && ( EPOLLIN == wait_event.events & (EPOLLIN|EPOLLERR) ))
            {
                int n = recv(wait_event.data.fd, buf, MAX_SIZE, MSG_DONTWAIT);
                if(n == 0)  // 数据读取完毕, 关闭套接字
                {
                    close(wait_event.data.fd);
                    printf("close conn :%d", wait_event.data.fd );
                }
                printf("connfd:%d,recv_data:%s\n", wait_event.data.fd , buf);
                send(wait_event.data.fd , buf, strlen(buf), MSG_DONTWAIT);
                if(--ret <= 0)  // 没有就绪的描述符，不用继续循环了，继续监听描述符
                {
                    break;
                }
            }
        }

    }

    return 0;
}