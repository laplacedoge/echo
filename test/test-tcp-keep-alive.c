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

#define EcoPtrChk(cond) assert((cond) != NULL)

#define EcoResChk(cond) assert((cond) == EcoRes_Ok)

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
    int sockFd;
    const char *addrStr;
    EcoHttpCli *cli = NULL;
    EcoHttpReq *req = NULL;
    EcoHdrTab *tab = NULL;
    EcoRes res;

    if (argc < 1 + 2) {
        printf("Usage: %s <ip:port> <path> [ <path> ... ]\r\n", argv[0]);

        return EXIT_FAILURE;
    }

    addrStr = argv[1];

    /* Create HTTP request header table. */
    EcoPtrChk(tab = EcoHdrTab_New());

    /* Create HTTP request. */
    EcoPtrChk(req = EcoHttpReq_New());

    /* Set headers for HTTP request. */
    EcoResChk(res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Headers, tab));

    /* Create HTTP client. */
    EcoPtrChk(cli = EcoHttpCli_New());

    /* Set request for HTTP client. */
    EcoResChk(EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_Request, req));



    /* Set hook functions for HTTP client. */
    EcoResChk(EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_ChanHookArg, &sockFd));
    EcoResChk(EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_ChanOpenHook, ecoChanOpenHook));
    EcoResChk(EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_ChanCloseHook, ecoChanCloseHook));
    EcoResChk(EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_ChanReadHook, ecoChanReadHook));
    EcoResChk(EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_ChanWriteHook, ecoChanWriteHook));
    EcoResChk(EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_ReqHdrHook, ecoReqHdrHook));
    EcoResChk(EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_RspHdrHook, ecoRspHdrHook));
    EcoResChk(EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_BodyWriteHook, ecoBodyWriteHook));



    /* Set HTTP request parameters. */
    EcoResChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Method, (EcoArg)EcoHttpMeth_Get));
    EcoResChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Verion, (EcoArg)EcoHttpVer_1_1));



    /* Add HTTP headers. */
    EcoResChk(EcoHdrTab_Add(tab, "Accept", "*/*"));



    /* Set IP address and port. */
    EcoResChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, (EcoArg)addrStr));



    /* Enable keep-alive. */
    EcoResChk(EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_KeepAlive, (EcoArg)true));



    /* Issue HTTP requests. */
    for (int i = 2; i < argc; i++) {
        EcoResChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Path, argv[i]));

        /* Issue HTTP request. */
        EcoResChk(EcoHttpCli_Issue(cli));
    }



    /* Delete HTTP client. */
    EcoHttpCli_Del(cli);

    return EXIT_SUCCESS;
}
