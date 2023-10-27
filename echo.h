#include <stdint.h>

typedef enum _EcoRes {
    EcoRes_Ok               = 0,

    EcoRes_Err              = -1,
    EcoRes_NoMem            = -2,
} EcoRes;

typedef enum _EcoOpt {
    EcoOpt_HttpHeader
} EcoOpt;



typedef struct _EcoHttpReq {
    uint32_t placeholder;
} EcoHttpReq;



typedef struct _EcoHttpRsp {
    uint32_t placeholder;
} EcoHttpRsp;



typedef struct _EcoHttpCli {
    uint32_t placeholder;
} EcoHttpCli;





EcoHttpReq *EcoHttpReq_New(void);

void EcoHttpReq_Del(EcoHttpReq *req);



EcoHttpCli *EcoHttpCli_New(void);

void EcoHttpCli_Del(EcoHttpCli *cli);
