/*
 * httpd.c -- sample http server.
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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>

static char host[128] = "0.0.0.0";
static int port = 8080;
static int debug = 0;
static int quiet = 0;


static void
usage(void) {
    printf("httpd is a simple http server example.\n");
    printf("httpd version %s running on libhttpd %d.%d.%d.\n\n", "0.0.0", 0, 0, 0);
    printf("Usage: httpd [-h host] [-p port] [-k keepalive]\n");
    printf("                     [-d] [--quiet]\n");
    printf("       httpd --help\n\n");
    printf(" -d : enable debug messages.\n");
    printf(" -h : httpd bind host. Defaults to localhost.\n");
    printf(" -p : httpd bind port. Defaults to 8080.\n");
    printf(" -k : keep alive in seconds for this client. Defaults to 300.\n");
    printf(" --help : display this message.\n");
    printf(" --quiet : don't print error messages.\n");
    printf("\nSee https://github.com/zhoukk/libhttpd for more information.\n\n");
    exit(0);
}

static void
config(int argc, char *argv[]) {
    int i;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--host")) {
            if (i == argc-1) {
                fprintf(stderr, "Error: -h argument given but no host specified.\n\n");
                goto e;
            } else {
                strcpy(host, argv[i+1]);
            }
            i++;
        } else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--port")) {
            if (i == argc-1) {
                fprintf(stderr, "Error: -p argument given but no port specified.\n\n");
                goto e;
            } else {
                port = atoi(argv[i+1]);
                if (port < 1 || port > 65535) {
                    fprintf(stderr, "Error: Invalid port given: %d\n", port);
                    goto e;
                }
            }
            i++;
        } else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug")) {
            debug = 1;
        } else if (!strcmp(argv[i], "--help")) {
            usage();
        } else if (!strcmp(argv[i], "--quiet")) {
            quiet = 1;
        } else {
            fprintf(stderr, "Error: Unknown option '%s'.\n", argv[i]);
            goto e;
        }
    }
    return;

e:
    fprintf(stderr, "\nUse 'httpd --help' to see usage.\n");
    exit(0);
}

static void
httpd_cb(void *ud, struct libhttpd_request *req, struct libhttpd_response *res) {
    const char *method = libhttpd_request_method(req);
    const char *url = libhttpd_request_url(req);

    libhttpd_response_header(res, "Content-Type", "text/html");
    libhttpd_response_header(res, "Server", "libhttpd");
    libhttpd_response_header(res, "Connection", "close");

    libhttpd_response_write(res, "hello libhttpd", 14);
    libhttpd_response_end(res, 200);
}

int
main(int argc, char *argv[]) {
    setlocale(LC_COLLATE, "");
    srand(time(NULL)^getpid());

    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    config(argc, argv);

    libhttpd__loglevel(LIBHTTPD_LOG_DEBUG);

    libhttpd__serve(host, port, 0, httpd_cb);
    return 0;
}

