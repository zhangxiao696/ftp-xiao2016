#include "worker_process.h"

static void priv_sock_get_data_sock(session_t *sess);
static void priv_sock_pasv_active(session_t *sess);
static void priv_sock_pasv_listen(session_t *sess);
static void priv_sock_pasv_accept(session_t *sess);

void handle_parent(session_t *sess)
{
	struct passwd *pw = getpwnam("nobody");
    if(pw == NULL)
	   return;

    if(setegid(pw->pw_gid) < 0)
	   ERR_EXIT("setegid");
    if(seteuid(pw->pw_uid) < 0)
	   ERR_EXIT("seteuid");

	char cmd;
	while(1)
	{
		//read(sess->parent_fd, &cmd, 1);
		cmd = priv_sock_get_cmd(sess->parent_fd);
		switch(cmd)
		{
		case PRIV_SOCK_GET_DATA_SOCK:
		     priv_sock_get_data_sock(sess);
		     break;
		case PRIV_SOCK_PASV_ACTIVE:
		     priv_sock_pasv_active(sess);
		     break;
		case PRIV_SOCK_PASV_LISTEN:
		     priv_sock_pasv_listen(sess);
		     break;
		case PRIV_SOCK_PASV_ACCEPT:
		     priv_sock_pasv_accept(sess);
		     break;
		}
	}
}

static void priv_sock_get_data_sock(session_t *sess)
{
	unsigned short port = (unsigned short)priv_sock_get_int(sess->parent_fd);
	char ip[16] = {0};
	priv_sock_recv_buf(sess->parent_fd, ip ,sizeof(ip));

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);

	int fd = tcp_client(20);
	if(fd == -1)
	{
		priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
		return;
	}
	if(connect_timeout(fd, &addr, tunable_connect_timeout) < 0)
	{
		close(fd); 
		priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
		return;
	}

	priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_OK);
	priv_sock_send_fd(sess->parent_fd, fd);
	close(fd);
}

static void priv_sock_pasv_active(session_t *sess)
{

}

static void priv_sock_pasv_listen(session_t *sess)
{

}

static void priv_sock_pasv_accept(session_t *sess)
{

}