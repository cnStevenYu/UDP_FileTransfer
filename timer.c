#include "timer.h"
void registerTimer(void (*action)(int signo))
{
	/*register timer*/
	struct sigaction act;
	act.sa_handler = action;
	sigemptyset(&act.sa_mask);
	act.sa_flags |= SA_RESTART;
	sigaction(SIGALRM, &act, NULL);
}

void setTimer(int interval, struct itimerval *oval) 
{
	struct itimerval val;
	val.it_value.tv_sec=interval;
	val.it_value.tv_usec=0;
	val.it_interval.tv_sec=0; 
	val.it_interval.tv_usec=0;
	setitimer(ITIMER_REAL,&val,oval);
}
