#include "../include/client_logger.h"
#include "../include/client_logger_builder.h"
//#include "json_builder_concrete.h"

int main()
{
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

    return 0;
}


//хранить коллекцию фалйа и его счетчик ссылок(кол-во логгеров)