#ifndef _SERVICE_PROCESS_H
#define _SERVICE_PROCESS_H

#include "session.h"
#include "socket_operate.h"
#include "string_operate.h"
#include "ftp_cmd_rules.h"
#include "parameter.h"
#include "internal_sock.h"

void handle_child(session_t *sess);
//int list_common(void);

#endif /*_SERVICE_PROCESS_H*/