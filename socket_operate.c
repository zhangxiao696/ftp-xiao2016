#include "socket_operate.h"


int getlocalip(char *ip)
{
	//gethostbyname()用着在里获取的ip地址不是虚拟机的ip地址 而是127.0.1.1
/*	char host[100] = {0};
	if(gethostname(host, sizeof(host)) < 0)
		return -1;
	struct hostent *hp;
	if((hp = gethostbyname(host)) == NULL)
		return -1;
	strcpy(ip, inet_ntoa(*(struct in_addr*)hp->h_addr));*/

	int inet_sock;
    struct ifreq ifr;
    inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
    strcpy(ifr.ifr_name, "eth0");
    if (ioctl(inet_sock, SIOCGIFADDR, &ifr) <  0)//SIOCGIFADDR标志代表获取接口地址
        ERR_EXIT("ioctl");
    //printf("%s\n", inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));
    strcpy(ip, inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));

	return 0;
}

//
void active_nonblock(int fd)
{
	int ret ;
	int flags = fcntl(fd, F_GETFL);
	if(flags == -1)	
		ERR_EXIT("fctnl");

    flags |= O_NONBLOCK;
	ret = fcntl(fd, F_SETFL, flags);
	if(ret == -1)
		ERR_EXIT("fcntl");
}

//
void deactive_nonblock(int fd)
{
	int ret ;
	int flags = fcntl(fd, F_GETFL);
	if(flags == -1)
		ERR_EXIT("fcntl");

	ret = fcntl(fd, F_SETFL, 0);
	if(ret == -1)
		ERR_EXIT("fcntl");
}

//
int read_timeout(int fd, unsigned int wait_seconds)
{
	int ret = 0;
	if(wait_seconds > 0)
	{
		fd_set read_fd;
		FD_ZERO(&read_fd);
		FD_SET(fd, &read_fd);

		struct timeval timeout;
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;

		while(1)
		{
			ret = select(fd+1, &read_fd, NULL, NULL, &timeout);
			switch(ret)
			{
				case -1:
				    break;
				case 0:
				    ret = -1;
				    errno = ETIMEDOUT;
				    break;
				default:
				    ret = 0;
				    break;
			}
		}		
	}
	return ret;
}

//
int write_timeout(int fd, unsigned int wait_seconds)
{
	int ret = 0;
	if(wait_seconds > 0)
	{
		fd_set wirte_fd;
		FD_ZERO(&wirte_fd);
		FD_SET(fd, &wirte_fd);

		struct timeval timeout;
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;

		while(1)
		{
			ret = select(fd+1, &wirte_fd, NULL, NULL, &timeout);
			switch(ret)
			{
				case -1:
				    break;
				case 0:
				    ret = -1;
				    errno = ETIMEDOUT;
				    break;	
				default:
				    ret = 0;
				    break;
			}
		}
	}
	return ret;
}

int accept_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds)
{
	int ret;
	socklen_t addrlen = sizeof(struct sockaddr_in);

	if (wait_seconds > 0)
	{
		fd_set accept_fdset;
		struct timeval timeout;
		FD_ZERO(&accept_fdset);
		FD_SET(fd, &accept_fdset);
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do
		{
			ret = select(fd + 1, &accept_fdset, NULL, NULL, &timeout);
		} while (ret < 0 && errno == EINTR);
		if (ret == -1)
			return -1;
		else if (ret == 0)
		{
			errno = ETIMEDOUT;
			return -1;
		}
	}

	if (addr != NULL)
		ret = accept(fd, (struct sockaddr*)addr, &addrlen);
	else
		ret = accept(fd, NULL, NULL);
/*	if (ret == -1)
		ERR_EXIT("accept");
		*/

	return ret;
}

