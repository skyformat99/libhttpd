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

#include "http_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/socket.h>

#define LIBHTTPD_BACKLOG 511
#define LIBHTTPD_READ_LEN 4096
#define LIBHTTPD_LOG_LEN 4096
#define LIBHTTPD_RES_HEADER_LEN 40960
#define LIBHTTPD_MAX_ACCEPTS_PER_CALL 1000
#define LIBHTTPD_NET_IP_STR_LEN 46

#define UNUSED(V) ((void) V)

#define __DEBUG(...) __log(LIBHTTPD_LOG_DEBUG, __VA_ARGS__)
#define __INFO(...) __log(LIBHTTPD_LOG_INFO, __VA_ARGS__)
#define __WARN(...) __log(LIBHTTPD_LOG_WARN, __VA_ARGS__)
#define __ERROR(...) __log(LIBHTTPD_LOG_ERROR, __VA_ARGS__)

struct libhttpd;
struct libhttpd_connection;

struct libhttpd_buffer {
    char *data;
    int size;

    struct libhttpd_buffer *next;
};

struct libhttpd_header {
    char *field;
    char *value;

    struct libhttpd_header *next;
};

struct libhttpd_request {
    struct libhttpd_connection *conn;

    struct {
        struct libhttpd_header *head;
        struct libhttpd_header *tail;
    } header;

    char *url;
    int content_length;

    char *body;
    int body_size;
};

struct libhttpd_response {
    struct libhttpd_connection *conn;

    struct {
        struct libhttpd_header *head;
        struct libhttpd_header *tail;
    } header;

    struct {
        struct libhttpd_buffer *head;
        struct libhttpd_buffer *tail;
    } body;
    int buffer_pos;
    int body_size;
};

struct libhttpd_connection {
    int fd;

    http_parser parser;
    struct libhttpd *httpd;

    struct libhttpd_request *req;
    struct libhttpd_response *res;
};

struct libhttpd {
    aeEventLoop *el;
    char neterr[LIBHTTPD_NET_IP_STR_LEN];

    int fd;

    void *ud;
    libhttpd_cb cb;
};

static int g_log_level = LIBHTTPD_LOG_INFO;

void libhttpd__loglevel(int level) {
    g_log_level = level;
}

static void
__log(int level, const char *fmt, ...) {
    int n;
    va_list ap;
    char logbuf[LIBHTTPD_LOG_LEN];

    if (level < g_log_level) return;

    va_start(ap, fmt);
    n = vsnprintf(logbuf, LIBHTTPD_LOG_LEN, fmt, ap);
    va_end(ap);
    logbuf[n] = '\0';
    fprintf(stdout, "%s\n", logbuf);
}

static const char *
http_status_str(enum http_status s) {
    switch (s) {
#define XX(num, name, string) case HTTP_STATUS_##name: return #num " " #string;
        HTTP_STATUS_MAP(XX)
#undef XX
    default: return "<unknown>";
    }
}


static void
__httpd_request_free(struct libhttpd_request *req) {
    struct libhttpd_header *header;

    header = req->header.head;
    while (header) {
        struct libhttpd_header *next;
        if (header->field) free(header->field);
        if (header->value) free(header->value);
        next = header->next;
        free(header);
        header = next;
    }
    if (req->url) free(req->url);
    if (req->body) free(req->body);
    free(req);
    __DEBUG("__httpd_request_free");
}

static void
__httpd_response_free(struct libhttpd_response *res) {
    struct libhttpd_header *header;
    struct libhttpd_buffer *buffer;

    header = res->header.head;
    while (header) {
        struct libhttpd_header *next;
        if (header->field) free(header->field);
        if (header->value) free(header->value);
        next = header->next;
        free(header);
        header = next;
    }

    buffer = res->body.head;
    while (buffer) {
        struct libhttpd_buffer *next;
        if (buffer->data) free(buffer->data);
        next = buffer->next;
        free(buffer);
        buffer = next;
    }
    free(res);
    __DEBUG("__httpd_response_free");
}


static void
__httpd_connection_free(struct libhttpd_connection *conn) {
    aeDeleteFileEvent(conn->httpd->el, conn->fd, AE_READABLE);
    aeDeleteFileEvent(conn->httpd->el, conn->fd, AE_WRITABLE);
    close(conn->fd);
    if (conn->req) __httpd_request_free(conn->req);
    if (conn->res) __httpd_response_free(conn->res);
    free(conn);
    __DEBUG("__httpd_connection_free");
}

static void
__libhttpd_connection_write(struct libhttpd_connection *conn, int writeable);

