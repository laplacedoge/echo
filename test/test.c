#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include "echo.h"

#define SAVE_FILE_PATH  "save.bin"

EcoRes ecoChanOpenHook(EcoChanAddr *addr, void *arg) {
    struct sockaddr_in srvAddr;
    int sockFd;
    int ret;

    sockFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockFd == -1) {
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



int ecoBodyWriteHook(int off, const void *buf, int len, void *arg) {
    int fileFd;
    int ret;

    fileFd = *(int *)arg;

    if (off == 0) {
        fileFd = open(SAVE_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fileFd < 0) {
            return EcoRes_Err;
        }

        *(int *)arg = fileFd;
    }

    if (len > 0) {
        ret = (int)write(fileFd, buf, (size_t)len);
        if (ret != len) {
            return EcoRes_Err;
        }
    }

    if (len == 0) {
        fileFd = *(int *)arg;

        close(fileFd);
    }

    return len;
}



int main(int argc, char *argv[]) {
    char bodyBuf[] = "{\"version\":\"1\",\"config\":{\"sleepTimeout\":60}}";
    size_t bodyLen = sizeof(bodyBuf) - 1;
    int sockFd;
    int fileFd;
    EcoHttpCli *cli = NULL;
    EcoHttpReq *req = NULL;
    EcoHdrTab *tab = NULL;
    EcoRes res;

    req = EcoHttpReq_New();

    res = EcoHttpReq_SetOpt(req, EcoOpt_Url, "https://192.168.8.72:22120");

    res = EcoHttpReq_SetOpt(req, EcoOpt_Verion, (EcoArg)EcoHttpVer_1_1);

    res = EcoHttpReq_SetOpt(req, EcoOpt_Method, (EcoArg)EcoHttpMethod_Post);

    /* Create HTTP header table. */
    tab = EcoHdrTab_New();
    res = EcoHdrTab_Add(tab, "Connection", "keep-alive");
    res = EcoHdrTab_Add(tab, "Content-Type", "application/json");
    res = EcoHdrTab_Add(tab, "Accept-Encoding", "gzip");

    /* Set HTTP header field of HTTP request. */
    res = EcoHttpReq_SetOpt(req, EcoOpt_Header, (EcoArg)tab);

    /* Set POST body field. */
    res = EcoHttpReq_SetOpt(req, EcoOpt_BodyBuf, (EcoArg)&bodyBuf);
    res = EcoHttpReq_SetOpt(req, EcoOpt_BodyLen, (EcoArg)bodyLen);

    /* Create HTTP client. */
    cli = EcoHttpCli_New();

    /* Set channel hook functions of HTTP client. */
    res = EcoHttpCli_SetOpt(cli, EcoOpt_ChanHookArg, &sockFd);
    res = EcoHttpCli_SetOpt(cli, EcoOpt_ChanOpenHook, ecoChanOpenHook);
    res = EcoHttpCli_SetOpt(cli, EcoOpt_ChanCloseHook, ecoChanCloseHook);
    res = EcoHttpCli_SetOpt(cli, EcoOpt_ChanReadHook, ecoChanReadHook);
    res = EcoHttpCli_SetOpt(cli, EcoOpt_ChanWriteHook, ecoChanWriteHook);

    /* Set body write hook function of HTTP client. */
    res = EcoHttpCli_SetOpt(cli, EcoOpt_BodyHookArg, &fileFd);
    res = EcoHttpCli_SetOpt(cli, EcoOpt_BodyWriteHook, ecoBodyWriteHook);

    /* Set HTTP request of HTTP client. */
    res = EcoHttpCli_SetOpt(cli, EcoOpt_Request, req);

    /* Issue HTTP request. */
    res = EcoHttpCli_Issue(cli);

    /* Delete HTTP client. */
    EcoHttpCli_Del(cli);

    return EXIT_SUCCESS;
}
