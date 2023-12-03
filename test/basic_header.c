#include "echo.h"

#include "greatest.h"

TEST AddCommonHeaderSeparately(void) {
    EcoHdrTab *tab;
    EcoRes res;

    tab = EcoHdrTab_New();
    ASSERT_NEQ(NULL, tab);

    res = EcoHdrTab_Add(tab, "Accept", "application/json");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept", "text/html");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept", "image/png");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept-Charset", "UTF-8");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept-Charset", "ISO-8859-1");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept-Encoding", "gzip");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept-Encoding", "deflate");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept-Language", "en-US");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept-Language", "fr-FR");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Authorization", "Basic dXNlcjpwYXNzd29yZA==");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Cache-Control", "no-cache");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Cache-Control", "max-age=3600");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Connection", "keep-alive");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Connection", "close");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Content-Encoding", "gzip");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Content-Encoding", "identity");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Content-Type", "2048");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Cookie", "sessionID=abc123");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Date", "Sun, 01 Jan 2023 12:00:00 GMT");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "ETag", "\"1234567890\"");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Expire", "Sat, 01 Jan 2023 12:00:00 GMT");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Host", "www.example.com");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Server", "Illapa/3.8.23 (Unix)");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "Set-Cookie", "id=a3fWa; Expires=Wed, 21 Oct 2015 07:28:00 GMT");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    res = EcoHdrTab_Add(tab, "User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36 Edg/91.0.864.59");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    EcoHdrTab_Del(tab);

    PASS();
}

TEST AddWeirdHeaderSeparately(void) {
    EcoHdrTab *tab;
    EcoRes res;

    tab = EcoHdrTab_New();
    ASSERT_NEQ(NULL, tab);

    res = EcoHdrTab_Add(tab, "Accept", "");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");

    EcoHdrTab_Del(tab);

    PASS();
}

TEST AddInvalidHeaderKeySeparately(void) {
    EcoHdrTab *tab;
    EcoRes res;

    tab = EcoHdrTab_New();
    ASSERT_NEQ(NULL, tab);

    res = EcoHdrTab_Add(tab, "(Accept", "text/html");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept)", "text/html");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "<Accept", "text/html");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept>", "text/html");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "Acc@ept", "text/html");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept,", "text/html");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept;", "text/html");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept:", "image/png");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "\\Accept", "image/png");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "\"Accept\"", "image/png");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept/", "image/png");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "[Accept", "image/png");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept]", "image/png");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept?", "image/png");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept=", "image/png");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "{Accept", "image/png");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept}", "image/png");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "Acc ept", "image/png");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept\r", "image/png");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept\n", "image/png");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept\e", "image/png");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept\x7F", "image/png");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    EcoHdrTab_Del(tab);

    PASS();
}