int connect_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds)
{
	int ret;
	socklen_t addrlen = sizeof(struct sockaddr_in);

	if(wait_seconds > 0)
		active_nonblock(fd);

	ret = connect(fd, (struct sockaddr*)addr, addrlen);
	if (ret<0 && errno==EINPROGRESS)
	{
		fd_set connect_fdset;
		struct timeval timeout;
		FD_ZERO(&connect_fdset);
		FD_SET(fd, &connect_fdset);
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;

		do
		{
			ret = select(fd+1, NULL, &connect_fdset, NULL, &timeout);
		}while(ret<0 && errno==EINTR);
		if (ret == 0)
		{
			ret = -1;
			errno = ETIMEDOUT;
		}
		else if (ret < 0)
		{
			return -1;
		}
		else if (ret == 1)
		{
			int err;
			socklen_t socklen = sizeof(err);
			int sockoptret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &socklen);
			if (sockoptret == -1)
			{
				return -1;
			}
			if (err == 0)
			{
				ret = 0;
			}
			else
			{
				errno = err;
				ret = -1;
			}
		}
	}

	if (wait_seconds > 0)
	{
		deactive_nonblock(fd);
	}
	return ret;
}

//
ssize_t readn(int fd, void *buf, size_t count)
{
	size_t nleft = count;
	ssize_t nread;
	char *bufp = (char*)buf;

	while(nleft > 0)
	{
		if ((nread = read(fd, bufp, nleft)) < 0)
		{
			if (errno == EINTR)
				continue;
			return -1;
		}
		else if (nread == 0)
		{
			return count - nleft;
		}

		bufp += nread;
		nleft -= nread;
	}

	return count;
}

//
ssize_t writen(int fd, const void *buf, size_t count)
{
	size_t nleft = count;
	ssize_t nwritten;
	char *bufp = (char*)buf;

	while (nleft > 0)
	{
		if ((nwritten = write(fd, bufp, nleft)) < 0)
		{
			if (errno == EINTR)
				continue;
			return -1;
		}
		else if (nwritten == 0)
			continue;

		bufp += nwritten;
		nleft -= nwritten;
	}

	return count;
}

//仅查看数据套接字缓冲区数据
ssize_t recv_peek(int sockfd, void *buf, size_t len)
{
	while(1)
	{
		int ret = recv(sockfd, buf, len, MSG_PEEK);
		if(ret==-1 && errno==EINTR)
			continue;
		return ret;
	}
}

//按行读取数据
ssize_t readline(int sockfd, void *buf, size_t maxline)
{
	int ret;
	int nread;
	char *bufp = buf;
	int nleft = maxline;

	while(1)
	{
		ret = recv_peek(sockfd, bufp, nleft);
		if(ret < 0)
			return ret;
		else if(ret == 0)
			return ret;

		nread = ret;
		int i;
		for (i = 0; i < nread; i++)
		{
			if (bufp[i] == '\n')
			{
				ret = readn(sockfd, bufp, i+1);
				if(ret != i+1)
					exit(EXIT_FAILURE);
				return ret;
			}
		}

		if(nread > nleft)
			exit(EXIT_FAILURE);

		nleft -= nread;
		ret = readn(sockfd, bufp, nread);
		if(ret != nread)
			exit(EXIT_FAILURE);
		bufp += nread;
	}
	return -1;
}

//发送文件描述符
void send_fd(int sock_fd, int fd)
{
    int ret;
    struct msghdr msg;
    struct cmsghdr *p_cmsg;
    struct iovec vec;
    char cmsgbuf[CMSG_SPACE(sizeof(fd))];
    int *p_fds;
    char sendchar = 0;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);
    p_cmsg = CMSG_FIRSTHDR(&msg);
    p_cmsg->cmsg_level = SOL_SOCKET;
    p_cmsg->cmsg_type = SCM_RIGHTS;
    p_cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
    p_fds = (int*)CMSG_DATA(p_cmsg);
    *p_fds = fd;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;

    vec.iov_base = &sendchar;
    vec.iov_len = sizeof(sendchar);
    ret = sendmsg(sock_fd, &msg, 0);
    if(ret != 1)
    	ERR_EXIT("sendmsg");
    
}

