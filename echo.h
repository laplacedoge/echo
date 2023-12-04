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

#ifndef __ECHO_H__
#define __ECHO_H__

#include <stddef.h>
#include <stdint.h>

typedef enum _EcoRes {

    /* Operation success. */
    EcoRes_Ok   = 0,

    /* Operation failure. */
    EcoRes_Err  = -128,

    /* General errors. */
    EcoRes_NoMem,
    EcoRes_NotFound,
    EcoRes_BadOpt,
    EcoRes_BadArg,
    EcoRes_Again,

    /* Errors used while parsing HTTP URL. */
    EcoRes_BadScheme,
    EcoRes_BadHost,
    EcoRes_BadPort,
    EcoRes_BadPath,
    EcoRes_BadQuery,
    EcoRes_BadFmt,
    EcoRes_BadChar,

    /* Errors used while parsing HTTP response. */
    EcoRes_BadStatLine,
    EcoRes_BadHttpVer,
    EcoRes_BadStatCode,
    EcoRes_BadReasonPhase,
    EcoRes_BadHdrLine,
    EcoRes_BadHdrKey,
    EcoRes_BadHdrVal,
    EcoRes_BadEmpLine,

    /* Errors returned by channel hooks. */
    EcoRes_BadChanOpen,
    EcoRes_BadChanSetOpt,
    EcoRes_BadChanRead,
    EcoRes_BadChanWrite,
    EcoRes_BadChanClose,
    EcoRes_ReachEnd,

    /* Errors used in HTTP client. */
    EcoRes_NoChanHook,
    EcoRes_NoReq,
} EcoRes;

typedef enum _EcoScheme {
    EcoScheme_Unknown = -1,

    EcoScheme_Http,
    EcoScheme_Https,
} EcoScheme;

typedef enum _EcoHttpVer {
    EcoHttpVer_Unknown = -1,

    EcoHttpVer_0_9,
    EcoHttpVer_1_0,
    EcoHttpVer_1_1,
} EcoHttpVer;

typedef enum _EcoHttpMeth {
    EcoHttpMeth_Get,
    EcoHttpMeth_Post,
    EcoHttpMeth_Head,
    EcoHttpMeth_Put,
} EcoHttpMeth;

typedef enum _EcoHttpReqOpt {
    EcoHttpReqOpt_Scheme,
    EcoHttpReqOpt_Url,
    EcoHttpReqOpt_Host,
    EcoHttpReqOpt_Port,
    EcoHttpReqOpt_Addr,
    EcoHttpReqOpt_Path,
    EcoHttpReqOpt_Query,
    EcoHttpReqOpt_Method,
    EcoHttpReqOpt_Version,
    EcoHttpReqOpt_Headers,
    EcoHttpReqOpt_BodyBuf,
    EcoHttpReqOpt_BodyLen,
} EcoHttpReqOpt;

typedef enum _EcoHttpCliOpt {
    EcoHttpCliOpt_ChanHookArg,
    EcoHttpCliOpt_ChanOpenHook,
    EcoHttpCliOpt_ChanCloseHook,
    EcoHttpCliOpt_ChanSetOptHook,
    EcoHttpCliOpt_ChanReadHook,
    EcoHttpCliOpt_ChanWriteHook,

    EcoHttpCliOpt_ReqHdrHookArg,
    EcoHttpCliOpt_ReqHdrHook,

    EcoHttpCliOpt_RspHdrHookArg,
    EcoHttpCliOpt_RspHdrHook,

    EcoHttpCliOpt_BodyHookArg,
    EcoHttpCliOpt_BodyWriteHook,

    EcoHttpCliOpt_KeepAlive,
    EcoHttpCliOpt_Request,

    /* Set send chunk buffer capacity (in bytes).

       This option will clear all data
       in the send chunk buffer. */
    EcoHttpCliOpt_SndChunkCap,
} EcoHttpCliOpt;

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

typedef struct _EcoChanAddr {
    uint8_t addr[4];
    uint16_t port;
} EcoChanAddr;

typedef struct _EcoHttpReq {
    EcoScheme scheme;
    EcoHttpMeth meth;
    EcoChanAddr chanAddr;

    char *pathBuf;
    size_t pathLen;

    char *queryBuf;
    size_t queryLen;

    EcoHttpVer ver;

    EcoHdrTab *hdrTab;

    /* Body field is not dynamicly allocated. */
    uint8_t *bodyBuf;
    size_t bodyLen;
} EcoHttpReq;

typedef enum _EcoStatCode {
    EcoStatCode_Unknown             = -1,

    EcoStatCode_Ok                  = 200,
    EcoStatCode_PartialContent      = 206,
    EcoStatCode_BadRequest          = 400,
    EcoStatCode_Unauthorized        = 401,
    EcoStatCode_NotFound            = 404,
    EcoStatCode_ServerError         = 500,
} EcoStatCode;

typedef struct _EcoHttpRsp {
    EcoHttpVer ver;
    EcoStatCode statCode;
    EcoHdrTab *hdrTab;
    size_t contLen;
    uint8_t *bodyBuf;
    size_t bodyLen;
} EcoHttpRsp;

typedef EcoRes (*EcoChanOpenHook)(EcoChanAddr *addr, EcoArg arg);

typedef EcoRes (*EcoChanCloseHook)(EcoArg arg);

