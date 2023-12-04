/**
 * MIT License
 * 
 * Copyright (c) 2023 Alex Chen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdbool.h>
#include <strings.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "echo.h"
#include "conf.h"



#define SND_CHUNK_LEN_MIN   512



#if ECO_CONF_DEF_SND_CHUNK_LEN < SND_CHUNK_LEN_MIN
    #error "Configuration ECO_CONF_DEF_SND_CHUNK_LEN is too small."
#endif



#define ECO_VER_MAJOR_STR   "1"
#define ECO_VER_MINOR_STR   "0"
#define ECO_VER_MICRO_STR   "0"

#define ECO_NAME_STR        "ecHo"

#define ECO_USER_AGENT_STR  ECO_NAME_STR "/"        \
                            ECO_VER_MAJOR_STR "."   \
                            ECO_VER_MINOR_STR "."   \
                            ECO_VER_MICRO_STR



const char *EcoRes_ToStr(EcoRes res) {
    switch (res) {
    case EcoRes_Ok: return "Operation success";
    case EcoRes_Err: return "Operation failure";
    case EcoRes_NoMem: return "No enough memory";
    case EcoRes_NotFound: return "Not found";
    case EcoRes_BadOpt: return "Bad option";
    case EcoRes_BadArg: return "Bad argument";
    case EcoRes_Again: return "Try again";
    case EcoRes_BadScheme: return "Invalid scheme";
    case EcoRes_BadHost: return "Invalid host";
    case EcoRes_BadPort: return "Invalid port";
    case EcoRes_BadPath: return "Invalid path";
    case EcoRes_BadQuery: return "Invalid query";
    case EcoRes_BadFmt: return "Invalid format";
    case EcoRes_BadChar: return "Invalid character";
    case EcoRes_BadStatLine: return "Invalid status line";
    case EcoRes_BadHttpVer: return "Invalid HTTP version";
    case EcoRes_BadStatCode: return "Invalid status code";
    case EcoRes_BadReasonPhase: return "Invalid reason phase";
    case EcoRes_BadHdrLine: return "Invalid header line";
    case EcoRes_BadHdrKey: return "Invalid header key";
    case EcoRes_BadHdrVal: return "Invalid header value";
    case EcoRes_BadEmpLine: return "Invalid empty line";
    case EcoRes_BadChanOpen: return "Channel open failed";
    case EcoRes_BadChanSetOpt: return "Channel set option failed";
    case EcoRes_BadChanRead: return "Channel read failed";
    case EcoRes_BadChanWrite: return "Channel write failed";
    case EcoRes_BadChanClose: return "Channel close failed";
    case EcoRes_ReachEnd: return "Reach end";
    case EcoRes_NoChanHook: return "No channel hook set";
    case EcoRes_NoReq: return "No request set";
    default: return "Unknown result";
    }
}



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
    case EcoHttpMeth_Put: return "PUT";
    default: return "GET";
    }
}



EcoHttpRsp *EcoHttpRsp_New(void);

void EcoHttpRsp_Del(EcoHttpRsp *cli);


#define KVP_ARY_INIT_CAP    8

static uint32_t EcoHash_HashBuf(const void *buf, size_t len) {
    uint32_t hash = 5381;
    uint8_t byte;

    for (size_t i = 0; i < len; i++) {
        byte = ((uint8_t *)buf)[i];

        if (byte >= 'A' &&
            byte <= 'Z') {
            byte = byte - 'A' + 'a';
        }

        hash = ((hash << 5) + hash) + byte;
    }

    return hash;
}

static uint32_t EcoHash_HashStr(const char *str) {
    return EcoHash_HashBuf(str, strlen(str));
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

/**
 * @brief Convert header key to lowercase.
 */
static void LowHdrKey(char *keyBuf, size_t keyLen) {
    for (size_t i = 0; i < keyLen; i++) {
        char ch = keyBuf[i];

        if (ch >= 'A' &&
            ch <= 'Z') {
            keyBuf[i] = ch - 'A' + 'a';
        }
    }
}

/**
 * @brief Check if the byte is valid in header key string.
 * 
 * @param byte Byte to be checked.
 */
