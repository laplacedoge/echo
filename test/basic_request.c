#include <stdint.h>

#include "conf.h"
#include "echo.h"

#include "greatest.h"

TEST SetCommonUrl(void) {
    uint8_t ipAddr[4];
    EcoHttpReq *req;
    EcoRes res;

    req = EcoHttpReq_New();
    ASSERT_NEQ(NULL, req);

    memcpy(ipAddr, (uint8_t [4]){127, 0, 0, 1}, 4);

    res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "http://127.0.0.1:80/index.html");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(EcoScheme_Http, req->scheme, "%d");
    ASSERT_MEM_EQ(ipAddr, req->chanAddr.addr, 4);
    ASSERT_EQ_FMT(80, req->chanAddr.port, "%d");

    res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "https://127.0.0.1:443/index.html");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(EcoScheme_Https, req->scheme, "%d");
    ASSERT_MEM_EQ(ipAddr, req->chanAddr.addr, 4);
    ASSERT_EQ_FMT(443, req->chanAddr.port, "%d");

    res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "http://127.0.0.1:80/favicon.ico");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(EcoScheme_Http, req->scheme, "%d");
    ASSERT_MEM_EQ(ipAddr, req->chanAddr.addr, 4);
    ASSERT_EQ_FMT(80, req->chanAddr.port, "%d");

    res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "https://127.0.0.1:443/favicon.ico");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(EcoScheme_Https, req->scheme, "%d");
    ASSERT_MEM_EQ(ipAddr, req->chanAddr.addr, 4);
    ASSERT_EQ_FMT(443, req->chanAddr.port, "%d");

    res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "127.0.0.1:443/index.html");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(ECO_CONF_DEF_SCHEME, req->scheme, "%d");
    ASSERT_MEM_EQ(ipAddr, req->chanAddr.addr, 4);
    ASSERT_EQ_FMT(443, req->chanAddr.port, "%d");

    res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "127.0.0.1/index.html");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(ECO_CONF_DEF_SCHEME, req->scheme, "%d");
    ASSERT_MEM_EQ(ipAddr, req->chanAddr.addr, 4);
    if (ECO_CONF_DEF_SCHEME == EcoScheme_Http) {
        ASSERT_EQ_FMT(ECO_CONF_DEF_HTTP_PORT, req->chanAddr.port, "%d");
    } else if (ECO_CONF_DEF_SCHEME == EcoScheme_Https) {
        ASSERT_EQ_FMT(ECO_CONF_DEF_HTTPS_PORT, req->chanAddr.port, "%d");
    }

    EcoHttpReq_Del(req);

    PASS();
}

TEST SetInvalidUrl(void) {
    uint8_t ipAddr[4];
    EcoHttpReq *req;
    EcoRes res;

    req = EcoHttpReq_New();
    ASSERT_NEQ(NULL, req);

    res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "httq://127.0.0.1:80/index.html");
    ASSERT_EQ_FMT(EcoRes_BadScheme, res, "%d");

    res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "http://299.0.0.1:80/index.html");
    ASSERT_EQ_FMT(EcoRes_BadHost, res, "%d");

    res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "http://127.0.0.1.1:80/index.html");
    ASSERT_EQ_FMT(EcoRes_BadHost, res, "%d");

    res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "http://127.0.0.1:80000/index.html");
    ASSERT_EQ_FMT(EcoRes_BadPort, res, "%d");

    res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "http://127.0.0.1-80index.html");
    ASSERT_NEQ(EcoRes_Ok, res);

    res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, "http://127.0.0.1:80index.html");
    ASSERT_NEQ(EcoRes_Ok, res);

    EcoHttpReq_Del(req);

    PASS();
}

SUITE(BasicRequestSuite) {
    RUN_TEST(SetCommonUrl);
    RUN_TEST(SetInvalidUrl);
}
