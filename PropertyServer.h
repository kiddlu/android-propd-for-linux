//
// Copyright 2007 The Android Open Source Project
//
// Serve properties to the simulated runtime.
//
#ifndef _PROPERTY_SERVER_H
#define _PROPERTY_SERVER_H

#define PROPERTY_KEY_MAX    32
#define PROPERTY_VALUE_MAX  128
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))

#define SYSTEM_PROPERTY_PIPE_NAME       "/tmp/android-sysprop"

enum {
    kSystemPropertyUnknown = 0,
    kSystemPropertyGet,
    kSystemPropertySet,
    kSystemPropertyList
};

#include <list>

using namespace std;

/* one property entry */
typedef struct Property {
    char    key[PROPERTY_KEY_MAX];
    char    value[PROPERTY_VALUE_MAX];
} Property;



#endif // PROPERTY_SERVER_H
