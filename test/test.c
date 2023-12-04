#include "greatest.h"

void BasicHeaderSuite(void);

void BasicRequestSuite(void);

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(BasicHeaderSuite);
    RUN_SUITE(BasicRequestSuite);

    GREATEST_MAIN_END();
}
