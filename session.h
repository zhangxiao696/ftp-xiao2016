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
	int data_process;

	unsigned int bw_upload_rate_max;
	unsigned int bw_download_rate_max;
	long bw_transfer_start_sec;
	long bw_transfer_start_usec;

	int parent_fd;
	int child_fd;

	int is_ascii;
	long long restart_pos;
	char *rnfr_name;
	int abor_received;

	unsigned int num_clients;
	unsigned int num_this_ip;
}session_t;

void begin_session(session_t *sess);

#endif /*_SESSION_H*/