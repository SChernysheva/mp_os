#include <not_implemented.h>
#include <iostream>
#include<vector>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <fstream>
#include "../include/server_logger.h"
#define NAME "/QUEUE"

server_logger::server_logger(
    server_logger const &other)
{
    throw not_implemented("server_logger::server_logger(server_logger const &other)", "your code should be here...");
}
server_logger::server_logger(){}
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
    //pid_t id_process;
    int count_packets;
    int num_of_packet;
    char text[100];
    char file_path[50];
    logger::severity sever;
    int id_;
};

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
        // Получение сообщения из очереди
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
                if (received_msg.count_packets == 1)
                {
                    if (strcmp(received_msg.file_path, "console") == 0)
                    {
                        std::cout << "[" << severity_to_string(received_msg.sever) << "]" << "[" << current_dt << "] " << received_msg.text << std::endl;
                    }
                    else 
                    {
                        std::fstream str(received_msg.file_path, std::ios::in | std::ios::out | std::ios::app);
                        if (!str.is_open()) {}
                        str << "[" << severity_to_string(received_msg.sever) << "]" << "[" << current_dt << "] " << received_msg.text << std::endl;
                    }
                }
                else
                {
                    std::string file_path_cash = "file_path_cash_id" + std::to_string(received_msg.id_) + ".txt";
                    std::fstream file(file_path_cash, std::ios::in | std::ios::out | std::ios::app);
                    if (file.is_open())
                    {
                        file << received_msg.text;
                    }
                    file.seekg(std::ios::beg);
                    if (strcmp(received_msg.file_path, "console") == 0)
                    {
                        std::cout << "[" << severity_to_string(received_msg.sever) << "]"
                                  << "[" << current_dt << "] ";
                        std::string line;
                        while (std::getline(file, line))
                        {
                            std::cout << line << std::endl;
                        }
                        file.close();
                        std::remove(file_path_cash.c_str());
                    }
                    else
                    {
                        std::fstream str(received_msg.file_path, std::ios::in | std::ios::out | std::ios::app);
                        if (!str.is_open())
                        {}
                        str << "[" << severity_to_string(received_msg.sever) << "]"
                             << "[" << current_dt << "] ";
                        std::string line;
                        while (std::getline(file, line))
                        {
                            str << line << std::endl;
                        }
                        file.close();
                        std::remove(file_path_cash.c_str());
                    }
                }
            }
            else
            {
                if (received_msg.num_of_packet == 1)
                {
                    //создать файл записать текст тцуда
                    std::string file_path_cash = "file_path_cash_id" + std::to_string(received_msg.id_) + ".txt";
                    std::ofstream file(file_path_cash);
                    if (file.is_open())
                    {
                        file << received_msg.text;
                    }
                }
                else
                {
                    std::string file_path_cash =  "file_path_cash_id" + std::to_string(received_msg.id_) + ".txt";
                    std::ofstream file(file_path_cash, std::ios::app);
                    if (file.is_open())
                    {
                        file << received_msg.text;
                    }
                }
            }
            // Обработка полученного сообщения
        }
    }
    mq_close(mq);
    mq_unlink(NAME);
}

logger const *server_logger::log(
    const std::string &text,
    logger::severity severity) const noexcept
{
}