static bool IsHdrKeyByte(uint8_t byte) {
    static const uint8_t lookUpTab[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    return lookUpTab[byte] == 1 ? true : false;
}

/**
 * @brief Validate header key string.
 * 
 * @param keyBuf Key string buffer.
 * @param keyLen Key string length.
 */
static EcoRes ChkHdrKey(const char *keyBuf, size_t keyLen) {
    for (size_t i = 0; i < keyLen; i++) {
        if (IsHdrKeyByte(keyBuf[i]) == false) {
            return EcoRes_BadHdrKey;
        }
    }

    return EcoRes_Ok;
}

/**
 * @brief Check if the byte is valid in header value string.
 * 
 * @param byte Byte to be checked.
 */
static bool IsHdrValByte(uint8_t byte) {
    static const uint8_t lookUpTab[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    };

    return lookUpTab[byte] == 1 ? true : false;
}

/**
 * @brief Validate header value string.
 * 
 * @param valBuf Value string buffer.
 * @param valLen Value string length.
 */
static EcoRes ChkHdrVal(const char *valBuf, size_t valLen) {
    for (size_t i = 0; i < valLen; i++) {
        if (IsHdrValByte(valBuf[i]) == false) {
            return EcoRes_BadHdrVal;
        }
    }

    return EcoRes_Ok;
}

/**
 * @brief Try to search a key in the given header table.
 * 
 * @param tab Header table.
 * @param keyBuf Key string buffer.
 * @param keyLen Key string length.
 * @param kvp Key-value pair result.
 */
static EcoRes EcoHdrTab_FindByBufAndLen(EcoHdrTab *tab, const char *keyBuf,
                                        size_t keyLen, EcoKvp **kvp) {
    uint32_t keyHash = EcoHash_HashBuf(keyBuf, keyLen);

    for (size_t i = 0; i < tab->kvpNum; i++) {
        EcoKvp *curKvp = tab->kvpAry + i;

        if (keyHash == curKvp->keyHash &&
            keyLen == curKvp->keyLen &&
            strncasecmp(keyBuf, curKvp->keyBuf, curKvp->keyLen) == 0) {
            if (kvp != NULL) {
                *kvp = curKvp;
            }

            return EcoRes_Ok;
        }
    }

    return EcoRes_NotFound;
}

static EcoRes EcoHdrTab_GetNextSlot(EcoHdrTab *tab, EcoKvp **kvp) {
    if (tab->kvpAry == NULL) {
        tab->kvpAry = (EcoKvp *)malloc(sizeof(EcoKvp) * KVP_ARY_INIT_CAP);
        if (tab->kvpAry == NULL) {
            return EcoRes_NoMem;
        }

        tab->kvpCap = KVP_ARY_INIT_CAP;
        tab->kvpNum = 0;

        *kvp = tab->kvpAry;
    } else {
        if (tab->kvpNum + 1 > tab->kvpCap) {
            tab->kvpAry = (EcoKvp *)realloc(tab->kvpAry, sizeof(EcoKvp) * tab->kvpCap * 2);
            if (tab->kvpAry == NULL) {
                return EcoRes_NoMem;
            }

            tab->kvpCap *= 2;
        }

        *kvp = tab->kvpAry + tab->kvpNum;
    }

    return EcoRes_Ok;
}

static EcoRes EcoHdrTab_AddByBufAndLen(EcoHdrTab *tab,
                                       const void *keyBuf, size_t keyLen,
                                       const void *valBuf, size_t valLen) {
    EcoKvp *kvp;
    EcoRes res;

    res = EcoHdrTab_FindByBufAndLen(tab, keyBuf, keyLen, &kvp);
    if (res == EcoRes_NotFound) {
        EcoHdrTab_GetNextSlot(tab, &kvp);

        kvp->keyBuf = (char *)malloc(keyLen + 1);
        if (kvp->keyBuf == NULL) {
            return EcoRes_NoMem;
        }

        kvp->valBuf = (char *)malloc(valLen + 1);
        if (kvp->valBuf == NULL) {
            free(kvp->keyBuf);
            kvp->keyBuf = NULL;

            return EcoRes_NoMem;
        }

        memcpy(kvp->keyBuf, keyBuf, keyLen);
        kvp->keyBuf[keyLen] = '\0';
        memcpy(kvp->valBuf, valBuf, valLen);
        kvp->valBuf[valLen] = '\0';

        kvp->keyLen = keyLen;
        kvp->valLen = valLen;

        LowHdrKey(kvp->keyBuf, kvp->keyLen);

        kvp->keyHash = EcoHash_HashBuf(kvp->keyBuf, kvp->keyLen);;

        tab->kvpNum++;

        return EcoRes_Ok;
    } else {
        char *newBuf;

        newBuf = (char *)malloc(valLen + 1);
        if (newBuf == NULL) {
            return EcoRes_NoMem;
        }

        free(kvp->valBuf);
        memcpy(newBuf, valBuf, valLen);
        newBuf[valLen] = '\0';

        kvp->valBuf = newBuf;
        kvp->valLen = valLen;

        return EcoRes_Ok;
    }
}

EcoRes EcoHdrTab_Add(EcoHdrTab *tab, const char *key, const char *val) {
    size_t keyLen = strlen(key);
    size_t valLen = strlen(val);
    EcoKvp *curKvp;
    EcoRes res;

    res = ChkHdrKey(key, keyLen);
    if (res != EcoRes_Ok) {
        return res;
    }

    res = ChkHdrVal(val, valLen);
    if (res != EcoRes_Ok) {
        return res;
    }

    res = EcoHdrTab_AddByBufAndLen(tab, key, keyLen, val, valLen);
    if (res != EcoRes_Ok) {
        return res;
    }

    return EcoRes_Ok;
}

EcoRes EcoHdrTab_AddLine(EcoHdrTab *tab, const char *line) {
    typedef enum _FsmStat {
        FsmStat_1stKeyCh,
        FsmStat_OthKeyChOrColon,
        FsmStat_1stSpcOr1stValCh,
        FsmStat_OthSpcOr1stValCh,
        FsmStat_OthValCh,
    } FsmStat;

    FsmStat fsmStat = FsmStat_1stKeyCh;
    size_t lineLen = strlen(line);
    const char *colon;
    const char *valBuf;
    size_t keyLen;
    size_t valLen;
    EcoRes res;

    if (lineLen < 2) {
        return EcoRes_BadHdrLine;
    }

    for (size_t i = 0; i < lineLen; i++) {
        uint8_t byte = line[i];

        switch (fsmStat) {
        case FsmStat_1stKeyCh:
            if (IsHdrKeyByte(byte)) {
                fsmStat = FsmStat_OthKeyChOrColon;
                break;
            }

            return EcoRes_BadHdrKey;

        case FsmStat_OthKeyChOrColon:
            if (byte == ':') {
                colon = line + i;
                fsmStat = FsmStat_1stSpcOr1stValCh;
                break;
            }

            if (IsHdrKeyByte(byte)) {
                break;
            }

            return EcoRes_BadHdrKey;

        case FsmStat_1stSpcOr1stValCh:
            if (byte == ' ') {
                fsmStat = FsmStat_OthSpcOr1stValCh;
                break;
            }

            if (IsHdrValByte(byte)) {
                valBuf = line + i;
                fsmStat = FsmStat_OthValCh;
                break;
            }

            return EcoRes_BadHdrVal;

        case FsmStat_OthSpcOr1stValCh:
            if (byte == ' ') {
                break;
            }

            if (IsHdrValByte(byte)) {
                valBuf = line + i;
                fsmStat = FsmStat_OthValCh;
                break;
            }

            return EcoRes_BadHdrVal;

        case FsmStat_OthValCh:
            if (IsHdrValByte(byte)) {
                break;
            }

            return EcoRes_BadHdrVal;
        }
    }

    keyLen = (size_t)(colon - line);

    switch (fsmStat) {
    case FsmStat_1stKeyCh:
    case FsmStat_OthKeyChOrColon:
        return EcoRes_BadHdrLine;

    case FsmStat_1stSpcOr1stValCh:
    case FsmStat_OthSpcOr1stValCh:
        valBuf = "";
        valLen = 0;
        break;

    case FsmStat_OthValCh:
        valLen = lineLen - (valBuf - line);
        break;
    }

    res = EcoHdrTab_AddByBufAndLen(tab, line, keyLen, valBuf, valLen);
    if (res != EcoRes_Ok) {
        return res;
    }

    return EcoRes_Ok;
}

EcoRes EcoHdrTab_AddFmt(EcoHdrTab *tab, const char *key, const char *fmt, ...) {
    char *valBuf;
    size_t valLen;
    va_list ap;
    EcoRes res;

    va_start(ap, fmt);
    valLen = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    valBuf = (char *)malloc(valLen + 1);
    if (valBuf == NULL) {
        return EcoRes_NoMem;
    }

    va_start(ap, fmt);
    vsnprintf(valBuf, valLen + 1, fmt, ap);
    va_end(ap);

    res = EcoHdrTab_Add(tab, key, valBuf);
    if (res != EcoRes_Ok) {
        free(valBuf);

        return res;
    }

    free(valBuf);

    return EcoRes_Ok;
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
    return EcoHdrTab_FindByBufAndLen(tab, key, strlen(key), kvp);
}

void EcoHttpReq_Init(EcoHttpReq *req) {
    req->scheme = ECO_CONF_DEF_SCHEME;
    req->meth = ECO_CONF_DEF_HTTP_METH;
    req->pathBuf = NULL;
    req->pathLen = 0;
    req->queryBuf = NULL;
    req->queryLen = 0;
    memcpy(req->chanAddr.addr, ECO_CONF_DEF_IP_ADDR,
           sizeof(req->chanAddr.addr));
    req->ver = ECO_CONF_DEF_HTTP_VER;
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
    if (req->pathBuf != NULL) {
        free(req->pathBuf);
    }

    if (req->queryBuf != NULL) {
        free(req->queryBuf);
    }

    if (req->hdrTab != NULL) {
        EcoHdrTab_Del(req->hdrTab);
    }

    EcoHttpReq_Init(req);
}

void EcoHttpReq_Del(EcoHttpReq *req) {
    EcoHttpReq_Deinit(req);

    free(req);
}

#define ECO_URL_SCHEME_MAX_LEN  (8 - 1)
#define ECO_URL_SCHEME_BUF_LEN  (ECO_URL_SCHEME_MAX_LEN + 1)

#define ECO_URL_PATH_MAX_LEN    (1024 - 1)
#define ECO_URL_PATH_BUF_LEN    (ECO_URL_PATH_MAX_LEN + 1)

#define ECO_URL_QUERY_MAX_LEN   (1024 - 1)
#define ECO_URL_QUERY_BUF_LEN   (ECO_URL_PATH_MAX_LEN + 1)

/* URL parsing cache. */
typedef struct _EcoUrlParCac {

    /* Scheme (optional). */
    bool schemeSet;
    char schemeBuf[ECO_URL_SCHEME_BUF_LEN];
    size_t schemeLen;

    /* IPv4 address. */
    uint32_t ipv4Buf[4];
    size_t ipv4Len;

    /* Port (optional). */
    bool portSet;
    uint32_t port;

    /* Path (optional). */
    bool pathSet;
    char pathBuf[ECO_URL_PATH_BUF_LEN];
    size_t pathLen;

    /* Query (optional). */
    bool querySet;
    char queryBuf[ECO_URL_QUERY_BUF_LEN];
    size_t queryLen;
} EcoUrlParCac;

void EcoUrlParCac_Init(EcoUrlParCac *cache) {
    cache->schemeSet = false;
    memset(cache->schemeBuf, 0, sizeof(cache->schemeBuf));
    cache->schemeLen = 0;

    memset(cache->ipv4Buf, 0, sizeof(cache->ipv4Buf));
    cache->ipv4Len = 0;

    cache->portSet = false;
    cache->port = 0;

    cache->pathSet = false;
    memset(cache->pathBuf, 0, sizeof(cache->pathBuf));
    cache->pathLen = 0;

    cache->querySet = false;
    memset(cache->queryBuf, 0, sizeof(cache->queryBuf));
    cache->queryLen = 0;
}

EcoUrlParCac *EcoUrlParCac_New(void) {
    EcoUrlParCac *newCache;

    newCache = (EcoUrlParCac *)malloc(sizeof(EcoUrlParCac));
    if (newCache == NULL) {
        return NULL;
    }

    EcoUrlParCac_Init(newCache);

    return newCache;
}

void EcoUrlParCac_Deinit(EcoUrlParCac *cache) {
    EcoUrlParCac_Init(cache);
}

void EcoUrlParCac_Del(EcoUrlParCac *cache) {
    free(cache);
}

EcoRes EcoUrlParCac_ParseUrl(EcoUrlParCac *cache, const char *url) {
    typedef enum _FsmStat {
        FsmStat_Start,
        FsmStat_Scheme1stCh,
        FsmStat_SchemeOthCh,
        FsmStat_ColonAfterProto,
        FsmStat_Slash1AfterProto,
        FsmStat_Slash2AfterProto,
        FsmStat_Ipv41stDigit,
        FsmStat_Ipv4Dot,
        FsmStat_Ipv4OthDigit,
        FsmStat_ColonAfterHost,
        FsmStat_Host1stDigit,
        FsmStat_HostOthDigit,
        FsmStat_PathSlash,
        FsmStat_PathSegCh,
        FsmStat_QuesAfterPath,
        FsmStat_Query1stCh,
        FsmStat_QueryOthCh,
    } FsmStat;

    FsmStat fsmStat = FsmStat_Start;
    size_t urlLen = strlen(url);

    for (size_t i = 0; i < urlLen; i++) {
        char ch = url[i];

        switch (fsmStat) {
        case FsmStat_Start:
        case FsmStat_Scheme1stCh:
            if ((ch >= 'a' && ch <= 'z') ||
                (ch >= 'A' && ch <= 'Z')) {
                cache->schemeSet = true;
                cache->schemeBuf[0] = ch;
                cache->schemeLen = 1;

                fsmStat = FsmStat_SchemeOthCh;
                break;
            }

            if (ch >= '0' &&
                ch <= '9') {
                cache->ipv4Buf[cache->ipv4Len] = ch - '0';

                fsmStat = FsmStat_Ipv4OthDigit;
                break;
            }

            return EcoRes_BadChar;

        case FsmStat_SchemeOthCh:
            if ((ch >= 'a' && ch <= 'z') ||
                (ch >= 'A' && ch <= 'Z') ||
                (ch >= '0' && ch <= '9') ||
                ch == '+' ||
                ch == '-' ||
                ch == '.') {
                if (cache->schemeLen == ECO_URL_SCHEME_MAX_LEN) {
                    return EcoRes_BadScheme;
                }

                cache->schemeBuf[cache->schemeLen] = ch;
                cache->schemeLen++;

                fsmStat = FsmStat_SchemeOthCh;
                break;
            }

            if (ch == ':') {
                cache->schemeBuf[cache->schemeLen] = '\0';

                fsmStat = FsmStat_ColonAfterProto;
                break;
            }

            return EcoRes_BadChar;

        case FsmStat_ColonAfterProto:
            if (ch == '/') {
                fsmStat = FsmStat_Slash1AfterProto;
                break;
            }

            return EcoRes_BadChar;

        case FsmStat_Slash1AfterProto:
            if (ch == '/') {
                fsmStat = FsmStat_Slash2AfterProto;
                break;
            }

            return EcoRes_BadChar;

        case FsmStat_Slash2AfterProto:
        case FsmStat_Ipv41stDigit:
        case FsmStat_Ipv4Dot:
            if (ch >= '0' &&
                ch <= '9') {
                cache->ipv4Buf[cache->ipv4Len] = ch - '0';

                fsmStat = FsmStat_Ipv4OthDigit;
                break;
            }

            return EcoRes_BadChar;

        case FsmStat_Ipv4OthDigit:
            if (ch >= '0' &&
                ch <= '9') {
                cache->ipv4Buf[cache->ipv4Len] *= 10;
                cache->ipv4Buf[cache->ipv4Len] += ch - '0';

                if (cache->ipv4Buf[cache->ipv4Len] > 255) {
                    return EcoRes_BadHost;
                }

                break;
            }

            if (ch == '.') {
                if (cache->ipv4Len >= 3) {
                    return EcoRes_BadHost;
                }

                cache->ipv4Len++;

                fsmStat = FsmStat_Ipv4Dot;
                break;
            }

            if (ch == ':') {
                if (cache->ipv4Len != 3) {
                    return EcoRes_BadHost;
                }

                cache->ipv4Len = 4;

                cache->portSet = true;

                fsmStat = FsmStat_ColonAfterHost;
                break;
            }

            if (ch == '/') {
                if (cache->ipv4Len != 3) {
                    return EcoRes_BadHost;
                }

                cache->ipv4Len = 4;

                cache->pathSet = true;
                cache->pathBuf[0] = ch;
                cache->pathLen = 1;

                fsmStat = FsmStat_PathSegCh;
                break;
            }

            return EcoRes_BadChar;

        case FsmStat_ColonAfterHost:
        case FsmStat_Host1stDigit:
            if (ch >= '0' &&
                ch <= '9') {
                cache->port = ch - '0';

                fsmStat = FsmStat_HostOthDigit;
                break;
            }

            return EcoRes_BadChar;

        case FsmStat_HostOthDigit:
            if (ch >= '0' &&
                ch <= '9') {
                cache->port *= 10;
                cache->port += ch - '0';

                if (cache->port > 65535) {
                    return EcoRes_BadPort;
                }

                fsmStat = FsmStat_HostOthDigit;
                break;
            }

            if (ch == '/') {
                cache->pathSet = true;
                cache->pathBuf[0] = ch;
                cache->pathLen = 1;

                fsmStat = FsmStat_PathSegCh;
                break;
            }

            return EcoRes_BadChar;

        case FsmStat_PathSlash:
            if ((ch >= 'a' && ch <= 'z') ||
                (ch >= 'A' && ch <= 'Z') ||
                (ch >= '0' && ch <= '9') ||
                ch == '-' ||
                ch == '_' ||
                ch == '.' ||
                ch == '!' ||
                ch == '~' ||
                ch == '*' ||
                ch == '\'' ||
                ch == '(' ||
                ch == ')' ||
                ch == ':' ||
                ch == '@' ||
                ch == '&' ||
                ch == '=' ||
                ch == '+' ||
                ch == '$' ||
                ch == ',') {
                cache->pathBuf[cache->pathLen] = ch;
                cache->pathLen++;

                if (cache->pathLen == ECO_URL_PATH_MAX_LEN) {
                    return EcoRes_BadPath;
                }

                fsmStat = FsmStat_PathSegCh;
                break;
            }

            return EcoRes_BadChar;

        case FsmStat_PathSegCh:
            if ((ch >= 'a' && ch <= 'z') ||
                (ch >= 'A' && ch <= 'Z') ||
                (ch >= '0' && ch <= '9') ||
                ch == '-' ||
                ch == '_' ||
                ch == '.' ||
                ch == '!' ||
                ch == '~' ||
                ch == '*' ||
                ch == '\'' ||
                ch == '(' ||
                ch == ')' ||
                ch == ':' ||
                ch == '@' ||
                ch == '&' ||
                ch == '=' ||
                ch == '+' ||
                ch == '$' ||
                ch == ',') {
                cache->pathBuf[cache->pathLen] = ch;
                cache->pathLen++;

                if (cache->pathLen == ECO_URL_PATH_MAX_LEN) {
                    return EcoRes_BadPath;
                }

                break;
            }

            if (ch == '/') {
                cache->pathBuf[cache->pathLen] = ch;
                cache->pathLen++;

                if (cache->pathLen == ECO_URL_PATH_MAX_LEN) {
                    return EcoRes_BadPath;
                }

                fsmStat = FsmStat_PathSlash;
                break;
            }

            if (ch == '?') {
                cache->pathBuf[cache->pathLen] = '\0';

                fsmStat = FsmStat_QuesAfterPath;
                break;
            }

            return EcoRes_BadChar;

        case FsmStat_QuesAfterPath:
        case FsmStat_Query1stCh:
            if ((ch >= 'a' && ch <= 'z') ||
                (ch >= 'A' && ch <= 'Z') ||
                (ch >= '0' && ch <= '9') ||
                ch == ';' ||
                ch == '/' ||
                ch == '?' ||
                ch == ':' ||
                ch == '@' ||
                ch == '&' ||
                ch == '=' ||
                ch == '+' ||
                ch == '$' ||
                ch == ',' ||
                ch == '-' ||
                ch == '_' ||
                ch == '.' ||
                ch == '!' ||
                ch == '~' ||
                ch == '*' ||
                ch == '\'' ||
                ch == '(' ||
                ch == ')' ) {
                cache->querySet = true;
                cache->queryBuf[0] = ch;
                cache->queryLen = 1;

                fsmStat = FsmStat_QueryOthCh;
                break;
            }

            return EcoRes_BadChar;

        case FsmStat_QueryOthCh:
            if ((ch >= 'a' && ch <= 'z') ||
                (ch >= 'A' && ch <= 'Z') ||
                (ch >= '0' && ch <= '9') ||
                ch == ';' ||
                ch == '/' ||
                ch == '?' ||
                ch == ':' ||
                ch == '@' ||
                ch == '&' ||
                ch == '=' ||
                ch == '+' ||
                ch == '$' ||
                ch == ',' ||
                ch == '-' ||
                ch == '_' ||
                ch == '.' ||
                ch == '!' ||
                ch == '~' ||
                ch == '*' ||
                ch == '\'' ||
                ch == '(' ||
                ch == ')' ) {
                cache->queryBuf[cache->queryLen] = ch;
                cache->queryLen++;

                break;
            }

            return EcoRes_BadChar;
        }
    }

    switch (fsmStat) {
    case FsmStat_Start:
    case FsmStat_Scheme1stCh:
    case FsmStat_SchemeOthCh:
    case FsmStat_ColonAfterProto:
    case FsmStat_Slash1AfterProto:
    case FsmStat_Slash2AfterProto:
    case FsmStat_Ipv41stDigit:
    case FsmStat_Ipv4Dot:
        return EcoRes_BadFmt;

    case FsmStat_Ipv4OthDigit:
        if (cache->ipv4Len != 3) {
            return EcoRes_BadFmt;
        }
        break;

    case FsmStat_ColonAfterHost:
    case FsmStat_Host1stDigit:
        return EcoRes_BadFmt;

    case FsmStat_HostOthDigit:
        break;

    case FsmStat_PathSlash:
    case FsmStat_PathSegCh:
        cache->pathBuf[cache->pathLen] = '\0';
        break;

    case FsmStat_QuesAfterPath:
    case FsmStat_Query1stCh:
        return EcoRes_BadFmt;

    case FsmStat_QueryOthCh:
        cache->queryBuf[cache->queryLen] = '\0';
        break;
    }

    return EcoRes_Ok;
}

static EcoRes EcoHttpReq_SetOpt_Url(EcoHttpReq *req, const char *url) {
    EcoChanAddr *addr = &req->chanAddr;
    EcoUrlParCac *cache;
    char *pathBuf;
    char *queryBuf;
    EcoRes res;

    /* Create URL parsing cache. */
    cache = EcoUrlParCac_New();
    if (cache == NULL) {
        return EcoRes_NoMem;
    }

    /* Parse URL. */
    res = EcoUrlParCac_ParseUrl(cache, url);
    if (res != EcoRes_Ok) {
        goto Finally;
    }

    /* Check scheme. */
    if (cache->schemeSet) {
        if (strcmp(cache->schemeBuf, "http") == 0) {
            req->scheme = EcoScheme_Http;
        } else if (strcmp(cache->schemeBuf, "https") == 0) {
            req->scheme = EcoScheme_Https;
        } else {
            res = EcoRes_BadScheme;
            goto Finally;
        }
    } else {
        if (ECO_CONF_DEF_SCHEME == EcoScheme_Https) {
            req->scheme = EcoScheme_Https;
        } else {
            req->scheme = EcoScheme_Http;
        }
    }

    /* Copy path and query string. */
    if (cache->pathSet) {
        pathBuf = (char *)malloc(cache->pathLen + 1);
        if (pathBuf == NULL) {
            res = EcoRes_NoMem;
            goto Finally;
        }

        memcpy(pathBuf, cache->pathBuf, cache->pathLen + 1);
    }
    if (cache->querySet) {
        queryBuf = (char *)malloc(cache->queryLen + 1);
        if (pathBuf == NULL) {
            free(pathBuf);

            res = EcoRes_NoMem;
            goto Finally;
        }

        memcpy(queryBuf, cache->queryBuf, cache->queryLen + 1);
    }

    /* Copy IP address and port. */
    addr->addr[0] = (uint8_t)cache->ipv4Buf[0];
    addr->addr[1] = (uint8_t)cache->ipv4Buf[1];
    addr->addr[2] = (uint8_t)cache->ipv4Buf[2];
    addr->addr[3] = (uint8_t)cache->ipv4Buf[3];

    if (cache->portSet) {
        addr->port = (uint16_t)cache->port;
    } else {
        if (req->scheme == EcoScheme_Https) {
            addr->port = ECO_CONF_DEF_HTTPS_PORT;
        } else {
            addr->port = ECO_CONF_DEF_HTTP_PORT;
        }
    }

    /* Replace path and query string */
    if (cache->pathSet) {
        if (req->pathBuf != NULL) {
            free(req->pathBuf);
        }

        req->pathBuf = pathBuf;
        req->pathLen = cache->pathLen;
    }
    if (cache->querySet) {
        if (req->queryBuf != NULL) {
            free(req->queryBuf);
        }

        req->queryBuf = queryBuf;
        req->queryLen = cache->queryLen;
    }

    res = EcoRes_Ok;

Finally:
    EcoUrlParCac_Del(cache);

    return res;
}

/**
 * @brief Parse host IPv4 address string.
 * 
 * @param req HTTP request.
 * @param host Host IPv4 address string.
 */
EcoRes EcoHttpReq_SetOpt_Host(EcoHttpReq *req, const char *host) {
    typedef enum _FsmStat {
        FsmStat_1stChInNum,
        FsmStat_OthChOrDot,
    } FsmStat;

    EcoChanAddr *chanAddr = &req->chanAddr;
    FsmStat fsmStat = FsmStat_1stChInNum;
    size_t hostLen = strlen(host);
    int numBuf[4] = {0};
    int numIdx = 0;

    for (int i = 0; i < hostLen; i++) {
        char ch = host[i];

        switch (fsmStat) {
        case FsmStat_1stChInNum:
            if (ch >= '0' && ch <= '9') {
                numBuf[numIdx] = ch - '0';

                fsmStat = FsmStat_OthChOrDot;
                break;
            }

            return EcoRes_BadChar;

        case FsmStat_OthChOrDot:
            if (numBuf[numIdx] == 0) {
                if (ch == '.') {
                    numIdx++;
                    if (numIdx == 4) {
                        return EcoRes_BadFmt;
                    }

                    fsmStat = FsmStat_1stChInNum;
                    break;
                }
            } else {
                if (ch >= '0' && ch <= '9') {
                    numBuf[numIdx] *= 10;
                    numBuf[numIdx] += ch - '0';
                    if (numBuf[numIdx] > 255) {
                        return EcoRes_BadFmt;
                    }

                    break;
                }

                if (ch == '.') {
                    numIdx++;
                    if (numIdx == 4) {
                        return EcoRes_BadFmt;
                    }

                    fsmStat = FsmStat_1stChInNum;
                    break;
                }
            }

            return EcoRes_BadChar;
        }
    }

    if (fsmStat != FsmStat_OthChOrDot ||
        numIdx != 3) {
        return EcoRes_BadFmt;
    }

    chanAddr->addr[0] = (uint8_t)numBuf[0];
    chanAddr->addr[1] = (uint8_t)numBuf[1];
    chanAddr->addr[2] = (uint8_t)numBuf[2];
    chanAddr->addr[3] = (uint8_t)numBuf[3];

    return EcoRes_Ok;
}

EcoRes EcoHttpReq_SetOpt(EcoHttpReq *req, EcoHttpReqOpt opt, EcoArg arg) {
    EcoRes res;

    switch (opt) {
    case EcoHttpReqOpt_Scheme:
        switch ((size_t)arg) {
        case EcoScheme_Http:
        case EcoScheme_Https:
            break;

        default:
            return EcoRes_BadScheme;
        }

        req->scheme = (EcoScheme)(size_t)arg;

        break;

    case EcoHttpReqOpt_Url:
        res = EcoHttpReq_SetOpt_Url(req, (char *)arg);
        if (res != EcoRes_Ok) {
            return res;
        }

        break;

    case EcoHttpReqOpt_Host:
        res = EcoHttpReq_SetOpt_Host(req, (char *)arg);
        if (res != EcoRes_Ok) {
            return res;
        }

        break;

    case EcoHttpReqOpt_Addr:
        memcpy(req->chanAddr.addr, (uint8_t *)arg, 4);
        break;

    case EcoHttpReqOpt_Port:
        req->chanAddr.port = (uint16_t)(size_t)arg;
        break;

    case EcoHttpReqOpt_Path: {
        size_t pathLen;
        char *pathBuf;

        // TODO: I probably should parse not simply copy it :(

        /* If `arg` is NULL or it's empty string, set path string to empty. */
        if (arg == NULL || (pathLen = strlen((char *)arg)) == 0) {
            if (req->pathBuf != NULL) {
                free(req->pathBuf);
                req->pathBuf = NULL;
            }

            req->pathLen = 0;

            break;
        }

        /* Create a new string for storing path string. */
        pathBuf = (char *)malloc(pathLen + 1);
        if (pathBuf == NULL) {
            return EcoRes_NoMem;
        }

        memcpy(pathBuf, arg, pathLen + 1);

        if (req->pathBuf != NULL) {
            free(req->pathBuf);
        }

        req->pathBuf = pathBuf;
        req->pathLen = pathLen;

        break;
    }

    case EcoHttpReqOpt_Query: {
        size_t queryLen;
        char *queryBuf;

        // TODO: I probably should parse not simply copy it :(

        /* If `arg` is NULL or it's empty string, set query string to empty. */
        if (arg == NULL || (queryLen = strlen((char *)arg)) == 0) {
            if (req->queryBuf != NULL) {
                free(req->queryBuf);
                req->queryBuf = NULL;
            }

            req->queryLen = 0;

            break;
        }

        /* Create a new string for storing query string. */
        queryBuf = (char *)malloc(queryLen + 1);
        if (queryBuf == NULL) {
            return EcoRes_NoMem;
        }

        memcpy(queryBuf, arg, queryLen + 1);

        if (req->queryBuf != NULL) {
            free(req->queryBuf);
        }

        req->queryBuf = queryBuf;
        req->queryLen = queryLen;

        break;
    }

    case EcoHttpReqOpt_Method:
        req->meth = (EcoHttpMeth)(size_t)arg;
        break;

    case EcoHttpReqOpt_Version:
        req->ver = (EcoHttpVer)(size_t)arg;
        break;

    case EcoHttpReqOpt_Headers:
        if (req->hdrTab != NULL) {
            EcoHdrTab_Del(req->hdrTab);
        }
        req->hdrTab = (EcoHdrTab *)arg;
        break;

    case EcoHttpReqOpt_BodyBuf:
        req->bodyBuf = (uint8_t *)arg;
        break;

    case EcoHttpReqOpt_BodyLen:
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
    rsp->contLen = 0;
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

    rsp->contLen = 0;

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
    cli->req = NULL;
    cli->rsp = NULL;

    cli->sndChunkBuf = NULL;
    cli->sndChunkCap = ECO_CONF_DEF_SND_CHUNK_LEN;
    cli->sndChunkLen = 0;

    cli->chanHookArg = NULL;
    cli->chanOpenHook = NULL;
    cli->chanCloseHook = NULL;
    cli->chanSetOptHook = NULL;
    cli->chanReadHook = NULL;
    cli->chanWriteHook = NULL;

    cli->reqHdrHookArg = NULL;
    cli->reqHdrHook = NULL;

    cli->rspHdrHookArg = NULL;
    cli->rspHdrHook = NULL;

    cli->bodyHookArg = NULL;
    cli->bodyWriteHook = NULL;

    cli->chanOpened = false;
    cli->keepAlive = false;
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

    /* If channel has been opened, close it. */
    if (cli->chanCloseHook != NULL &&
        cli->chanOpened) {
        cli->chanCloseHook(cli->chanHookArg);
    }

    if (cli->req != NULL) {
        EcoHttpReq_Del(cli->req);
    }

    if (cli->rsp != NULL) {
        EcoHttpRsp_Del(cli->rsp);
    }

    if (cli->sndChunkBuf != NULL) {
        free(cli->sndChunkBuf);
    }

    EcoHttpCli_Init(cli);
}

void EcoHttpCli_Del(EcoHttpCli *cli) {
    EcoHttpCli_Deinit(cli);

    free(cli);
}

EcoRes EcoHttpCli_SetOpt(EcoHttpCli *cli, EcoHttpCliOpt opt, EcoArg arg) {
    switch (opt) {
    case EcoHttpCliOpt_ChanHookArg:
        cli->chanHookArg = arg;
        break;

    case EcoHttpCliOpt_ChanOpenHook:
        cli->chanOpenHook = (EcoChanOpenHook)arg;
        break;

    case EcoHttpCliOpt_ChanCloseHook:
        cli->chanCloseHook = (EcoChanCloseHook)arg;
        break;

    case EcoHttpCliOpt_ChanSetOptHook:
        cli->chanSetOptHook = (EcoChanSetOptHook)arg;
        break;

    case EcoHttpCliOpt_ChanReadHook:
        cli->chanReadHook = (EcoChanReadHook)arg;
        break;

    case EcoHttpCliOpt_ChanWriteHook:
        cli->chanWriteHook = (EcoChanWriteHook)arg;
        break;

    case EcoHttpCliOpt_ReqHdrHookArg:
        cli->reqHdrHookArg = arg;
        break;

    case EcoHttpCliOpt_ReqHdrHook:
        cli->reqHdrHook = (EcoReqHdrHook)arg;
        break;

    case EcoHttpCliOpt_RspHdrHookArg:
        cli->rspHdrHookArg = arg;
        break;

    case EcoHttpCliOpt_RspHdrHook:
        cli->rspHdrHook = (EcoRspHdrHook)arg;
        break;

    case EcoHttpCliOpt_BodyHookArg:
        cli->bodyHookArg = arg;
        break;

    case EcoHttpCliOpt_BodyWriteHook:
        cli->bodyWriteHook = (EcoBodyWriteHook)arg;
        break;

    case EcoHttpCliOpt_KeepAlive:
        cli->keepAlive = (size_t)arg ? true : false;
        break;

    case EcoHttpCliOpt_Request:
        if (cli->req != NULL) {
            EcoHttpReq_Del(cli->req);
        }
        cli->req = (EcoHttpReq *)arg;
        break;

    case EcoHttpCliOpt_SndChunkCap: {
        size_t newChunkCap = (size_t)arg;

        if (newChunkCap < SND_CHUNK_LEN_MIN) {
            return EcoRes_BadArg;
        }

        if (cli->sndChunkBuf != NULL) {
            uint8_t *newChunkBuf;

            newChunkBuf = (uint8_t *)malloc(newChunkCap);
            if (newChunkBuf == NULL) {
                return EcoRes_NoMem;
            }

            free(cli->sndChunkBuf);

            cli->sndChunkBuf = newChunkBuf;
        } else {
            cli->sndChunkBuf = (uint8_t *)malloc(newChunkCap);
            if (cli->sndChunkBuf == NULL) {
                return EcoRes_NoMem;
            }
        }

        cli->sndChunkCap = newChunkCap;
        cli->sndChunkLen = 0;

        break;
    }

    default:
        return EcoRes_BadOpt;
    }

    return EcoRes_Ok;
}

static EcoRes EcoCli_AutoGenHdrs(EcoHttpCli *cli) {
    EcoHttpReq *req = cli->req;
    EcoChanAddr *chanAddr = &req->chanAddr;
    EcoRes res;

    /* If request header table is not created yet, create it. */
    if (req->hdrTab == NULL) {
        req->hdrTab = EcoHdrTab_New();
        if (req->hdrTab == NULL) {
            return EcoRes_NoMem;
        }
    }

    res = EcoHdrTab_Find(req->hdrTab, "host", NULL);
    if (res == EcoRes_NotFound) {
        res = EcoHdrTab_AddFmt(req->hdrTab, "host", "%u.%u.%u.%u:%u",
                               chanAddr->addr[0], chanAddr->addr[1],
                               chanAddr->addr[2], chanAddr->addr[3],
                               chanAddr->port);
        if (res != EcoRes_Ok) {
            return res;
        }
    }

    res = EcoHdrTab_Find(req->hdrTab, "user-agent", NULL);
    if (res == EcoRes_NotFound) {
        res = EcoHdrTab_AddFmt(req->hdrTab, "user-agent", ECO_USER_AGENT_STR);
        if (res != EcoRes_Ok) {
            return res;
        }
    }

    res = EcoHdrTab_Find(req->hdrTab, "content-length", NULL);
    if (res == EcoRes_NotFound) {
        res = EcoHdrTab_AddFmt(req->hdrTab, "content-length", "%zu", req->bodyLen);
        if (res != EcoRes_Ok) {
            return res;
        }
    }

    /* If keep-alive is enabled, make sure request
       has `Connection: keep-alive` header. */
    if (cli->keepAlive) {
        res = EcoHdrTab_Add(req->hdrTab, "connection", "keep-alive");
        if (res != EcoRes_Ok) {
            return res;
        }
    }

    return EcoRes_Ok;
}

/**
 * @brief Capitalize the first letter of each word in the header key.
 */
static void CapHdrKey(char *keyBuf, size_t keyLen) {
    bool capNextCh = true;

    for (size_t i = 0; i < keyLen; i++) {
        char ch = keyBuf[i];

        if (ch >= 'a' &&
            ch <= 'z') {
            if (capNextCh) {
                capNextCh = false;

                keyBuf[i] = ch - 'a' + 'A';
            }
        } else if (ch >= 'A' && ch <= 'Z') {
            if (capNextCh == false) {
                keyBuf[i] = ch - 'A' + 'a';
            }
        } else {
            if (capNextCh == false) {
                capNextCh = true;
            }
        }
    }
}

static void EcoCli_CapReqHdrKey(EcoHttpCli *cli) {
    EcoHttpReq *req = cli->req;

    if (req == NULL || req->hdrTab == NULL) {
        return;
    }

    for (size_t i = 0; i < req->hdrTab->kvpNum; i++) {
        EcoKvp *curKvp = req->hdrTab->kvpAry + i;

        CapHdrKey(curKvp->keyBuf, curKvp->keyLen);
    }
}

static void EcoCli_CapRspHdrKey(EcoHttpCli *cli) {
    EcoHttpRsp *rsp = cli->rsp;

    if (rsp == NULL || rsp->hdrTab == NULL) {
        return;
    }

    for (size_t i = 0; i < rsp->hdrTab->kvpNum; i++) {
        EcoKvp *curKvp = rsp->hdrTab->kvpAry + i;
        
        CapHdrKey(curKvp->keyBuf, curKvp->keyLen);
    }
}

static void EcoCli_LowReqHdrKey(EcoHttpCli *cli) {
    EcoHttpReq *req = cli->req;

    if (req == NULL || req->hdrTab == NULL) {
        return;
    }

    for (size_t i = 0; i < req->hdrTab->kvpNum; i++) {
        EcoKvp *curKvp = req->hdrTab->kvpAry + i;

        LowHdrKey(curKvp->keyBuf, curKvp->keyLen);
    }
}

/**
 * @brief Send request message data.
 * @note This function will accumulate data in the send buffer and send them
 *       when the send buffer is full.
 * 
 * @param cli HTTP client.
 * @param buf Data buffer.
 * @param len Data length.
 */
static EcoRes SendReqData(EcoHttpCli *cli, void *buf, int len) {
    int restLen;
    int remLen;
    int curLen;
    int wrLen;

    /* If the send cache is full, send them immediately. */
    if (cli->sndChunkLen == cli->sndChunkCap) {
        wrLen = cli->chanWriteHook(cli->sndChunkBuf, cli->sndChunkLen, cli->chanHookArg);
        if (wrLen != cli->sndChunkLen) {
            return EcoRes_BadChanWrite;
        }

        cli->sndChunkLen = 0;
    }

    remLen = len;
    while (remLen != 0) {
        restLen = cli->sndChunkCap - cli->sndChunkLen;
        if (remLen >= restLen) {
            curLen = restLen;
        } else {
            curLen = remLen;
        }

        if (curLen == cli->sndChunkCap) {
            wrLen = cli->chanWriteHook((uint8_t *)buf + len - remLen, curLen, cli->chanHookArg);
            if (wrLen != cli->sndChunkLen) {
                return EcoRes_BadChanWrite;
            }
        } else {
            memcpy(cli->sndChunkBuf + cli->sndChunkLen, (uint8_t *)buf + len - remLen, curLen);
            cli->sndChunkLen += curLen;

            remLen -= curLen;

            /* If the send cache is full, send them immediately. */
            if (cli->sndChunkLen == cli->sndChunkCap) {
                wrLen = cli->chanWriteHook(cli->sndChunkBuf, cli->sndChunkLen, cli->chanHookArg);
                if (wrLen != cli->sndChunkLen) {
                    return EcoRes_BadChanWrite;
                }

                cli->sndChunkLen = 0;
            }
        }
    }

    return EcoRes_Ok;
}

/**
 * @brief Flush data in the send buffer.
 * @note This function will send all data in the send buffer.
 * 
 * @param cli HTTP client.
 */
static EcoRes FlushReqData(EcoHttpCli *cli) {
    int wrLen;

    if (cli->sndChunkLen == 0) {
        return EcoRes_Ok;
    }

    wrLen = cli->chanWriteHook(cli->sndChunkBuf, cli->sndChunkLen, cli->chanHookArg);
    if (wrLen != cli->sndChunkLen) {
        return EcoRes_Err;
    }

    cli->sndChunkLen = 0;

    return EcoRes_Ok;
}

static EcoRes SendReqMsg(EcoHttpCli *cli) {
    EcoHttpReq *req = cli->req;
    char tmpBuf[32];
    int sndLen;
    int wrLen;
    int ret;
    EcoRes res;

    /* Allocate memory for send chunk if needed. */
    if (cli->sndChunkBuf == NULL) {
        cli->sndChunkBuf = (uint8_t *)malloc(cli->sndChunkCap);
        if (cli->sndChunkBuf == NULL) {
            return EcoRes_NoMem;
        }
    }

    /* Clear residual data if needed. */
    if (cli->sndChunkLen != 0) {
        cli->sndChunkLen = 0;
    }

    /* Send start line. */
    sndLen = snprintf(tmpBuf, sizeof(tmpBuf), "%s ", EcoHttpMeth_ToStr(req->meth));
    res = SendReqData(cli, tmpBuf, sndLen);
    if (res != EcoRes_Ok) {
        return res;
    }

    /* Send URL path. */
    if (req->pathBuf != NULL) {
        res = SendReqData(cli, req->pathBuf, req->pathLen);
        if (res != EcoRes_Ok) {
            return res;
        }
    } else {
        res = SendReqData(cli, "/", 1);
        if (res != EcoRes_Ok) {
            return res;
        }
    }

    /* Send URL query string. */
    if (req->queryBuf != NULL) {
        res = SendReqData(cli, "?", 1);
        if (res != EcoRes_Ok) {
            return res;
        }

        res = SendReqData(cli, req->queryBuf, req->queryLen);
        if (res != EcoRes_Ok) {
            return res;
        }
    }

    /* Send HTTP version. */
    sndLen = snprintf(tmpBuf, sizeof(tmpBuf), " HTTP/%s\r\n", EcoHttpVer_ToStr(req->ver));
    res = SendReqData(cli, tmpBuf, sndLen);
    if (res != EcoRes_Ok) {
        return res;
    }

    /* Send header lines. */
    for (size_t i = 0; i < cli->req->hdrTab->kvpNum; i++) {
        EcoKvp *curKvp = cli->req->hdrTab->kvpAry + i;

        res = SendReqData(cli, curKvp->keyBuf, curKvp->keyLen);
        if (res != EcoRes_Ok) {
            return res;
        }

        res = SendReqData(cli, ": ", 2);
        if (res != EcoRes_Ok) {
            return res;
        }

        res = SendReqData(cli, curKvp->valBuf, curKvp->valLen);
        if (res != EcoRes_Ok) {
            return res;
        }

        res = SendReqData(cli, "\r\n", 2);
        if (res != EcoRes_Ok) {
            return res;
        }
    }

    /* Send empty line. */
    res = SendReqData(cli, "\r\n", 2);
    if (res != EcoRes_Ok) {
        return res;
    }

    /* Send body data. */
    if (cli->req->bodyBuf != NULL) {
        res = SendReqData(cli, cli->req->bodyBuf, cli->req->bodyLen);
        if (res != EcoRes_Ok) {
            return res;
        }
    }

    /* Flush the send cache. */
    res = FlushReqData(cli);
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
    case EcoStatCode_PartialContent: return EcoStatCode_PartialContent;
    case EcoStatCode_BadRequest: return EcoStatCode_BadRequest;
    case EcoStatCode_Unauthorized: return EcoStatCode_Unauthorized;
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

static EcoRes ParseRspMsg(EcoHttpCli *cli) {
    typedef enum _FsmStat {
        FsmStat_StatLine = 0,
        FsmStat_HdrLine,
        FsmStat_EmpLine,
        FsmStat_SaveBodyData,
        FsmStat_WriteBodyData,
    } FsmStat;

    ParseCache cache;
    uint8_t rcvBuf[512];
    int rcvLen;
    int remLen;
    int procLen;
    EcoRes res;

    ParseCache_Init(&cache);

    /* Create a HTTP response if it does not exist,
       or if it exists, then deinitialize it. */
    if (cli->rsp == NULL) {
        cli->rsp = EcoHttpRsp_New();
        if (cli->rsp == NULL) {
            return EcoRes_NoMem;
        }
    } else {
        EcoHttpRsp_Deinit(cli->rsp);
    }

    /* Create a new header table for HTTP response. */
    cli->rsp->hdrTab = EcoHdrTab_New();
    if (cli->rsp->hdrTab == NULL) {
        return EcoRes_NoMem;
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
                    cli->rsp->statCode = cache.statCode;

                    cache.rspMsgFsmStat = FsmStat_HdrLine;
                }

                remLen -= procLen;

                break;

            case FsmStat_HdrLine: {
                uint8_t byte = *(rcvBuf + rcvLen - remLen);

                /* If the current byte is CR character, then
                   it may reach the end of the header line. */
                if (byte == '\r') {
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
                    cli->rsp->contLen = cache.contLen;

                    /* If the current method is HEAD. */
                    if (cli->req->meth == EcoHttpMeth_Head) {
                        return EcoRes_Ok;
                    }

                    /* Determine whther to save or write body data. */
                    if (cli->bodyWriteHook == NULL) {
                        if (cache.contLen == 0) {
                            return EcoRes_Ok;
                        }

                        /* Allocate buffer for body data. */
                        cli->rsp->bodyBuf = (uint8_t *)malloc((size_t)cache.contLen);
                        if (cli->rsp->bodyBuf == NULL) {
                            return EcoRes_NoMem;
                        }

                        cli->rsp->bodyLen = 0;

                        cache.rspMsgFsmStat = FsmStat_SaveBodyData;
                    } else {
                        if (cache.contLen == 0) {
                            cli->bodyWriteHook(0, NULL, 0, cli->bodyHookArg);

                            return EcoRes_Ok;
                        }

                        cache.rspMsgFsmStat = FsmStat_WriteBodyData;
                    }
                }

                remLen -= procLen;

                break;

            case FsmStat_SaveBodyData: {
                EcoHttpRsp *rsp = cli->rsp;
                int waitLen;
                int curLen;

                waitLen = cache.contLen - rsp->bodyLen;
                if (remLen > waitLen) {
                    curLen = waitLen;
                } else {
                    curLen = remLen;
                }

                memcpy(rsp->bodyBuf + rsp->bodyLen, rcvBuf + rcvLen - remLen, curLen);

                rsp->bodyLen += curLen;

                remLen -= curLen;

                if (rsp->bodyLen == cache.contLen) {
                    return EcoRes_Ok;
                }

                break;
            }

            case FsmStat_WriteBodyData: {
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

                remLen -= curLen;

                if (cache.bodyLen == cache.contLen) {
                    cli->bodyWriteHook(cache.bodyOff, NULL, 0, cli->bodyHookArg);

                    return EcoRes_Ok;
                }

                break;
            }
            }
        }
    }
}

/**
 * @brief Call HTTP request header hook.
 * @note Hook won't be called if it's not set yet.
 * 
 * @param cli The HTTP client where the request is located.
 */
static EcoRes EcoCli_CallReqHdrHook(EcoHttpCli *cli) {
    EcoHdrTab *tab;
    EcoRes res;

    if (cli->reqHdrHook == NULL ||
        cli->req == NULL ||
        cli->req->hdrTab == NULL) {
        return EcoRes_Ok;
    }

    tab = cli->req->hdrTab;

    if (cli->reqHdrHook != NULL) {
        for (size_t i = 0; i < tab->kvpNum; i++) {
            EcoKvp *kvp = tab->kvpAry + i;

            res = cli->reqHdrHook(tab->kvpNum, i,
                                  kvp->keyBuf, kvp->keyLen,
                                  kvp->valBuf, kvp->valLen,
                                  cli->rspHdrHookArg);
            if (res != EcoRes_Ok) {
                return res;
            }
        }
    }

    return EcoRes_Ok;
}

/**
 * @brief Call HTTP response header hook.
 * @note Hook won't be called if it's not set yet.
 * 
 * @param cli The HTTP client where the response is located.
 */
static EcoRes EcoCli_CallRspHdrHook(EcoHttpCli *cli) {
    EcoHdrTab *tab;
    EcoRes res;

    if (cli->rspHdrHook == NULL ||
        cli->rsp == NULL ||
        cli->rsp->hdrTab == NULL) {
        return EcoRes_Ok;
    }

    tab = cli->rsp->hdrTab;

    if (cli->rspHdrHook != NULL) {
        for (size_t i = 0; i < tab->kvpNum; i++) {
            EcoKvp *kvp = tab->kvpAry + i;

            res = cli->rspHdrHook(tab->kvpNum, i,
                                  kvp->keyBuf, kvp->keyLen,
                                  kvp->valBuf, kvp->valLen,
                                  cli->rspHdrHookArg);
            if (res != EcoRes_Ok) {
                return res;
            }
        }
    }

    return EcoRes_Ok;
}

static EcoRes EcoHttpCli_SendReqAndParseRsp_OpenAndClose(EcoHttpCli *cli) {
    EcoChanAddr chanAddr;
    EcoRes res;

    /* Set channel address. */
    memcpy(chanAddr.addr, cli->req->chanAddr.addr, 4);
    chanAddr.port = cli->req->chanAddr.port;

    /* Open channel. */
    res = cli->chanOpenHook(&chanAddr, cli->chanHookArg);
    if (res != EcoRes_Ok) {
        return res;
    }

    if (cli->chanOpened == false) {
        cli->chanOpened = true;
    }

    /* Send HTTP request. */
    res = SendReqMsg(cli);
    if (res != EcoRes_Ok) {
        goto CloseChan;
    }

    /* Receive and parse HTTP response. */
    res = ParseRspMsg(cli);
    if (res != EcoRes_Ok) {
        goto CloseChan;
    }

    res = EcoRes_Ok;

CloseChan:

    /* Close channel. */
    cli->chanCloseHook(cli->chanHookArg);

    if (cli->chanOpened) {
        cli->chanOpened = false;
    }

    return res;
}

static EcoRes EcoHttpCli_SendReqAndParseRsp_KeepAlive(EcoHttpCli *cli) {
    EcoChanAddr chanAddr;
    EcoKvp *kvp;
    EcoRes res;

    if (cli->chanOpened) {
        goto SendReqMsg;
    }

    /* Set channel address. */
    memcpy(chanAddr.addr, cli->req->chanAddr.addr, 4);
    chanAddr.port = cli->req->chanAddr.port;

    /* Open channel. */
    res = cli->chanOpenHook(&chanAddr, cli->chanHookArg);
    if (res != EcoRes_Ok) {
        return res;
    }

    if (cli->chanOpened == false) {
        cli->chanOpened = true;
    }

SendReqMsg:

    /* Send HTTP request. */
    res = SendReqMsg(cli);
    if (res != EcoRes_Ok) {
        return res;
    }

    /* Receive and parse HTTP response. */
    res = ParseRspMsg(cli);
    if (res != EcoRes_Ok) {
        return res;
    }

    /* Check if server has refused keep-alive. */
    res = EcoHdrTab_Find(cli->rsp->hdrTab, "connection", &kvp);
    if (res == EcoRes_Ok &&
        strcmp(kvp->valBuf, "close") == 0) {
        cli->chanCloseHook(cli->chanHookArg);

        if (cli->chanOpened) {
            cli->chanOpened = false;
        }
    }

    return EcoRes_Ok;
}

EcoRes EcoHttpCli_Issue(EcoHttpCli *cli) {
    EcoRes res;

    if (cli->chanOpenHook == NULL ||
        cli->chanCloseHook == NULL ||
        cli->chanReadHook == NULL ||
        cli->chanWriteHook == NULL) {
        return EcoRes_NoChanHook;
    }

    /* HTTP request must exist. */
    if (cli->req == NULL) {
        return EcoRes_NoReq;
    }

    /* Lowercase all request headers. */
    EcoCli_LowReqHdrKey(cli);

    /* Automatically generate necessary request headers. */
    res = EcoCli_AutoGenHdrs(cli);
    if (res != EcoRes_Ok) {
        return res;
    }

    /* Capitalize the first letter of the request header key. */
    EcoCli_CapReqHdrKey(cli);

    /* Call request header hook. */
    EcoCli_CallReqHdrHook(cli);

    /* Send HTTP request, then receive and parse HTTP response. */
    if (cli->keepAlive) {
        res = EcoHttpCli_SendReqAndParseRsp_KeepAlive(cli);
    } else {
        res = EcoHttpCli_SendReqAndParseRsp_OpenAndClose(cli);
    }

    if (res != EcoRes_Ok) {
        return res;
    }

    /* Capitalize the first letter of the response header key. */
    EcoCli_CapRspHdrKey(cli);

    /* Call response header hook. */
    EcoCli_CallRspHdrHook(cli);

    return EcoRes_Ok;
}
