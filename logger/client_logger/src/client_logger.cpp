//#include <not_implemented.h>

#include "../include/client_logger.h"
#include <iostream>
#include<vector>
#include<algorithm>
#include <fstream>

std::map<std::string, std::pair<std::ofstream *, size_t> > client_logger::streams =
    std::map<std::string, std::pair<std::ofstream *, size_t> >();

std::map<logger::severity, std::string> client_logger::severity_to_str = {
    {logger::severity::trace, "trace"},
    {logger::severity::debug, "debug"},
    {logger::severity::information, "information"},
    {logger::severity::warning, "warning"},
    {logger::severity::error, "error"},
    {logger::severity::critical, "critical"}
};

client_logger::client_logger(
    std::map<std::string, std::vector<logger::severity>> const & targets)
{
    for (auto & target : targets) //идем по парам (файл(консоль) - северити)
    {
        auto global_stream = streams.find(target.first);
        std::ofstream *stream = nullptr; 

        if (global_stream == streams.end()) //если не нашли такой выходной файл
        {
            if (target.first != "console")//если это не консоль
            {
                stream = new std::ofstream;
                stream->open(target.first); //открываем файл с именем ключа
            }

            streams.insert(std::make_pair(target.first, std::make_pair(stream, 1))); 
        }
        else //если нашли то берем поток вывода за стрим
        {
            stream = global_stream->second.first;
            global_stream->second.second++;
        }

        logger_streams.insert(std::make_pair(target.first, std::make_pair(stream, target.second)));
    }
}



client_logger::~client_logger() noexcept 
{
    for (auto & logger_stream : logger_streams)
    {
        auto global_stream = streams.find(logger_stream.first);

        if (--(global_stream->second.second) == 0)
        {
            if (global_stream->second.first != nullptr)
            {
                global_stream->second.first->flush();
                global_stream->second.first->close();
                delete global_stream->second.first;
            }

            streams.erase(global_stream);
        }
    }
}

logger const *client_logger::log(
    const std::string &text,
    logger::severity severity) const noexcept
{
    std::string current_dt = current_datetime_to_string();
    for (auto & logger_stream : logger_streams)
    {
        if ( std::find(logger_stream.second.second.begin(), logger_stream.second.second.end(), severity) == logger_stream.second.second.end())
        {
            continue;
        }

        if (logger_stream.second.first == nullptr)
        {
            std::cout << "[" << severity_to_string(severity) << "]" 
            << "[" << current_dt << "] " << text << std::endl;
        }
        else
        {
            (*logger_stream.second.first) << "[" << severity_to_string(severity) << "]" << "[" << current_dt << "] " << text << std::endl;
        }
    }

    return this;
}