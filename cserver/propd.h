#ifndef _PROPD_H
#define _PROPD_H

#include "properties.h"

/* one property entry */
typedef struct property {
    char    key[PROPERTY_KEY_MAX];
    char    value[PROPERTY_VALUE_MAX];
}Property;


void propd_entry(void);

#endif//_PROPD_H