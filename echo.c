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
