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

#include <stddef.h>
#include <stdint.h>

typedef enum _EcoRes {
    EcoRes_Ok               = 0,

    EcoRes_Err              = -1,
    EcoRes_NoMem            = -2,

    EcoRes_NotFound         = -3,

    EcoRes_BadOpt           = -4,

    EcoRes_ReachEnd         = -5,

    EcoRes_NoChanHook       = -6,

    EcoRes_Again            = -7,

    EcoRes_BadStatLine      = -8,
    EcoRes_BadHttpVer       = -9,
    EcoRes_BadStatCode      = -10,
    EcoRes_BadReasonPhase   = -11,
    EcoRes_BadHdrLine       = -12,
    EcoRes_BadHdrKey        = -13,
    EcoRes_BadHdrVal        = -14,
    EcoRes_BadEmpLine       = -15,

    EcoRes_BadChanOpen      = -16,
    EcoRes_BadChanSetOpt    = -17,
    EcoRes_BadChanRead      = -18,
    EcoRes_BadChanWrite     = -19,
    EcoRes_BadChanClose     = -20,

    EcoRes_TooSmall         = -21,

    EcoRes_TooBig           = -22,

    EcoRes_NoReq            = -23,

    EcoRes_NoRsp            = -24,
} EcoRes;

typedef enum _EcoHttpVer {
    EcoHttpVer_Unknown = -1,

    EcoHttpVer_0_9,
    EcoHttpVer_1_0,
    EcoHttpVer_1_1,
} EcoHttpVer;

#define ECO_DEF_HTTP_VER    EcoHttpVer_1_1

typedef enum _EcoHttpMeth {
    EcoHttpMeth_Get,
    EcoHttpMeth_Post,
    EcoHttpMeth_Head,
} EcoHttpMeth;

#define ECO_DEF_HTTP_METH   EcoHttpMeth_Get

typedef enum _EcoOpt {

    /* Options for HTTP request. */
    EcoOpt_Url,
    EcoOpt_Addr,
    EcoOpt_Port,
    EcoOpt_Path,
    EcoOpt_Method,
    EcoOpt_Verion,
    EcoOpt_Headers,
    EcoOpt_BodyBuf,
    EcoOpt_BodyLen,

    /* Options for HTTP client. */
    EcoOpt_ChanHookArg,
    EcoOpt_ChanOpenHook,
    EcoOpt_ChanCloseHook,
    EcoOpt_ChanSetOptHook,
    EcoOpt_ChanReadHook,
    EcoOpt_ChanWriteHook,
    EcoOpt_ReqHdrHookArg,
    EcoOpt_ReqHdrHook,
    EcoOpt_RspHdrHookArg,
    EcoOpt_RspHdrHook,
    EcoOpt_BodyHookArg,
    EcoOpt_BodyWriteHook,
    EcoOpt_Request,
} EcoOpt;



typedef void * EcoArg;



typedef enum _EcoChanOpt {
    EcoChanOpt_SyncReadWrite,
    EcoChanOpt_ReadWriteTimeout,
} EcoChanOpt;



typedef struct _EcoKvp {
    char *keyBuf;
    char *valBuf;
    size_t keyLen;
    size_t valLen;
    uint32_t keyHash;
} EcoKvp;

typedef struct _EcoHdrTab {
    EcoKvp *kvpAry;
    size_t kvpCap;
    size_t kvpNum;
} EcoHdrTab;



void EcoHdrTab_Init(EcoHdrTab *tab);

EcoHdrTab *EcoHdrTab_New(void);

void EcoHdrTab_Deinit(EcoHdrTab *tab);

void EcoHdrTab_Del(EcoHdrTab *tab);

/**
 * @brief Add a key-value pair to the header table.
 * 
 * @param tab Header table.
 * @param key Key string.
 * @param val Value string.
 */
EcoRes EcoHdrTab_Add(EcoHdrTab *tab, const char *key, const char *val);

