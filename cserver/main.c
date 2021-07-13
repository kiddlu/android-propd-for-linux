#include <stdio.h>
#include <pthread.h>
#include "propd.h"

int main()
{
	pthread_t thread_id;
	propd_entry(&thread_id);
	printf("propd_thread is %d\n", thread_id);

	while(1)
	{
		sleep(10);
	}

	return 0;
}