//接收文件描述符
int recv_fd(const int sock_fd)
{
	int ret;
	struct msghdr msg;
	char recvchar;
	struct iovec vec;
	int recv_fd;
	char cmsgbuf[CMSG_SPACE(sizeof(recv_fd))];
	struct cmsghdr *p_cmsg;
	int *p_fd;
	vec.iov_base = &recvchar;
	vec.iov_len = sizeof(recvchar);
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	msg.msg_flags = 0;

	p_fd = (int*)CMSG_DATA(CMSG_FIRSTHDR(&msg));
	*p_fd = -1;
	ret = recvmsg(sock_fd, &msg, 0);
	if(ret != 1)//重大错误：之前这里是写着ret！=-1,然后由于判断错误导致recvmsg不能成功返回，使得文件描述符不能传送成功
		ERR_EXIT("recvmsg");

	p_cmsg = CMSG_FIRSTHDR(&msg);
	if(p_cmsg == NULL)
		ERR_EXIT("no passed fd");

	p_fd = (int*)CMSG_DATA(p_cmsg);
	recv_fd = *p_fd;
	if(recv_fd == -1)
		ERR_EXIT("no passed fd");

	return recv_fd;
}

int tcp_server(const char *host, unsigned short port)
{
	int listenfd;
	if ((listenfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		ERR_EXIT("tcp_server");

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	if (host != NULL)
	{
		if (inet_aton(host, &servaddr.sin_addr) == 0)
		{
			struct hostent *hp;
			hp = gethostbyname(host);
			if (hp == NULL)
				ERR_EXIT("gethostbyname");

			servaddr.sin_addr = *(struct in_addr*)hp->h_addr;
		}
	}
	else
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	servaddr.sin_port = htons(port);

	int on = 1;
	if ((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on))) < 0)
		ERR_EXIT("setsockopt");

	if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
		ERR_EXIT("bind");

	if (listen(listenfd, SOMAXCONN) < 0)
		ERR_EXIT("listen");

	return listenfd;
}

