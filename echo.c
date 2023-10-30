#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "echo.h"



const char *EcoHttpVer_ToStr(EcoHttpVer ver) {
    switch (ver) {
    case EcoHttpVer_0_9: return "0.9";
    case EcoHttpVer_1_0: return "1.0";
    case EcoHttpVer_1_1: return "1.1";
    default: return "1.1";
    }
}

const char *EcoHttpMeth_ToStr(EcoHttpMeth meth) {
    switch (meth) {
    case EcoHttpMeth_Get: return "GET";
    case EcoHttpMeth_Post: return "POST";
    case EcoHttpMeth_Head: return "HEAD";
    default: return "GET";
    }
}



EcoHttpRsp *EcoHttpRsp_New(void);

void EcoHttpRsp_Del(EcoHttpRsp *cli);


#define KVP_ARY_INIT_CAP    8

static uint32_t EcoHash_HashKey(const char *str) {
    uint32_t hash = 5381;
    int c;

    while ((c = *str++)) {

        /* hash * 33 + c */
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

void EcoHdrTab_Init(EcoHdrTab *tab) {
    tab->kvpAry = NULL;
    tab->kvpCap = 0;
    tab->kvpNum = 0;
}

EcoHdrTab *EcoHdrTab_New(void) {
    EcoHdrTab *newTab;

    newTab = (EcoHdrTab *)malloc(sizeof(EcoHdrTab));
    if (newTab == NULL) {
        return NULL;
    }

    EcoHdrTab_Init(newTab);

    return newTab;
}

void EcoHdrTab_Deinit(EcoHdrTab *tab) {
    if (tab->kvpAry != NULL) {
        for (size_t i = 0; i < tab->kvpNum; i++) {
            EcoKvp *curKvp = tab->kvpAry + i;

            free(curKvp->keyBuf);
            free(curKvp->valBuf);
        }

        free(tab->kvpAry);
        tab->kvpAry = NULL;
    }

    tab->kvpCap = 0;
    tab->kvpNum = 0;
}

void EcoHdrTab_Del(EcoHdrTab *tab) {
    EcoHdrTab_Deinit(tab);

    free(tab);
}

EcoRes EcoHdrTab_Add(EcoHdrTab *tab, const char *key, const char *val) {
    EcoKvp *curKvp;

    if (EcoHdrTab_Find(tab, key, &curKvp) == EcoRes_NotFound) {
        char *keyBuf;
        char *valBuf;
        size_t keyLen;
        size_t valLen;
        uint32_t keyHash;

        if (tab->kvpAry == NULL) {
            tab->kvpAry = (EcoKvp *)malloc(sizeof(EcoKvp) * KVP_ARY_INIT_CAP);
            if (tab->kvpAry == NULL) {
                return EcoRes_NoMem;
            }

            tab->kvpCap = KVP_ARY_INIT_CAP;
            tab->kvpNum = 0;

            curKvp = tab->kvpAry;
        } else {
            if (tab->kvpNum + 1 > tab->kvpCap) {
                tab->kvpAry = (EcoKvp *)realloc(tab->kvpAry, sizeof(EcoKvp) * tab->kvpCap * 2);
                if (tab->kvpAry == NULL) {
                    return EcoRes_NoMem;
                }

                tab->kvpCap *= 2;
            }

            curKvp = tab->kvpAry + tab->kvpNum;
        }

        keyLen = strlen(key);
        valLen = strlen(val);

        keyBuf = (char *)malloc(keyLen + 1);
        if (keyBuf == NULL) {
            return EcoRes_NoMem;
        }

        valBuf = (char *)malloc(valLen + 1);
        if (valBuf == NULL) {
            free(keyBuf);

            return EcoRes_NoMem;
        }

        memcpy(keyBuf, key, keyLen + 1);
        memcpy(valBuf, val, valLen + 1);

        keyHash = EcoHash_HashKey(keyBuf);

        curKvp->keyBuf = keyBuf;
        curKvp->valBuf = valBuf;
        curKvp->keyLen = keyLen;
        curKvp->valLen = valLen;
        curKvp->keyHash = keyHash;

        tab->kvpNum++;

        return EcoRes_Ok;
    } else {
        char *valBuf;
        size_t valLen;

        valLen = strlen(val);

        valBuf = (char *)malloc(valLen + 1);
        if (valBuf == NULL) {
            return EcoRes_NoMem;
        }

        memcpy(valBuf, val, valLen + 1);

        free(curKvp->valBuf);

        curKvp->valBuf = valBuf;
        curKvp->valLen = valLen;

        return EcoRes_Ok;
    }
}

EcoRes EcoHdrTab_Drop(EcoHdrTab *tab, const char *key) {
    EcoKvp *curKvp;

    if (EcoHdrTab_Find(tab, key, &curKvp) == EcoRes_NotFound) {
        return EcoRes_NotFound;
    } else {
        free(curKvp->keyBuf);
        free(curKvp->valBuf);

        memmove(curKvp, curKvp + 1, sizeof(EcoKvp) * (tab->kvpNum - 1));

        tab->kvpNum--;

        return EcoRes_Ok;
    }
}

void EcoHdrTab_Clear(EcoHdrTab *tab) {
    EcoHdrTab_Deinit(tab);
}

EcoRes EcoHdrTab_Find(EcoHdrTab *tab, const char *key, EcoKvp **kvp) {
    const char *keyBuf;
    size_t keyLen;
    uint32_t keyHash;

    if (tab == NULL) {
        return EcoRes_NotFound;
    }

    keyBuf = key;
    keyLen = strlen(key);
    keyHash = EcoHash_HashKey(keyBuf);

    for (size_t i = 0; i < tab->kvpNum; i++) {
        EcoKvp *curKvp = tab->kvpAry + i;

        if (keyHash == curKvp->keyHash &&
            keyLen == curKvp->keyLen &&
            memcmp(keyBuf, curKvp->keyBuf, keyLen) == 0) {
            if (kvp != NULL) {
                *kvp = curKvp;
            }

            return EcoRes_Ok;
        }
    }

    return EcoRes_NotFound;
}

void EcoHttpReq_Init(EcoHttpReq *req) {
    req->meth = ECO_DEF_HTTP_METH;
    req->urlBuf = NULL;
    req->urlLen = 0;
    memcpy(&req->chanAddr, ECO_DEF_CHAN_ADDR, sizeof(req->chanAddr));
    req->ver = ECO_DEF_HTTP_VER;
    req->hdrTab = NULL;
    req->bodyBuf = NULL;
    req->bodyLen = 0;
}

EcoHttpReq *EcoHttpReq_New(void) {
    EcoHttpReq *newReq;

    newReq = (EcoHttpReq *)malloc(sizeof(EcoHttpReq));
    if (newReq == NULL) {
        return NULL;
    }

    EcoHttpReq_Init(newReq);

    return newReq;
}

void EcoHttpReq_Deinit(EcoHttpReq *req) {
    req->meth = ECO_DEF_HTTP_METH;

    if (req->urlBuf != NULL) {
        free(req->urlBuf);
        req->urlBuf = NULL;
    }

    req->urlLen = 0;

    memcpy(&req->chanAddr, ECO_DEF_CHAN_ADDR, sizeof(req->chanAddr));

    req->ver = ECO_DEF_HTTP_VER;

    if (req->hdrTab != NULL) {
        EcoHdrTab_Del(req->hdrTab);
        req->hdrTab = NULL;
    }

    if (req->bodyBuf != NULL) {
        free(req->bodyBuf);
        req->bodyBuf = NULL;
    }

    req->bodyLen = 0;
}

void EcoHttpReq_Del(EcoHttpReq *req) {
    EcoHttpReq_Deinit(req);

    free(req);
}

#define ECO_URL_MAX_LEN     (256 - 1)
#define ECO_URL_BUF_LEN     (ECO_URL_MAX_LEN + 1)

typedef struct _EcoUrlDsc {
    uint8_t addr[4];
    uint16_t port;
    char urlBuf[ECO_URL_BUF_LEN];
    size_t urlLen;
} EcoUrlDsc;

void EcoUrlDsc_Init(EcoUrlDsc *dsc) {
    dsc->addr[0] = 127;
    dsc->addr[1] = 0;
    dsc->addr[2] = 0;
    dsc->addr[3] = 1;

    dsc->port = 80;

    memset(dsc->urlBuf, 0, sizeof(dsc->urlBuf));
    dsc->urlLen = 0;
}

EcoRes EcoUrlDsc_Parse(EcoUrlDsc *dsc, const char *url) {
    enum _FsmStat {
        FsmStat_Start,
        FsmStat_AddrDigit,
        FsmStat_AddrDot,
        FsmStat_AddrPortColon,
        FsmStat_PortDigit,
        FsmStat_UrlChar,
    } fsmStat = FsmStat_Start;

    const char *urlBuf = url;
    size_t urlLen = strlen(url);
    // uint8_t addrBuf[4];
    size_t addrIdx = 0;
    // uint16_t port;

    EcoUrlDsc_Init(dsc);

    for (size_t i = 0; i < urlLen; i++) {
        char ch = urlBuf[i];

        switch (fsmStat) {
        case FsmStat_Start:
            if (ch >= '0' &&
                ch <= '9') {
                dsc->addr[0] = ch - '0';
                addrIdx = 0;

                fsmStat = FsmStat_AddrDigit;

                break;
            }

            return EcoRes_Err;

        case FsmStat_AddrDigit:
            if (ch >= '0' &&
                ch <= '9') {
                dsc->addr[addrIdx] *= 10;
                dsc->addr[addrIdx] += ch - '0';

                break;
            }

            if (ch == '.') {
                addrIdx++;

                fsmStat = FsmStat_AddrDot;

                break;
            }

            if (ch == ':') {
                fsmStat = FsmStat_AddrPortColon;

                break;
            }

            return EcoRes_Err;

        case FsmStat_AddrDot:
            if (ch >= '0' &&
                ch <= '9') {
                dsc->addr[addrIdx] = ch - '0';

                fsmStat = FsmStat_AddrDigit;

                break;
            }

            return EcoRes_Err;

        case FsmStat_AddrPortColon:
            if (ch >= '0' &&
                ch <= '9') {
                dsc->port = ch - '0';

                fsmStat = FsmStat_PortDigit;

                break;
            }

            return EcoRes_Err;

        case FsmStat_PortDigit:
            if (ch >= '0' &&
                ch <= '9') {
                dsc->port *= 10;
                dsc->port += ch - '0';

                break;
            }

            if (ch == '/') {
                dsc->urlBuf[0] = ch;
                dsc->urlLen = 1;

                fsmStat = FsmStat_UrlChar;

                break;
            }

            return EcoRes_Err;

        case FsmStat_UrlChar:
            if ((ch >= '0' && ch <= '9') ||
                (ch >= 'a' && ch <= 'z') ||
                (ch >= 'A' && ch <= 'Z') ||
                ch == '-' ||
                ch == '_' ||
                ch == '.' ||
                ch == '~' ||

                ch == '/' ||
                ch == '?' ||
                ch == '#' ||
                ch == '&' ||
                ch == '=') {
                if (dsc->urlLen == ECO_URL_MAX_LEN) {
                    return EcoRes_TooBig;
                }

                dsc->urlBuf[dsc->urlLen] = ch;
                dsc->urlLen++;

                break;
            }

            return EcoRes_Err;
        }
    }

    if (fsmStat == FsmStat_AddrDigit) {
        if (addrIdx != 3) {
            return EcoRes_Err;
        }

        dsc->port = 80;
        dsc->urlBuf[0] = '/';
        dsc->urlBuf[1] = '\0';
        dsc->urlLen = 1;
    } else if (fsmStat == FsmStat_PortDigit) {
        dsc->urlBuf[0] = '/';
        dsc->urlBuf[1] = '\0';
        dsc->urlLen = 1;
    } else if (fsmStat == FsmStat_UrlChar) {
        dsc->urlBuf[dsc->urlLen] = '\0';
    } else {
        return EcoRes_Err;
    }

    return EcoRes_Ok;
}

EcoRes EcoHttpReq_SetOpt(EcoHttpReq *req, EcoOpt opt, EcoArg arg) {
    switch (opt) {
    case EcoOpt_Url: {
        EcoUrlDsc urlDsc;
        EcoRes res;

        res = EcoUrlDsc_Parse(&urlDsc, (char *)arg);
        if (res != EcoRes_Ok) {
            return res;
        }

        memcpy(req->chanAddr.addr, urlDsc.addr, 4);
        req->chanAddr.port = urlDsc.port;

        /* Set new URL. */
        if (req->urlBuf != NULL) {
            free(req->urlBuf);
        }

        req->urlBuf = (char *)malloc(urlDsc.urlLen + 1);
        if (req->urlBuf == NULL) {
            return EcoRes_NoMem;
        }

        memcpy(req->urlBuf, urlDsc.urlBuf, urlDsc.urlLen + 1);
        req->urlLen = urlDsc.urlLen;

        break;
    }

    case EcoOpt_Method:
        req->meth = (EcoHttpMeth)arg;
        break;

    case EcoOpt_Verion:
        req->ver = (EcoHttpVer)arg;
        break;

    case EcoOpt_Headers:
        if (req->hdrTab != NULL) {
            EcoHdrTab_Del(req->hdrTab);
        }
        req->hdrTab = (EcoHdrTab *)arg;
        break;

    case EcoOpt_BodyBuf:
        if (req->bodyBuf != NULL) {
            free(req->bodyBuf);
        }
        req->bodyBuf = (uint8_t *)arg;
        break;

    case EcoOpt_BodyLen:
        req->bodyLen = (size_t)arg;
        break;

    default:
        return EcoRes_BadOpt;
    }

    return EcoRes_Ok;
}

void EcoHttpRsp_Init(EcoHttpRsp *rsp) {
    rsp->ver = EcoHttpVer_Unknown;
    rsp->statCode = EcoStatCode_Unknown;
    rsp->hdrTab = NULL;
    rsp->bodyBuf = NULL;
    rsp->bodyLen = 0;
}

EcoHttpRsp *EcoHttpRsp_New(void) {
    EcoHttpRsp *newRsp;

    newRsp = (EcoHttpRsp *)malloc(sizeof(EcoHttpRsp));
    if (newRsp == NULL) {
        return NULL;
    }

    EcoHttpRsp_Init(newRsp);

    return newRsp;
}

void EcoHttpRsp_Deinit(EcoHttpRsp *rsp) {
    rsp->ver = EcoHttpVer_Unknown;
    rsp->statCode = EcoStatCode_Unknown;

    if (rsp->hdrTab != NULL) {
        EcoHdrTab_Del(rsp->hdrTab);
        rsp->hdrTab = NULL;
    }

    if (rsp->bodyBuf != NULL) {
        free(rsp->bodyBuf);
        rsp->bodyBuf = NULL;
    }

    rsp->bodyLen = 0;
}

void EcoHttpRsp_Del(EcoHttpRsp *rsp) {
    EcoHttpRsp_Deinit(rsp);

    free(rsp);
}

void EcoHttpCli_Init(EcoHttpCli *cli) {
    cli->chanHookArg = NULL;
    cli->chanOpenHook = NULL;
    cli->chanCloseHook = NULL;
    cli->chanSetOptHook = NULL;
    cli->chanReadHook = NULL;
    cli->chanWriteHook = NULL;
    cli->hdrHookArg = NULL;
    cli->hdrWriteHook = NULL;
    cli->bodyHookArg = NULL;
    cli->bodyWriteHook = NULL;
    cli->req = NULL;
    cli->rsp = NULL;
}

EcoHttpCli *EcoHttpCli_New(void) {
    EcoHttpCli *newCli;

    newCli = (EcoHttpCli *)malloc(sizeof(EcoHttpCli));
    if (newCli == NULL) {
        return NULL;
    }

    EcoHttpCli_Init(newCli);

    return newCli;
}

void EcoHttpCli_Deinit(EcoHttpCli *cli) {
    cli->chanHookArg = NULL;
    cli->chanOpenHook = NULL;
    cli->chanCloseHook = NULL;
    cli->chanSetOptHook = NULL;
    cli->chanReadHook = NULL;
    cli->chanWriteHook = NULL;
    cli->bodyHookArg = NULL;
    cli->bodyWriteHook = NULL;

    if (cli->req != NULL) {
        EcoHttpReq_Del(cli->req);
        cli->req = NULL;
    }

    if (cli->rsp != NULL) {
        EcoHttpRsp_Del(cli->rsp);
        cli->rsp = NULL;
    }
}

void EcoHttpCli_Del(EcoHttpCli *cli) {
    EcoHttpCli_Deinit(cli);

    free(cli);
}

EcoRes EcoHttpCli_SetOpt(EcoHttpCli *cli, EcoOpt opt, EcoArg arg) {
    switch (opt) {
    case EcoOpt_ChanHookArg:
        cli->chanHookArg = arg;
        break;

    case EcoOpt_ChanOpenHook:
        cli->chanOpenHook = (EcoChanOpenHook)arg;
        break;

    case EcoOpt_ChanCloseHook:
        cli->chanCloseHook = (EcoChanCloseHook)arg;
        break;

    case EcoOpt_ChanSetOptHook:
        cli->chanSetOptHook = (EcoChanSetOptHook)arg;
        break;

    case EcoOpt_ChanReadHook:
        cli->chanReadHook = (EcoChanReadHook)arg;
        break;

    case EcoOpt_ChanWriteHook:
        cli->chanWriteHook = (EcoChanWriteHook)arg;
        break;

    case EcoOpt_HdrHookArg:
        cli->hdrHookArg = arg;
        break;

    case EcoOpt_HdrWriteHook:
        cli->hdrWriteHook = (EcoHdrWriteHook)arg;
        break;

    case EcoOpt_BodyHookArg:
        cli->bodyHookArg = arg;
        break;

    case EcoOpt_BodyWriteHook:
        cli->bodyWriteHook = (EcoBodyWriteHook)arg;
        break;

    case EcoOpt_Request:
        if (cli->req != NULL) {
            EcoHttpReq_Del(cli->req);
        }
        cli->req = (EcoHttpReq *)arg;
        break;

    default:
        return EcoRes_BadOpt;
    }

    return EcoRes_Ok;
}

#define ECO_SND_CACHE_BUF_LEN   128

typedef struct _SendCache {
    uint8_t datBuf[ECO_SND_CACHE_BUF_LEN];
    size_t datLen;
} SendCache;

void SendCache_Init(SendCache *cache) {
    cache->datLen = 0;
}

EcoRes EcoCli_SendData(EcoHttpCli *cli, SendCache *cache, void *buf, int len) {
    int restLen;
    int remLen;
    int curLen;
    int wrLen;

    /* If the send cache is full, send them immediately. */
    if (cache->datLen == sizeof(cache->datBuf)) {
        wrLen = cli->chanWriteHook(cache->datBuf, cache->datLen, cli->chanHookArg);
        if (wrLen != cache->datLen) {
            return EcoRes_BadChanWrite;
        }

        cache->datLen = 0;
    }

    remLen = len;
    while (remLen != 0) {
        restLen = sizeof(cache->datBuf) - cache->datLen;
        if (remLen >= restLen) {
            curLen = restLen;
        } else {
            curLen = remLen;
        }

        if (curLen == sizeof(cache->datBuf)) {
            wrLen = cli->chanWriteHook((uint8_t *)buf + len - remLen, curLen, cli->chanHookArg);
            if (wrLen != cache->datLen) {
                return EcoRes_BadChanWrite;
            }
        } else {
            memcpy(cache->datBuf + cache->datLen, (uint8_t *)buf + len - remLen, curLen);
            cache->datLen += curLen;

            remLen -= curLen;

            /* If the send cache is full, send them immediately. */
            if (cache->datLen == sizeof(cache->datBuf)) {
                wrLen = cli->chanWriteHook(cache->datBuf, cache->datLen, cli->chanHookArg);
                if (wrLen != cache->datLen) {
                    return EcoRes_BadChanWrite;
                }

                cache->datLen = 0;
            }
        }
    }

    return EcoRes_Ok;
}

EcoRes EcoCli_FlushCache(EcoHttpCli *cli, SendCache *cache) {
    int wrLen;

    if (cache->datLen == 0) {
        return EcoRes_Ok;
    }

    wrLen = cli->chanWriteHook(cache->datBuf, cache->datLen, cli->chanHookArg);
    if (wrLen != cache->datLen) {
        return EcoRes_Err;
    }

    cache->datLen = 0;

    return EcoRes_Ok;
}

EcoRes EcoCli_SendReqMsg(EcoHttpCli *cli) {
    char *startLineBuf[512];
    SendCache cache;
    int wrLen;
    int ret;
    EcoRes res;

    SendCache_Init(&cache);

    /* Send start line. */
    ret = snprintf((char *)startLineBuf, sizeof(startLineBuf),
        "%s %s HTTP/%s\r\n",
        EcoHttpMeth_ToStr(cli->req->meth),
        cli->req->urlBuf,
        EcoHttpVer_ToStr(cli->req->ver));
    if (ret >= sizeof(startLineBuf)) {
        return EcoRes_TooBig;
    }

    res = EcoCli_SendData(cli, &cache, startLineBuf, ret);
    if (res != EcoRes_Ok) {
        return res;
    }

    /* Send header lines. */
    for (size_t i = 0; i < cli->req->hdrTab->kvpNum; i++) {
        EcoKvp *curKvp = cli->req->hdrTab->kvpAry + i;

        res = EcoCli_SendData(cli, &cache, curKvp->keyBuf, curKvp->keyLen);
        if (res != EcoRes_Ok) {
            return res;
        }

        res = EcoCli_SendData(cli, &cache, ": ", 2);
        if (res != EcoRes_Ok) {
            return res;
        }

        res = EcoCli_SendData(cli, &cache, curKvp->valBuf, curKvp->valLen);
        if (res != EcoRes_Ok) {
            return res;
        }

        res = EcoCli_SendData(cli, &cache, "\r\n", 2);
        if (res != EcoRes_Ok) {
            return res;
        }
    }

    /* Send empty line. */
    res = EcoCli_SendData(cli, &cache, "\r\n", 2);
    if (res != EcoRes_Ok) {
        return res;
    }

    /* Send body data. */
    if (cli->req->bodyBuf != NULL) {
        res = EcoCli_SendData(cli, &cache, cli->req->bodyBuf, cli->req->bodyLen);
        if (res != EcoRes_Ok) {
            return res;
        }
    }

    /* Flush the send cache. */
    res = EcoCli_FlushCache(cli, &cache);
    if (res != EcoRes_Ok) {
        return res;
    }

    return EcoRes_Ok;
}

#define ECO_RF_MAX_LEN          (32 - 1)
#define ECO_RF_BUF_LEN          (ECO_RF_MAX_LEN + 1)

#define ECO_HDR_KEY_MAX_LEN     (128 - 1)
#define ECO_HDR_KEY_BUF_LEN     (ECO_HDR_KEY_MAX_LEN + 1)

#define ECO_HDR_VAL_MAX_LEN     (1024 - 1)
#define ECO_HDR_VAL_BUF_LEN     (ECO_HDR_VAL_MAX_LEN + 1)

typedef struct _ParseCache {
    uint32_t rspMsgFsmStat;

    uint32_t statLineFsmStat;
    uint32_t verMajor;
    uint32_t verMinor;
    uint32_t statCode;
    char rfBuf[ECO_RF_BUF_LEN];
    size_t rfLen;

    uint32_t hdrLineFsmStat;
    char keyBuf[ECO_HDR_KEY_BUF_LEN];
    char valBuf[ECO_HDR_VAL_BUF_LEN];
    size_t keyLen;
    size_t valLen;

    uint32_t contLen;

    uint32_t empLineFsmStat;

    uint32_t bodyOff;
    uint32_t bodyLen;
} ParseCache;

void ParseCache_Init(ParseCache *cache) {
    cache->rspMsgFsmStat = 0;

    cache->statLineFsmStat = 0;
    cache->verMajor = 0;
    cache->verMinor = 0;
    cache->statCode = 0;
    memset(cache->rfBuf, 0, sizeof(cache->rfBuf));
    cache->rfLen = 0;

    cache->hdrLineFsmStat = 0;
    memset(cache->keyBuf, 0, sizeof(cache->keyBuf));
    memset(cache->valBuf, 0, sizeof(cache->valBuf));
    cache->keyLen = 0;
    cache->valLen = 0;

    cache->contLen = 0;

    cache->empLineFsmStat = 0;

    cache->bodyOff = 0;
    cache->bodyLen = 0;
}

void ParseCache_Deinit(ParseCache *cache) {
    ParseCache_Init(cache);
}

static EcoRes EcoCli_ParseStatLine(EcoHttpCli *cli, ParseCache *cache, const void *buf, int availLen, int *procLen) {
    typedef enum _FsmStat {
        FsmStat_Start = 0,

        FsmStat_ChHGot,
        FsmStat_ChT1Got,
        FsmStat_ChT2Got,
        FsmStat_ChPGot,
        FsmStat_ChSlashGot,
        FsmStat_VerMajorGot,
        FsmStat_ChDotGot,
        FsmStat_VerMinorGot,

        FsmStat_SpcAfterVerGot,

        FsmStat_StatCodeGot,

        FsmStat_SpcAfterStatCodeGot,

        FsmStat_ReasonPhraseGot,

        FsmStat_TrailingChCrGot,
        FsmStat_TrailingChLfGot,

        FsmStat_Done = FsmStat_TrailingChLfGot,
    } FsmStat;

    for (int i = 0; i < availLen; i++) {
        uint8_t byte = ((uint8_t *)buf)[i];

        switch (cache->statLineFsmStat) {
        StartParse:
            cache->statLineFsmStat = FsmStat_Start;

        case FsmStat_Start:
            if (byte == 'H') {
                cache->statLineFsmStat = FsmStat_ChHGot;
                break;
            }

            break;

        case FsmStat_ChHGot:
            if (byte == 'T') {
                cache->statLineFsmStat = FsmStat_ChT1Got;
                break;
            }

            goto StartParse;

        case FsmStat_ChT1Got:
            if (byte == 'T') {
                cache->statLineFsmStat = FsmStat_ChT2Got;
                break;
            }

            goto StartParse;

        case FsmStat_ChT2Got:
            if (byte == 'P') {
                cache->statLineFsmStat = FsmStat_ChPGot;
                break;
            }

            goto StartParse;

        case FsmStat_ChPGot:
            if (byte == '/') {
                cache->statLineFsmStat = FsmStat_ChSlashGot;
                break;
            }

            goto StartParse;

        case FsmStat_ChSlashGot:
            if (byte >= '0' &&
                byte <= '9') {
                cache->verMajor = byte - '0';

                cache->statLineFsmStat = FsmStat_VerMajorGot;
                break;
            }

            goto StartParse;

        case FsmStat_VerMajorGot:
            if (byte == '.') {
                cache->statLineFsmStat = FsmStat_ChDotGot;
                break;
            }

            if (byte == ' ') {
                cache->statLineFsmStat = FsmStat_SpcAfterVerGot;
                break;
            }

            goto StartParse;

        case FsmStat_ChDotGot:
            if (byte >= '0' &&
                byte <= '9') {
                cache->verMinor = byte - '0';

                cache->statLineFsmStat = FsmStat_VerMinorGot;
                break;
            }

            goto StartParse;

        case FsmStat_VerMinorGot:
            if (byte == ' ') {
                cache->statLineFsmStat = FsmStat_SpcAfterVerGot;
                break;
            }

            goto StartParse;

        case FsmStat_SpcAfterVerGot:
            if (byte >= '1' &&
                byte <= '9') {
                cache->statCode = byte - '0';

                cache->statLineFsmStat = FsmStat_StatCodeGot;
                break;
            }

            goto StartParse;

        case FsmStat_StatCodeGot:
            if (byte >= '0' &&
                byte <= '9') {
                cache->statCode *= 10;
                cache->statCode += byte - '0';

                break;
            }

            if (byte == ' ') {
                cache->statLineFsmStat = FsmStat_SpcAfterStatCodeGot;
                break;
            }

            return EcoRes_BadStatLine;

        case FsmStat_SpcAfterStatCodeGot:
            if ((byte >= 'a' && byte <= 'z') ||
                (byte >= 'A' && byte <= 'Z') ||
                byte == '-') {
                cache->rfBuf[0] = byte;
                cache->rfLen = 1;

                cache->statLineFsmStat = FsmStat_ReasonPhraseGot;
                break;
            }

            return EcoRes_BadStatLine;

        case FsmStat_ReasonPhraseGot:
            if ((byte >= 'a' && byte <= 'z') ||
                (byte >= 'A' && byte <= 'Z') ||
                byte == ' ' ||
                byte == '-') {
                if (cache->rfLen == ECO_RF_MAX_LEN) {
                    return EcoRes_BadReasonPhase;
                }

                cache->rfBuf[cache->rfLen] = byte;
                cache->rfLen++;

                break;
            }

            if (byte == '\r') {
                cache->rfBuf[cache->rfLen] = '\0';

                cache->statLineFsmStat = FsmStat_TrailingChCrGot;
                break;
            }

            return EcoRes_BadStatLine;

        case FsmStat_TrailingChCrGot:
            if (byte == '\n') {
                *procLen = i + 1;

                cache->statLineFsmStat = FsmStat_TrailingChLfGot;
                return EcoRes_Ok;
            }

            return EcoRes_BadStatLine;

        case FsmStat_TrailingChLfGot:
            break;
        }
    }

    *procLen = availLen;

    return EcoRes_Again;
}

static EcoHttpVer EcoHttpVer_FromNum(uint32_t major, uint32_t minor) {
    if (major == 0) {
        if (minor == 9) {
            return EcoHttpVer_0_9;
        }
    } else if (major == 1) {
        if (minor == 0) {
            return EcoHttpVer_1_0;
        } else if (minor == 1) {
            return EcoHttpVer_1_1;
        }
    }

    return EcoHttpVer_Unknown;
}

static EcoStatCode EcoStatCode_FromNum(uint32_t statCode) {
    switch (statCode) {
    case EcoStatCode_Ok: return EcoStatCode_Ok;
    case EcoStatCode_BadRequest: return EcoStatCode_BadRequest;
    case EcoStatCode_NotFound: return EcoStatCode_NotFound;
    case EcoStatCode_ServerError: return EcoStatCode_ServerError;
    default: return EcoStatCode_Unknown;
    }
}

static const char hdrKeyLcChTab[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0,
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 0, 0, 0, 0, 0, 0,
    0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y',  'z', 0, 0, 0, 0, 0,
    0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y',  'z', 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static EcoRes EcoCli_ParseHdrLine(EcoHttpCli *cli, ParseCache *cache, const void *buf, int availLen, int *procLen) {
    typedef enum _FsmStat {
        FsmStat_Start = 0,

        FsmStat_KeyGot,

        FsmStat_ChColonGot,

        FsmStat_ChSpaceGot,

        FsmStat_ValGot,

        FsmStat_TrailingChCrGot,
        FsmStat_TrailingChLfGot,

        FsmStat_Done = FsmStat_TrailingChLfGot,
    } FsmStat;

    for (int i = 0; i < availLen; i++) {
        uint8_t byte = ((uint8_t *)buf)[i];

        switch (cache->hdrLineFsmStat) {
        case FsmStat_Start:
            if ((byte >= 'a' && byte <= 'z') ||
                (byte >= 'A' && byte <= 'Z') ||
                byte == '-') {
                cache->keyBuf[0] = hdrKeyLcChTab[byte];
                cache->keyLen = 1;

                cache->hdrLineFsmStat = FsmStat_KeyGot;
                break;
            }

            return EcoRes_BadHdrLine;

        case FsmStat_KeyGot:
            if (byte == ':') {
                cache->keyBuf[cache->keyLen] = '\0';

                cache->hdrLineFsmStat = FsmStat_ChColonGot;
                break;
            }

            if (byte >= 0x20 && byte <= 0x7E) {
                if (cache->keyLen == ECO_HDR_KEY_MAX_LEN) {
                    return EcoRes_BadHdrKey;
                }

                cache->keyBuf[cache->keyLen] = hdrKeyLcChTab[byte];
                cache->keyLen++;

                cache->hdrLineFsmStat = FsmStat_KeyGot;
                break;
            }

            return EcoRes_BadHdrLine;

        case FsmStat_ChColonGot:
            if (byte == ' ') {
                cache->hdrLineFsmStat = FsmStat_ChSpaceGot;
                break;
            }

            if (byte >= 0x21 &&
                byte <= 0x7E) {
                cache->valBuf[0] = byte;
                cache->valLen = 1;

                cache->hdrLineFsmStat = FsmStat_ValGot;
                break;
            }

            return EcoRes_BadHdrLine;

        case FsmStat_ChSpaceGot:
            if (byte == ' ') {
                break;
            }

            if (byte >= 0x21 &&
                byte <= 0x7E) {
                cache->valBuf[0] = byte;
                cache->valLen = 1;

                cache->hdrLineFsmStat = FsmStat_ValGot;
                break;
            }

            return EcoRes_BadHdrLine;

        case FsmStat_ValGot:
            if (byte >= 0x20 &&
                byte <= 0x7E) {
                if (cache->valLen == ECO_HDR_VAL_MAX_LEN) {
                    return EcoRes_BadHdrVal;
                }

                cache->valBuf[cache->valLen] = byte;
                cache->valLen++;

                break;
            }

            if (byte == '\r') {
                cache->valBuf[cache->valLen] = '\0';

                cache->hdrLineFsmStat = FsmStat_TrailingChCrGot;
                break;
            }

            return EcoRes_BadHdrLine;

        case FsmStat_TrailingChCrGot:
            if (byte == '\n') {
                *procLen = i + 1;

                cache->hdrLineFsmStat = FsmStat_TrailingChLfGot;
                return EcoRes_Ok;
            }

            return EcoRes_BadHdrLine;

        case FsmStat_TrailingChLfGot:
            break;
        }
    }

    *procLen = availLen;

    return EcoRes_Again;
}

static EcoRes EcoCli_ParseEmpLine(EcoHttpCli *cli, ParseCache *cache, const void *buf, int availLen, int *procLen) {
    typedef enum _FsmStat {
        FsmStat_Start = 0,

        FsmStat_TrailingChCrGot,
        FsmStat_TrailingChLfGot,

        FsmStat_Done = FsmStat_TrailingChLfGot,
    } FsmStat;

    for (int i = 0; i < availLen; i++) {
        uint8_t byte = ((uint8_t *)buf)[i];

        switch (cache->hdrLineFsmStat) {
        case FsmStat_Start:
            if (byte == '\r') {
                cache->hdrLineFsmStat = FsmStat_TrailingChCrGot;
                break;
            }

            return EcoRes_BadEmpLine;

        case FsmStat_TrailingChCrGot:
            if (byte == '\n') {
                *procLen = i + 1;

                cache->hdrLineFsmStat = FsmStat_TrailingChLfGot;
                return EcoRes_Ok;
            }

            return EcoRes_BadEmpLine;

        case FsmStat_TrailingChLfGot:
            break;
        }
    }

    *procLen = availLen;

    return EcoRes_Again;
}

static EcoRes EcoCli_ParseRspMsg(EcoHttpCli *cli) {
    typedef enum _FsmStat {
        FsmStat_StatLine = 0,
        FsmStat_HdrLine,
        FsmStat_EmpLine,
        FsmStat_BodyData,
    } FsmStat;

    ParseCache cache;
    uint8_t rcvBuf[512];
    int rcvLen;
    int remLen;
    int procLen;
    EcoRes res;

    ParseCache_Init(&cache);

    /* Create a HTTP response structure if it does not exist. */
    if (cli->rsp == NULL) {
        cli->rsp = EcoHttpRsp_New();
        if (cli->rsp == NULL) {
            return EcoRes_NoMem;
        }
    }

    /* Create a header table if it does not exist. */
    if (cli->rsp->hdrTab == NULL) {
        cli->rsp->hdrTab = EcoHdrTab_New();
        if (cli->rsp->hdrTab == NULL) {
            return EcoRes_NoMem;
        }
    }

    while (true) {
        rcvLen = cli->chanReadHook(rcvBuf, (int)sizeof(rcvBuf), cli->chanHookArg);
        if (rcvLen < 0) {
            return (EcoRes)rcvLen;
        }

        remLen = rcvLen;
        while (remLen != 0) {
            switch (cache.rspMsgFsmStat) {
            case FsmStat_StatLine:

                /* Try to parse the status line. */
                res = EcoCli_ParseStatLine(cli, &cache, rcvBuf + rcvLen - remLen, remLen, &procLen);
                if (res != EcoRes_Ok &&
                    res != EcoRes_Again) {
                    return res;
                }

                /* If this status line is parsed successfully,
                   then add the cache to the response structure. */
                if (res == EcoRes_Ok) {
                    EcoHttpVer ver;
                    EcoStatCode code;

                    /* Validate the HTTP version. */
                    ver = EcoHttpVer_FromNum(cache.verMajor, cache.verMinor);
                    if (ver == EcoHttpVer_Unknown) {
                        return EcoRes_BadHttpVer;
                    }

                    cli->rsp->ver = ver;

                    /* Validate the HTTP status code. */
                    code = EcoStatCode_FromNum(cache.statCode);
                    if (code == EcoStatCode_Unknown) {
                        return EcoRes_BadStatCode;
                    }

                    cli->rsp->statCode = code;

                    cache.rspMsgFsmStat = FsmStat_HdrLine;
                }

                remLen -= procLen;

                break;

            case FsmStat_HdrLine: {
                uint8_t byte = *(rcvBuf + rcvLen - remLen);

                /* If the current byte is CR character, then
                   it may reach the end of the header line. */
                if (byte == '\r') {
                    if (cli->hdrWriteHook != NULL) {
                        for (size_t i = 0; i < cli->rsp->hdrTab->kvpNum; i++) {
                            EcoKvp *curKvp = cli->rsp->hdrTab->kvpAry + i;

                            res = cli->hdrWriteHook(cli->rsp->hdrTab->kvpNum, i,
                                                    curKvp->keyBuf, curKvp->keyLen,
                                                    curKvp->valBuf, curKvp->valLen,
                                                    cli->hdrHookArg);
                            if (res != EcoRes_Ok) {
                                return res;
                            }
                        }
                    }

                    cache.rspMsgFsmStat = FsmStat_EmpLine;

                    break;
                }

                /* Try to parse this header line. */
                res = EcoCli_ParseHdrLine(cli, &cache, rcvBuf + rcvLen - remLen, remLen, &procLen);
                if (res != EcoRes_Ok &&
                    res != EcoRes_Again) {
                    return res;
                }

                /* If this header line is parsed successfully,
                   then add it to the header table. */
                if (res == EcoRes_Ok) {
                    cache.hdrLineFsmStat = 0;

                    res = EcoHdrTab_Add(cli->rsp->hdrTab, cache.keyBuf, cache.valBuf);
                    if (res != EcoRes_Ok) {
                        return res;
                    }

                    /* If this header line is "Content-Length",
                       then get the content length. */
                    if (strcmp(cache.keyBuf, "content-length") == 0) {
                        char *endPtr;

                        cache.contLen = (uint32_t)strtol(cache.valBuf, &endPtr, 10);
                        if (*endPtr != '\0') {
                            return EcoRes_BadHdrVal;
                        }
                    }
                }

                remLen -= procLen;

                break;
            }

            case FsmStat_EmpLine:
                res = EcoCli_ParseEmpLine(cli, &cache, rcvBuf + rcvLen - remLen, remLen, &procLen);
                if (res != EcoRes_Ok &&
                    res != EcoRes_Again) {
                    return res;
                }

                if (res == EcoRes_Ok) {

                    /* If the current method is HEAD. */
                    if (cli->req->meth == EcoHttpMeth_Head) {
                        return EcoRes_Ok;
                    }

                    /* If body data does not exist. */
                    if (cache.contLen == 0) {
                        cli->bodyWriteHook(0, NULL, 0, cli->bodyHookArg);

                        return EcoRes_Ok;
                    }

                    cache.rspMsgFsmStat = FsmStat_BodyData;
                }

                remLen -= procLen;

                break;

            case FsmStat_BodyData: {
                int waitLen;
                int curLen;
                int wrLen;

                waitLen = cache.contLen - cache.bodyLen;
                if (remLen > waitLen) {
                    curLen = waitLen;
                } else {
                    curLen = remLen;
                }

                wrLen = cli->bodyWriteHook(cache.bodyOff, rcvBuf + rcvLen - remLen, curLen, cli->bodyHookArg);
                if (wrLen < 0) {
                    return (EcoRes)wrLen;
                }

                if (wrLen != curLen) {
                    return EcoRes_Err;
                }

                cache.bodyOff += curLen;
                cache.bodyLen += curLen;

                if (cache.bodyLen == cache.contLen) {
                    cli->bodyWriteHook(cache.bodyOff, NULL, 0, cli->bodyHookArg);

                    return EcoRes_Ok;
                }

                remLen -= curLen;

                break;
            }
            }
        }
    }
}

EcoRes EcoHttpCli_Issue(EcoHttpCli *cli) {
    EcoChanAddr chanAddr;
    EcoRes res;

    if (cli->chanOpenHook == NULL ||
        cli->chanCloseHook == NULL ||
        cli->chanReadHook == NULL ||
        cli->chanWriteHook == NULL) {
        return EcoRes_NoChanHook;
    }

    /* Set channel address. */
    memcpy(chanAddr.addr, cli->req->chanAddr.addr, 4);
    chanAddr.port = cli->req->chanAddr.port;

    /* Open channel. */
    res = cli->chanOpenHook(&chanAddr, cli->chanHookArg);
    if (res != EcoRes_Ok) {
        return res;
    }

    /* Send HTTP request. */
    res = EcoCli_SendReqMsg(cli);
    if (res != EcoRes_Ok) {
        return res;
    }

    res = EcoCli_ParseRspMsg(cli);
    if (res != EcoRes_Ok) {
        return res;
    }

    /* Close channel. */
    res = cli->chanCloseHook(cli->chanHookArg);
    if (res != EcoRes_Ok) {
        return res;
    }

    return EcoRes_Ok;
}
