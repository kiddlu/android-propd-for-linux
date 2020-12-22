//
// Copyright 2007 The Android Open Source Project
//
// Serve properties to the simulated runtime.
//
#ifndef _PROPERTY_SERVER_H
#define _PROPERTY_SERVER_H

#include "properties.h"
#include "List.h"
/*
 * Define a thread that responds to requests from clients to get/set/list
 * system properties.
 */
class PropertyServer {
public:

    /* thread entry point */
    void* Entry(void);

    /* clear out all properties */
    void ClearProperties(void);

    /* add some default values */
    void SetDefaultProperties(void);

    /* copy a property into valueBuf; returns false if property not found */
    bool GetProperty(const char* key, char* valueBuf);

    /* set the property, replacing it if it already exists */
    bool SetProperty(const char* key, const char* value);

private:
    /* one property entry */
    typedef struct Property {
        char    key[PROPERTY_KEY_MAX];
        char    value[PROPERTY_VALUE_MAX];
    } Property;

    /* create the UNIX-domain socket we listen on */
    bool CreateSocket(const char* fileName);

    /* serve up properties */
    void ServeProperties(void);

    /* handle a client request */
    bool HandleRequest(int fd);

    /* listen here for new connections */
    int     mListenSock;

    /* list of connected fds to scan */
    List<int>      mClientList;

    /* set of known properties */
    List<Property> mPropList;
};

#endif // PROPERTY_SERVER_H
