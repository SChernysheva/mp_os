#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BUDDIES_SYSTEM_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BUDDIES_SYSTEM_H

#include <allocator_guardant.h>
#include <allocator_test_utils.h>
#include <allocator_with_fit_mode.h>
#include <logger_guardant.h>
#include <typename_holder.h>
#include <iostream>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

class allocator_buddies_system final:
    private allocator_guardant,
    public allocator_test_utils,
    public allocator_with_fit_mode,
    private logger_guardant,
    private typename_holder
{

private:

    void *_trusted_memory;

public:

    ~allocator_buddies_system() override;

    allocator_buddies_system(
        allocator_buddies_system const &other) = delete;

    allocator_buddies_system &operator=(
        allocator_buddies_system const &other) = delete;

    allocator_buddies_system(
        allocator_buddies_system &&other) noexcept = delete;

    allocator_buddies_system &operator=(
        allocator_buddies_system &&other) noexcept = delete;

public:

    explicit allocator_buddies_system(
        size_t space_size_power_of_two,
        allocator *parent_allocator = nullptr,
        logger *logger = nullptr,
        allocator_with_fit_mode::fit_mode allocate_fit_mode = allocator_with_fit_mode::fit_mode::first_fit);

public:

    [[nodiscard]] void *allocate(
        size_t value_size,
        size_t values_count) override;

    void deallocate(
        void *at) override;

public:

    inline void set_fit_mode(
        allocator_with_fit_mode::fit_mode mode) override;

private:

    inline allocator *get_allocator() const override;

    sem_t *get_sem() const noexcept;

public:

    std::vector<allocator_test_utils::block_info> get_blocks_info() const noexcept override;

private:

    inline logger *get_logger() const override;

private:

    inline std::string get_typename() const noexcept override;



private:

    short get_power_2_for_num_less(size_t num);

    short get_num_for_power_more(size_t num);

    size_t get_ancillary_size();

    void* get_next_free_block(void* block_address);

    void* get_prev_free_block(void* block_address);

    short get_size_block(void* block_address);

    void* get_first_aviable_block();

    void* get_next_block(void* block_address) const;

    size_t get_meta_block_size();

    allocator_with_fit_mode::fit_mode get_fit_mode();

    void log_with_guard_my(
    std::string const &message,
    logger::severity severity) const;
    
};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_ALLOCATOR_ALLOCATOR_BUDDIES_SYSTEM_H
