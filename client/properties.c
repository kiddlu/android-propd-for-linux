/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <assert.h>

#include "properties.h"

/*
 * The Linux simulator provides a "system property server" that uses IPC
 * to set/get/list properties.  The file descriptor is shared by all
 * threads in the process, so we use a mutex to ensure that requests
 * from multiple threads don't get interleaved.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

static pthread_once_t gInitOnce = PTHREAD_ONCE_INIT;
static pthread_mutex_t gPropertyFdLock = PTHREAD_MUTEX_INITIALIZER;
static int gPropFd = -1;

/*
 * Connect to the properties server.
 *
 * Returns the socket descriptor on success.
 */
static int connectToServer(const char* fileName)
{
    int sock = -1;
    int cc;

    struct sockaddr_un addr;
    
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("UNIX domain socket create failed (errno=%d)\n", errno);
        return -1;
    }

    /* connect to socket; fails if file doesn't exist */
    strcpy(addr.sun_path, fileName);    // max 108 bytes
    addr.sun_family = AF_UNIX;
    cc = connect(sock, (struct sockaddr*) &addr, SUN_LEN(&addr));
    if (cc < 0) {
        // ENOENT means socket file doesn't exist
        // ECONNREFUSED means socket exists but nobody is listening
        //LOGW("AF_UNIX connect failed for '%s': %s\n",
        //    fileName, strerror(errno));
        close(sock);
        return -1;
    }

    return sock;
}

/*
 * Perform one-time initialization.
 */
static void init(void)
{
    assert(gPropFd == -1);

    gPropFd = connectToServer(SYSTEM_PROPERTY_PIPE_NAME);
    if (gPropFd < 0) {
        //LOGW("not connected to system property server\n");
    } else {
        //LOGV("Connected to system property server\n");
    }
}

int property_get(const char *key, char *value, const char *default_value)
{
    char sendBuf[1+PROPERTY_KEY_MAX];
    char recvBuf[1+PROPERTY_VALUE_MAX];
    int len = -1;

    //LOGV("PROPERTY GET [%s]\n", key);

    pthread_once(&gInitOnce, init);
    if (gPropFd < 0) {
        /* this mimics the behavior of the device implementation */
        if (default_value != NULL) {
            strcpy(value, default_value);
            len = strlen(value);
        }
        return len;
    }

    if (strlen(key) >= PROPERTY_KEY_MAX) return -1;

    memset(sendBuf, 0xdd, sizeof(sendBuf));    // placate valgrind

    sendBuf[0] = (char) kSystemPropertyGet;
    strcpy(sendBuf+1, key);

    pthread_mutex_lock(&gPropertyFdLock);
    if (write(gPropFd, sendBuf, sizeof(sendBuf)) != sizeof(sendBuf)) {
        pthread_mutex_unlock(&gPropertyFdLock);
        return -1;
    }
    if (read(gPropFd, recvBuf, sizeof(recvBuf)) != sizeof(recvBuf)) {
        pthread_mutex_unlock(&gPropertyFdLock);
        return -1;
    }
    pthread_mutex_unlock(&gPropertyFdLock);

    /* first byte is 0 if value not defined, 1 if found */
    if (recvBuf[0] == 0) {
        if (default_value != NULL) {
            strcpy(value, default_value);
            len = strlen(value);
        } else {
            /*
             * If the value isn't defined, hand back an empty string and
             * a zero length, rather than a failure.  This seems wrong,
             * since you can't tell the difference between "undefined" and
             * "defined but empty", but it's what the device does.
             */
            value[0] = '\0';
            len = 0;
        }
    } else if (recvBuf[0] == 1) {
        strcpy(value, recvBuf+1);
        len = strlen(value);
    } else {
        printf("Got strange response to property_get request (%d)\n",
            recvBuf[0]);
        assert(0);
        return -1;
    }
    //LOGV("PROP [found=%d def='%s'] (%d) [%s]: [%s]\n",
    //    recvBuf[0], default_value, len, key, value);

    return len;
}


int property_set(const char *key, const char *value)
{
    char sendBuf[1+PROPERTY_KEY_MAX+PROPERTY_VALUE_MAX];
    char recvBuf[1];
    int result = -1;

    //LOGV("PROPERTY SET [%s]: [%s]\n", key, value);

    pthread_once(&gInitOnce, init);
    if (gPropFd < 0)
        return -1;

    if (strlen(key) >= PROPERTY_KEY_MAX) return -1;
    if (strlen(value) >= PROPERTY_VALUE_MAX) return -1;

    memset(sendBuf, 0xdd, sizeof(sendBuf));    // placate valgrind

    sendBuf[0] = (char) kSystemPropertySet;
    strcpy(sendBuf+1, key);
    strcpy(sendBuf+1+PROPERTY_KEY_MAX, value);

    pthread_mutex_lock(&gPropertyFdLock);
    if (write(gPropFd, sendBuf, sizeof(sendBuf)) != sizeof(sendBuf)) {
        pthread_mutex_unlock(&gPropertyFdLock);
        return -1;
    }
    if (read(gPropFd, recvBuf, sizeof(recvBuf)) != sizeof(recvBuf)) {
        pthread_mutex_unlock(&gPropertyFdLock);
        return -1;
    }
    pthread_mutex_unlock(&gPropertyFdLock);

    if (recvBuf[0] != 1)
        return -1;
    return 0;
}
/*
int property_list(void (*propfn)(const char *key, const char *value, void *cookie), 
                  void *cookie)
{
    //LOGV("PROPERTY LIST\n");
    pthread_once(&gInitOnce, init);
    if (gPropFd < 0)
        return -1;

    return 0;
}
*/
int property_list(char *path)
{
    char sendBuf[1];
    char recvBuf[1];
	

    //LOGV("PROPERTY LIST\n");
    pthread_once(&gInitOnce, init);
    if (gPropFd < 0)
        return -1;

    memset(sendBuf, 0xdd, sizeof(sendBuf));    // placate valgrind

    sendBuf[0] = (char) kSystemPropertyList;

    pthread_mutex_lock(&gPropertyFdLock);
    if (write(gPropFd, sendBuf, sizeof(sendBuf)) != sizeof(sendBuf)) {
        pthread_mutex_unlock(&gPropertyFdLock);
        return -1;
    }
    if (read(gPropFd, recvBuf, sizeof(recvBuf)) != sizeof(recvBuf)) {
        pthread_mutex_unlock(&gPropertyFdLock);
        return -1;
    }
    pthread_mutex_unlock(&gPropertyFdLock);

	sprintf(path, "%s", SYSTEM_PROPERTY_LIST_NAME);

    if (recvBuf[0] != 1)
        return -1;
    return 0;
}
