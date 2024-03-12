//#include <not_implemented.h>

#include "../include/client_logger_builder.h"

client_logger_builder::client_logger_builder()
{}

client_logger_builder::~client_logger_builder() noexcept
{}

logger_builder *client_logger_builder::add_file_stream(
    std::string const &stream_file_path,
    logger::severity severity)
{
    data[stream_file_path].push_back(severity);

    return this; 
}

logger_builder *client_logger_builder::add_console_stream(
    logger::severity severity)
{
    data["console"].push_back(severity);
    return this; 
}


logger_builder* client_logger_builder::transform_with_configuration(
    std::string const &configuration_file_path, std::string const &configuration_path) //добавить путь
{
    //std::cout << configuration_file_path << std::endl;
    std::ifstream fin(configuration_file_path);
    auto info = nlohmann::json::parse(fin);
    auto pairs = info[configuration_path];

    //std::cout<< pairs << std::endl;
    for(auto& elem : pairs){
        data[elem.value("file", "Not found!")].push_back( elem["severity"]);
    }

    return this;
}

logger_builder *client_logger_builder::clear()
{
    data.clear();
}

logger *client_logger_builder::build() const
{
    return new client_logger(data);
}
