#include <stdio.h>
#include <unistd.h>
#include <time.h>

void main()
{
	time_t start, end;
	start = time(0);
	sleep(1);
	end = time(0);
	double uptime = difftime(end,start);
	printf("elapsed: %f\n", uptime);
}
