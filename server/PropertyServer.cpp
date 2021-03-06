//
// Copyright 2007 The Android Open Source Project
//
// Property sever.  Mimics behavior provided on the device by init(8) and
// some code built into libc.

#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#include "properties.h"
#include "PropertyServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>



/*
 * Clear out the list.
 */
void PropertyServer::ClearProperties(void)
{
    typedef List<Property>::iterator PropIter;

    for (PropIter pi = mPropList.begin(); pi != mPropList.end(); ++pi) {
        pi = mPropList.erase(pi);
    }
}

/*
 * Set default values for several properties.
 */
void PropertyServer::SetDefaultProperties(void)
{
    static const struct {
        const char* key;
        const char* value;
    } propList[] = {
        { "ro.proccess.name", "propd" },
        { "ro.os.name", "GNU/Linux" },

    };

    for (int i = 0; i < NELEM(propList); i++)
        SetProperty(propList[i].key, propList[i].value);

}

/*
 * Get the value of a property.
 *
 * "valueBuf" must hold at least PROPERTY_VALUE_MAX bytes.
 *
 * Returns "true" if the property was found.
 */
bool PropertyServer::GetProperty(const char* key, char* valueBuf)
{
    typedef List<Property>::iterator PropIter;

    assert(key != NULL);
    assert(valueBuf != NULL);

    for (PropIter pi = mPropList.begin(); pi != mPropList.end(); ++pi) {
        Property& prop = *pi;
        if (strcmp(prop.key, key) == 0) {
            if (strlen(prop.value) >= PROPERTY_VALUE_MAX) {
                fprintf(stderr,
                    "GLITCH: properties table holds '%s' '%s' (len=%d)\n",
                    prop.key, prop.value, (int) strlen(prop.value));
                abort();
            }
            assert(strlen(prop.value) < PROPERTY_VALUE_MAX);
            strcpy(valueBuf, prop.value);
            return true;
        }
    }

    //printf("Prop: get [%s] not found\n", key);
    return false;
}

/*
 * Set the value of a property, replacing it if it already exists.
 *
 * If "value" is NULL, the property is removed.
 *
 * If the property is immutable, this returns "false" without doing
 * anything.  (Not implemented.)
 */
bool PropertyServer::SetProperty(const char* key, const char* value)
{
    typedef List<Property>::iterator PropIter;

    assert(key != NULL);
    assert(value != NULL);

    for (PropIter pi = mPropList.begin(); pi != mPropList.end(); ++pi) {
        Property& prop = *pi;
        if (strcmp(prop.key, key) == 0) {
            if (value != NULL) {
                //printf("Prop: replacing [%s]: [%s] with [%s]\n",
                //    prop.key, prop.value, value);
                strcpy(prop.value, value);
            } else {
                //printf("Prop: removing [%s]\n", prop.key);
                mPropList.erase(pi);
            }
            return true;
        }
    }

    //printf("Prop: adding [%s]: [%s]\n", key, value);
    Property tmp;
    strcpy(tmp.key, key);
    strcpy(tmp.value, value);
    mPropList.push_back(tmp);
    return true;
}

bool PropertyServer::CreateListFile(const char* fileName)
{
    typedef List<Property>::iterator PropIter;

    struct stat sb;
    bool result = false;
    FILE *fp = NULL;
    int cc;
	char lineBuf[PROPERTY_KEY_MAX + PROPERTY_VALUE_MAX + 20];

    cc = stat(fileName, &sb);
    if (cc < 0) {
        if (errno != ENOENT) {
            printf(
                "Unable to stat '%s' (errno=%d)\n", fileName, errno);
            goto bail;
        }
    } else {
        /* don't touch it if it's not a socket */
        if (!(S_ISREG(sb.st_mode))) {
            printf(
                "File '%s' exists and is not a reg file\n", fileName);
            goto bail;
        }

        /* remove the cruft */
        if (unlink(fileName) < 0) {
            printf(
                "Unable to remove '%s' (errno=%d)\n", fileName, errno);
            goto bail;
        }
    }	

	fp = fopen(fileName, "w");

    for (PropIter pi = mPropList.begin(); pi != mPropList.end(); ++pi) {
        Property& prop = *pi;

		memset(lineBuf, 0x00, sizeof(lineBuf));
		sprintf(lineBuf, "%s: [%s]\n", prop.key, prop.value);
		//printf("%s,%s\n", prop.key, prop.value);
        fputs(lineBuf, fp);
    }

    if (fp != NULL)
        fclose(fp);

	return true;

bail:
    if (fp != NULL)
        fclose(fp);
    return result;
}


