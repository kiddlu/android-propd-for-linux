#ifndef _PROPD_H
#define _PROPD_H
#define _GNU_SOURCE
#include <pthread.h>
#include "properties.h"
#include "list.h"

/* one property entry */
typedef struct property {
    char    key[PROPERTY_KEY_MAX];
    char    value[PROPERTY_VALUE_MAX];

	struct listnode plist;
}Property;


void propd_entry(void);

#endif//_PROPD_H