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

#define ENABLE_EASY_URL_SETTING

#define ENABLE_BODY_WRITE_HOOK

EcoRes ecoChanOpenHook(EcoChanAddr *addr, void *arg) {
    struct sockaddr_in srvAddr;
    int opt = 1;
    int sockFd;
    int ret;

    sockFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockFd == -1) {
        return EcoRes_Err;
    }

    ret = setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    if (ret != 0) {
        close(sockFd);

        return EcoRes_Err;
    }

    srvAddr.sin_family = AF_INET;
    srvAddr.sin_port = htons(addr->port);
    memcpy(&srvAddr.sin_addr, &addr->addr, 4);

    ret = connect(sockFd, (struct sockaddr *)&srvAddr, sizeof(srvAddr));
    if (ret != 0) {
        close(sockFd);

        return EcoRes_Err;
    }

    *(int *)arg = sockFd;

    return EcoRes_Ok;
}

int ecoChanReadHook(void *buf, int len, void *arg) {
    int sockFd;
    int ret;

    sockFd = *(int *)arg;

    ret = (int)read(sockFd, buf, (size_t)len);
    if (ret <= 0) {
        if (ret == 0) {
            return EcoRes_ReachEnd;
        }

        return EcoRes_Err;
    }

    return ret;
}

int ecoChanWriteHook(const void *buf, int len, void *arg) {
    int sockFd;
    int ret;

    sockFd = *(int *)arg;

    ret = (int)write(sockFd, buf, (size_t)len);
    if (ret <= 0) {
        if (ret == 0) {
            return EcoRes_ReachEnd;
        }

        return EcoRes_Err;
    }

    return ret;
}

EcoRes ecoChanCloseHook(void *arg) {
    int sockFd;
    int ret;

    sockFd = *(int *)arg;

    ret = close(sockFd);
    if (ret != 0) {
        return EcoRes_Err;
    }

    return EcoRes_Ok;
}



EcoRes ecoReqHdrHook(size_t hdrNum, size_t hdrIdx,
                       const char *keyBuf, size_t keyLen,
                       const char *valBuf, size_t valLen,
                       EcoArg arg) {
    printf("Request Header [%zu/%zu]: `%.*s: %.*s`\r\n", hdrIdx + 1, hdrNum, (int)keyLen, keyBuf, (int)valLen, valBuf);

    return EcoRes_Ok;
}



EcoRes ecoRspHdrHook(size_t hdrNum, size_t hdrIdx,
                       const char *keyBuf, size_t keyLen,
                       const char *valBuf, size_t valLen,
                       EcoArg arg) {
    printf("Response Header [%zu/%zu]: `%.*s: %.*s`\r\n", hdrIdx + 1, hdrNum, (int)keyLen, keyBuf, (int)valLen, valBuf);

    return EcoRes_Ok;
}



#ifdef ENABLE_BODY_WRITE_HOOK

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

#endif



int main(int argc, char *argv[]) {
    int sockFd;
    EcoHttpCli *cli = NULL;
    EcoHttpReq *req = NULL;
    EcoHdrTab *tab = NULL;
    EcoHttpMeth meth;
    EcoRes res;

    req = EcoHttpReq_New();
    assert(req != NULL);

#ifdef ENABLE_EASY_URL_SETTING
    res = EcoHttpReq_SetOpt(req, EcoOpt_Url, "192.168.8.176:1180/e69974f04b05dcf07f2a.svg");
    assert(res == EcoRes_Ok);
    assert(memcmp(req->chanAddr.addr, (uint8_t [4]){192, 168, 8, 176}, 4) == 0);
    assert(req->chanAddr.port == 1180);
    assert(strcmp(req->urlBuf, "/e69974f04b05dcf07f2a.svg") == 0);
#else
    res = EcoHttpReq_SetOpt(req, EcoOpt_Addr, (EcoArg)(uint8_t [4]){192, 168, 8, 176});
    assert(res == EcoRes_Ok);
    assert(memcmp(req->chanAddr.addr, (uint8_t [4]){192, 168, 8, 176}, 4) == 0);

    res = EcoHttpReq_SetOpt(req, EcoOpt_Port, (EcoArg)1180);
    assert(res == EcoRes_Ok);
    assert(req->chanAddr.port == 1180);

    res = EcoHttpReq_SetOpt(req, EcoOpt_Path, "/e69974f04b05dcf07f2a.svg");
    assert(res == EcoRes_Ok);
    assert(strcmp(req->urlBuf, "/e69974f04b05dcf07f2a.svg") == 0);
#endif

    res = EcoHttpReq_SetOpt(req, EcoOpt_Verion, (EcoArg)EcoHttpVer_1_1);
    assert(res == EcoRes_Ok);
    assert(req->ver == EcoHttpVer_1_1);

    meth = EcoHttpMeth_Get;

    res = EcoHttpReq_SetOpt(req, EcoOpt_Method, (EcoArg)meth);
    assert(res == EcoRes_Ok);
    assert(req->meth == meth);

    /* Create HTTP header table. */
    tab = EcoHdrTab_New();
    assert(tab != NULL);

    res = EcoHdrTab_Add(tab, "Accept", "*/*");
    assert(res == EcoRes_Ok);

    /* Set HTTP header field of HTTP request. */
    res = EcoHttpReq_SetOpt(req, EcoOpt_Headers, (EcoArg)tab);
    assert(res == EcoRes_Ok);

    /* Create HTTP client. */
    cli = EcoHttpCli_New();
    assert(cli != NULL);

    /* Set channel hook functions of HTTP client. */
    res = EcoHttpCli_SetOpt(cli, EcoOpt_ChanHookArg, &sockFd);
    assert(res == EcoRes_Ok);

    res = EcoHttpCli_SetOpt(cli, EcoOpt_ChanOpenHook, ecoChanOpenHook);
    assert(res == EcoRes_Ok);

    res = EcoHttpCli_SetOpt(cli, EcoOpt_ChanCloseHook, ecoChanCloseHook);
    assert(res == EcoRes_Ok);

    res = EcoHttpCli_SetOpt(cli, EcoOpt_ChanReadHook, ecoChanReadHook);
    assert(res == EcoRes_Ok);

    res = EcoHttpCli_SetOpt(cli, EcoOpt_ChanWriteHook, ecoChanWriteHook);
    assert(res == EcoRes_Ok);

    /* Set request header getting hook function of HTTP client. */
    res = EcoHttpCli_SetOpt(cli, EcoOpt_ReqHdrHook, ecoReqHdrHook);

    /* Set response header getting hook function of HTTP client. */
    res = EcoHttpCli_SetOpt(cli, EcoOpt_RspHdrHook, ecoRspHdrHook);
    assert(res == EcoRes_Ok);

    /* Set body write hook function of HTTP client. */
#ifdef ENABLE_BODY_WRITE_HOOK
    res = EcoHttpCli_SetOpt(cli, EcoOpt_BodyWriteHook, ecoBodyWriteHook);
#else
    res = EcoHttpCli_SetOpt(cli, EcoOpt_BodyWriteHook, NULL);
#endif
    assert(res == EcoRes_Ok);

    /* Set HTTP request of HTTP client. */
    res = EcoHttpCli_SetOpt(cli, EcoOpt_Request, req);
    assert(res == EcoRes_Ok);

    /* Issue HTTP request. */
    res = EcoHttpCli_Issue(cli);
    assert(res == EcoRes_Ok);

    /* Delete HTTP client. */
    EcoHttpCli_Del(cli);

    return EXIT_SUCCESS;
}