/*
 * Create a UNIX domain socket, carefully removing it if it already
 * exists.
 */
bool PropertyServer::CreateSocket(const char* fileName)
{
    struct stat sb;
    bool result = false;
    int sock = -1;
    int cc;

    cc = stat(fileName, &sb);
    if (cc < 0) {
        if (errno != ENOENT) {
            printf(
                "Unable to stat '%s' (errno=%d)\n", fileName, errno);
            goto bail;
        }
    } else {
        /* don't touch it if it's not a socket */
        if (!(S_ISSOCK(sb.st_mode))) {
            printf(
                "File '%s' exists and is not a socket\n", fileName);
            goto bail;
        }

        /* remove the cruft */
        if (unlink(fileName) < 0) {
            printf(
                "Unable to remove '%s' (errno=%d)\n", fileName, errno);
            goto bail;
        }
    }

    struct sockaddr_un addr;

    sock = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        printf(
            "UNIX domain socket create failed (errno=%d)\n", errno);
        goto bail;
    }

    /* bind the socket; this creates the file on disk */
    strcpy(addr.sun_path, fileName);    // max 108 bytes
    addr.sun_family = AF_UNIX;
    cc = ::bind(sock, (struct sockaddr*) &addr, SUN_LEN(&addr));
    if (cc < 0) {
        printf("AF_UNIX bind failed for '%s' (errno=%d)\n", fileName, errno);
        goto bail;
    }

    cc = ::listen(sock, 5);
    if (cc < 0) {
        printf("AF_UNIX listen failed (errno=%d)\n", errno);
        goto bail;
    }

    mListenSock = sock;
    sock = -1;
    result = true;

bail:
    if (sock >= 0)
        close(sock);
    return result;
}

/*
 * Handle a client request.
 *
 * Returns true on success, false if the fd should be closed.
 */
bool PropertyServer::HandleRequest(int fd)
{
    char reqBuf[PROPERTY_KEY_MAX + PROPERTY_VALUE_MAX];
    char valueBuf[1 + PROPERTY_VALUE_MAX];
    ssize_t actual;

    memset(valueBuf, 'x', sizeof(valueBuf));        // placate valgrind

    /* read the command byte; this determines the message length */
    actual = read(fd, reqBuf, 1);
    if (actual <= 0)
        return false;

    if (reqBuf[0] == kSystemPropertyGet) {
        actual = read(fd, reqBuf, PROPERTY_KEY_MAX);

        if (actual != PROPERTY_KEY_MAX) {
            fprintf(stderr, "Bad read on get: %d of %d\n",
                (int) actual, PROPERTY_KEY_MAX);
            return false;
        }
        if (GetProperty(reqBuf, valueBuf+1))
            valueBuf[0] = 1;
        else
            valueBuf[0] = 0;
        //printf("GET property [%s]: (found=%d) [%s]\n",
        //    reqBuf, valueBuf[0], valueBuf+1);
        if (write(fd, valueBuf, sizeof(valueBuf)) != sizeof(valueBuf)) {
            fprintf(stderr, "Bad write on get\n");
            return false;
        }
    } else if (reqBuf[0] == kSystemPropertySet) {
        actual = read(fd, reqBuf, PROPERTY_KEY_MAX + PROPERTY_VALUE_MAX);
        if (actual != PROPERTY_KEY_MAX + PROPERTY_VALUE_MAX) {
            fprintf(stderr, "Bad read on set: %d of %d\n",
                (int) actual, PROPERTY_KEY_MAX + PROPERTY_VALUE_MAX);
            return false;
        }
        //printf("SET property '%s'\n", reqBuf);
        if (SetProperty(reqBuf, reqBuf + PROPERTY_KEY_MAX))
            valueBuf[0] = 1;
        else
            valueBuf[0] = 0;
        if (write(fd, valueBuf, 1) != 1) {
            fprintf(stderr, "Bad write on set\n");
            return false;
        }
    } else if (reqBuf[0] == kSystemPropertyList) {
        /* TODO */
        //assert(false);
		CreateListFile(SYSTEM_PROPERTY_LIST_NAME);
        valueBuf[0] = 1;
        if (write(fd, valueBuf, 1) != 1) {
            fprintf(stderr, "Bad write on set\n");
            return false;
        }			
    } else {
        fprintf(stderr, "Unexpected request %d from prop client\n", reqBuf[0]);
        return false;
    }

    return true;
}

