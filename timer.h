#pragma once
#include "public.h"
#include <ctype.h>
#include <signal.h>
#include <sys/time.h>
#define INIT_INTERVAL 3
#define MAX_REPEAT 3
#define INC_INTERVAL 1

/*timer */
int curInterval;
int repeat;
struct itimerval orignalTime;

void registerTimer(void (*action)(int signo));
void setTimer(int interval, struct itimerval* oval);
