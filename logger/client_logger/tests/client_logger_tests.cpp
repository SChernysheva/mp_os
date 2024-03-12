#include <gtest/gtest.h>
#include "client_logger.h"
#include "client_logger_builder.h"


int main(
    int argc,
    char *argv[])
 {
    testing::InitGoogleTest(&argc, argv);
    
    logger_builder* builder = new client_logger_builder(); //map   key-path  value - pair<path, severity> 

    logger *constructed_logger = builder
        ->add_file_stream("file1.txt", logger::severity::information)
        ->add_file_stream("file1.txt", logger::severity::debug)
        ->add_file_stream("file2.txt", logger::severity::warning)
        ->add_console_stream(logger::severity::warning)
        ->add_console_stream(logger::severity::debug)
        ->build();

    // logger* constructed_logger = builder
    // ->transform_with_configuration("json_file.json", "logger")
    // ->build();



    constructed_logger->warning("Деление на 0!!");
    constructed_logger->debug("Супер длинное сообщение которое я хочу чтобы было в файле номер 1 и в консоле лялялялялял");
    constructed_logger->warning("Wanring!");
    constructed_logger->warning("file1");
    constructed_logger->warning("file2");


    delete constructed_logger;
    delete builder;

    return RUN_ALL_TESTS();
    
}