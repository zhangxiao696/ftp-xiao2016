#include "service_process.h"

void ftp_reply(session_t *sess, int cmd_rules, const char *str);
void ftp_lreply(session_t *sess, int cmd_rules, const char *str);

int list_common(session_t *sess);

int port_active(session_t *sess);
int pasv_active(session_t *sess);

int get_transfer_fd(session_t *sess);
int get_port_fd(session_t *sess);

static void do_user(session_t *sess);
static void do_pass(session_t *sess);

static void do_cwd(session_t *sess);
static void do_cdup(session_t *sess);
static void do_quit(session_t *sess);
static void do_port(session_t *sess);
static void do_pasv(session_t *sess);
static void do_type(session_t *sess);
/*static void do_stru(session_t *sess);
static void do_mode(session_t *sess);*/
static void do_retr(session_t *sess);
static void do_stor(session_t *sess);
static void do_appe(session_t *sess);
static void do_list(session_t *sess);
static void do_nlst(session_t *sess);
static void do_rest(session_t *sess);
static void do_abor(session_t *sess);
static void do_pwd(session_t *sess);
static void do_mkd(session_t *sess);
static void do_rmd(session_t *sess);
static void do_dele(session_t *sess);
static void do_rnfr(session_t *sess);
static void do_rnto(session_t *sess);
static void do_site(session_t *sess);
static void do_syst(session_t *sess);
static void do_feat(session_t *sess);
static void do_size(session_t *sess);
static void do_stat(session_t *sess);
static void do_noop(session_t *sess);
static void do_help(session_t *sess);

typedef struct ftpcmd
{
	const char *cmd;
	void (*cmd_handler)(session_t *sess);
}ftpcmd_t;

static ftpcmd_t ctrl_cmds[] = 
{
	/* 访问控制命令 */
	{"USER",	do_user	},
	{"PASS",	do_pass	},
	{"CWD",		do_cwd	},
	{"XCWD",	do_cwd	},
	{"CDUP",	do_cdup	},
	{"XCUP",	do_cdup	},
	{"QUIT",	do_quit	},
	{"ACCT",	NULL	},
	{"SMNT",	NULL	},
	{"REIN",	NULL	},
	/* 传输参数命令 */
	{"PORT",	do_port	},
	{"PASV",	do_pasv	},
	{"TYPE",	do_type	},
	{"STRU",	/*do_stru*/NULL	},
	{"MODE",	/*do_mode*/NULL	},

	/* 服务命令 */
	{"RETR",	do_retr	},
	{"STOR",	do_stor	},
	{"APPE",	do_appe	},
	{"LIST",	do_list	},
	{"NLST",	do_nlst	},
	{"REST",	do_rest	},
	{"ABOR",	do_abor	},
	{"\377\364\377\362ABOR", do_abor},
	{"PWD",		do_pwd	},
	{"XPWD",	do_pwd	},
	{"MKD",		do_mkd	},
	{"XMKD",	do_mkd	},
	{"RMD",		do_rmd	},
	{"XRMD",	do_rmd	},
	{"DELE",	do_dele	},
	{"RNFR",	do_rnfr	},
	{"RNTO",	do_rnto	},
	{"SITE",	do_site	},
	{"SYST",	do_syst	},
	{"FEAT",	do_feat },
	{"SIZE",	do_size	},
	{"STAT",	do_stat	},
	{"NOOP",	do_noop	},
	{"HELP",	do_help	},
	{"STOU",	NULL	},
	{"ALLO",	NULL	}
};

