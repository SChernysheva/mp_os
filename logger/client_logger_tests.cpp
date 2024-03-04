#include <gtest/gtest.h>
#include "client_logger.h"
#include "client_logger_builder.h"


int main(
    int argc,
    char *argv[])
 {
//     testing::InitGoogleTest(&argc, argv);

//     return RUN_ALL_TESTS();
    logger_builder* builder = new client_logger_builder(); //map   key-path  value - pair<path, severity> 

    logger *constructed_logger = builder
        ->add_file_stream("file1.txt", logger::severity::information)
        ->add_file_stream("file1.txt", logger::severity::debug)
        ->add_file_stream("file2.txt", logger::severity::warning)
        ->add_console_stream(logger::severity::warning)
        ->build();


    constructed_logger->warning("Деление на 0!!");

    delete constructed_logger;
    delete builder;

}