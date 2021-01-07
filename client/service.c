#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "properties.h"


int main(int argc, char *argv[])
{
    if(argc != 3) {
        fprintf(stderr,"usage: service start/stop <svc>\n");
        return 1;
    }


    if(strncmp(argv[1],"start", 5) == 0){
		    property_set("ctl.start", argv[2]);	
    } else if(strncmp(argv[1],"stop", 4) == 0){
		    property_set("ctl.stop", argv[2]);
    } else {
        fprintf(stderr,"usage: service start/stop <svc>\n");
        return 1;
    }
    return 0;
}