int tcp_client(unsigned short port)
{
	int data_fd;
	if ((data_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		ERR_EXIT("tcp_server");

	if(port > 0)
	{
		int on = 1;
     	if ((setsockopt(data_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on))) < 0)
		    ERR_EXIT("setsockopt");

		char ip[16] = {0};
		getlocalip(ip);
		struct sockaddr_in localaddr;
		memset(&localaddr, 0, sizeof(localaddr));
		localaddr.sin_family = AF_INET;
		localaddr.sin_port = htons(port);
		localaddr.sin_addr.s_addr = inet_addr(ip);
	    if (bind(data_fd, (struct sockaddr*)&localaddr, sizeof(localaddr)) < 0)
		    ERR_EXIT("bind");
	}

	return data_fd;
}

const char* statbuf_get_perms(struct stat *sbuf)
{
        static char perms[] = "----------";
		perms[0] = '?';

		mode_t mode = sbuf->st_mode;
		switch(mode & S_IFMT)
		{
		case S_IFREG:
			perms[0] = '-';
			break;
		case S_IFDIR:
			perms[0] = 'd';
			break;
		case S_IFLNK:
			perms[0] = 'l';
			break;
		case S_IFIFO:
			perms[0] = 'p';
			break;
		case S_IFSOCK:
			perms[0] = 's';
			break;
		case S_IFCHR:
			perms[0] = 'c';
			break;
		case S_IFBLK:
			perms[0] = 'b';
			break;
		}

		if(mode & S_IRUSR)
			perms[1] = 'r';
		if(mode & S_IWUSR)
			perms[2] = 'w';
		if(mode & S_IXUSR)
			perms[3] = 'x';
		if(mode & S_IRGRP)
			perms[4] = 'r';
		if(mode & S_IWGRP)
			perms[5] = 'w';
		if(mode & S_IXGRP)
			perms[6] = 'x';
		if(mode & S_IROTH)
			perms[7] = 'r';
		if(mode & S_IWOTH)
			perms[8] = 'w';
		if(mode & S_IXOTH)
			perms[9] = 'x';

		if(mode & S_ISUID)
			perms[3] = (perms[3] == 'x') ? 's' : 'S';
		if(mode & S_ISGID)
			perms[6] = (perms[6] == 'x') ? 's' : 'S';
		if(mode & S_ISVTX)
			perms[9] = (perms[9] == 'x') ? 't' : 'T'; 

		return perms;
}

const char* statbuf_get_date(struct stat *sbuf)
{
    const char *p_date_format = "  %b %e %H:%M ";
    struct timeval tv;
	gettimeofday(&tv, NULL);
	time_t local_time = tv.tv_sec;
	if(sbuf->st_mtime>local_time || (local_time-sbuf->st_mtime)>60*60*24*182)
		p_date_format = "  %b %e  %Y  ";

    static char datebuf[64] = {0};
	struct tm *p_tm = localtime(&local_time);
	strftime(datebuf, sizeof(datebuf), p_date_format, p_tm);

	return datebuf;
}

static int lock_internal(int fd, int lock_type)
{
	int ret;
	struct flock the_lock;
	memset(&the_lock, 0, sizeof(the_lock));
	the_lock.l_type = lock_type;
	the_lock.l_whence = SEEK_SET;
	the_lock.l_start = 0;
	the_lock.l_len = 0;
	do
	{
		ret = fcntl(fd, F_SETLKW, &the_lock);
	}while(ret<0 && errno==EINTR);

	return ret;
}

int lock_file_read(int fd)
{
	return lock_internal(fd, F_RDLCK);
}

int lock_file_write(int fd)
{
	return lock_internal(fd, F_WRLCK);
}

int unlock_file(int fd)
{
	int ret;
	struct flock the_lock;
	memset(&the_lock, 0, sizeof(the_lock));
	the_lock.l_type = F_UNLCK;
	the_lock.l_whence = SEEK_SET;
	the_lock.l_start = 0;
	the_lock.l_len = 0;

	ret = fcntl(fd, F_SETLK, &the_lock);

	return ret;	
}

static struct timeval s_curr_time;
long get_time_sec(void)
{
	if (gettimeofday(&s_curr_time, NULL) < 0)
	{
		ERR_EXIT("gettimeofday");
	}

	return s_curr_time.tv_sec;
}

long get_time_usec(void)
{
	return s_curr_time.tv_usec;
}

void nano_sleep(double seconds)
{
/*	time_t sesc = (time_t)seconds;
	double fractional = seconds - (double)sesc;

	struct timespec ts;
	ts.tv_sec = sesc;
	ts.tv_nsec = (long)(fractional * (double)1000000000);

	int ret;
	do
	{
		ret = nanosleep(&ts, &ts);
	}
	while(ret==-1 && errno==EINTR);*/

	time_t secs = (time_t)seconds;					// 整数部分
	double fractional = seconds - (double)secs;		// 小数部分

	struct timespec ts;
	ts.tv_sec = secs;
	ts.tv_nsec = (long)(fractional * (double)1000000000);
	
	int ret;
	do
	{
		ret = nanosleep(&ts, &ts);
	}
	while (ret == -1 && errno == EINTR);
}

void active_oobinline(int fd)
{
	int oob_inline = 1;
	int ret;
	ret = setsockopt(fd, SOL_SOCKET, SO_OOBINLINE, &oob_inline, sizeof(oob_inline));
	if(ret == -1)
		ERR_EXIT("setsockopt");
}

void active_sigurg(int fd)
{
	int ret;
	ret = fcntl(fd, F_SETOWN, getpid());
	if(ret == -1)
		ERR_EXIT("fcntl");
}
