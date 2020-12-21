#include <stdio.h>

#include "properties.h"

static void proplist(const char *key, const char *name, 
                     void *user __attribute__((unused)))
{
    printf("[%s]: [%s]\n", key, name);
}

int main(int argc, char *argv[])
{
    int n = 0;

    if (argc == 1) {
        (void)property_list(proplist, NULL);
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