/**
 * @brief User defined channel option setting hook function.
 * 
 * @param opt Option to set.
 * @param arg Option data to set.
 * @param hookArg Extra user data which can be set by option `EcoOpt_ChanHookArg`.
 * 
 * @return `EcoRes_Ok` for success, otherwise an error code.
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
 * @return `EcoRes_Ok` for success, otherwise an error code.
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
 * @return `EcoRes_Ok` for success, otherwise an error code.
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
    EcoHttpReq *req;
    EcoHttpRsp *rsp;

    uint8_t *sndChunkBuf;   // Send chunk buffer.
    size_t sndChunkCap;     // Send chunk buffer capacity.
    size_t sndChunkLen;     // Send chunk buffer data length.

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

    /* Flags. */
    uint32_t chanOpened: 1;
    uint32_t keepAlive: 1;
} EcoHttpCli;

/**
 * @brief Convert `EcoRes` to a descriptive string.
 * 
 * @param res Result to be converted.
 */
const char *EcoRes_ToStr(EcoRes res);



/**
 * @brief Initialize a header table.
 * 
 * @param tab Header table.
 */
void EcoHdrTab_Init(EcoHdrTab *tab);

/**
 * @brief Create a new header table.
 */
EcoHdrTab *EcoHdrTab_New(void);

/**
 * @brief Deinitialize a header table.
 * 
 * @param tab Header table.
 */
void EcoHdrTab_Deinit(EcoHdrTab *tab);

/**
 * @brief Delete a header table.
 * 
 * @param tab Header table.
 */
void EcoHdrTab_Del(EcoHdrTab *tab);

/**
 * @brief Add a header to the header table.
 * @note Header key and value should be passed separately.
 * 
 * @param tab Header table.
 * @param key Key string.
 * @param val Value string.
 */
EcoRes EcoHdrTab_Add(EcoHdrTab *tab, const char *key, const char *val);

/**
 * @brief Add a header to the header table.
 * @note `line` is a string consists both header key and value.
 *        For example: "Accept: application/json".
 * 
 * @param tab Header table.
 * @param line Header string.
 */
EcoRes EcoHdrTab_AddLine(EcoHdrTab *tab, const char *line);

/**
 * @brief Add a header to the header table.
 * @note Header key and value should be passed separately, of which `fmt` is the
 *       format string of the header key.
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



/**
 * @brief Initialize a HTTP request.
 * 
 * @param req HTTP request.
 */
void EcoHttpReq_Init(EcoHttpReq *req);

/**
 * @brief Create a new HTTP request.
 */
EcoHttpReq *EcoHttpReq_New(void);

/**
 * @brief Deinitialize a HTTP request.
 * 
 * @param req HTTP request.
 */
void EcoHttpReq_Deinit(EcoHttpReq *req);

/**
 * @brief Delete a HTTP request.
 * @note This function will also delete the header table if it exists.
 * 
 * @param req HTTP request.
 */
void EcoHttpReq_Del(EcoHttpReq *req);

/**
 * @brief Set a HTTP request option.
 * 
 * @param req HTTP request.
 * @param opt Option to set.
 * @param arg Option data to set.
 * 
 * @return `EcoRes_Ok` for success, otherwise an error code.
 */
EcoRes EcoHttpReq_SetOpt(EcoHttpReq *req, EcoHttpReqOpt opt, EcoArg arg);



/**
 * @brief Initialize a HTTP response.
 * 
 * @param rsp HTTP response.
 */
void EcoHttpRsp_Init(EcoHttpRsp *rsp);

/**
 * @brief Create a new HTTP response.
 */
EcoHttpRsp *EcoHttpRsp_New(void);

/**
 * @brief Deinitialize a HTTP response.
 * 
 * @param rsp HTTP response.
 */
void EcoHttpRsp_Deinit(EcoHttpRsp *rsp);

/**
 * @brief Delete a HTTP response.
 * @note This function will also delete the header
 *       table and body buffer if they exist.
 * 
 * @param rsp HTTP response.
 */
void EcoHttpRsp_Del(EcoHttpRsp *rsp);



/**
 * @brief Initialize a HTTP client.
 * 
 * @param cli HTTP client.
 */
void EcoHttpCli_Init(EcoHttpCli *cli);

/**
 * @brief Create a new HTTP client.
 */
EcoHttpCli *EcoHttpCli_New(void);

/**
 * @brief Deinitialize a HTTP client.
 * 
 * @param cli HTTP client.
 */
void EcoHttpCli_Deinit(EcoHttpCli *cli);

/**
 * @brief Delete a HTTP client.
 * @note This function will also delete the request and response if they exist.
 * 
 * @param cli HTTP client.
 */
void EcoHttpCli_Del(EcoHttpCli *cli);

/**
 * @brief Set a HTTP client option.
 * 
 * @param cli HTTP client.
 * @param opt Option to set.
 * @param arg Option data to set.
 * 
 * @return `EcoRes_Ok` for success, otherwise an error code.
 */
EcoRes EcoHttpCli_SetOpt(EcoHttpCli *cli, EcoHttpCliOpt opt, EcoArg arg);

/**
 * @brief Issue a HTTP request.
 * 
 * @param cli HTTP client.
 * 
 * @return `EcoRes_Ok` for success, otherwise an error code.
 */
EcoRes EcoHttpCli_Issue(EcoHttpCli *cli);

#endif