void handle_child(session_t *sess)
{
	writen(sess->ctrl_fd, "220 (ftp_xiao 2016)\r\n", strlen("220 (ftp_xiao 2016)\r\n"));//欢迎语，并且必须加上\r\n的换行符，否则不能显示出来欢迎
	int ret;
	while(1)
	{
		memset(sess->cmdline, 0, sizeof(sess->cmdline));
		memset(sess->cmd, 0, sizeof(sess->cmd));
		memset(sess->arg, 0, sizeof(sess->arg));

		ret = readline(sess->ctrl_fd, sess->cmdline, MAX_COMMAND_LINE);
		if(ret == -1)
			ERR_EXIT("readline");
		else if(ret == 0)
			exit(EXIT_SUCCESS);

		printf("cmdline = [%s]\n", sess->cmdline);
		
        str_trim_crlf(sess->cmdline);
		printf("cmdline = [%s]\n", sess->cmdline);

		str_split(sess->cmdline, sess->cmd, sess->arg, ' ');
		printf("cmd=[%s], arg=[%s]\n", sess->cmd, sess->arg);

		str_upper(sess->cmd);

/*		if(strcmp("USER", sess->cmd) == 0)
			do_user(sess);
		else if(strcmp("PASS", sess->cmd) == 0)
			do_pass(sess);*/

		int size_cmd = sizeof(ctrl_cmds)/sizeof(ctrl_cmds[0]);
	    int i;
	    for(i = 0; i<size_cmd; i++)
	    {
	    	if(strcmp(ctrl_cmds[i].cmd, sess->cmd) == 0)
	    	{
	    		if(ctrl_cmds[i].cmd_handler != NULL)
	    			ctrl_cmds[i].cmd_handler(sess);
	    		else
	    			ftp_reply(sess, FTP_COMMANDNOTIMPL, "Unimplement command.");
	    		break;
	    	}
	    }

	    if(i == size_cmd)
	    	ftp_reply(sess, FTP_BADCMD, "Unimplement command.");

		
	}
}

void ftp_reply(session_t *sess, int cmd_rules, const char *str)
{
	char buf[1024] = {0};
	sprintf(buf, "%d %s\r\n", cmd_rules, str);
	writen(sess->ctrl_fd, buf, strlen(buf));
}

void ftp_lreply(session_t *sess, int cmd_rules, const char *str)
{
	char buf[1024] = {0};
	sprintf(buf, "%d-%s\r\n", cmd_rules, str);
	writen(sess->ctrl_fd, buf, strlen(buf));
}

int list_common(session_t *sess)
{
	DIR *dir = opendir(".");
	if(dir == NULL)
		return 0;

	struct dirent *dt;
	struct stat sbuf;
	while((dt = readdir(dir)) != NULL)
	{
		if(lstat(dt->d_name, &sbuf) < 0)
			continue;

		if(dt->d_name[0] == '.')
			continue;

		char perms[] = "----------";
		perms[0] = '?';

		mode_t mode = sbuf.st_mode;
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

		char buf[1024]= {0};
		int off = 0;
		off += sprintf(buf, "%s", perms);
		off += sprintf(buf + off, "%3d %-8d %-8d", sbuf.st_nlink, sbuf.st_uid, sbuf.st_gid);
		off += sprintf(buf + off, "%8lu", sbuf.st_size); 

        const char *p_date_format = "  %b %e %H:%M ";
        struct timeval tv;
		gettimeofday(&tv, NULL);
		time_t local_time = tv.tv_sec;
		if(sbuf.st_mtime>local_time || (local_time-sbuf.st_mtime)>60*60*24*182)
			p_date_format = "  %b %e  %Y  ";

        char datebuf[64] = {0};
		struct tm *p_tm = localtime(&local_time);
		strftime(datebuf, sizeof(datebuf), p_date_format, p_tm);
		off += sprintf(buf + off, "%s", datebuf);

		if(S_ISLNK(sbuf.st_mode))
		{
			char tmp[1024] = {0};
			readlink(dt->d_name, tmp, sizeof(tmp));
			off += sprintf(buf + off, "%s -> %s\r\n", dt->d_name, tmp);
		}
		else
			sprintf(buf + off, "%s\r\n", dt->d_name);

		/*printf("%s", buf);*/
		writen(sess->data_fd, buf, strlen(buf));

	}
	closedir(dir);
	return 1;
}

static void do_user(session_t *sess)
{
	//root-zhang
	struct passwd *pw = getpwnam(sess->arg);
	if(pw == NULL)
	{
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
		return;
	}

	sess->uid = pw->pw_uid;
	ftp_reply(sess, FTP_GIVEPWORD, "Please specify the password.");
}

