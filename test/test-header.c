#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "echo.h"

void TestAddMoreHeader(void) {
    EcoHdrTab *tab;
    EcoRes res;

    tab = EcoHdrTab_New();
    assert(tab != NULL);
    assert(tab->kvpAry == NULL);
    assert(tab->kvpCap == 0);
    assert(tab->kvpNum == 0);

    res = EcoHdrTab_Add(tab, "Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 1);

    res = EcoHdrTab_Add(tab, "Accept-Encoding", "gzip, deflate, br");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 2);

    res = EcoHdrTab_Add(tab, "Accept-Language", "en");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 3);

    res = EcoHdrTab_Add(tab, "Cache-Control", "max-age=0");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 4);

    res = EcoHdrTab_Add(tab, "Connection", "keep-alive");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 5);

    res = EcoHdrTab_Add(tab, "User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/118.0.0.0 Safari/537.36");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 6);

    res = EcoHdrTab_Add(tab, "Sec-Ch-Ua", "\"Chromium\";v=\"118\", \"Google Chrome\";v=\"118\", \"Not=A?Brand\";v=\"99\"");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 7);

    res = EcoHdrTab_Add(tab, "Sec-Ch-Ua-Mobile", "?0");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 8);

    res = EcoHdrTab_Add(tab, "Sec-Ch-Ua-Platform", "\"Windows\"");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 9);

    res = EcoHdrTab_Add(tab, "Sec-Fetch-Dest", "document");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 10);

    EcoHdrTab_Del(tab);
    tab = NULL;
}

void TestAddDupHeader(void) {
    EcoHdrTab *tab;
    EcoRes res;

    tab = EcoHdrTab_New();
    assert(tab != NULL);
    assert(tab->kvpAry == NULL);
    assert(tab->kvpCap == 0);
    assert(tab->kvpNum == 0);

    res = EcoHdrTab_Add(tab, "Accept-Encoding", "gzip, deflate, br");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 1);

    res = EcoHdrTab_Add(tab, "Accept-Language", "en");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 2);

    res = EcoHdrTab_Add(tab, "Accept-Encoding", "gzip");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 2);

    EcoHdrTab_Del(tab);
    tab = NULL;
}

void TestDropHeader(void) {
    EcoHdrTab *tab;
    EcoRes res;

    tab = EcoHdrTab_New();
    assert(tab != NULL);
    assert(tab->kvpAry == NULL);
    assert(tab->kvpCap == 0);
    assert(tab->kvpNum == 0);

    res = EcoHdrTab_Add(tab, "Accept-Encoding", "gzip, deflate, br");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 1);

    res = EcoHdrTab_Add(tab, "Accept-Language", "en");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 2);

    res = EcoHdrTab_Drop(tab, "Accept-Encoding");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 1);

    EcoHdrTab_Del(tab);
    tab = NULL;
}

void TestFindHeader(void) {
    EcoHdrTab *tab;
    EcoRes res;

    tab = EcoHdrTab_New();
    assert(tab != NULL);
    assert(tab->kvpAry == NULL);
    assert(tab->kvpCap == 0);
    assert(tab->kvpNum == 0);

    res = EcoHdrTab_Add(tab, "Sec-Ch-Ua-Mobile", "?0");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 1);

    res = EcoHdrTab_Add(tab, "Sec-Ch-Ua-Platform", "\"Windows\"");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 2);

    res = EcoHdrTab_Add(tab, "Sec-Fetch-Dest", "document");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 3);

    res = EcoHdrTab_Find(tab, "Sec-Ch-Ua-Mobile", NULL);
    assert(res == EcoRes_Ok);

    res = EcoHdrTab_Find(tab, "Sec-Ch32d2-Ua-Md23dobile", NULL);
    assert(res == EcoRes_NotFound);

    EcoHdrTab_Del(tab);
    tab = NULL;
}

void TestClearHeader(void) {
    EcoHdrTab *tab;
    EcoRes res;

    tab = EcoHdrTab_New();
    assert(tab != NULL);
    assert(tab->kvpAry == NULL);
    assert(tab->kvpCap == 0);
    assert(tab->kvpNum == 0);

    res = EcoHdrTab_Add(tab, "Sec-Ch-Ua-Mobile", "?0");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 1);

    res = EcoHdrTab_Add(tab, "Sec-Ch-Ua-Platform", "\"Windows\"");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 2);

    res = EcoHdrTab_Add(tab, "Sec-Fetch-Dest", "document");
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry != NULL);
    assert(tab->kvpCap != 0);
    assert(tab->kvpNum == 3);

    EcoHdrTab_Clear(tab);
    assert(res == EcoRes_Ok);
    assert(tab->kvpAry == NULL);
    assert(tab->kvpCap == 0);
    assert(tab->kvpNum == 0);

    EcoHdrTab_Del(tab);
    tab = NULL;
}

int main(int argc, char *argv[]) {
    TestAddMoreHeader();

    TestAddDupHeader();

    TestDropHeader();

    TestFindHeader();

    TestClearHeader();

    return EXIT_SUCCESS;
}
