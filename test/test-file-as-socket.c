#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>

#include "echo.h"

#define SAVE_FILE_PATH  "save.bin"

static const char *gFilePath = NULL;

EcoRes ecoChanOpenHook(EcoChanAddr *addr, void *arg) {
    int fileFd;

    fileFd = open(gFilePath, O_RDONLY);
    if (fileFd < 0) {
        return EcoRes_Err;
    }

    *(int *)arg = fileFd;

    return EcoRes_Ok;
}

int ecoChanReadHook(void *buf, int len, void *arg) {
    int fileFd;
    int ret;

    fileFd = *(int *)arg;

    ret = (int)read(fileFd, buf, (size_t)len);
    if (ret <= 0) {
        if (ret == 0) {
            return EcoRes_ReachEnd;
        }

        return EcoRes_Err;
    }

    return ret;
}

int ecoChanWriteHook(const void *buf, int len, void *arg) {
    printf("Transfer: `");

    for (int i = 0; i < len; i++) {
        uint8_t byte = ((uint8_t *)buf)[i];

        if (byte >= 0x20 && byte <= 0x7E) {
            putc(byte, stdout);
        } else {
            printf("\\x%02X", byte);
        }
    }

    printf("`\r\n");

    return len;
}

EcoRes ecoChanCloseHook(void *arg) {
    int fileFd;
    int ret;

    fileFd = *(int *)arg;

    ret = close(fileFd);
    if (ret != 0) {
        return EcoRes_Err;
    }

    return EcoRes_Ok;
}



int ecoBodyWriteHook(int off, const void *buf, int len, void *arg) {
    int ret;

    if (off == 0) {
        printf("Start to receive HTTP body data:\r\n");
    }

    if (len > 0) {
        printf("Slice: `%.*s`\r\n", len, (const char *)buf);
    }

    if (len == 0) {
        printf("Receive HTTP body data completed.\r\n");
    }

    return len;
}



int main(int argc, char *argv[]) {
    int fileFd;
    EcoHttpCli *cli = NULL;
    EcoHttpReq *req = NULL;
    EcoHdrTab *tab = NULL;
    EcoRes res;

    if (argc != 2) {
        dprintf(STDERR_FILENO, "Usage: %s <file-path>\r\n", argv[0]);

        return EXIT_FAILURE;
    }

    gFilePath = argv[1];

    req = EcoHttpReq_New();
    assert(req != NULL);

    res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "192.168.8.72:8080/api/3/query-status?name=hello&age=18");
    assert(res == EcoRes_Ok);
    assert(memcmp(req->chanAddr.addr, (uint8_t [4]){192, 168, 8, 72}, 4) == 0);
    assert(req->chanAddr.port == 8080);

    res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Verion, (EcoArg)EcoHttpVer_1_1);
    assert(res == EcoRes_Ok);
    assert(req->ver == EcoHttpVer_1_1);

    res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Method, (EcoArg)EcoHttpMeth_Get);
    assert(res == EcoRes_Ok);
    assert(req->meth == EcoHttpMeth_Get);

    /* Create HTTP header table. */
    tab = EcoHdrTab_New();
    assert(tab != NULL);

    res = EcoHdrTab_Add(tab, "Connection", "keep-alive");
    assert(res == EcoRes_Ok);

    res = EcoHdrTab_Add(tab, "Content-Type", "application/json");
    assert(res == EcoRes_Ok);

    res = EcoHdrTab_Add(tab, "Accept-Encoding", "gzip");
    assert(res == EcoRes_Ok);

    assert(tab->kvpNum == 3);

    /* Set HTTP header field of HTTP request. */
    res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Headers, (EcoArg)tab);
    assert(res == EcoRes_Ok);

    /* Create HTTP client. */
    cli = EcoHttpCli_New();
    assert(cli != NULL);

    /* Set channel hook functions of HTTP client. */
    res = EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_ChanHookArg, &fileFd);
    assert(res == EcoRes_Ok);

    res = EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_ChanOpenHook, ecoChanOpenHook);
    assert(res == EcoRes_Ok);

    res = EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_ChanCloseHook, ecoChanCloseHook);
    assert(res == EcoRes_Ok);

    res = EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_ChanReadHook, ecoChanReadHook);
    assert(res == EcoRes_Ok);

    res = EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_ChanWriteHook, ecoChanWriteHook);
    assert(res == EcoRes_Ok);

    /* Set body write hook function of HTTP client. */
    res = EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_BodyWriteHook, ecoBodyWriteHook);
    assert(res == EcoRes_Ok);

    /* Set HTTP request of HTTP client. */
    res = EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_Request, req);
    assert(res == EcoRes_Ok);

    /* Issue HTTP request. */
    res = EcoHttpCli_Issue(cli);
    assert(res == EcoRes_Ok);

    /* Delete HTTP client. */
    EcoHttpCli_Del(cli);

    return EXIT_SUCCESS;
}
