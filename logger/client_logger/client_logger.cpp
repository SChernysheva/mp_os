//#include <not_implemented.h>
#include "../include/client_logger.h"
#include <iostream>
#include<vector>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <fstream>
#define NAME "/QUEUE"
#define PACKET_SIZE 10

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

struct my_message
{
    //pid_t id_process;
    int count_packets;
    int num_of_packet;
    char text[100];
    char file_path[50];
    logger::severity sever;
    int id_;
}; 

logger const *client_logger::log(
    const std::string &text,
    logger::severity severity) const noexcept
{   
    //получить очередь
    //собрать структуру
    //поместить в очередь
    static int id_ = 0;
    struct mq_attr attr;
    mqd_t mq = mq_open(NAME, O_WRONLY);
    if (mq == (mqd_t)-1) { // Проверка на успешное открытие очереди
        perror("mq_open"); // Вывод ошибки, если открытие не удалось
        exit(1); // Выход из программы с ошибкой
    }
    if (mq_getattr(mq, &attr) == -1) {
        std::cout << "Ошибка получения атрибутов очереди сообщений." << std::endl;
    }
    //std::string current_dt = current_datetime_to_string();
    for (auto & logger_stream : logger_streams)
    {
        if ( std::find(logger_stream.second.second.begin(), logger_stream.second.second.end(), severity) == logger_stream.second.second.end())
        {
            continue;
        }

        if (logger_stream.second.first == nullptr)
        {
            int message_size = std::strlen(text.c_str());
            int num_packets = (message_size + PACKET_SIZE - 1) / PACKET_SIZE; // Вычисляем количество пакетов
            for (int i = 0; i < num_packets; i++)
            {
                my_message msg;

                char packet[PACKET_SIZE + 1]; // +1 для завершающего символа null
                int startIndex = i * PACKET_SIZE;
                int endIndex = std::min(startIndex + PACKET_SIZE, message_size);

                memcpy(packet, &text[startIndex], endIndex - startIndex);
                packet[endIndex - startIndex] = '\0'; // Завершаем строку null-символом
                strcpy(msg.text, packet);
                strcpy(msg.file_path,"console");
                //pid_t pid = getpid(); //убунту
                //msg.id_process = getpid();
                msg.num_of_packet = i + 1;
                msg.count_packets = num_packets;
                msg.sever = severity;
                msg.id_ = id_;
                if (mq_send(mq, (char *)&msg, sizeof(my_message), 0) == -1)
                {
                    perror("mq_send");
                    mq_close(mq);
                    exit(1);
                }
            }
            id_++;
                // std::cout << "[" << severity_to_string(severity) << "]"
                // << "[" << current_dt << "] " << text << std::endl;
            }
        else
        {
            std:: cout << "sever " << severity_to_string(severity) << "find in " << logger_stream.first << std::endl;
            int message_size = std::strlen(text.c_str());
            int num_packets = (message_size + PACKET_SIZE - 1) / PACKET_SIZE; // Вычисляем количество пакетов
            for (int i = 0; i < num_packets; i++)
            {
                my_message msg;

                char packet[PACKET_SIZE + 1]; // +1 для завершающего символа null
                int startIndex = i * PACKET_SIZE;
                int endIndex = std::min(startIndex + PACKET_SIZE, message_size);

                memcpy(packet, &text[startIndex], endIndex - startIndex);
                packet[endIndex - startIndex] = '\0'; // Завершаем строку null-символом
                strcpy(msg.text, packet);
                strcpy(msg.file_path, logger_stream.first.c_str());
                std::cout << "file : " << msg.file_path << std::endl;
                //msg.id_process = getpid();
                msg.num_of_packet = i + 1;
                msg.count_packets = num_packets;
                msg.sever = severity;
                msg.id_ = id_;
                if (mq_send(mq, (char *)&msg, sizeof(my_message), 0) == -1)
                {
                    perror("mq_send");
                    mq_close(mq);
                    exit(1);
                }
            }
            id_++;
            
            //(*logger_stream.second.first) << "[" << severity_to_string(severity) << "]" << "[" << current_dt << "] " << text << std::endl;
        }
    }
    mq_close(mq);

    return this;
}