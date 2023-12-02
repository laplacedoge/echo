#include "greatest.h"

void BasicHeaderSuite(void);

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(BasicHeaderSuite);

    GREATEST_MAIN_END();
}
