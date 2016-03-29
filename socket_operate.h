#ifndef _SOCKET_OPERATE_H
#define _SOCKET_OPERATE_H

#include "ftp_header.h"//将所需头文件整合在ftp_header.h当中

int tcp_server(const char *host, unsigned short port); 
int tcp_client(unsigned short port); 

int getlocalip(char *ip);//获取当地ip地址

void active_nonblock(int fd);//将文件描述符设置为非阻塞模式
void deactive_nonblock(int fd);//将文件描述符设置为阻塞模式

int read_timeout(int fd, unsigned int wait_seconds);//读超时的封装
int write_timeout(int fd, unsigned int wait_seconds);//写超时的一个封装

int accept_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds);//接受连接超时函数
int connect_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds);//连接超时函数

ssize_t readn(int fd, void *buf, size_t count);//读取固定字节
ssize_t writen(int fd, const void *buf, size_t count);//发送固定字节
ssize_t recv_peek(int sockfd, void *buf, size_t len);//仅查看数据套接字缓冲区数据
ssize_t readline(int sockfd, void *buf, size_t maxline);//按行读取数据

void send_fd(int sock_fd, int fd);//发送文件描述符
int recv_fd(const int sock_fd);//接收文件描述符

const char* statbuf_get_perms(struct stat *sbuf);
const char* statbuf_get_date(struct stat *sbuf);

int lock_file_read(int fd);
/*static int lock_internal(int fd, int lock_type);*/
int lock_file_write(int fd);
int unlock_file(int fd);

long get_time_sec(void);
long get_time_usec(void);
void nano_sleep(double seconds);

void active_oobinline(int fd);
void active_sigurg(int fd);

#endif /*_SOCKET_OPERATE_H*/