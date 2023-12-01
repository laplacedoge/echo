
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "echo.h"

static bool gHelpNeeded = false;

static char *gOutputFile = NULL;

static FILE *gLogFileStm = NULL;

static EcoHdrTab *gReqHdrTab = NULL;

#define EOL "\n"

#define VERSION_MAJOR "1"

#define VERSION_MINOR "0"

#define VERSION_PATCH "0"

#define VERSION VERSION_MAJOR "." \
                VERSION_MINOR "." \
                VERSION_PATCH

#define HELP    EOL \
                "Loot " VERSION ", a tiny HTTP(S) client." EOL \
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

static int AppendHeader(char *header) {
    char *data = NULL;
    char *colon;
    EcoRes res;

    data = strdup(header);
    if (data == NULL) {
        goto ErrExit;
    }

    colon = strchr(data, ':');
    if (colon == NULL) {
        goto ErrExit;
    }

    *colon = '\0';

    res = EcoHdrTab_Add(gReqHdrTab, data, colon + 1);
    if (res != EcoRes_Ok) {
        goto ErrExit;
    }

    return 0;

ErrExit:
    if (data != NULL) {
        free(data);
    }

    return -1;
}

static int ParseParam(int argc, char **argv) {
    int ret;

    if (argc == 1) {
        Log("No argument specified!");

        goto ErrExit;
    }

    for (int i = 0; i < argc; i++) {
        char *arg = argv[i];

        if (strcmp(arg, "-h") == 0 ||
            strcmp(arg, "--help") == 0) {
            gHelpNeeded = true;
        } else if (strcmp(arg, "-o") == 0 ||
                   strcmp(arg, "--output-file") == 0) {
            if (i + 1 < argc) {
                gOutputFile = argv[i + 1];
                i = i + 1;
            } else {
                Log("Missing argument for option \"%s\"!", arg);

                goto ErrExit;
            }
        } else if (strcmp(arg, "-H") == 0 ||
                   strcmp(arg, "--header") == 0) {
            if (i + 1 >= argc) {
                Log("Missing argument for option \"%s\"!", arg);

                goto ErrExit;
            }

            ret = AppendHeader(argv[i + 1]);
            if (ret != 0) {
                Log("Failed to append header \"%s\"!", argv[i + 1]);

                goto ErrExit;
            }

            i++;
        }
    }

    return 0;

ErrExit:
    puts(HELP);

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

int main(int argc, char **argv) {
    int ret;

    ret = InitParam();
    if (ret != 0) {
        return EXIT_FAILURE;
    }

    ret = ParseParam(argc, argv);
    if (ret != 0) {
        return EXIT_FAILURE;
    }

    ShowReqHeader();

    if (gHelpNeeded) {
        puts(HELP);

        return EXIT_SUCCESS;
    }

    DeinitParam();
}