static void
__httpd_write(aeEventLoop *el, int fd, void *privdata, int mask) {
    struct libhttpd_connection *conn;
    UNUSED(el);
    UNUSED(mask);

    conn = (struct libhttpd_connection *)privdata;

    __DEBUG("__httpd_write");
    __libhttpd_connection_write(conn, 1);
}

static void
__libhttpd_connection_write(struct libhttpd_connection *conn, int writeable) {
    struct libhttpd *httpd;
    struct libhttpd_response *res;
    struct libhttpd_buffer *buffer;
    ssize_t nwritten = 0;

    httpd = conn->httpd;
    res = conn->res;
    while (res->body.head) {
        buffer = res->body.head;
        nwritten = write(conn->fd, buffer->data+res->buffer_pos, buffer->size-res->buffer_pos);
        __DEBUG("__libhttpd_connection_write %d", nwritten);
        if (nwritten <= 0) break;
        res->buffer_pos += nwritten;
        if (res->buffer_pos == buffer->size) {
            res->buffer_pos = 0;
            res->body.head = buffer->next;
            free(buffer->data);
            free(buffer);
        }
    }
    if (nwritten == -1) {
        if (errno == EAGAIN) {
            nwritten = 0;
        } else {
            __WARN("__libhttpd_connection_write error: %s", strerror(errno));
            __httpd_connection_free(conn);
            return;
        }
    }
    if (!res->body.head) {
        res->buffer_pos = 0;
        if (writeable) {
            aeDeleteFileEvent(httpd->el, conn->fd, AE_WRITABLE);
        }
    } else {
        if (aeCreateFileEvent(httpd->el, conn->fd, AE_WRITABLE, __httpd_write, conn) == AE_ERR) {
            __WARN("aeCreateFileEvent error: %s", strerror(errno));
            __httpd_connection_free(conn);
        }
    }
}

const char *libhttpd_request_method(struct libhttpd_request *req) {
    return http_method_str(req->conn->parser.method);
}

const char *libhttpd_request_url(struct libhttpd_request *req) {
    return req->url;
}

const char *libhttpd_request_header(struct libhttpd_request *req, const char *field) {
    struct libhttpd_header *header;

    header = req->header.head;
    while (header) {
        if (0 == strcasecmp(header->field, field)) {
            return header->value;
        }
        header = header->next;
    }
    return 0;
}

const char *libhttpd_request_body(struct libhttpd_request *req, int *size) {
    *size = req->body_size;
    return req->body;
}


void libhttpd_response_header(struct libhttpd_response *res, const char *field, const char *value) {
    struct libhttpd_header *header;

    if (!field) return;
    header = res->header.head;
    while (header) {
        if (0 == strcasecmp(header->field, field)) {
            if (header->value) {
                free(header->value);
                header->value = 0;
            }
            if (value) header->value = strdup(value);
            __DEBUG("libhttpd_response_header set header %s:%s", field, value);
            return;
        }
        header = header->next;
    }

    if (!value) return;
    header = (struct libhttpd_header *)malloc(sizeof *header);
    memset(header, 0, sizeof *header);

    header->field = strdup(field);
    header->value = strdup(value);
    if (res->header.head == 0) {
        res->header.head = res->header.tail = header;
    } else {
        res->header.tail->next = header;
        res->header.tail = header;
    }
    __DEBUG("libhttpd_response_header add header %s:%s", field, value);
}

char *libhttpd_response_write(struct libhttpd_response *res, const char *data, int size) {
    struct libhttpd_buffer *buffer;

    buffer = (struct libhttpd_buffer *)malloc(sizeof *buffer);
    memset(buffer, 0, sizeof *buffer);
    buffer->data = malloc(size);
    memcpy(buffer->data, data, size);
    buffer->size = size;

    if (res->body.head == 0) {
        res->body.head = res->body.tail = buffer;
    } else {
        res->body.tail->next = buffer;
        res->body.tail = buffer;
    }

    res->body_size += size;
    __DEBUG("libhttpd_response_write size:%d", size);

    return buffer->data;
}

