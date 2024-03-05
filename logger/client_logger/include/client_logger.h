#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H
#include <map>
#include <vector>
#include "../../logger/include/logger.h"
#include "client_logger_builder.h"

class client_logger final:
    public logger
{

    friend class client_logger_builder;

public:

    client_logger(
        std::map<std::string, std::vector<logger::severity>> const & );

    client_logger(
        client_logger const &other) = delete;

    client_logger &operator=(
        client_logger const &other) = delete;

    client_logger(
        client_logger &&other) noexcept = delete;

    client_logger &operator=(
        client_logger &&other) noexcept = delete;

    ~client_logger() noexcept final;

private:

    std::map<std::string, std::pair<std::ofstream * , std::vector<logger::severity>>> logger_streams;

private:

    static std::map<std::string, std::pair<std::ofstream *, size_t> > streams;

private:

    static std::map<logger::severity, std::string> severity_to_str;

     

public:

    [[nodiscard]] logger const *log(
        const std::string &message,
        logger::severity severity) const noexcept override;

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H