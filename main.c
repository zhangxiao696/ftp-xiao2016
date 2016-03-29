#include "ftp_header.h"
#include "socket_operate.h"
#include "session.h"
#include "string_operate.h"
#include "para_operate.h"
#include "service_process.h"
#include "hash.h"

session_t *p_sess;

static unsigned int s_children;
void check_limits(session_t *sess);
void handle_sigchld(int sig);

static hash_t *s_ip_count_hash;//ip跟连接数对应的hash表
static hash_t *s_pid_ip_hash;//进程号跟ip地址对应的hash表
unsigned int hash_func(unsigned int buckets, void *key);//hash函数的原型
unsigned int handle_ip_count(void *ip);
void drop_ip_count(void *ip);

int main(void)
{
	/****字符串测试代码****/
/*	char *str1 = "   	   ";
	char *str2 = "      a";
	if(str_all_space(str1))
		printf("str1 all space\n");
	else
		printf("str1 not all space\n");

	if(str_all_space(str2))
		printf("str2 all space\n");
	else
		printf("str2 not all space\n");

	//char *str3 = "abcDef";//指针指向一个字符串常量，常量不能被修改
	char str3[] = "abcDef";
	str_upper(str3);
	printf("str3 = %s\n", str3);;

	long long result = str_to_longlong("123456789012345");
	printf("result = %lld\n", result);

	int n = str_octal_to_uint("0711");
	printf("n = %d\n", n);*/

	session_t sess = 
	{
		0, -1, "", "", "",

		NULL, -1, -1, 0,

		0, 0, 0, 0,

		-1, -1,

		0, 0, NULL,

		0, 0
	};

	p_sess = &sess;

    /*测试配置文件的读取*/
	parseconf_load_file("ftp-xiao.conf");//一定要载入配置文件。否则配置文件参数无法读取

	daemon(0, 0);//如果是daemon(1， 1)的话就是一个守护进程，但是会显示printf的信息，写成daemon(0， 0)的样式，不仅可以使其为守护进程，而且也不会显示printf信息

/*	printf("pasv_enable=%d\n", tunable_pasv_enable);
	printf("port_enable=%d\n", tunable_port_enable);

	printf("listen_port=%u\n", tunable_listen_port);
	printf("max_clients=%u\n", tunable_max_clients);
	printf("max_per_ip=%u\n", tunable_max_per_ip);
	printf("accept_timeout=%u\n", tunable_accept_timeout);
	printf("connect_timeout=%u\n", tunable_connect_timeout);
	printf("idle_session_timeout=%u\n", tunable_idle_session_timeout);
	printf("data_connection_timeout=%u\n", tunable_data_connection_timeout);

	printf("local_umask=0%o\n", tunable_local_umask);

	printf("upload_max_rate=%u\n", tunable_upload_max_rate);
	printf("download_max_rate=%u\n", tunable_download_max_rate);

	if(tunable_listen_address == NULL)
		printf("listen_address = NULL\n");
	else
		printf("tunable_listen_address=%s\n", tunable_listen_address);*/
   
    /*目录列表显示的测试(权限位+连接数+用户id+组id+文件大小+时间+文件名)*/
	/*list_common();
    exit(EXIT_SUCCESS);*/

	if(getuid() != 0)
	{
		fprintf(stderr, "my_ftp:must be started as root\n");
		exit(EXIT_FAILURE);
	}

	s_ip_count_hash = hash_alloc(256, hash_func);
	s_pid_ip_hash = hash_alloc(256, hash_func);
	
	/*在父进程中安装信号处理函数，当子进程退出的时候，向父进程发送SIGCHILD信号*/
	//signal(SIGCHLD, SIG_IGN);//子进程退出会向父进程发送一个SIGCHILD信号,当前的做法是忽略该信号，避免僵尸进程的出现
	signal(SIGCHLD, handle_sigchld);//子进程退出会向父进程发送一个SIGCHILD信号，现在的做法是捕捉这个信号

	int listenfd = tcp_server(/*NULL*//*tunable_listen_address*/NULL, 5188);
	int conn;
	pid_t pid;
	struct sockaddr_in addr;

	sess.bw_upload_rate_max = tunable_upload_max_rate;
	sess.bw_download_rate_max = tunable_download_max_rate;

	while(1)
	{

		conn = accept_timeout(listenfd, /*NULL*/&addr, 0);

		if(conn == -1)
			ERR_EXIT("accept_timeout");

		unsigned int ip = addr.sin_addr.s_addr;

		++s_children;
		sess.num_clients = s_children;
		sess.num_this_ip = handle_ip_count(&ip);

		pid = fork();

		if(pid == -1)
		{
			--s_children;
			ERR_EXIT("fork");
		}			

		if(pid == 0)
		{
			close(listenfd);
			sess.ctrl_fd = conn;

			check_limits(&sess);
			signal(SIGCHLD, SIG_IGN);/*这句的作用：在begin_session中，原本子进程ftp服务器在退出的时候会继承上面的SIGCHLD信号，
			并发送给父进程nobody，这里为了让该操作不对handle_sigchld进行处理，再次安装信号使其忽略SIGCHLD信号*/

			begin_session(&sess);
		}
		else
		{
			hash_add_entry(s_pid_ip_hash, &pid, sizeof(pid),
				&ip, sizeof(unsigned int));//维护一个父进程pid跟ip对应的hash表

			close(conn);
		}
	}
	return 0;
}

