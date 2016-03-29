#ifndef _COMMON_H
#define _COMMON_H

#include <unistd.h>//unix编程所说需要的头文件
#include <fcntl.h>
#include <errno.h>//出错处理的头文件
#include <sys/types.h>
#include <sys/socket.h>//套接字变成所需要头文件
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>
#include <signal.h>

#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/sendfile.h>
 #include <sys/wait.h>

#include <linux/capability.h>
#include <sys/syscall.h>

#include <net/if.h>
#include <sys/ioctl.h>

#include <stdlib.h>//标准C编程所需要的头文件
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_COMMAND_LINE 1024
#define MAX_COMMAND 32
#define MAX_ARG 1024

#define ERR_EXIT(m) \
  do \
  {\
  	perror(m);\
  	exit(EXIT_FAILURE);\
  }\
  while(0)
#endif /*_COMMON_H*/
