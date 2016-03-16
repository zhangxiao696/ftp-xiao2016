#include "ftp_header.h"
#include "socket_operate.h"
#include "session.h"
#include "string_operate.h"
#include "para_operate.h"
#include "service_process.h"


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

     /*测试配置文件的读取*/
/*	parseconf_load_file("./ftp-xiao.conf");
	printf("pasv_enable=%d\n", tunable_pasv_enable);
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
	
	signal(SIGCHLD, SIG_IGN);
	int listenfd = tcp_server(/*NULL*//*tunable_listen_address*/NULL, 5188);
	int conn;
	pid_t pid;
	//struct sockaddr_in addr;

	session_t sess = 
	{
		0, -1, "", "", "",

		NULL, -1, -1,

		-1, -1,

		0
	};

	while(1)
	{

		conn = accept_timeout(listenfd, NULL, 0);

		if(conn == -1)
			ERR_EXIT("accept_timeout");

		pid = fork();

		if(pid == -1)
			ERR_EXIT("fork");

		if(pid == 0)
		{
			close(listenfd);
			sess.ctrl_fd = conn;
			begin_session(&sess);
		}
		else
		{
			close(conn);
		}
	}
	return 0;
}