//
// Copyright 2007 The Android Open Source Project
//
// Property sever.  Mimics behavior provided on the device by init(8) and
// some code built into libc.

#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#include "properties.h"
#include "propd.h"
#include "slist.h"
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

#ifndef bool
typedef unsigned char bool;
#endif

#ifndef true
#define true (1)
#endif

#ifndef false
#define false (0)
#endif

extern bool set_property(const char* key, const char* value);

int     listen_sock;
#define	FD_ARRAY_SIZE	255
int     fd_arr[FD_ARRAY_SIZE];
slist *prophead = NULL;

/*
 * Clear out the list.
 */
void clear_properties(void)
{

}

/*
 * Set default values for several properties.
 */
void set_default_properties(void)
{
    static const struct {
        const char* key;
        const char* value;
    } propList[] = {
        { "ro.proccess.name", "propd" },
        { "ro.os.name", "GNU/Linux" },
        { "ro.test.string", "HelloWorld" },
    };

    for (int i = 0; i < NELEM(propList); i++)
        set_property(propList[i].key, propList[i].value);

}

/*
 * Get the value of a property.
 *
 * "valueBuf" must hold at least PROPERTY_VALUE_MAX bytes.
 *
 * Returns "true" if the property was found.
 */
bool get_property(const char* key, char* valueBuf)
{
    assert(key != NULL);
    assert(valueBuf != NULL);

	struct node *curr =  prophead->head;
	int length = proplist_get_length(prophead);

    for (int i = 0; i < length; i++, curr=curr->next) {
        Property *prop = &(curr->prop);
        if (strcmp(prop->key, key) == 0) {
            if (strlen(prop->value) >= PROPERTY_VALUE_MAX) {
                fprintf(stderr,
                    "GLITCH: properties table holds '%s' '%s' (len=%d)\n",
                    prop->key, prop->value, (int) strlen(prop->value));
                abort();
            }
            assert(strlen(prop->value) < PROPERTY_VALUE_MAX);
            strcpy(valueBuf, prop->value);
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
bool set_property(const char* key, const char* value)
{


    assert(key != NULL);
    //assert(value != NULL);
	struct node *curr =  prophead->head;
	int length = proplist_get_length(prophead);

    for (int i = 0; i < length; i++, curr=curr->next) {
        Property *prop = &(curr->prop);
        if (strcmp(prop->key, key) == 0) {
            if (value != NULL) {
                printf("Prop: replacing [%s]: [%s] with [%s]\n",
                    prop->key, prop->value, value);
                strcpy(prop->value, value);
            } else {
                printf("Prop: removing [%s] index is [%d]\n", prop->key, i);
                proplist_delete(prophead, i+1);
            }
            return true;
        }
    }

    //printf("Prop: adding [%s]: [%s]\n", key, value);
    Property tmp;
    strcpy(tmp.key, key);
    strcpy(tmp.value, value);
	proplist_insert(prophead, length+1, &tmp);
    return true;
}

bool create_list_file(const char* fileName)
{

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
	struct node *curr =  prophead->head;
	int length = proplist_get_length(prophead);

    for (int i = 0; i < length; i++, curr=curr->next) {
        Property *prop = &(curr->prop);
		memset(lineBuf, 0x00, sizeof(lineBuf));
		sprintf(lineBuf, "%s: [%s]\n", prop->key, prop->value);
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
bool create_socket(const char* fileName)
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

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        printf(
            "UNIX domain socket create failed (errno=%d)\n", errno);
        goto bail;
    }

    /* bind the socket; this creates the file on disk */
    strcpy(addr.sun_path, fileName);    // max 108 bytes
    addr.sun_family = AF_UNIX;
    cc = bind(sock, (struct sockaddr*) &addr, SUN_LEN(&addr));
    if (cc < 0) {
        printf("AF_UNIX bind failed for '%s' (errno=%d)\n", fileName, errno);
        goto bail;
    }

    cc = listen(sock, 5);
    if (cc < 0) {
        printf("AF_UNIX listen failed (errno=%d)\n", errno);
        goto bail;
    }

    listen_sock = sock;
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
bool handle_request(int fd)
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
        if (get_property(reqBuf, valueBuf+1))
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
        if (set_property(reqBuf, reqBuf + PROPERTY_KEY_MAX))
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
		create_list_file(SYSTEM_PROPERTY_LIST_NAME);
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
void serve_properties(void)
{
    //typedef List<int>::iterator IntIter;
    fd_set readfds;
    int maxfd;

    while (true) {
        int cc;

        FD_ZERO(&readfds);
        FD_SET(listen_sock, &readfds);
        maxfd = listen_sock;

        for (int i = 0; i < FD_ARRAY_SIZE; ++i) {
			if(fd_arr[i] != 0) {
            	int fd = fd_arr[i];

            	FD_SET(fd, &readfds);
            	if (maxfd < fd)
                	maxfd = fd;
			}
        }

        cc = select(maxfd+1, &readfds, NULL, NULL, NULL);
        if (cc < 0) {
            if (errno == EINTR) {
                printf("hiccup!\n");
                continue;
            }
            return;
        }
        if (FD_ISSET(listen_sock, &readfds)) {
            struct sockaddr_un from;
            socklen_t fromlen;
            int newSock;

            fromlen = sizeof(from);
            newSock = accept(listen_sock, (struct sockaddr*) &from, &fromlen);
            if (newSock < 0 || newSock >= FD_ARRAY_SIZE) {
                printf(
                    "AF_UNIX accept failed (errno=%d)\n", errno);
            } else {
                //printf("new props connection on %d --> %d\n",
                //    mListenSock, newSock);

                fd_arr[newSock] = newSock;
            }
        }

        for (int i = 0; i < FD_ARRAY_SIZE; ++i) {
			if(fd_arr[i] != 0) {
            int fd = fd_arr[i];
            bool ok = true;

            if (FD_ISSET(fd, &readfds)) {
                //printf("--- activity on %d\n", fd);

                ok = handle_request(fd);
            }

            if (ok) {

            } else {
                //printf("--- closing %d\n", fd);
				fd_arr[fd] = 0;
                close(fd);
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
void propd_entry(void)
{
    if (create_socket(SYSTEM_PROPERTY_PIPE_NAME)) {
        assert(listen_sock >= 0);
		
		prophead = proplist_init();

        set_default_properties();

        /* loop until it's time to exit or we fail */
        serve_properties();

        clear_properties();

        /*
         * Close listen socket and all clients.
         */
        printf("Cleaning up socket list\n");
        for (int i = 0; i < FD_ARRAY_SIZE; ++i) {
			if(fd_arr[i] != 0) {
            	close(fd_arr[i]);
			}
		}
        close(listen_sock);
    }

    printf("PropertyServer thread exiting\n");
    return;
}

