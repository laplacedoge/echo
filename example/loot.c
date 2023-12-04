#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <stdio.h>

#include "echo.h"

static bool gHelpNeeded = false;

static char *gOutputFilePath = "trophy";

static FILE *gLogFileStm = NULL;

static EcoHdrTab *gReqHdrTab = NULL;

static int gUrlArgIdx = -1;

static int gUrlArgNum = 0;

#define EOL "\n"

#define VERSION_MAJOR "1"

#define VERSION_MINOR "0"

#define VERSION_PATCH "0"

#define VERSION VERSION_MAJOR "." \
                VERSION_MINOR "." \
                VERSION_PATCH

#define HELP    "Loot " VERSION ", a tiny HTTP(S) client." EOL \
                EOL \
                "Usage: loot [OPTIONS] <url>" EOL \
                EOL \
                "Options:" EOL \
                "    -h, --help" EOL \
                "        Print this help message." EOL \
                EOL \
                "    -o, --output-file" EOL \
                "        Output file." EOL \
                EOL \
                "    -H, --header" EOL \
                "        Request header." EOL \

#define Log(fmt, ...) \
    fprintf(gLogFileStm, fmt EOL, ##__VA_ARGS__)

static int InitParam(void) {
    gLogFileStm = stderr;

    gReqHdrTab = EcoHdrTab_New();
    if (gReqHdrTab == NULL) {
        Log("Failed to create request header table!");

        return -1;
    }

    return 0;
}

static int ParseParam(int argc, char **argv) {
    int ret;

    if (argc == 1) {
        Log("No argument specified!");

        goto ErrExit;
    }

    while (true) {
        const struct option longOpts[] = {
            { "help", no_argument, NULL, 'h'},
            { "output-file", required_argument, NULL, 'o'},
            { "header", required_argument, NULL, 'H'},
            { NULL, 0, NULL, 0}
        };
        EcoRes res;
        int optIdx;
        int ch;

        opterr = 0;

        ch = getopt_long(argc, argv, "ho:H:", longOpts, &optIdx);
        if (ch == -1) {
            break;
        }

        switch (ch) {
            case 'h':
                gHelpNeeded = true;

                break;

            case 'o':
                gOutputFilePath = optarg;

                break;

            case 'H':
                res = EcoHdrTab_AddLine(gReqHdrTab, optarg);
                if (res != EcoRes_Ok) {
                    const char *reason;

                    switch (res) {
                    case EcoRes_BadHdrLine:
                        reason = "Invalid header format";
                        break;

                    case EcoRes_BadHdrKey:
                        reason = "Invalid key format";
                        break;

                    case EcoRes_BadHdrVal:
                        reason = "Invalid value format";
                        break;

                    default:
                        reason = "Unknown error";
                        break;
                    }

                    Log("Failed to add header line \"%s\": %s!", optarg, reason);
                }

                break;

            case '?':
                Log("Unknown option \"%s\"!", argv[optind - 1]);

                break;

            default:
                goto ErrExit;
        }
    }

    gUrlArgNum = argc - optind;
    if (gHelpNeeded == false &&
        gUrlArgNum == 0) {
        Log("No URL specified!");

        goto ErrExit;
    }

    gUrlArgIdx = optind;

    return 0;

ErrExit:

    return -1;
}

static void ShowReqHeader(void) {
    Log("Using request header:");

    for (size_t i = 0; i < gReqHdrTab->kvpNum; i++) {
        EcoKvp *kvp = gReqHdrTab->kvpAry + i;

        Log("    %s: %s", kvp->keyBuf, kvp->valBuf);
    }
}

static void DeinitParam(void) {
    if (gReqHdrTab != NULL) {
        EcoHdrTab_Del(gReqHdrTab);
    }

    if (gLogFileStm != stdout ||
        gLogFileStm != stderr) {
        fclose(gLogFileStm);
    }
}

EcoRes LootChanOpenHook(EcoChanAddr *addr, void *arg) {
    struct sockaddr_in srvAddr;
    int opt = 1;
    int sockFd;
    int ret;

    sockFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockFd == -1) {
        return EcoRes_Err;
    }

    ret = setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    if (ret != 0) {
        close(sockFd);

        return EcoRes_Err;
    }

    srvAddr.sin_family = AF_INET;
    srvAddr.sin_port = htons(addr->port);
    memcpy(&srvAddr.sin_addr, &addr->addr, 4);

    ret = connect(sockFd, (struct sockaddr *)&srvAddr, sizeof(srvAddr));
    if (ret != 0) {
        close(sockFd);

        return EcoRes_Err;
    }

    *(int *)arg = sockFd;

    return EcoRes_Ok;
}

