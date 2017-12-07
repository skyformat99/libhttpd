/*
 * libhttpd.h -- http server library.
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

#ifndef _LIBHTTPD_H
#define _LIBHTTPD_H

#ifdef __cplusplus
extern "C" {
#endif

/* generic includes. */
#include <stdint.h>
#include <sys/types.h>

#if defined(__GNUC__) && (__GNUC__ >= 4)
# define LIBHTTPD_API __attribute__((visibility("default")))
#else
# define LIBHTTPD_API
#endif

/* libhttpd log level enums. */
enum {
	LIBHTTPD_LOG_DEBUG,
	LIBHTTPD_LOG_INFO,
	LIBHTTPD_LOG_WARN,
	LIBHTTPD_LOG_ERROR,
};

/* libhttpd structures. */
struct libhttpd_request;
struct libhttpd_response;

/* libhttpd callback. */
typedef void (* libhttpd_cb)(void *ud, struct libhttpd_request *req, struct libhttpd_response *res);

extern LIBHTTPD_API void libhttpd__loglevel(int level);

/* generic libhttpd request functions. */
extern LIBHTTPD_API const char *libhttpd_request_method(struct libhttpd_request *req);
extern LIBHTTPD_API const char *libhttpd_request_url(struct libhttpd_request *req);
extern LIBHTTPD_API const char *libhttpd_request_header(struct libhttpd_request *req, const char *header);

/* generic libhttpd response functions. */
extern LIBHTTPD_API void libhttpd_response_header(struct libhttpd_response *res, const char *header, const char *value);
extern LIBHTTPD_API void libhttpd_response_write(struct libhttpd_response *res, const char *body, int size);
extern LIBHTTPD_API void libhttpd_response_end(struct libhttpd_response *res, int status);

/* generic libhttpd functions. */
extern LIBHTTPD_API void libhttpd__serve(char *host, int port, void *ud, libhttpd_cb cb);

#ifdef __cplusplus
}
#endif

#endif /* _LIBHTTPD_H */