static void do_pass(session_t *sess)
{
	//PASS 199074
	struct passwd *pw = getpwuid(sess->uid);//get the shadow file
	if(pw == NULL)
	{
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
		return;
	}

	printf("name=[%s]\n", pw->pw_name);
	struct spwd *sp = getspnam(pw->pw_name);
	if(sp ==NULL)
	{
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
		return;
	}

	char *encrypted_pass = crypt(sess->arg, sp->sp_pwdp);
	if(strcmp(encrypted_pass, sp->sp_pwdp) != 0)
	{
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
		return;
	}

	setegid(pw->pw_gid);
	seteuid(pw->pw_uid);
	chdir(pw->pw_dir);

	ftp_reply(sess, FTP_LOGINOK, "Login seccessful.");
}

static void do_cwd(session_t *sess)
{

}

static void do_cdup(session_t *sess)
{

}

static void do_quit(session_t *sess)
{

}

static void do_port(session_t *sess)
{
	unsigned int v[6];
	sscanf(sess->arg, "%u,%u,%u,%u,%u,%u", &v[2], &v[3], &v[4], &v[5], &v[0], &v[1]);
	sess->port_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	memset(sess->port_addr, 0, sizeof(struct sockaddr_in));
	sess->port_addr->sin_family = AF_INET;
	unsigned char *p = (unsigned char *)&sess->port_addr->sin_port;
	p[0] = v[0];
	p[1] = v[1];

	p = (unsigned char *)&sess->port_addr->sin_addr;
	p[0] = v[2];
	p[1] = v[3];
	p[2] = v[4];
	p[3] = v[5];

	ftp_reply(sess, FTP_PORTOK, "PORT command successful.Consider using PASV.");
}

static void do_pasv(session_t *sess)
{
	char ip[16] = {0};
	getlocalip(ip);
	sess->pasv_listen_fd = tcp_server(ip, 0);
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	if(getsockname(sess->pasv_listen_fd, (struct sockaddr *)&addr, &addrlen) < 0)
		ERR_EXIT("getsockname");

	unsigned short port = ntohs(addr.sin_port);
	unsigned int v[4];
	sscanf(ip, "%u.%u.%u.%u", &v[0], &v[1], &v[2], &v[3]);
	char text[1024] = {0};
	sprintf(text, "Entering Passive Mode (%u,%u,%u,%u,%u,%u).", v[0], v[1], v[2], v[3], port>>8, port&0xFF);

	ftp_reply(sess, FTP_PASVOK, text);
}

static void do_type(session_t *sess)
{
	if(strcmp(sess->arg, "A") == 0)
	{
		sess->is_ascii = 1;
		ftp_reply(sess, FTP_TYPEOK, "Swtiching to ASCII mode.");
	}
	else if(strcmp(sess->arg, "I") == 0)
	{
		sess->is_ascii = 0;
		ftp_reply(sess, FTP_TYPEOK, "Swtiching to ASCII mode.");
	}
	else
		ftp_reply(sess, FTP_BADCMD, "Unrecognised TYPE command.");
}

/*static void do_stru(session_t *sess)
{

}

static void do_mode(session_t *sess)
{

}*/

static void do_retr(session_t *sess)
{

}

static void do_stor(session_t *sess)
{

}

static void do_appe(session_t *sess)
{

}

int pasv_active(session_t *sess)
{
	if(sess->pasv_listen_fd != -1)
	{
		if(port_active(sess))
		{
			fprintf(stderr, "botn port an pasv are active\n");
			exit(EXIT_FAILURE);
		}
		return 1;
	}
	return 0;
}

int port_active(session_t *sess)
{
	if(sess->port_addr)
	{
		if(pasv_active(sess))
		{
			fprintf(stderr, "botn port an pasv are active\n");
			exit(EXIT_FAILURE);
		}
		return 1;
	}

	return 0;
}

