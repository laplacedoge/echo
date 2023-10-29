#include <stddef.h>
#include <stdint.h>

typedef enum _EcoRes {
    EcoRes_Ok               = 0,

    EcoRes_Err              = -1,
    EcoRes_NoMem            = -2,

    EcoRes_NotFound         = -3,

    EcoRes_BadOpt           = -4,

    EcoRes_ReachEnd         = -5,
} EcoRes;

typedef enum _EcoHttpVer {
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
    EcoOpt_Request,
    EcoOpt_BodyHookArg,
    EcoOpt_BodyWriteHook,
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



typedef struct _EcoHttpRsp {
    uint32_t placeholder;
} EcoHttpRsp;



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
    EcoArg bodyHookArg;
    EcoBodyWriteHook bodyWriteHook;
    EcoHttpReq *req;
} EcoHttpCli;

void EcoHttpCli_Init(EcoHttpCli *cli);

EcoHttpCli *EcoHttpCli_New(void);

void EcoHttpCli_Deinit(EcoHttpCli *cli);

void EcoHttpCli_Del(EcoHttpCli *cli);

EcoRes EcoHttpCli_SetOpt(EcoHttpCli *cli, EcoOpt opt, EcoArg arg);

EcoRes EcoHttpCli_Issue(EcoHttpCli *cli);
