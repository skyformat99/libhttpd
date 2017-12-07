/*
 * libhttpd.c -- libhttpd library implementation.
 *
 * Copyright (c) zhoukk <izhoukk@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "libhttpd.h"

#include "lib/ae.h"
#include "lib/anet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/socket.h>

#define LIBHTTPD_READ_BUFF   4096
#define LIBHTTPD_LOG_BUFF    4096

#define MAX_ACCEPTS_PER_CALL 1000
#define NET_IP_STR_LEN 46

#define UNUSED(V) ((void) V)

struct libhttpd {
    pid_t pid;
    aeEventLoop *el;
    int fd;
    char neterr[ANET_ERR_LEN];

    void *ud;
    libhttpd_cb cb;

    char *host;
    int port;
};

// static int
// __write(struct libmqtt *mqtt, const char *data, int size) {
//     if (-1 == write(mqtt->fd, data, size)) {
//         return -1;
//     }
//     mqtt->t.send = mqtt->t.now;
//     return 0;
// }

// static void
// __read(aeEventLoop *el, int fd, void *privdata, int mask) {
//     struct libmqtt *mqtt;
//     int nread;
//     char buff[LIBMQTT_READ_BUFF];
//     struct mqtt_b b;

//     mqtt = (struct libmqtt *)privdata;
//     nread = read(fd, buff, sizeof(buff));
//     if (nread == -1 && errno == EAGAIN) {
//         return;
//     }
//     b.s = buff;
//     b.n = nread;
//     if (nread <= 0 || mqtt__parse(&mqtt->p, mqtt, &b)) {
//         aeDeleteFileEvent(el, fd, AE_READABLE);
//         aeDeleteTimeEvent(el, mqtt->id);
//         close(fd);
//         mqtt->fd = 0;
//         if (nread == 0 || __connect(mqtt)) {
//             aeStop(el);
//         }
//     }
// }

static void
__log(const char *fmt, ...) {
    int n;
    va_list ap;
    char logbuf[LIBHTTPD_LOG_BUFF];

    va_start(ap, fmt);
    n = vsnprintf(logbuf, LIBHTTPD_LOG_BUFF, fmt, ap);
    va_end(ap);
    logbuf[n] = '\0';
    fprintf(stdout, "%s\n", logbuf);
}

// static int
// __connect(struct libmqtt *mqtt) {
//     int fd;
//     long long id;

//     if (ANET_ERR == (fd = anetTcpConnect(0, mqtt->host, mqtt->port))) {
//         goto e1;
//     }
//     anetNonBlock(0, fd);
//     anetEnableTcpNoDelay(0, fd);
//     anetTcpKeepAlive(0, fd);
//     if (AE_ERR == aeCreateFileEvent(mqtt->el, fd, AE_READABLE, __read, mqtt)) {
//         goto e2;
//     }
//     if (mqtt->c.keep_alive > 0) {
//         if (AE_ERR == (id = aeCreateTimeEvent(mqtt->el, 1000, __update, mqtt, 0))) {
//             goto e3;
//         }
//         mqtt->id = id;
//     }
//     mqtt->fd = fd;
//     return 0;

// e3:
//     aeDeleteFileEvent(mqtt->el, fd, AE_READABLE);
// e2:
//     close(fd);
// e1:
//     return -1;
// }

const char *libhttpd__strerror(int rc) {
    static const char *__libhttpd_error_strings[] = {
        "success",
        "null pointer access",
        "memory allocation error",
        "error mqtt qos",
        "error mqtt protocol version",
        "tcp connection error",
        "tcp write error",
        "max topic/qos per subscribe or unsubscribe",
    };

    if (-rc <= 0 || -rc > sizeof(__libhttpd_error_strings)/sizeof(char *))
        return 0;
    return __libhttpd_error_strings[-rc];
}

// int libmqtt__connect(struct libmqtt *mqtt, const char *host, int port) {
//     struct mqtt_packet p;
//     struct mqtt_b b;
//     int rc;

//     if (!mqtt) {
//         return LIBMQTT_ERROR_NULL;
//     }
//     mqtt->host = strdup(host);
//     mqtt->port = port;
//     if (!mqtt->host) {
//         return LIBMQTT_ERROR_MALLOC;
//     }

//     memset(&p, 0, sizeof p);
//     p.h.type = CONNECT;
//     p.v.connect = mqtt->c;
//     p.v.connect.proto_name.s = (char *)MQTT_PROTOCOL_NAMES[mqtt->c.proto_ver];
//     p.v.connect.proto_name.n = strlen(p.v.connect.proto_name.s);

//     if (mqtt__serialize(&p, &b)) {
//         return LIBMQTT_ERROR_MALLOC;
//     }

//     if (__connect(mqtt)) {
//         mqtt_b_free(&b);
//         return LIBMQTT_ERROR_CONNECT;
//     }
//     rc = __write(mqtt, b.s, b.n);
//     mqtt_b_free(&b);
//     if (rc) {
//         return LIBMQTT_ERROR_WRITE;
//     }
//     __log(mqtt, "sending CONNECT (%s, c%d, k%d, u\'%.*s\', p\'%.*s\')", MQTT_PROTOCOL_NAMES[mqtt->c.proto_ver],
//           mqtt->c.clean_sess, mqtt->c.keep_alive, mqtt->c.username.n, mqtt->c.username.s,
//           mqtt->c.password.n, mqtt->c.password.s);
//     return LIBMQTT_SUCCESS;
// }

// int libmqtt__disconnect(struct libmqtt *mqtt) {
//     char b[] = MQTT_DISCONNECT;
//     int rc;

//     if (!mqtt) {
//         return LIBMQTT_ERROR_NULL;
//     }
//     rc = __write(mqtt, b, sizeof b);
//     shutdown(mqtt->fd, SHUT_WR);
//     if (rc) {
//         return LIBMQTT_ERROR_WRITE;
//     }
//     __log(mqtt, "sending DISCONNECT");
//     return LIBMQTT_SUCCESS;
// }

static void
__httpd_accept(aeEventLoop *el, int fd, void *privdata, int mask) {
    int cport, cfd, max = MAX_ACCEPTS_PER_CALL;
    char cip[NET_IP_STR_LEN];
    struct libhttpd *httpd = (struct libhttpd *)privdata;
    UNUSED(el);
    UNUSED(mask);

    while(max--) {
        cfd = anetTcpAccept(httpd->neterr, fd, cip, sizeof cip, &cport);
        if (cfd == ANET_ERR) {
            if (errno != EWOULDBLOCK)
                __log("Accepting client connection: %s", httpd->neterr);
            return;
        }
        __log("Accepted %s:%d", cip, cport);
        // acceptCommonHandler(cfd,0,cip);
    }
}

int libhttpd__listen(char *host, int port, void *ud, libhttpd_cb cb) {
    struct libhttpd httpd = {};
    int rc;

    httpd.pid = getpid();
    httpd.ud = ud;
    httpd.cb = cb;

    if ((httpd.el = aeCreateEventLoop(128)) == 0) {
        rc = LIBHTTPD_ERROR_EVENT_CREATE;
        goto e1;
    }

    httpd.fd = anetTcpServer(httpd.neterr, port, host, 511);
    if (httpd.fd == ANET_ERR) {
        __log("anetTcpServer: %s", httpd.neterr);
        rc = LIBHTTPD_ERROR_EVENT_CREATE;
        goto e2;
    }
    anetNonBlock(0, httpd.fd);
    if (aeCreateFileEvent(httpd.el, httpd.fd, AE_READABLE, __httpd_accept, &httpd) == AE_ERR) {
        rc = LIBHTTPD_ERROR_EVENT_CREATE;
        goto e3;
    }

    aeMain(httpd.el);
    aeDeleteEventLoop(httpd.el);

    return LIBHTTPD_SUCCESS;

e3:
    close(httpd.fd);
e2:
    aeDeleteEventLoop(httpd.el);
e1:
    return rc;
}
