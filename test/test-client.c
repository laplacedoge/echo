#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "echo.h"

void TestSetClientParam(void) {
    EcoHttpCli *cli;
    EcoRes res;

    cli = EcoHttpCli_New();
    assert(cli != NULL);

    res = EcoHttpCli_SetOpt(cli, EcoOpt_ChanHookArg, NULL);
    assert(res == EcoRes_Ok);
    assert(cli->chanHookArg == NULL);

    res = EcoHttpCli_SetOpt(cli, EcoOpt_ChanOpenHook, NULL);
    assert(res == EcoRes_Ok);
    assert(cli->chanOpenHook == NULL);

    res = EcoHttpCli_SetOpt(cli, EcoOpt_ChanCloseHook, NULL);
    assert(res == EcoRes_Ok);
    assert(cli->chanCloseHook == NULL);

    res = EcoHttpCli_SetOpt(cli, EcoOpt_ChanSetOptHook, NULL);
    assert(res == EcoRes_Ok);
    assert(cli->chanSetOptHook == NULL);

    res = EcoHttpCli_SetOpt(cli, EcoOpt_ChanReadHook, NULL);
    assert(res == EcoRes_Ok);
    assert(cli->chanReadHook == NULL);

    res = EcoHttpCli_SetOpt(cli, EcoOpt_ChanWriteHook, NULL);
    assert(res == EcoRes_Ok);
    assert(cli->chanWriteHook == NULL);

    res = EcoHttpCli_SetOpt(cli, EcoOpt_Request, NULL);
    assert(res == EcoRes_Ok);
    assert(cli->req == NULL);

    res = EcoHttpCli_SetOpt(cli, EcoOpt_BodyHookArg, NULL);
    assert(res == EcoRes_Ok);
    assert(cli->bodyHookArg == NULL);

    res = EcoHttpCli_SetOpt(cli, EcoOpt_BodyWriteHook, NULL);
    assert(res == EcoRes_Ok);
    assert(cli->bodyWriteHook == NULL);

    EcoHttpCli_Del(cli);
    cli = NULL;
}

int main(int argc, char *argv[]) {
    TestSetClientParam();

    return EXIT_SUCCESS;
}
