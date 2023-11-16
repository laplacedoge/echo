#ifndef __ECHO_CONF_H__
#define __ECHO_CONF_H__

#include "echo.h"

/* Default HTTP version. */
#define ECO_CONF_DEF_HTTP_VER       EcoHttpVer_1_1

/* Default HTTP method. */
#define ECO_CONF_DEF_HTTP_METH      EcoHttpMeth_Get

/* Default HTTP IP address. */
#define ECO_CONF_DEF_IP_ADDR        ((uint8_t [4]){ 127, 0, 0, 1 })

/* Default HTTP port. */
#define ECO_CONF_DEF_HTTP_PORT      80

/* Default HTTPS port. */
#define ECO_CONF_DEF_HTTPS_PORT     443

/* Default HTTP message send chunk length.

   When the whole HTTP request message is
   too long, it will be sent in chunks, and
   this macro defines the maximum length of
   each chunk.

   The send chunk buffer will be allocated
   dynamically on the heap. */
#define ECO_CONF_DEF_SND_CHUNK_LEN  512

#endif
