#include <stdlib.h>
#include <stdio.h>

#include "echo.h"


int main(int argc, char *argv[]) {
    EcoHttpCli *cli = NULL;
    EcoHttpReq *req = NULL;
    EcoHttpRsp *rsp = NULL;
    EcoRes res;

    req = EcoHttpReq_New();
    if (req == NULL) {
        return EXIT_FAILURE;
    }

    res = EcoHttpReq_SetOpt();



    return EXIT_SUCCESS;
}












