#include <not_implemented.h>
#include <iostream>
#include<vector>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <fstream>
#include "../include/server_logger.h"


#define NAME "/QUEUE"
#define PACKET_SIZE 10
//#define IMPLEMENTATION_FOR_UNIX 1
// #define IMPLEMENTATION_FOR_WIN 1
server_logger::server_logger(){};
server_logger::server_logger(
    server_logger const &other)
{
    throw not_implemented("server_logger::server_logger(server_logger const &other)", "your code should be here...");
}
server_logger &server_logger::operator=(
    server_logger const &other)
{
    throw not_implemented("server_logger &server_logger::operator=(server_logger const &other)", "your code should be here...");
}

server_logger::server_logger(
    server_logger &&other) noexcept
{
    throw not_implemented("server_logger::server_logger(server_logger &&other) noexcept", "your code should be here...");
}

server_logger &server_logger::operator=(
    server_logger &&other) noexcept
{
    throw not_implemented("server_logger &server_logger::operator=(server_logger &&other) noexcept", "your code should be here...");
}

server_logger::~server_logger() noexcept
{
    throw not_implemented("server_logger::~server_logger() noexcept", "your code should be here...");
}


struct my_message
{
    char opt[25];
    int count_packets;
    int num_of_packet;
    char text[100];
    char file_path[50];
    logger::severity sever;
    int id_;
};