TEST AddInvalidHeaderValueSeparately(void) {
    EcoHdrTab *tab;
    EcoRes res;

    tab = EcoHdrTab_New();
    ASSERT_NEQ(NULL, tab);

    res = EcoHdrTab_Add(tab, "Accept", "text/html\r");
    ASSERT_EQ_FMT(EcoRes_BadHdrVal, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept", "text/html\n");
    ASSERT_EQ_FMT(EcoRes_BadHdrVal, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept", "text/html\e");
    ASSERT_EQ_FMT(EcoRes_BadHdrVal, res, "%d");

    res = EcoHdrTab_Add(tab, "Accept", "text/html\x7F");
    ASSERT_EQ_FMT(EcoRes_BadHdrVal, res, "%d");

    EcoHdrTab_Del(tab);

    PASS();
}

TEST OverwriteHeaderSeparately(void) {
    EcoHdrTab *tab;
    EcoRes res;

    tab = EcoHdrTab_New();
    ASSERT_NEQ(NULL, tab);

    res = EcoHdrTab_Add(tab, "Accept", "application/json");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(1UL, tab->kvpNum, "%zu");
    ASSERT_STR_EQ("accept", tab->kvpAry[0].keyBuf);
    ASSERT_STR_EQ("application/json", tab->kvpAry[0].valBuf);

    res = EcoHdrTab_Add(tab, "Accept", "text/html");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(1UL, tab->kvpNum, "%zu");
    ASSERT_STR_EQ("accept", tab->kvpAry[0].keyBuf);
    ASSERT_STR_EQ("text/html", tab->kvpAry[0].valBuf);

    res = EcoHdrTab_Add(tab, "Accept", "image/png");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(1UL, tab->kvpNum, "%zu");
    ASSERT_STR_EQ("accept", tab->kvpAry[0].keyBuf);
    ASSERT_STR_EQ("image/png", tab->kvpAry[0].valBuf);

    res = EcoHdrTab_Add(tab, "Accept-Charset", "UTF-8");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(2UL, tab->kvpNum, "%zu");
    ASSERT_STR_EQ("accept-charset", tab->kvpAry[1].keyBuf);
    ASSERT_STR_EQ("UTF-8", tab->kvpAry[1].valBuf);

    res = EcoHdrTab_Add(tab, "Accept-Charset", "ISO-8859-1");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(2UL, tab->kvpNum, "%zu");
    ASSERT_STR_EQ("accept-charset", tab->kvpAry[1].keyBuf);
    ASSERT_STR_EQ("ISO-8859-1", tab->kvpAry[1].valBuf);

    res = EcoHdrTab_Add(tab, "Accept-Encoding", "gzip");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(3UL, tab->kvpNum, "%zu");
    ASSERT_STR_EQ("accept-encoding", tab->kvpAry[2].keyBuf);
    ASSERT_STR_EQ("gzip", tab->kvpAry[2].valBuf);

    res = EcoHdrTab_Add(tab, "Accept-Encoding", "deflate");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(3UL, tab->kvpNum, "%zu");
    ASSERT_STR_EQ("accept-encoding", tab->kvpAry[2].keyBuf);
    ASSERT_STR_EQ("deflate", tab->kvpAry[2].valBuf);

    res = EcoHdrTab_Add(tab, "Accept-Language", "en-US");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(4UL, tab->kvpNum, "%zu");
    ASSERT_STR_EQ("accept-language", tab->kvpAry[3].keyBuf);
    ASSERT_STR_EQ("en-US", tab->kvpAry[3].valBuf);

    res = EcoHdrTab_Add(tab, "Accept-Language", "fr-FR");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(4UL, tab->kvpNum, "%zu");
    ASSERT_STR_EQ("accept-language", tab->kvpAry[3].keyBuf);
    ASSERT_STR_EQ("fr-FR", tab->kvpAry[3].valBuf);

    EcoHdrTab_Del(tab);

    PASS();
}

TEST AddValidHeaderLine(void) {
    EcoHdrTab *tab;
    EcoRes res;

    tab = EcoHdrTab_New();
    ASSERT_NEQ(NULL, tab);

    res = EcoHdrTab_AddLine(tab, "Accept: application/json");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(1UL, tab->kvpNum, "%zu");
    ASSERT_STR_EQ("accept", tab->kvpAry[0].keyBuf);
    ASSERT_STR_EQ("application/json", tab->kvpAry[0].valBuf);

    res = EcoHdrTab_AddLine(tab, "Accept: text/html");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(1UL, tab->kvpNum, "%zu");
    ASSERT_STR_EQ("accept", tab->kvpAry[0].keyBuf);
    ASSERT_STR_EQ("text/html", tab->kvpAry[0].valBuf);

    res = EcoHdrTab_AddLine(tab, "Accept-Charset:UTF-8");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(2UL, tab->kvpNum, "%zu");
    ASSERT_STR_EQ("accept-charset", tab->kvpAry[1].keyBuf);
    ASSERT_STR_EQ("UTF-8", tab->kvpAry[1].valBuf);

    res = EcoHdrTab_AddLine(tab, "Accept-Encoding:             gzip");
    ASSERT_EQ_FMT(EcoRes_Ok, res, "%d");
    ASSERT_EQ_FMT(3UL, tab->kvpNum, "%zu");
    ASSERT_STR_EQ("accept-encoding", tab->kvpAry[2].keyBuf);
    ASSERT_STR_EQ("gzip", tab->kvpAry[2].valBuf);

    EcoHdrTab_Del(tab);

    PASS();
}

TEST AddInvalidHeaderLine(void) {
    EcoHdrTab *tab;
    EcoRes res;

    tab = EcoHdrTab_New();
    ASSERT_NEQ(NULL, tab);

    res = EcoHdrTab_AddLine(tab, "Accept");
    ASSERT_EQ_FMT(EcoRes_BadHdrLine, res, "%d");

    res = EcoHdrTab_AddLine(tab, "Accept;: application/json");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_AddLine(tab, "{Accept: application/json");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_AddLine(tab, "Accept]: application/json");
    ASSERT_EQ_FMT(EcoRes_BadHdrKey, res, "%d");

    res = EcoHdrTab_AddLine(tab, "Accept: \rapplication/json");
    ASSERT_EQ_FMT(EcoRes_BadHdrVal, res, "%d");

    res = EcoHdrTab_AddLine(tab, "Accept: application/json\n");
    ASSERT_EQ_FMT(EcoRes_BadHdrVal, res, "%d");

    EcoHdrTab_Del(tab);

    PASS();
}

SUITE(BasicHeaderSuite) {
    RUN_TEST(AddCommonHeaderSeparately);
    RUN_TEST(AddWeirdHeaderSeparately);
    RUN_TEST(AddInvalidHeaderKeySeparately);
    RUN_TEST(AddInvalidHeaderValueSeparately);
    RUN_TEST(OverwriteHeaderSeparately);
    RUN_TEST(AddValidHeaderLine);
    RUN_TEST(AddInvalidHeaderLine);
}