int get_port_fd(session_t *sess)
{
	priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_GET_DATA_SOCK);
	unsigned short port = ntohs(sess->port_addr->sin_port);
	char *ip = inet_ntoa(sess->port_addr->sin_addr);
	priv_sock_send_int(sess->child_fd, (int)port);
	priv_sock_send_buf(sess->child_fd, ip ,strlen(ip));

	char res = priv_sock_get_result(sess->child_fd);
	if(res == PRIV_SOCK_RESULT_BAD)
		return 0;
	else if(res == PRIV_SOCK_RESULT_OK)
		sess->data_fd = priv_sock_recv_fd(sess->child_fd);

	return 1;
}

int get_transfer_fd(session_t *sess)
{
	if(!port_active(sess) && !pasv_active(sess))
	{
		return 0;
	}

    int ret = 1;
	if(port_active(sess))
	{
/*		socket  
		bind 
		connect*/
		/*int fd = tcp_client(0);
		if(connect_timeout(fd, sess->port_addr, tunable_connect_timeout) < 0)
		{
			close(fd);
			return 0;
		}
		sess->data_fd = fd;*/

		if(get_port_fd(sess) == 0)
			ret = 0;
	}

	if(pasv_active(sess))
	{
		int fd = accept_timeout(sess->pasv_listen_fd, NULL, tunable_accept_timeout);
		close(sess->pasv_listen_fd);
		if(fd == -1)
			return 0;
		sess->data_fd = fd;
	}

	if(sess->port_addr)
	{
		free(sess->port_addr);
		sess->port_addr = NULL;
	}

	return ret;
}

static void do_list(session_t *sess)
{
	if(get_transfer_fd(sess) == 0)
	{
		ftp_reply(sess, FTP_BADSENDCONN, "Use PORT or PASV first.");
		return;		
	}

	ftp_reply(sess, FTP_DATACONN, "Here comes the directory listing.");

	list_common(sess);

	close(sess->data_fd);
	sess->data_fd = -1;

	ftp_reply(sess, FTP_TRANSFEROK, "Directory send ok.");
}

static void do_nlst(session_t *sess)
{

}

static void do_rest(session_t *sess)
{

}

static void do_abor(session_t *sess)
{

}

static void do_pwd(session_t *sess)
{
	char text[1024] = {0};
	char dir[1024] = {0};
	getcwd(dir, 1024);
	sprintf(text, "\"%s\"", dir);

	ftp_reply(sess, FTP_PWDOK, text);
}

static void do_mkd(session_t *sess)
{

}

static void do_rmd(session_t *sess)
{

}

static void do_dele(session_t *sess)
{

}

static void do_rnfr(session_t *sess)
{

}

static void do_rnto(session_t *sess)
{

}

static void do_site(session_t *sess)
{

}

static void do_syst(session_t *sess)
{
	ftp_reply(sess, FTP_SYSTOK, "UNIX Type: L8");
}

static void do_feat(session_t *sess)
{
	ftp_lreply(sess, FTP_FEAT, "Features:");
	writen(sess->ctrl_fd, "EPRT\r\n", strlen("EPRT\r\n"));
	writen(sess->ctrl_fd, " EPSV\r\n", strlen(" EPSV\r\n"));
	writen(sess->ctrl_fd, " MDTM\r\n", strlen(" MDTM\r\n"));
	writen(sess->ctrl_fd, " PASV\r\n", strlen(" PASV\r\n"));
	writen(sess->ctrl_fd, " REST STREAM\r\n", strlen(" REST STREAM\r\n"));
	writen(sess->ctrl_fd, " SIZE\r\n", strlen(" SIZE\r\n"));
	writen(sess->ctrl_fd, " TVFS\r\n", strlen(" TVFS\r\n"));
	writen(sess->ctrl_fd, " UTF8\r\n", strlen(" UTF8\r\n"));
	ftp_reply(sess, FTP_FEAT, "End");
}

static void do_size(session_t *sess)
{

}

static void do_stat(session_t *sess)
{

}

static void do_noop(session_t *sess)
{

}

static void do_help(session_t *sess)
{

}
