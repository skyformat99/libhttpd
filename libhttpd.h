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


#define LIBHTTPD_SUCCESS             0       /* success. */

/* define errors. */
#define LIBHTTPD_ERROR_NULL          -1      /* null pointer access. */
#define LIBHTTPD_ERROR_MALLOC        -2      /* memory allocation error. */
#define LIBHTTPD_ERROR_CONNECT       -3      /* tcp connection error. */
#define LIBHTTPD_ERROR_WRITE         -4      /* tcp write error. */
#define LIBHTTPD_ERROR_EVENT_CREATE	 -5		 /* event create error. */

/* libhttpd structure. */
struct libhttpd_request;
struct libhttpd_response;

/* libhttpd callback. */
typedef void (* libhttpd_cb)(void *ud, struct libhttpd_request *req, struct libhttpd_response *res);

/* string error message for a libhttpd return code. */
extern LIBHTTPD_API const char *libhttpd__strerror(int rc);

/* generic libhttpd listen function. */
extern LIBHTTPD_API int libhttpd__listen(char *host, int port, void *ud, libhttpd_cb cb);

#ifdef __cplusplus
}
#endif

#endif /* _LIBHTTPD_H */