/**
 * @brief Add a key-value pair to the header table with format string.
 * 
 * @param tab Header table.
 * @param key Key string.
 * @param fmt Format string for value.
 */
EcoRes EcoHdrTab_AddFmt(EcoHdrTab *tab, const char *key, const char *fmt, ...);

/**
 * @brief Drop a key-value pair from the header table.
 * 
 * @param tab Header table.
 * @param key Key string.
 */
EcoRes EcoHdrTab_Drop(EcoHdrTab *tab, const char *key);

/**
 * @brief Clear all key-value pairs in the header table.
 * 
 * @param tab Header table.
 */
void EcoHdrTab_Clear(EcoHdrTab *tab);

/**
 * @brief Find a key-value pair in the header table.
 * 
 * @note If `kvp` is `NULL`, then the key-value pair will not be returned.
 * 
 * @param tab Header table.
 * @param key Key string.
 * @param kvp Pointer to the key-value pair.
 * 
 * @return `EcoRes_Ok` if found, `EcoRes_NotFound` if not found.
 */
EcoRes EcoHdrTab_Find(EcoHdrTab *tab, const char *key, EcoKvp **kvp);



typedef struct _EcoChanAddr {
    uint8_t addr[4];
    uint16_t port;
} EcoChanAddr;

#define ECO_DEF_CHAN_ADDR   (&(EcoChanAddr){{ 127, 0, 0, 1 }, 80})

typedef struct _EcoHttpReq {
    EcoHttpMeth meth;
    char *urlBuf;
    size_t urlLen;
    EcoChanAddr chanAddr;
    EcoHttpVer ver;
    EcoHdrTab *hdrTab;
    uint8_t *bodyBuf;
    size_t bodyLen;
} EcoHttpReq;

void EcoHttpReq_Init(EcoHttpReq *req);

EcoHttpReq *EcoHttpReq_New(void);

void EcoHttpReq_Deinit(EcoHttpReq *req);

void EcoHttpReq_Del(EcoHttpReq *req);

EcoRes EcoHttpReq_SetOpt(EcoHttpReq *req, EcoOpt opt, EcoArg arg);



typedef enum _EcoStatCode {
    EcoStatCode_Unknown             = -1,

    EcoStatCode_Ok                  = 200,
    EcoStatCode_PartialContent      = 206,
    EcoStatCode_BadRequest          = 400,
    EcoStatCode_NotFound            = 404,
    EcoStatCode_ServerError         = 500,
} EcoStatCode;

typedef struct _EcoHttpRsp {
    EcoHttpVer ver;
    EcoStatCode statCode;
    EcoHdrTab *hdrTab;
    uint8_t *bodyBuf;
    size_t bodyLen;
} EcoHttpRsp;

void EcoHttpRsp_Init(EcoHttpRsp *rsp);

EcoHttpRsp *EcoHttpRsp_New(void);

void EcoHttpRsp_Deinit(EcoHttpRsp *rsp);

void EcoHttpRsp_Del(EcoHttpRsp *rsp);



typedef EcoRes (*EcoChanOpenHook)(EcoChanAddr *addr, EcoArg arg);

typedef EcoRes (*EcoChanCloseHook)(EcoArg arg);

/**
 * @brief User defined channel option setting hook function.
 * 
 * @param opt Option to set.
 * @param arg Option data to set.
 * @param hookArg Extra user data which can be set by option `EcoOpt_ChanHookArg`.
 */
typedef EcoRes (*EcoChanSetOptHook)(EcoChanOpt opt, EcoArg arg, EcoArg hookArg);

/**
 * @brief User defined channel read hook function.
 * 
 * @param buf Buffer to store read data.
 * @param len Data length to read.
 * @param arg Extra user data which can be set by option `EcoOpt_ChanHookArg`.
 * 
 * @return The actual length of the read data.
 *         `EcoRes_ReachEnd` indicates the current channel has been closed.
 *         Other negative numbers represent corresponding errors.
 */
