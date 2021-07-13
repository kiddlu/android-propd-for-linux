#include <stdio.h>
#include <pthread.h>
#include "propd.h"

int main()
{
	propd_entry();

	while(1)
	{
		sleep(10);
	}

	return 0;
}