void libhttpd_response_end(struct libhttpd_response *res, int status) {
    struct libhttpd_connection *conn;
    struct libhttpd_header *header;
    struct libhttpd_buffer *buffer;
    int size;
    char buff[LIBHTTPD_RES_HEADER_LEN];
    int buff_size = LIBHTTPD_RES_HEADER_LEN;

    conn = res->conn;
    size = snprintf(buff, buff_size, "HTTP/1.1 %s\r\n", http_status_str(status));

    header = res->header.head;
    while (header) {
        if (header->value) {
            size += snprintf(buff+size, buff_size-size, "%s: %s\r\n", header->field, header->value);
        }
        header = header->next;
    }
    size += snprintf(buff+size, buff_size-size, "Content-Length: %d\r\n\r\n", res->body_size);

    buffer = (struct libhttpd_buffer *)malloc(sizeof *buffer);
    memset(buffer, 0, sizeof *buffer);
    buffer->data = malloc(size);
    memcpy(buffer->data, buff, size);
    buffer->size = size;

    buffer->next = res->body.head;
    res->body.head = buffer;

    __libhttpd_connection_write(conn, 0);
    __DEBUG("libhttpd_response_end %s", http_status_str(status));
}


static int
__httpd_on_message_begin(http_parser *p) {
    struct libhttpd_connection *conn;
    struct libhttpd_request *req;
    struct libhttpd_response *res;

    conn = (struct libhttpd_connection *)p->data;
    req = (struct libhttpd_request *)malloc(sizeof *req);
    memset(req, 0, sizeof *req);
    res = (struct libhttpd_response *)malloc(sizeof *res);
    memset(res, 0, sizeof *res);

    conn->req = req;
    conn->res = res;
    req->conn = conn;
    res->conn = conn;

    __DEBUG("__httpd_on_message_begin");
    return 0;
}

static int
__httpd_on_url(http_parser *p, const char *at, size_t length) {
    struct libhttpd_connection *conn;
    struct libhttpd_request *req;

    conn = (struct libhttpd_connection *)p->data;
    req = conn->req;

    req->url = strndup(at, length);

    __DEBUG("__httpd_on_url %.*s", length, at);
    return 0;
}

static int
__httpd_on_status(http_parser *p, const char *at, size_t length) {

    __DEBUG("__httpd_on_status %.*s", length, at);
    return 0;
}

static int
__httpd_on_header_field(http_parser *p, const char *at, size_t length) {
    struct libhttpd_connection *conn;
    struct libhttpd_request *req;
    struct libhttpd_header *header;

    conn = (struct libhttpd_connection *)p->data;
    req = conn->req;
    header = (struct libhttpd_header *)malloc(sizeof *header);
    memset(header, 0, sizeof *header);

    header->field = strndup(at ,length);
    if (req->header.head == 0) {
        req->header.head = req->header.tail = header;
    } else {
        req->header.tail->next = header;
        req->header.tail = header;
    }

    __DEBUG("__httpd_on_header_field %.*s", length, at);
    return 0;
}

static int
__httpd_on_header_value(http_parser *p, const char *at, size_t length) {
    struct libhttpd_connection *conn;
    struct libhttpd_request *req;
    struct libhttpd_header *header;

    conn = (struct libhttpd_connection *)p->data;
    req = conn->req;
    header = req->header.tail;

    header->value = strndup(at, length);
    if (0 == strcasecmp(header->field, "Content-Length")) {
        req->content_length = atoi(header->value);
    }

    __DEBUG("__httpd_on_header_value %.*s", length, at);
    return 0;
}

static int
__httpd_on_headers_complete(http_parser *p) {
    struct libhttpd_connection *conn;
    struct libhttpd_request *req;

    conn = (struct libhttpd_connection *)p->data;
    req = conn->req;

    if (req->content_length > 0) {
        req->body = malloc(req->content_length);
        memset(req->body, 0, req->content_length);
    }

    __DEBUG("__httpd_on_headers_complete");
    return 0;
}

static int
__httpd_on_body(http_parser *p, const char *at, size_t length) {
    struct libhttpd_connection *conn;
    struct libhttpd_request *req;

    conn = (struct libhttpd_connection *)p->data;
    req = conn->req;

    if (req->body && req->body_size < req->content_length) {
        if (req->content_length - req->body_size < length) {
            length = req->content_length - req->body_size;
            __WARN("__httpd_on_body content_length:%d body_size:%d length:%u",
                   req->content_length, req->body_size, length);
        }
        memcpy(req->body+req->body_size, at, length);
        req->body_size += length;
    }

    __DEBUG("__httpd_on_body %.*s", length, at);
    return 0;
}

static int
__httpd_on_message_complete(http_parser *p) {
    struct libhttpd_connection *conn;
    struct libhttpd_request *req;
    struct libhttpd_response *res;
    struct libhttpd *httpd;

    conn = (struct libhttpd_connection *)p->data;
    req = conn->req;
    res = conn->res;
    httpd = conn->httpd;

    httpd->cb(httpd->ud, req, res);

    __httpd_request_free(req);
    __httpd_response_free(res);
    conn->req = 0;
    conn->res = 0;

    __DEBUG("__httpd_on_message_complete");
    return 0;
}