int LootChanReadHook(void *buf, int len, void *arg) {
    int sockFd;
    int ret;

    sockFd = *(int *)arg;

    ret = (int)read(sockFd, buf, (size_t)len);
    if (ret <= 0) {
        if (ret == 0) {
            return EcoRes_ReachEnd;
        }

        return EcoRes_Err;
    }

    return ret;
}

int LootChanWriteHook(const void *buf, int len, void *arg) {
    int sockFd;
    int ret;

    sockFd = *(int *)arg;

    ret = (int)write(sockFd, buf, (size_t)len);
    if (ret <= 0) {
        if (ret == 0) {
            return EcoRes_ReachEnd;
        }

        return EcoRes_Err;
    }

    return ret;
}

EcoRes LootChanCloseHook(void *arg) {
    int sockFd;
    int ret;

    sockFd = *(int *)arg;

    ret = close(sockFd);
    if (ret != 0) {
        return EcoRes_Err;
    }

    return EcoRes_Ok;
}

static int SaveToFile(EcoHttpRsp *rsp, const char *path) {
    ssize_t wrLen;
    int fd;

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        Log("Failed to open file \"%s\"!", path);

        return -1;
    }

    wrLen = write(fd, rsp->bodyBuf, rsp->bodyLen);
    if (wrLen != (ssize_t)rsp->bodyLen) {
        Log("Failed to write to file \"%s\"!", path);

        close(fd);

        return -1;
    }

    close(fd);

    return 0;
}

int main(int argc, char **argv) {
    EcoHttpReq *req;
    EcoHttpCli *cli;
    int socketFd;
    EcoRes res;
    int ret;

    req = EcoHttpReq_New();
    if (req == NULL) {
        Log("Failed to create request!");

        return EXIT_FAILURE;
    }

    EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Headers, gReqHdrTab);

    cli = EcoHttpCli_New();
    if (cli == NULL) {
        Log("Failed to create client!");

        return EXIT_FAILURE;
    }

    EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_Request, req);

    ret = InitParam();
    if (ret != 0) {
        return EXIT_FAILURE;
    }

    ret = ParseParam(argc, argv);
    if (ret != 0) {
        return EXIT_FAILURE;
    }

    if (gHelpNeeded) {
        puts(HELP);

        return EXIT_SUCCESS;
    }

    ShowReqHeader();

    EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_ChanHookArg, &socketFd);
    EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_ChanOpenHook, LootChanOpenHook);
    EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_ChanCloseHook, LootChanCloseHook);
    EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_ChanReadHook, LootChanReadHook);
    EcoHttpCli_SetOpt(cli, EcoHttpCliOpt_ChanWriteHook, LootChanWriteHook);

    EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Verion, (EcoArg)EcoHttpVer_1_1);
    EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Method, (EcoArg)EcoHttpMeth_Get);

    if (gUrlArgNum != 0) {
        for (int i = gUrlArgIdx; i < argc; i++) {
            res = EcoHttpReq_SetOpt(req, EcoHttpReqOpt_Url, argv[i]);
            if (res != EcoRes_Ok) {
                Log("Failed to set URL \"%s\": %s.", argv[i], EcoRes_ToStr(res));

                continue;
            }

            Log("Issuing URL \"%s\"...", argv[i]);

            res = EcoHttpCli_Issue(cli);
            if (res != EcoRes_Ok) {
                Log("    Failed to issue URL \"%s\": %s.", argv[i], EcoRes_ToStr(res));

                continue;
            }

            Log("    Done!");

            Log("Saving to \"%s\"...", gOutputFilePath);

            ret = SaveToFile(cli->rsp, gOutputFilePath);
            if (ret != 0) {
                Log("    Failed to save to \"%s\"!", gOutputFilePath);

                continue;
            }

            Log("    Done!");
        }
    }

    EcoHttpCli_Del(cli);

    DeinitParam();
}
