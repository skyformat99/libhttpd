/*
 * libhttpd_main.c -- sample http server.
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

static char *host = 0;
static int port = 1883;
static int debug = 0;
static int quiet = 0;


static void
usage(void) {
    printf("libmqtt_pub is a simple mqtt client that will publish a message on a single topic and exit.\n");
    printf("libmqtt_pub version %s running on libmqtt %d.%d.%d.\n\n", "0.0.0", 0, 0, 0);
    printf("Usage: libmqtt_pub [-h host] [-k keepalive] [-p port] [-q qos] [-r] {-f file | -l | -n | -m message} -t topic\n");
    printf("                     [-i id] [-I id_prefix]\n");
    printf("                     [-d] [--quiet]\n");
    printf("                     [-u username [-P password]]\n");
    printf("                     [--will-topic [--will-payload payload] [--will-qos qos] [--will-retain]]\n");
    printf("       libmqtt_pub --help\n\n");
    printf(" -d : enable debug messages.\n");
    printf(" -f : send the contents of a file as the message.\n");
    printf(" -h : mqtt host to connect to. Defaults to localhost.\n");
    printf(" -i : id to use for this client. Defaults to libmqtt_pub_ appended with the process id.\n");
    printf(" -I : define the client id as id_prefix appended with the process id. Useful for when the\n");
    printf("      broker is using the clientid_prefixes option.\n");
    printf(" -k : keep alive in seconds for this client. Defaults to 60.\n");
    printf(" -l : read messages from stdin, sending a separate message for each line.\n");
    printf(" -m : message payload to send.\n");
    printf(" -n : send a null (zero length) message.\n");
    printf(" -p : network port to connect to. Defaults to 1883.\n");
    printf(" -P : provide a password (requires MQTT 3.1 broker)\n");
    printf(" -q : quality of service level to use for all messages. Defaults to 0.\n");
    printf(" -r : message should be retained.\n");
    printf(" -s : read message from stdin, sending the entire input as a message.\n");
    printf(" -t : mqtt topic to publish to.\n");
    printf(" -u : provide a username (requires MQTT 3.1 broker)\n");
    printf(" -V : specify the version of the MQTT protocol to use when connecting.\n");
    printf("      Can be mqttv31 or mqttv311. Defaults to mqttv31.\n");
    printf(" --help : display this message.\n");
    printf(" --quiet : don't print error messages.\n");
    printf(" --will-payload : payload for the client Will, which is sent by the broker in case of\n");
    printf("                  unexpected disconnection. If not given and will-topic is set, a zero\n");
    printf("                  length message will be sent.\n");
    printf(" --will-qos : QoS level for the client Will.\n");
    printf(" --will-retain : if given, make the client Will retained.\n");
    printf(" --will-topic : the topic on which to publish the client Will.\n");
    printf("\nSee https://github.com/zhoukk/libmqtt for more information.\n\n");
    exit(0);
}

static void
config(int argc, char *argv[]) {
    int i;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--port")) {
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
        } else if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--file")) {
            i++;
        } else if (!strcmp(argv[i], "--help")) {
            usage();
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--host")) {
            if (i == argc-1) {
                fprintf(stderr, "Error: -h argument given but no host specified.\n\n");
                goto e;
            } else {
                host = strdup(argv[i+1]);
            }
            i++;
        } else if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--id")) {
            
            i++;
        } else if (!strcmp(argv[i], "-I") || !strcmp(argv[i], "--id-prefix")) {
            
            i++;
        } else if (!strcmp(argv[i], "-k") || !strcmp(argv[i], "--keepalive")) {
            
            i++;
        } else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--stdin-line")) {
            
        } else if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--message")) {
            
            i++;
        } else if (!strcmp(argv[i], "-n") || !strcmp(argv[i], "--null-message")) {
            
            i++;
        } else if (!strcmp(argv[i], "-V") || !strcmp(argv[i], "--protocol-version")) {
            
            i++;
        } else if (!strcmp(argv[i], "-q") || !strcmp(argv[i], "--qos")) {
            
            i++;
        } else if (!strcmp(argv[i], "--quiet")) {
            quiet = 1;
        } else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--retain")) {
            
        } else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--stdin-file")) {
            
        } else if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--topic")) {
            
            i++;
        } else if (!strcmp(argv[i], "-u") || !strcmp(argv[i], "--username")) {

            i++;
        } else if (!strcmp(argv[i], "-P") || !strcmp(argv[i], "--pw")) {

            i++;
        } else if (!strcmp(argv[i], "--will-payload")) {

            i++;
        } else if (!strcmp(argv[i], "--will-qos")) {

            i++;
        } else if (!strcmp(argv[i], "--will-retain")) {
            
        } else if (!strcmp(argv[i], "--will-topic")) {

            i++;
        } else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--disable-clean-session")) {
            
        } else {
            fprintf(stderr, "Error: Unknown option '%s'.\n", argv[i]);
            goto e;
        }
    }
    return;

e:
    fprintf(stderr, "\nUse 'libhttpd_main --help' to see usage.\n");
    exit(0);
}

static void
httpd_cb(void *ud, struct libhttpd_request *req, struct libhttpd_response *res) {
    printf("httpd_cb\n");
}

int
main(int argc, char *argv[]) {
    int rc;

    setlocale(LC_COLLATE,"");
    srand(time(NULL)^getpid());

    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    config(argc, argv);
    if (!host) {
        host = strdup("127.0.0.1");
    }

    rc = libhttpd__listen(host, 8080, 0, httpd_cb);
    if (rc != LIBHTTPD_SUCCESS) {
        if (!quiet) fprintf(stderr, "%s\n", libhttpd__strerror(rc));
    }

    free(host);
    return 0;
}

