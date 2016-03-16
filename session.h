#ifndef _SESSION_H
#define _SESSION_H

#include "ftp_header.h"

typedef struct session
{
	uid_t uid;
	int ctrl_fd;
	char cmdline[MAX_COMMAND_LINE];
	char cmd[MAX_COMMAND];
	char arg[MAX_ARG];

	struct sockaddr_in *port_addr;
	int pasv_listen_fd;
	int data_fd;

	int parent_fd;
	int child_fd;

	int is_ascii;
}session_t;

void begin_session(session_t *sess);

#endif /*_SESSION_H*/