/*
 * Serve up properties.
 */
void PropertyServer::ServeProperties(void)
{
    typedef List<int>::iterator IntIter;
    fd_set readfds;
    int maxfd;

    while (true) {
        int cc;

        FD_ZERO(&readfds);
        FD_SET(mListenSock, &readfds);
        maxfd = mListenSock;

        for (IntIter ii = mClientList.begin(); ii != mClientList.end(); ++ii) {
            int fd = (*ii);

            FD_SET(fd, &readfds);
            if (maxfd < fd)
                maxfd = fd;
        }

        cc = select(maxfd+1, &readfds, NULL, NULL, NULL);
        if (cc < 0) {
            if (errno == EINTR) {
                printf("hiccup!\n");
                continue;
            }
            return;
        }
        if (FD_ISSET(mListenSock, &readfds)) {
            struct sockaddr_un from;
            socklen_t fromlen;
            int newSock;

            fromlen = sizeof(from);
            newSock = ::accept(mListenSock, (struct sockaddr*) &from, &fromlen);
            if (newSock < 0) {
                printf(
                    "AF_UNIX accept failed (errno=%d)\n", errno);
            } else {
                //printf("new props connection on %d --> %d\n",
                //    mListenSock, newSock);

                mClientList.push_back(newSock);
            }
        }

        for (IntIter ii = mClientList.begin(); ii != mClientList.end(); ) {
            int fd = (*ii);
            bool ok = true;

            if (FD_ISSET(fd, &readfds)) {
                //printf("--- activity on %d\n", fd);

                ok = HandleRequest(fd);
            }

            if (ok) {
                ++ii;
            } else {
                //printf("--- closing %d\n", fd);
                close(fd);
                ii = mClientList.erase(ii);
            }
        }
    }
}

/*
 * Thread entry point.
 *
 * This just sits and waits for a new connection.  It hands it off to the
 * main thread and then goes back to waiting.
 *
 * There is currently no "polite" way to shut this down.
 */
void* PropertyServer::Entry(void)
{
    if (CreateSocket(SYSTEM_PROPERTY_PIPE_NAME)) {
        assert(mListenSock >= 0);
        SetDefaultProperties();

        /* loop until it's time to exit or we fail */
        ServeProperties();

        ClearProperties();

        /*
         * Close listen socket and all clients.
         */
        printf("Cleaning up socket list\n");
        typedef List<int>::iterator IntIter;
        for (IntIter ii = mClientList.begin(); ii != mClientList.end(); ++ii)
            close((*ii));
        close(mListenSock);
    }

    printf("PropertyServer thread exiting\n");
    return NULL;
}