typedef int (*EcoChanReadHook)(void *buf, int len, EcoArg arg);

/**
 * @brief User defined channel write hook function.
 * 
 * @param buf Buffer to store written data.
 * @param len Data length to write.
 * @param arg Extra user data which can be set by option `EcoOpt_ChanHookArg`.
 * 
 * @return The actual length of the written data.
 *         `EcoRes_ReachEnd` indicates the current channel has been closed.
 *         Other negative numbers represent corresponding errors.
 */
typedef int (*EcoChanWriteHook)(const void *buf, int len, EcoArg arg);



/**
 * @brief User defined request header getting hook function.
 * 
 * @param hdrNum Total number of headers to write.
 * @param hdrIdx Index of the current header.
 * @param keyBuf Header key buffer.
 * @param keyLen Header key length.
 * @param valBuf Header value buffer.
 * @param valLen Header value length.
 * @param arg Extra user data which can be set by option `EcoOpt_ReqHdrHookArg`.
 * 
 * @return `EcoRes_Ok` for success.
 *         Negative numbers represent corresponding errors.
 */
typedef EcoRes (*EcoReqHdrHook)(size_t hdrNum, size_t hdrIdx,
                                const char *keyBuf, size_t keyLen,
                                const char *valBuf, size_t valLen,
                                EcoArg arg);



/**
 * @brief User defined response header getting hook function.
 * 
 * @param hdrNum Total number of headers to write.
 * @param hdrIdx Index of the current header.
 * @param keyBuf Header key buffer.
 * @param keyLen Header key length.
 * @param valBuf Header value buffer.
 * @param valLen Header value length.
 * @param arg Extra user data which can be set by option `EcoOpt_RspHdrHookArg`.
 * 
 * @return `EcoRes_Ok` for success.
 *         Negative numbers represent corresponding errors.
 */
typedef EcoRes (*EcoRspHdrHook)(size_t hdrNum, size_t hdrIdx,
                                const char *keyBuf, size_t keyLen,
                                const char *valBuf, size_t valLen,
                                EcoArg arg);



/**
 * @brief User defined body write hook function.
 * 
 * @note When arg `off` equals 0, it indicates this is the first packet.
 *       When arg `len` equals 0, it indicates this is the last packet.
 * 
 * @param off Data offset.
 * @param buf Buffer to store written data.
 * @param len Data length to write.
 * @param arg Extra user data which can be set by option `EcoOpt_BodyHookArg`.
 * 
 * @return The actual length of the written data.
 *         Negative numbers represent corresponding errors.
 */
typedef int (*EcoBodyWriteHook)(int off, const void *buf, int len, EcoArg arg);



typedef struct _EcoHttpCli {
    EcoArg chanHookArg;
    EcoChanOpenHook chanOpenHook;
    EcoChanCloseHook chanCloseHook;
    EcoChanSetOptHook chanSetOptHook;
    EcoChanReadHook chanReadHook;
    EcoChanWriteHook chanWriteHook;
    EcoArg reqHdrHookArg;
    EcoRspHdrHook reqHdrHook;
    EcoArg rspHdrHookArg;
    EcoRspHdrHook rspHdrHook;
    EcoArg bodyHookArg;
    EcoBodyWriteHook bodyWriteHook;
    EcoHttpReq *req;
    EcoHttpRsp *rsp;
} EcoHttpCli;

void EcoHttpCli_Init(EcoHttpCli *cli);

EcoHttpCli *EcoHttpCli_New(void);

void EcoHttpCli_Deinit(EcoHttpCli *cli);

void EcoHttpCli_Del(EcoHttpCli *cli);

EcoRes EcoHttpCli_SetOpt(EcoHttpCli *cli, EcoOpt opt, EcoArg arg);

EcoRes EcoHttpCli_Issue(EcoHttpCli *cli);
