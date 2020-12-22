#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "properties.h"
/*
static void proplist(const char *key, const char *name, 
                     void *user __attribute__((unused)))
{
    printf("[%s]: [%s]\n", key, name);
}
*/

#define PATH_MAX 4096
static void get_property_list(void)
{
	char path[PATH_MAX];
	memset(path, 0x00, sizeof(path));
	property_list(path);

	//printf("%s\n", path);

	if(access(path, F_OK) == 0) {
	    FILE * fp;
        char * line = NULL;
        size_t len = 0;
        ssize_t read;

	    fp = fopen(path, "r");
        if (fp == NULL)
		    return;
	    while ((read = getline(&line, &len, fp)) != -1)
	    {
    	    //printf("Retrieved line of length %zu :\n", read);
    	    printf("%s", line);
        }
	    if (line)
            free(line);

		//unlink(path);
	}
}

int main(int argc, char *argv[])
{
    int n = 0;

    if (argc == 1) {
        (void)get_property_list();
    } else {
        char value[PROPERTY_VALUE_MAX];
        char *default_value;
        if(argc > 2) {
            default_value = argv[2];
        } else {
            default_value = "";
        }

        property_get(argv[1], value, default_value);
        printf("%s\n", value);
    }
    return 0;
}

