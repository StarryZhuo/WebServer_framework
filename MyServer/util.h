#ifndef __MyServer_UTIL_H__
#define __MyServer_UTIL_H__

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>

namespace MyServer{

pid_t GetThreadId();
uint32_t GetFiberId(); 


}



#endif