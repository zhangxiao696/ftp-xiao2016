#ifndef _WORKER_PROCESS_H
#define _WORKER_PROCESS_H

#include "session.h"
#include "internal_sock.h"
#include "socket_operate.h"
#include "parameter.h"

void handle_parent(session_t *sess);

#endif /*_WORKER_PROCESS_H*/