#ifdef __linux__
#include <mqueue.h>
void server_logger::init(void)
{
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 20;
    attr.mq_msgsize = sizeof(my_message) * 2;
    attr.mq_curmsgs = 0;

    mqd_t mq = mq_open(NAME, O_CREAT, 0644, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        mq_close(mq);
        mq_unlink(NAME);
        exit(1);
    }
    while(true) {
        // get message
        my_message received_msg;
        if (mq_receive(mq, (char*)&received_msg, attr.mq_msgsize, NULL) == -1) {
            perror("mq_receive");
            mq_close(mq);
            mq_unlink(NAME);
            exit(1);
        } else {
            if (received_msg.count_packets == received_msg.num_of_packet)
            {
                std::string current_dt = logger::current_datetime_to_string();
                if (received_msg.count_packets == 1) //if there is only 1 packet for this text/file
                {
                    if (strcmp(received_msg.file_path, "console") == 0)
                    {
                        for (int i = 0; received_msg.opt[i] != '\0' && received_msg.opt[i + 1] != '\0'; i++)
                        {
                            if (received_msg.opt[i] != '%')
                                std::cout << received_msg.opt[i];

                            if (received_msg.opt[i] == '%' && received_msg.opt[i + 1] == 'd')
                            {
                                std::cout << "[" << logger::current_date_to_string() << "]";
                                i++;
                            }
                            else if (received_msg.opt[i] == '%' && received_msg.opt[i + 1] == 't')
                            {
                                std::cout << "[" << logger::current_time_to_string() << "]";
                                i++;
                            }
                            else if (received_msg.opt[i] == '%' && received_msg.opt[i + 1] == 's')
                            {
                                std::cout << "[" << severity_to_string(received_msg.sever) << "]";
                                i++;
                            }
                            else if (received_msg.opt[i] == '%' && received_msg.opt[i + 1] == 'm')
                            {
                                std::cout << received_msg.text;
                                i++;
                            }
                            else if (received_msg.opt[i] == '%')
                                std::cout << "%";
                        }
                        std::cout << std::endl;
                    }
                    else 
                    {
                        std::fstream str(received_msg.file_path, std::ios::in | std::ios::out | std::ios::app);
                        for (int i = 0; received_msg.opt[i] != '\0' && received_msg.opt[i + 1] != '\0'; i++)
                        {
                            if (received_msg.opt[i] != '%')
                                str << received_msg.opt[i];

                            if (received_msg.opt[i] == '%' && received_msg.opt[i + 1] == 'd')
                            {
                                str << "[" << logger::current_date_to_string() << "]";
                                i++;
                            }
                            else if (received_msg.opt[i] == '%' && received_msg.opt[i + 1] == 't')
                            {
                                str << "[" << logger::current_time_to_string() << "]";
                                i++;
                            }
                            else if (received_msg.opt[i] == '%' && received_msg.opt[i + 1] == 's')
                            {
                                str << "[" << severity_to_string(received_msg.sever) << "]";
                                i++;
                            }
                            else if (received_msg.opt[i] == '%' && received_msg.opt[i + 1] == 'm')
                            {
                                str << received_msg.text;
                                i++;
                            }
                            else if (received_msg.opt[i] == '%')
                                str << "%";
                        }
                        str << std::endl;
                    }
                }
                else
                {
                    std::string file_path_cash = "file_path_cash_id" + std::to_string(received_msg.id_) + ".txt"; // (if last packet) find a file for cash info from packets
                    std::fstream file(file_path_cash, std::ios::in | std::ios::out | std::ios::app);
                    if (file.is_open())
                    {
                        file << received_msg.text; //write info in file
                    }
                    file.seekg(std::ios::beg); //back to the start
                    if (strcmp(received_msg.file_path, "console") == 0)
                    {
                        for (int i = 0; received_msg.opt[i] != '\0' && received_msg.opt[i + 1] != '\0'; i++)
                        {
                            if (received_msg.opt[i] != '%')
                                std::cout << received_msg.opt[i];

                            if (received_msg.opt[i] == '%' && received_msg.opt[i + 1] == 'd')
                            {
                                std::cout << "[" << logger::current_date_to_string() << "]";
                                i++;
                            }
                            else if (received_msg.opt[i] == '%' && received_msg.opt[i + 1] == 't')
                            {
                                std::cout << "[" << logger::current_time_to_string() << "]";
                                i++;
                            }
                            else if (received_msg.opt[i] == '%' && received_msg.opt[i + 1] == 's')
                            {
                                std::cout << "[" << severity_to_string(received_msg.sever) << "]";
                                i++;
                            }
                            else if (received_msg.opt[i] == '%' && received_msg.opt[i + 1] == 'm')
                            {
                                std::string line;
                                while (std::getline(file, line)) // get info from cash file and write it
                                {
                                    if (!file.eof()) std::cout << line << std::endl;
                                    else std::cout << line;
                                }
                                i++;
                            }
                            else if (received_msg.opt[i] == '%')
                                std::cout << "%";
                        }
                        std::cout << std::endl;
                        file.close();
                        std::remove(file_path_cash.c_str()); // remove cash file
    
                    }
                    else
                    {
                        std::fstream str(received_msg.file_path, std::ios::in | std::ios::out | std::ios::app); //in file from file
                        for (int i = 0; received_msg.opt[i] != '\0' && received_msg.opt[i + 1] != '\0'; i++)
                        {
                            if (received_msg.opt[i] != '%')
                                str << received_msg.opt[i];

                            if (received_msg.opt[i] == '%' && received_msg.opt[i + 1] == 'd')
                            {
                                str << "[" << logger::current_date_to_string() << "]";
                                i++;
                            }
                            else if (received_msg.opt[i] == '%' && received_msg.opt[i + 1] == 't')
                            {
                                str << "[" << logger::current_time_to_string() << "]";
                                i++;
                            }
                            else if (received_msg.opt[i] == '%' && received_msg.opt[i + 1] == 's')
                            {
                                str << "[" << severity_to_string(received_msg.sever) << "]";
                                i++;
                            }
                            else if (received_msg.opt[i] == '%' && received_msg.opt[i + 1] == 'm')
                            {
                                std::string line;
                                while (std::getline(file, line))
                                {
                                    if (!file.eof()) str << line << std::endl;
                                    else str << line;
                                }
                                i++;
                            }
                            else if (received_msg.opt[i] == '%')
                                str << "%";
                        }
                        str << std::endl;
                        file.close();
                        std::remove(file_path_cash.c_str());
                    }
                }
            }
            else
            {
                if (received_msg.num_of_packet == 1) //if there is first packet and count_packets > 1
                {
                    //make a new cash file and write
                    std::string file_path_cash = "file_path_cash_id" + std::to_string(received_msg.id_) + ".txt";
                    std::fstream file(file_path_cash, std::ios::in | std::ios::out | std::ios::app);
                    file << received_msg.text;
                    
                }
                else
                {
                    std::string file_path_cash =  "file_path_cash_id" + std::to_string(received_msg.id_) + ".txt";
                    std::ofstream file(file_path_cash, std::ios::app);
                    file << received_msg.text;
                }
            }
            // Обработка полученного сообщения
        }
    }
    mq_close(mq);
    mq_unlink(NAME);
}


#elif _WIN32
void server_logger::init(void)
{}

#endif
logger const *server_logger::log(
    const std::string &text,
    logger::severity severity) const noexcept
{
}