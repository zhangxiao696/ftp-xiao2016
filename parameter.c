#include "parameter.h"

/*两种写法都可以通过：1/头文件int+...，源文件extern int+...;2/头文件extern int+...，源文件int+...并初始化*/
extern int tunable_pasv_enable/* = 1*/;
extern int tunable_port_enable/* = 1*/;
extern unsigned int tunable_listen_port/* = 21*/;
extern unsigned int tunable_max_clients/* = 2000*/;
extern unsigned int tunable_max_per_ip/* = 50*/;
extern unsigned int tunable_accept_timeout/* = 60*/;
extern unsigned int tunable_connect_timeout/* = 60*/;
extern unsigned int tunable_idle_session_timeout/* = 300*/;
extern unsigned int tunable_data_connection_timeout/* = 300*/;
extern unsigned int tunable_local_umask/* = 077*/;
extern unsigned int tunable_upload_max_rate/* = 0*/;
extern unsigned int tunable_download_max_rate/* = 0*/;
extern const char *tunable_listen_address;