//#include <not_implemented.h>
#include "../include/client_logger.h"
#include <iostream>
#include<vector>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#define NAME "/QUEUE"
#define PACKET_SIZE 10
//#define IMPLEMENTATION_FOR_UNIX 1
 //#define IMPLEMENTATION_FOR_WIN 1
 #define IMPLEMENTATION_CLIENT 1

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
    std::map<std::string, std::vector<logger::severity>> const & targets, std::string const& opt)
{
    this->opt = opt;
    for (auto & target : targets) 
    {
        auto global_stream = streams.find(target.first);
        std::ofstream *stream = nullptr; 

        if (global_stream == streams.end())
        {
            if (target.first != "console")
            {
                try
                {
                    stream = new std::ofstream;
                    stream->open(target.first); 
                }
                catch (const std::bad_alloc &e)
                {
                    std::cerr << "Exception caught: " << e.what() << std::endl;
                }
            }

            streams.insert(std::make_pair(target.first, std::make_pair(stream, 1))); 
        }
        else
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
    int count_packets;
    int num_of_packet;
    char text[100];
    char file_path[50];
    logger::severity sever;
    char pid[20];
    int id_;
}; 

#if defined(IMPLEMENTATION_FOR_UNIX) //send to server by message queue
#include <mqueue.h>
    logger const *client_logger::log(
        const std::string &text,
        logger::severity severity) const noexcept
    {
        static int id_ = 0;
        struct mq_attr attr;
        mqd_t mq = mq_open(NAME, O_WRONLY); //get queue
        if (mq == (mqd_t)-1)
        {
            perror("mq_open");
            exit(1);
        }
        for (auto &logger_stream : logger_streams)
        {
            if (std::find(logger_stream.second.second.begin(), logger_stream.second.second.end(), severity) == logger_stream.second.second.end())
            {
                continue;
            }

            if (logger_stream.second.first == nullptr) //if console
            {
                int message_size = std::strlen(text.c_str());
                int num_packets = (message_size + PACKET_SIZE - 1) / PACKET_SIZE; 
                if (num_packets == 0)
                {
                    my_message msg;
                    char packet[PACKET_SIZE + 1]; // +1 для завершающего символа null
                    int startIndex = 0;
                    int endIndex = std::min(startIndex + PACKET_SIZE, message_size);

                    memcpy(packet, &text[startIndex], endIndex - startIndex);
                    packet[endIndex - startIndex] = '\0'; 
                    strcpy(msg.text, packet);
                    strcpy(msg.file_path, "console");
                    pid_t pid = getpid();
                    std::string pid_str = std::to_string(pid);
                    strncpy(msg.pid, pid_str.c_str(), 20);
                
                    msg.num_of_packet = 1;
                    msg.count_packets = 1;
                    msg.sever = severity;
                    msg.id_ = id_;
                    if (mq_send(mq, (char *)&msg, sizeof(my_message), 0) == -1)
                    {
                        perror("mq_send");
                        mq_close(mq);
                        exit(1);
                    }
                }
                for (int i = 0; i < num_packets; i++)
                {
                    my_message msg;
                    char packet[PACKET_SIZE + 1]; // +1 для завершающего символа null
                    int startIndex = i * PACKET_SIZE;
                    int endIndex = std::min(startIndex + PACKET_SIZE, message_size);

                    memcpy(packet, &text[startIndex], endIndex - startIndex);
                    packet[endIndex - startIndex] = '\0'; 
                    strcpy(msg.text, packet);
                    strcpy(msg.file_path, "console");
                    pid_t pid = getpid();
                    std::string pid_str = std::to_string(pid);
                    strncpy(msg.pid, pid_str.c_str(), 20);
                
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
            }
            else //if file
            {
                int message_size = std::strlen(text.c_str());
                int num_packets = (message_size + PACKET_SIZE - 1) / PACKET_SIZE; // Вычисляем количество пакетов
                if (num_packets == 0)
                {
                    my_message msg;
                    char packet[PACKET_SIZE + 1]; // +1 для завершающего символа null
                    int startIndex = 0;
                    int endIndex = std::min(startIndex + PACKET_SIZE, message_size);

                    memcpy(packet, &text[startIndex], endIndex - startIndex);
                    packet[endIndex - startIndex] = '\0'; 
                    strcpy(msg.text, packet);
                    strcpy(msg.file_path, logger_stream.first.c_str());
                    pid_t pid = getpid();
                    std::string pid_str = std::to_string(pid);
                    strncpy(msg.pid, pid_str.c_str(), 20);
                
                    msg.num_of_packet = 1;
                    msg.count_packets = 1;
                    msg.sever = severity;
                    msg.id_ = id_;
                    if (mq_send(mq, (char *)&msg, sizeof(my_message), 0) == -1)
                    {
                        perror("mq_send");
                        mq_close(mq);
                        exit(1);
                    }
                }
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
                    pid_t pid = getpid();
                    std::string pid_str = std::to_string(pid);
                    strncpy(msg.pid, pid_str.c_str(), 20);
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
            }
        }
        mq_close(mq);

        return this;
    }

#elif defined(IMPLEMENTATION_CLIENT)

    logger const *client_logger::log(
        const std::string &text,
        logger::severity severity) const noexcept
    {
        for (auto &logger_stream : logger_streams)
        {
            if (std::find(logger_stream.second.second.begin(), logger_stream.second.second.end(), severity) == logger_stream.second.second.end())
            {
                continue;
            }

            if (logger_stream.second.first == nullptr)
            {
                for (int i = 0; opt[i] != '\0' && opt[i + 1] != '\0'; i++)
                {
                    if (opt[i] != '%') std::cout << opt[i];

                    if (opt[i] == '%' && opt[i + 1] == 'd') 
                    {
                       std::cout << "[" << logger::current_date_to_string() << "]";
                       i++;
                    }
                    else if (opt[i] == '%' && opt[i + 1] == 't') 
                    {
                       std::cout << "[" << logger::current_time_to_string() << "]";
                       i++;
                    }
                    else if (opt[i] == '%' && opt[i + 1] == 's') 
                    {
                       std::cout << "[" << severity_to_string(severity) << "]";
                       i++;
                    }
                    else if (opt[i] == '%' && opt[i + 1] == 'm') 
                    {
                       std::cout << text << " ";
                       i++;
                    }
                    else if (opt[i] == '%') std::cout << "%";
                }
                std::cout << "\n";
            }
            else
            {
                for (int i = 0; opt[i] != '\0' && opt[i + 1] != '\0'; i++)
                {
                    if (opt[i] != '%')
                        (*logger_stream.second.first)  << opt[i];

                    if (opt[i] == '%' && opt[i + 1] == 'd')
                    {
                        (*logger_stream.second.first)  << "[" << logger::current_date_to_string() << "]";
                        i++;
                    }
                    else if (opt[i] == '%' && opt[i + 1] == 't')
                    {
                        (*logger_stream.second.first)  << "[" << logger::current_time_to_string() << "]";
                        i++;
                    }
                    else if (opt[i] == '%' && opt[i + 1] == 's')
                    {
                        (*logger_stream.second.first)  << "[" << severity_to_string(severity) << "]";
                        i++;
                    }
                    else if (opt[i] == '%' && opt[i + 1] == 'm')
                    {
                        (*logger_stream.second.first)  << text << " ";
                        i++;
                    }
                    else if (opt[i] == '%')
                        (*logger_stream.second.first)  << "%";
                }
                (*logger_stream.second.first)  << "\n";
            }
        }

        return this;
    }
#elif defined(IMPLEMENTATION_FOR_WIN)
#include <windows.h>
logger const *client_logger::log(
    const std::string &text,
    logger::severity severity) const noexcept
{
    static int id_1 = 0;
    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "SharedMemory");
    LPVOID pData = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(my_message));
    HANDLE hSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "Semaphore");

    if (hMapFile && pData && hSemaphore)
    {
        for (auto &logger_stream : logger_streams)
        {
            if (std::find(logger_stream.second.second.begin(), logger_stream.second.second.end(), severity) == logger_stream.second.second.end())
            {
                continue;
            }
            //WaitForSingleObject(hSemaphore, INFINITE);
            if (logger_stream.second.first == nullptr) // if console
            {
                int message_size = std::strlen(text.c_str());
                int num_packets = (message_size + PACKET_SIZE - 1) / PACKET_SIZE;
                if (num_packets == 0)
                {
                    WaitForSingleObject(hSemaphore, INFINITE);
                    my_message *msg = (my_message *)pData;
                    memset(msg->text, 0, sizeof(char) * 100);
                    char packet[PACKET_SIZE + 1]; // +1 для завершающего символа null
                    int startIndex = 0;
                    int endIndex = std::min(startIndex + PACKET_SIZE, message_size);
                    memcpy(packet, &text[startIndex], endIndex - startIndex);
                    packet[endIndex - startIndex] = '\0';
                    strcpy(msg->text, packet);
                    DWORD pid = GetCurrentProcessId();
                    std::string pid_str = std::to_string(pid);
                    strncpy(msg->pid, pid_str.c_str(), 20);
                    strcpy(msg->file_path, "console");
                    // msg->id_process = getpid();
                    msg->num_of_packet = 1;
                    msg->count_packets = 1;
                    msg->sever = severity;
                    msg->id_ = id_1;
                    ReleaseSemaphore(hSemaphore, 1, NULL);
                }
                for (int i = 0; i < num_packets; i++)
                {
                    WaitForSingleObject(hSemaphore, INFINITE);
                    my_message *msg = (my_message *)pData;
                    memset(msg->text, 0, sizeof(char) * 100);
                    char packet[PACKET_SIZE + 1]; // +1 для завершающего символа null
                    int startIndex = i * PACKET_SIZE;
                    int endIndex = std::min(startIndex + PACKET_SIZE, message_size);
                    memcpy(packet, &text[startIndex], endIndex - startIndex);
                    packet[endIndex - startIndex] = '\0';
                    strcpy(msg->text, packet);
                    DWORD pid = GetCurrentProcessId();
                    std::string pid_str = std::to_string(pid);
                    strncpy(msg->pid, pid_str.c_str(), 20);
                    strcpy(msg->file_path, "console");
                    // msg->id_process = getpid();
                    msg->num_of_packet = i + 1;
                    msg->count_packets = num_packets;
                    msg->sever = severity;
                    msg->id_ = id_1;
                    ReleaseSemaphore(hSemaphore, 1, NULL);
                }
            }
            else // if file
            {
                int message_size = std::strlen(text.c_str());
                int num_packets = (message_size + PACKET_SIZE - 1) / PACKET_SIZE; // Вычисляем количество пакетов
                if (num_packets == 0)
                {
                    WaitForSingleObject(hSemaphore, INFINITE);
                    my_message *msg = (my_message *)pData;
                    memset(msg->text, 0, sizeof(char) * 100);
                    char packet[PACKET_SIZE + 1]; // +1 для завершающего символа null
                    int startIndex = 0;
                    int endIndex = std::min(startIndex + PACKET_SIZE, message_size);
                    memcpy(packet, &text[startIndex], endIndex - startIndex);
                    packet[endIndex - startIndex] = '\0';
                    strcpy(msg->text, packet);
                    DWORD pid = GetCurrentProcessId();
                    std::string pid_str = std::to_string(pid);
                    strncpy(msg->pid, pid_str.c_str(), 20);
                    strcpy(msg->file_path, logger_stream.first.c_str());
                    // msg->id_process = getpid();
                    msg->num_of_packet = 1;
                    msg->count_packets = 1;
                    msg->sever = severity;
                    msg->id_ = id_1;
                    ReleaseSemaphore(hSemaphore, 1, NULL);
                }
                for (int i = 0; i < num_packets; i++)
                {
                    WaitForSingleObject(hSemaphore, INFINITE);
                    my_message *msg = (my_message *)pData;
                    memset(msg->text, 0, sizeof(char) * 100);
                    char packet[PACKET_SIZE + 1]; // +1 для завершающего символа null
                    int startIndex = i * PACKET_SIZE;
                    int endIndex = std::min(startIndex + PACKET_SIZE, message_size);

                    memcpy(packet, &text[startIndex], endIndex - startIndex);
                    packet[endIndex - startIndex] = '\0'; // Завершаем строку null-символом
                    strcpy(msg->text, packet);
                    strcpy(msg->file_path, logger_stream.first.c_str());
                    DWORD pid = GetCurrentProcessId();
                    std::string pid_str = std::to_string(pid);
                    strncpy(msg->pid, pid_str.c_str(), 20);
                    msg->num_of_packet = i + 1;
                    msg->count_packets = num_packets;
                    msg->sever = severity;
                    msg->id_ = id_1;
                    ReleaseSemaphore(hSemaphore, 1, NULL);
                }
            }
            id_1++;
        }
    WaitForSingleObject(hSemaphore, INFINITE);
    my_message *msg = (my_message *)pData;
    msg->num_of_packet = 0;
    ReleaseSemaphore(hSemaphore, 1, NULL);
    UnmapViewOfFile(pData);
    CloseHandle(hMapFile);
    CloseHandle(hSemaphore);

    }
    return this;
}

#endif