static void
__httpd_read(aeEventLoop *el, int fd, void *privdata, int mask) {
    struct libhttpd_connection *conn;
    int nread, parsed;
    char buff[LIBHTTPD_READ_LEN];
    UNUSED(el);
    UNUSED(mask);

    static http_parser_settings settings = {
        .on_message_begin = __httpd_on_message_begin,
        .on_url = __httpd_on_url,
        .on_status = __httpd_on_status,
        .on_header_field = __httpd_on_header_field,
        .on_header_value = __httpd_on_header_value,
        .on_headers_complete = __httpd_on_headers_complete,
        .on_body = __httpd_on_body,
        .on_message_complete = __httpd_on_message_complete
    };

    conn = (struct libhttpd_connection *)privdata;

    nread = read(fd, buff, LIBHTTPD_READ_LEN);
    __DEBUG("__httpd_read read %d", nread);
    if (nread == -1) {
        if (errno == EAGAIN) {
            return;
        } else {
            __WARN("__httpd_read reading from connection: %s", strerror(errno));
            __httpd_connection_free(conn);
            return;
        }
    } else if (nread == 0) {
        __DEBUG("__httpd_read connection closed");
        __httpd_connection_free(conn);
        return;
    }
    parsed = http_parser_execute(&conn->parser, &settings, buff, nread);
    if (parsed != nread) {
        __WARN("__httpd_read parsed: %d, nread:%d", parsed, nread);
    }
}

static void
__httpd_connection(struct libhttpd *httpd, int fd, char *ip) {
    int keepalive = 300;
    struct libhttpd_connection *conn;

    conn = (struct libhttpd_connection *)malloc(sizeof *conn);
    memset(conn, 0, sizeof *conn);

    anetNonBlock(0, fd);
    anetEnableTcpNoDelay(0, fd);
    anetKeepAlive(0, fd, keepalive);

    if (aeCreateFileEvent(httpd->el, fd, AE_READABLE, __httpd_read, conn) == AE_ERR) {
        __WARN("aeCreateFileEvent AE_READABLE __httpd_read fail");
        close(fd);
        free(conn);
        return;
    }

    conn->fd = fd;
    conn->httpd = httpd;

    http_parser_init(&conn->parser, HTTP_REQUEST);
    conn->parser.data = conn;
}

static void
__httpd_accept(aeEventLoop *el, int fd, void *privdata, int mask) {
    int cport, cfd, max = LIBHTTPD_MAX_ACCEPTS_PER_CALL;
    char cip[LIBHTTPD_NET_IP_STR_LEN];
    struct libhttpd *httpd = (struct libhttpd *)privdata;
    UNUSED(el);
    UNUSED(mask);

    while(max--) {
        cfd = anetTcpAccept(httpd->neterr, fd, cip, sizeof cip, &cport);
        if (cfd == ANET_ERR) {
            if (errno != EWOULDBLOCK)
                __WARN("anetTcpAccept: %s", httpd->neterr);
            return;
        }
        __DEBUG("__httpd_accept %s:%d", cip, cport);
        __httpd_connection(httpd, cfd, cip);
    }
}

void libhttpd__serve(char *host, int port, void *ud, libhttpd_cb cb) {
    struct libhttpd httpd = {};

    httpd.ud = ud;
    httpd.cb = cb;

    if ((httpd.el = aeCreateEventLoop(128)) == 0) {
        __ERROR("aeCreateEventLoop failed");
        exit(1);
    }

    httpd.fd = anetTcpServer(httpd.neterr, port, host, LIBHTTPD_BACKLOG);
    if (httpd.fd == ANET_ERR) {
        __ERROR("anetTcpServer: %s", httpd.neterr);
        exit(1);
    }
    anetNonBlock(0, httpd.fd);
    if (aeCreateFileEvent(httpd.el, httpd.fd, AE_READABLE, __httpd_accept, &httpd) == AE_ERR) {
        __ERROR("aeCreateFileEvent AE_READABLE __httpd_accept failed");
        exit(1);
    }

    __INFO("libhttpd serve at: %s:%d", host, port);
    aeMain(httpd.el);
    aeDeleteFileEvent(httpd.el, httpd.fd, AE_READABLE);
    close(httpd.fd);
    aeDeleteEventLoop(httpd.el);
}
