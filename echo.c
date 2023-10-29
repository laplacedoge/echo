#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "echo.h"



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

typedef struct _EcoUrlDsc {
    uint8_t addr[4];
    uint16_t port;
} EcoUrlDsc;

void EcoUrlDsc_Init(EcoUrlDsc *dsc) {
    dsc->addr[0] = 127;
    dsc->addr[1] = 0;
    dsc->addr[2] = 0;
    dsc->addr[3] = 1;

    dsc->port = 80;
}

EcoRes EcoUrlDsc_Parse(EcoUrlDsc *dsc, const char *url) {
    enum _FsmStat {
        FsmStat_Start,
        FsmStat_AddrDigit,
        FsmStat_AddrDot,
        FsmStat_AddrPortColon,
        FsmStat_PortDigit,
    } fsmStat = FsmStat_Start;

    const char *urlBuf = url;
    size_t urlLen = strlen(url);
    uint8_t addrBuf[4];
    size_t addrIdx = 0;
    uint16_t port;

    EcoUrlDsc_Init(dsc);

    for (size_t i = 0; i < urlLen; i++) {
        char ch = urlBuf[i];

        switch (fsmStat) {
        case FsmStat_Start:
            if (ch >= '0' &&
                ch <= '9') {
                addrBuf[0] = ch - '0';
                addrIdx = 0;

                fsmStat = FsmStat_AddrDigit;

                break;
            }

            return EcoRes_Err;

        case FsmStat_AddrDigit:
            if (ch >= '0' &&
                ch <= '9') {
                addrBuf[addrIdx] *= 10;
                addrBuf[addrIdx] += ch - '0';

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
                addrBuf[addrIdx] = ch - '0';

                fsmStat = FsmStat_AddrDigit;

                break;
            }

            return EcoRes_Err;

        case FsmStat_AddrPortColon:
            if (ch >= '0' &&
                ch <= '9') {
                port = ch - '0';

                fsmStat = FsmStat_PortDigit;

                break;
            }

            return EcoRes_Err;

        case FsmStat_PortDigit:
            if (ch >= '0' &&
                ch <= '9') {
                port *= 10;
                port += ch - '0';

                break;
            }

            return EcoRes_Err;
        }
    }

    if (fsmStat == FsmStat_AddrDigit) {
        if (addrIdx != 3) {
            return EcoRes_Err;
        }

        memcpy(dsc->addr, addrBuf, sizeof(addrBuf));
    } else if (fsmStat == FsmStat_PortDigit) {
        memcpy(dsc->addr, addrBuf, sizeof(addrBuf));
        dsc->port = port;
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
