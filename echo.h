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

typedef enum _EcoHttpMethod {
    EcoHttpMethod_Get,
    EcoHttpMethod_Post,
    EcoHttpMethod_Head,
} EcoHttpMethod;

typedef enum _EcoOpt {
    EcoOpt_Url,
    EcoOpt_Method,
    EcoOpt_Verion,
    EcoOpt_Header,
    EcoOpt_BodyBuf,
    EcoOpt_BodyLen,

    EcoOpt_Request,
    EcoOpt_ChanHookArg,
    EcoOpt_ChanOpenHook,
    EcoOpt_ChanCloseHook,
    EcoOpt_ChanSetOptHook,
    EcoOpt_ChanReadHook,
    EcoOpt_ChanWriteHook,

    EcoOpt_BodyHookArg,
    EcoOpt_BodyWriteHook,
} EcoOpt;



typedef void * EcoArg;


typedef enum _EcoChanOpt {
    EcoChanOpt_SyncReadWrite,
    EcoChanOpt_ReadWriteTimeout,
} EcoChanOpt;



typedef struct _EcoKvp {
    uint32_t placeholder;
} EcoKvp;

typedef struct _EcoHdrTab {
    uint32_t placeholder;
} EcoHdrTab;



void EcoHdrTab_Init(EcoHdrTab *tab);

EcoHdrTab *EcoHdrTab_New(void);

void EcoHdrTab_Deinit(EcoHdrTab *tab);

void EcoHdrTab_Del(EcoHdrTab *tab);

EcoRes EcoHdrTab_Add(EcoHdrTab *tab, const char *key, const char *val);

EcoRes EcoHdrTab_Drop(EcoHdrTab *tab, const char *key);

EcoRes EcoHdrTab_Clear(EcoHdrTab *tab);

EcoRes EcoHdrTab_Find(EcoHdrTab *tab, const char *key, EcoKvp *kvp);

EcoRes EcoHdrTab_Alter(EcoHdrTab *tab, const char *key, const char *val);

typedef struct _EcoHttpReq {
    uint32_t placeholder;
} EcoHttpReq;



typedef struct _EcoHttpRsp {
    uint32_t placeholder;
} EcoHttpRsp;



typedef struct _EcoHttpCli {
    uint32_t placeholder;
} EcoHttpCli;



typedef struct _EcoChanAddr {
    uint8_t addr[4];
    uint16_t port;
} EcoChanAddr;



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



EcoHttpReq *EcoHttpReq_New(void);

void EcoHttpReq_Del(EcoHttpReq *req);

EcoRes EcoHttpReq_SetOpt(EcoHttpReq *req, EcoOpt opt, EcoArg arg);



EcoHttpCli *EcoHttpCli_New(void);

void EcoHttpCli_Del(EcoHttpCli *cli);

EcoRes EcoHttpCli_SetOpt(EcoHttpCli *cli, EcoOpt opt, EcoArg arg);

EcoRes EcoHttpCli_Issue(EcoHttpCli *cli);