void check_limits(session_t *sess)
{
	// 当一个客户端退出的时候，那么该客户端对应ip的连接数要减1，
	// 处理过程是这样的，首先是客户端退出的时候，
	// 父进程需要知道这个客户端的ip，这可以通过在s_pid_ip_hash查找得到，

	if(tunable_max_clients>0 && sess->num_clients>tunable_max_clients)
	{
		ftp_reply(sess, FTP_TOO_MANY_USERS, "There are too many connected users, please try later.");
		exit(EXIT_FAILURE);
	}

	if(tunable_max_per_ip>0 && sess->num_this_ip>tunable_max_per_ip)
	{
		ftp_reply(sess, FTP_IP_LIMIT, "There are too many connections from your internet address.");
		
		exit(EXIT_FAILURE);//如果在这里将ip数目减1，它影响的是当前进程hash表中的元素，而不是父进程的hash表
		//子进程的hash表跟父进程的hash表是不同的hash表
	}
}

void handle_sigchld(int sig)
{
	// 当一个客户端退出的时候，那么该客户端对应ip的连接数要减1，
	// 处理过程是这样的，首先是客户端退出的时候，
	// 父进程需要知道这个客户端的ip，这可以通过在s_pid_ip_hash查找得到，

	pid_t pid;
	while((pid = waitpid(-1, NULL, WNOHANG)) > 0)//waitpid()会暂时停止目前进程的执行，直到有信号来到或子进程结束,返回子进程结束状态值
	{
		--s_children;
		unsigned int *ip = hash_lookup_entry(s_pid_ip_hash, &pid, sizeof(pid));
		if(ip == NULL)
			continue;

		drop_ip_count(ip);
		hash_free_entry(s_pid_ip_hash, &pid, sizeof(pid));
	}
}

unsigned int handle_ip_count(void *ip)
{
	// 当一个客户登录的时候，要在s_ip_count_hash更新这个表中的对应表项,
	// 即该ip对应的连接数要加1，如果这个表项还不存在，要在表中添加一条记录，
	// 并且将ip对应的连接数置1。

	unsigned int count;
	unsigned int *p_count = (unsigned int *)hash_lookup_entry(s_ip_count_hash, ip, sizeof(unsigned int));
    if(p_count == NULL)	
    {
    	count = 1;
    	hash_add_entry(s_ip_count_hash, ip, sizeof(unsigned int), &count, sizeof(unsigned int));    	
    }
    else
    {
    	count = *p_count;
    	++count;
    	*p_count = count;
    }

    return count;
}

void drop_ip_count(void *ip)
{
	// 得到了ip进而我们就可以在s_ip_count_hash表中找到对应的连接数，进而进行减1操作。

	unsigned int count;
	unsigned int *p_count = (unsigned int *)hash_lookup_entry(s_ip_count_hash, ip, sizeof(unsigned int));
	if(p_count == NULL)
		return;

	count = *p_count;
	if(count <= 0)
		return;
	--count;
	*p_count = count;

	if(count == 0)
		hash_free_entry(s_ip_count_hash, ip, sizeof(unsigned int));

}

unsigned int hash_func(unsigned int buckets, void *key)
{ 
	unsigned int *number = (unsigned int*)key;

	return (*number) % buckets;//映射到表空间中，而不会超过额定桶数
}