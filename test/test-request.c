#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "echo.h"

void TestSetRequestParam(void) {
    EcoHttpReq *req;
    EcoRes res;

    req = EcoHttpReq_New();
    assert(req != NULL);

    res = EcoHttpReq_SetOpt(req, EcoOpt_Method, (EcoArg)EcoHttpMeth_Get);
    assert(res == EcoRes_Ok);
    assert(req->meth == EcoHttpMeth_Get);

    res = EcoHttpReq_SetOpt(req, EcoOpt_Verion, (EcoArg)EcoHttpVer_1_1);
    assert(res == EcoRes_Ok);
    assert(req->ver == EcoHttpVer_1_1);

    res = EcoHttpReq_SetOpt(req, EcoOpt_Url, "192.168.1.47:8080");
    assert(res == EcoRes_Ok);
    assert(memcmp(req->chanAddr.addr, (uint8_t [4]){192, 168, 1, 47}, 4) == 0);
    assert(req->chanAddr.port == 8080);

    res = EcoHttpReq_SetOpt(req, EcoOpt_Url, "192.168.8.224");
    assert(res == EcoRes_Ok);
    assert(memcmp(req->chanAddr.addr, (uint8_t [4]){192, 168, 8, 224}, 4) == 0);
    assert(req->chanAddr.port == 80);

    res = EcoHttpReq_SetOpt(req, EcoOpt_Url, "0.0.0.0:0");
    assert(res == EcoRes_Ok);
    assert(memcmp(req->chanAddr.addr, (uint8_t [4]){0, 0, 0, 0}, 4) == 0);
    assert(req->chanAddr.port == 0);

    res = EcoHttpReq_SetOpt(req, EcoOpt_Url, "0.d0.0.0:0");
    assert(res == EcoRes_Err);

    EcoHttpReq_Del(req);
    req = NULL;
}

int main(int argc, char *argv[]) {
    TestSetRequestParam();

    return EXIT_SUCCESS;
}
