#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "echo.h"

#define EcoChk(cond)        assert(cond)

#define EcoPtrChk(cond)     assert((cond) != NULL)

#define EcoResChk(cond)     assert((cond) == EcoRes_Ok)

void TestReqHostSetting(void) {
    EcoHttpReq *req;

    EcoPtrChk(req = EcoHttpReq_New());

    /* Normal host string. */
    EcoResChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Host, "192.168.8.47"));
    EcoResChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Host, "255.255.255.255"));
    EcoResChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Host, "10.10.10.1"));
    EcoResChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Host, "8.8.8.8"));
    EcoResChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Host, "0.0.0.0"));

    /* Weird host string. */
    EcoChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Host, "02.0.0.0") == EcoRes_BadChar);
    EcoChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Host, "0.04.0.0") == EcoRes_BadChar);
    EcoChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Host, "0.0.0.009") == EcoRes_BadChar);
    EcoChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Host, "0.0.") == EcoRes_BadFmt);
    EcoChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Host, "") == EcoRes_BadFmt);
    EcoChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Host, "....") == EcoRes_BadChar);
    EcoChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Host, "3.3..") == EcoRes_BadChar);

    /* Number overflow. */
    EcoChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Host, "265.0.0.0") == EcoRes_BadFmt);
    EcoChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Host, "9999.9999.09999.999") == EcoRes_BadFmt);
    EcoChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Host, "128.99.456.72") == EcoRes_BadFmt);

    EcoHttpReq_Del(req);
}

void TestSetRequestParam(void) {
    EcoHttpReq *req;
    EcoRes res;

    EcoPtrChk(req = EcoHttpReq_New());

    EcoResChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Method, (EcoArg)EcoHttpMeth_Get));
    EcoChk(req->meth == EcoHttpMeth_Get);

    EcoResChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Verion, (EcoArg)EcoHttpVer_1_1));
    EcoChk(req->ver == EcoHttpVer_1_1);

    EcoResChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "http://192.168.1.47:8080/api/3/query-status?name=hello&age=18"));
    EcoChk(memcmp(req->chanAddr.addr, (uint8_t [4]){192, 168, 1, 47}, 4) == 0);
    EcoChk(req->chanAddr.port == 8080);
    EcoChk(strcmp(req->pathBuf, "/api/3/query-status") == 0);
    EcoChk(req->pathLen == 19);

    EcoResChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "http://192.168.1.47:8080/"));
    EcoChk(memcmp(req->chanAddr.addr, (uint8_t [4]){192, 168, 1, 47}, 4) == 0);
    EcoChk(req->chanAddr.port == 8080);
    EcoChk(strcmp(req->pathBuf, "/") == 0);
    EcoChk(req->pathLen == 1);

    EcoResChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "http://192.168.1.47:8080"));
    EcoChk(memcmp(req->chanAddr.addr, (uint8_t [4]){192, 168, 1, 47}, 4) == 0);
    EcoChk(req->chanAddr.port == 8080);
    EcoChk(strcmp(req->pathBuf, "/") == 0);
    EcoChk(req->pathLen == 1);

    EcoResChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "http://192.168.8.224"));
    EcoChk(memcmp(req->chanAddr.addr, (uint8_t [4]){192, 168, 8, 224}, 4) == 0);
    EcoChk(req->chanAddr.port == 80);
    EcoChk(strcmp(req->pathBuf, "/") == 0);
    EcoChk(req->pathLen == 1);

    EcoResChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "http://0.0.0.0:0"));
    EcoChk(memcmp(req->chanAddr.addr, (uint8_t [4]){0, 0, 0, 0}, 4) == 0);
    EcoChk(req->chanAddr.port == 0);
    EcoChk(strcmp(req->pathBuf, "/") == 0);
    EcoChk(req->pathLen == 1);

    EcoChk(EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "http://0.d0.0.0:0") == EcoRes_BadChar);

    EcoHttpReq_Del(req);
}

int main(int argc, char *argv[]) {
    TestReqHostSetting();
    TestSetRequestParam();

    return EXIT_SUCCESS;
}
