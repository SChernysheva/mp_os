#include <gtest/gtest.h>
#include "server_logger.h"


int main(
    int argc,
    char *argv[])
{
    server_logger* server = new server_logger();
    server->init();
    // testing::InitGoogleTest(&argc, argv);

    // return RUN_ALL_TESTS();
}