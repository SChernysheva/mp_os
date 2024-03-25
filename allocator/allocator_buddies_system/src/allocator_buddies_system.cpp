#include <not_implemented.h>
#include <cmath>
#include "../include/allocator_buddies_system.h"
#include <iostream>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <vector>
allocator_buddies_system::~allocator_buddies_system()
{
    sem_t* sem = get_sem();
    log_with_guard_my("allocator destructor started", logger::severity::debug);
    sem_unlink("/my_semaphore"); 
    sem_close(sem); 
    allocator* allc = *reinterpret_cast<allocator**>(_trusted_memory);

    if (allc != nullptr)
    {
        allc->deallocate(_trusted_memory);
    }
    else
    {
        ::operator delete(_trusted_memory);
    }
}


allocator_buddies_system::allocator_buddies_system(
    size_t space_size,
    allocator *parent_allocator,
    logger *logger,
    allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (1 << space_size < get_meta_block_size())
    {
         if (logger != nullptr)
        {
            logger->error("Size of the requested memory is too small\n");
        }

        throw std::logic_error("size too small");
    }
    class logger* log = logger;
    if (log != nullptr) log->debug("start create allocator");
    auto size_for_allocate = 1 << space_size;
    auto full_size = size_for_allocate + get_ancillary_size();
    try
    {
        _trusted_memory = parent_allocator == nullptr
                          ? ::operator new(full_size)
                          : parent_allocator->allocate(full_size, 1);
    }
    catch (std::bad_alloc const &ex)
    {
        if (logger != nullptr) logger->error("can't get memory");
        throw std::bad_alloc();
    }

    allocator** allocator_place = reinterpret_cast<allocator**>(_trusted_memory);
    class logger** logger_place = reinterpret_cast<class logger**>(allocator_place + 1);
    size_t* power_memory = reinterpret_cast<size_t*>(logger_place + 1);
    size_t* free_memory = reinterpret_cast<size_t*>(power_memory + 1);
    allocator_with_fit_mode::fit_mode* place_fit_mode = reinterpret_cast<allocator_with_fit_mode::fit_mode*>(free_memory + 1);
    void** point_to_first_aviable = reinterpret_cast<void**>(place_fit_mode + 1);

    sem_t** sem = reinterpret_cast<sem_t** >(point_to_first_aviable + 1);
    *sem = sem_open("/my_semaphore", O_CREAT | O_EXCL, 0666, 1); // Создаем или открываем семафор
    if (*sem == SEM_FAILED) {
        perror("sem_open");
        sem_unlink("/my_semaphore"); 
        sem_close(*sem);
        exit(1);
    }

    *allocator_place = parent_allocator;
    *logger_place = logger;
    *power_memory = space_size;
    *free_memory = 1 << space_size;
    *place_fit_mode = allocate_fit_mode;
    *point_to_first_aviable = sem + 1;

    short* power_block = reinterpret_cast<short*>(*point_to_first_aviable);
    *power_block = space_size;
    bool* is_occupied = reinterpret_cast<bool*>(power_block + 1);
    *is_occupied = 0;
    void** ptr_to_prev = reinterpret_cast<void**>(is_occupied + 1);
    void** ptr_to_next = reinterpret_cast<void**>(ptr_to_prev + 1);
    *ptr_to_prev = nullptr;
    *ptr_to_next = nullptr;
}

[[nodiscard]] void *allocator_buddies_system::allocate(
    size_t value_size,
    size_t values_count)
{
    sem_t* sem = get_sem();
    while (sem_wait(sem) == -1) {
        sleep(1);
    }
    log_with_guard_my("start allocate", logger::severity::debug);
    auto requested_size = value_size * values_count + get_meta_block_size();
    size_t power = get_num_for_power_more(requested_size);

    
    allocator_with_fit_mode::fit_mode fit_mode = get_fit_mode();
    size_t* free_size = reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t));
    void *target_block = nullptr;
    void *previous_to_target_block = nullptr;
    void *next_to_target_block = nullptr;

    { 
        void *previous_block = nullptr;
        void *current_block = get_first_aviable_block();
        while (current_block != nullptr)
        {//check first fit
            size_t current_block_size = *reinterpret_cast<size_t*>(current_block);
            if (current_block_size >= power &&
                (fit_mode == allocator_with_fit_mode::fit_mode::first_fit ||
                 fit_mode == allocator_with_fit_mode::fit_mode::the_best_fit &&
                 (target_block == nullptr || get_size_block(target_block) > current_block_size) ||
                 fit_mode == allocator_with_fit_mode::fit_mode::the_worst_fit && (target_block == nullptr ||
                                                                                  get_size_block(target_block) <
                                                                                  current_block_size)))
            {
                previous_to_target_block = previous_block;
                target_block = current_block;
                next_to_target_block = get_next_free_block(current_block);
            }
            previous_block = current_block;
            current_block = get_next_free_block(current_block);
        }
    }
    if (target_block == nullptr)
    {
        log_with_guard_my("can't allocate block", logger::severity::error);
        sem_unlink("/my_semaphore");
        sem_post(sem);
        //sem_close(sem);
        throw std::bad_alloc();
    }
    size_t power_target_block = *reinterpret_cast<size_t*>(target_block);
    auto blocks_sizes_difference =  1 << power_target_block - 1 << power;
    void* next_free_block = get_next_free_block(target_block);
    // if (blocks_sizes_difference > 0 &&  1 << blocks_sizes_difference < get_meta_block_size())
    // {
    //     power = power_target_block;
    //     log_with_guard_my("change size", logger::severity::debug);
    //     log_with_guard_my("requested size was changed", logger::severity::warning);
    // }
    while (((1 << power_target_block) >> 1) >= (1 << power))
    {
         power_target_block--;

         auto *space_power_current = reinterpret_cast<short*>(target_block);
         auto *space_is_occupied_current = reinterpret_cast<bool *>(space_power_current + 1);
         auto **prev_block_current = reinterpret_cast<void **>(space_is_occupied_current + 1);
         auto **next_block_current = reinterpret_cast<void **>(prev_block_current + 1);

         void *buddie = reinterpret_cast<void *>(reinterpret_cast<char *>(target_block) + (1 << power_target_block));

         auto *space_power_buddie = reinterpret_cast<short *>(buddie);
         auto *space_is_occupied_buddie = reinterpret_cast<bool *>(space_power_buddie + 1);
         auto **prev_block_buddie = reinterpret_cast<void **>(space_is_occupied_buddie + 1);
         auto **next_block_buddie = reinterpret_cast<void **>(prev_block_buddie + 1);

         *space_is_occupied_buddie = 0;
         *space_power_buddie = power_target_block;
         *prev_block_buddie = target_block;
         *next_block_buddie = next_free_block;

         *space_power_current = power_target_block;
         *next_block_current = buddie;
    }
    if (previous_to_target_block == nullptr)
    {
        void **ptr_first_aviable = reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) +  sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode));
        *ptr_first_aviable = get_next_block(target_block);
    }
    *reinterpret_cast<bool*>(reinterpret_cast<short*>(target_block) + 1) = true;

    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(short) + sizeof(bool)) = _trusted_memory;

    (*free_size) -= (1 << power_target_block);

    log_with_guard_my("free size:" + std::to_string(*free_size), logger::severity::information);

    get_blocks_info();

    sem_post(sem);
    log_with_guard_my("finish allocate", logger::severity::debug);

    return reinterpret_cast<unsigned char*>(target_block) + sizeof(short) + sizeof(bool) + sizeof(void*) + sizeof(void*);

}

void allocator_buddies_system::deallocate(
    void *at)
{
    sem_t* sem = get_sem();
    while (sem_wait(sem) == -1) {
        sleep(1);
    }
    
    log_with_guard_my("start deallocate", logger::severity::debug);
    size_t* free_size = reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t));
    void *target_block = reinterpret_cast<void*>(reinterpret_cast<char *>(at) - sizeof(short) - sizeof(bool) - sizeof(void*) - sizeof(void*));  
     
    if (*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(short) + sizeof(bool)) != _trusted_memory)
    {
        log_with_guard_my("block doesn't belong this allocator!", logger::severity::error);
        sem_unlink("/my_semaphore");
        sem_post(sem);
        throw std::logic_error("block doesn't belong this allocator!");
    }
    std::string result;
    auto* bytes = reinterpret_cast<unsigned char*>(at);

    size_t size_object = 1 << (*reinterpret_cast<short*>(target_block));

    for (int i = get_meta_block_size(); i < size_object; i++)
    {
        result += std::to_string(static_cast<int>(bytes[i])) + " ";
    }
    log_with_guard_my("state block:" + result, logger::severity::debug);
    void* current_frre_block = get_first_aviable_block();
    void* prev_free_block = nullptr;
    while (current_frre_block != nullptr && current_frre_block < target_block)
    {
        prev_free_block = current_frre_block;
        current_frre_block = get_next_free_block(current_frre_block);
    }

    short* space_power_target = reinterpret_cast<short*>(target_block);
    bool* space_bool_target = reinterpret_cast<bool*>(space_power_target + 1);
    void** space_prev_block_target = reinterpret_cast<void**>(space_bool_target + 1);
    void** space_next_block_target = reinterpret_cast<void**>(space_prev_block_target + 1);

    short power_target = *space_power_target;

    *space_prev_block_target = prev_free_block;
    *space_next_block_target = current_frre_block;
    *space_bool_target = 0;

    if (prev_free_block == nullptr)
    {
        void** ptr_to_first_aviable = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode));
        *ptr_to_first_aviable = target_block;
    }
    else
    {
        void** space_ptr_prev_to_next = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(prev_free_block) + sizeof(short) + sizeof(bool) + sizeof(void*));
        *space_ptr_prev_to_next = target_block;
    }

    if (current_frre_block != nullptr)
    {
        void** space_ptr_next_to_prev = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(current_frre_block) + sizeof(short) + sizeof(bool));
        *space_ptr_next_to_prev = target_block;
    }

    //here
    //void* buddie = target_block ^ (1 << power_target);
    void* buddie = get_next_block(target_block);
    if (buddie != nullptr)
    {
        short power_buddie = *reinterpret_cast<short *>(buddie);
        bool is_occupied_buddie = *reinterpret_cast<bool *>(reinterpret_cast<unsigned char*>(buddie) + sizeof(short));
        while (buddie != nullptr && power_buddie == power_target && is_occupied_buddie == 0)
        {
            auto *space_power_target = reinterpret_cast<short *>(target_block);
            auto *space_is_occupied_target = reinterpret_cast<short *>(space_power_target + 1);
            void **prev_block_current = reinterpret_cast<void **>(space_is_occupied_target + 1);
            void **next_block_current = reinterpret_cast<void **>(prev_block_current + 1);

            void *next_block_buddie = get_next_free_block(buddie);

            *next_block_current = next_block_buddie;

            (*space_power_target)++;
            power_target = *space_power_target;

            if (next_block_buddie != nullptr)
            {
                void **space_ptr_next_to_prev = reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(next_block_buddie) + sizeof(short) + sizeof(bool));
                *space_ptr_next_to_prev = target_block;
            }
            buddie = get_next_block(target_block);
            if (buddie != nullptr)
            {
                power_buddie = *reinterpret_cast<short *>(buddie);
                is_occupied_buddie = *reinterpret_cast<bool *>(reinterpret_cast<unsigned char*>(buddie) + sizeof(short));
                std:: cout << power_buddie << power_target <<std::endl;
            }
            // buddie = target_block ^ (1 << *reinterpret_cast<short*>(target_block));
        }
    }
    (*free_size) += (1 << power_target);
    log_with_guard_my("free size:" + std::to_string(*free_size), logger::severity::information); //check

    get_blocks_info();

    sem_post(sem);
}

inline void allocator_buddies_system::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    sem_t* sem = get_sem();
    while (sem_wait(sem) == -1) {
        sleep(1);
    }
    log_with_guard_my("start set_fit_mode", logger::severity::debug);
    *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(size_t)) = mode;
    sem_post(sem);
}


size_t allocator_buddies_system::get_ancillary_size()
{
    //no log cause use in costructor 
    return sizeof(allocator*) + sizeof(logger*) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(size_t) +  sizeof(size_t) + sizeof(void*) + sizeof(sem_t*);//mutex
}

size_t allocator_buddies_system::get_meta_block_size()
{
    //no log cause use in costructor 
    return sizeof(short) + sizeof(bool) + sizeof(void*) + sizeof(void*);
}

short allocator_buddies_system::get_power_2_for_num_less(size_t num)
{
    log_with_guard_my("start get_power_2_for_num", logger::severity::trace);
    return static_cast<short>(floor((log2(num))));
}

short allocator_buddies_system::get_num_for_power_more(size_t number)
{
    log_with_guard_my("start get_num_for_power", logger::severity::trace);
    return static_cast<short>(ceil(log2(number)));
}


std::vector<allocator_test_utils::block_info> allocator_buddies_system::get_blocks_info() const noexcept
{
    log_with_guard_my("get_blocks_info start", logger::severity::debug);

    std::vector<allocator_test_utils::block_info> data;
    sem_t** ans = reinterpret_cast<sem_t **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(void*));
    void* cur_block = reinterpret_cast<void **>(ans + 1);

    size_t current_size = 0;
    size_t size_trusted_memory =  1 << *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*));

    while(current_size < size_trusted_memory)
    {
        allocator_test_utils::block_info value;
        value.block_size =  1 << *reinterpret_cast<size_t*>(cur_block); 
        bool is_occupied = *reinterpret_cast<bool*>(reinterpret_cast<short *>(cur_block) + 1);
        value.is_block_occupied = is_occupied;
        data.push_back(value);
        cur_block = get_next_block(cur_block);
        current_size += value.block_size;
    }
    
    std::string data_str;
    for (block_info value : data)
    {
        std::string is_oc = value.is_block_occupied ? "YES" : "NO";
        data_str += (is_oc + "  " + std::to_string(value.block_size) + " | ");
    }
    log_with_guard_my("state blocks: " + data_str, logger::severity::debug);
    return data;
}
allocator_with_fit_mode::fit_mode allocator_buddies_system::get_fit_mode()
{
    log_with_guard_my("start get_fit_mode", logger::severity::debug);
    return *reinterpret_cast<allocator_with_fit_mode::fit_mode*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(size_t));
}

short allocator_buddies_system::get_size_block(void *block_address)
{
    log_with_guard_my("start get_size_block", logger::severity::trace);
    return *reinterpret_cast<short *>(block_address);
}

void* allocator_buddies_system::get_first_aviable_block()
{
    log_with_guard_my("start get_first_aviable_block", logger::severity::trace);
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode));
}

void* allocator_buddies_system::get_next_block(void* address_block) const
{
    log_with_guard_my("start get_next_block", logger::severity::trace);

    // if (1 << *reinterpret_cast<short*>(address_block) == *reinterpret_cast<short *>(_trusted_memory))
    // {
    //     return nullptr;
    // }

    // void** ans = reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode));
    // void* start_adr = reinterpret_cast<void **>(ans + 1);

    // size_t size_block = 1 << *reinterpret_cast<short*>(address_block);
    // size_t relative_address = reinterpret_cast<unsigned char *>(address_block) - reinterpret_cast<unsigned char *>(start_adr);
    // size_t result_xor = relative_address ^ size_block;
    size_t power_memory = *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*));
    if (reinterpret_cast<void *>(reinterpret_cast<char *>(address_block) + (1 << *reinterpret_cast<size_t*>(address_block))) >
        reinterpret_cast<void *>(reinterpret_cast<char *>(_trusted_memory) + (1 << power_memory)) )
    {
        return nullptr;
    }
    return  reinterpret_cast<void*>(reinterpret_cast<char *>(address_block) + (1 << *reinterpret_cast<size_t*>(address_block)));
}

void* allocator_buddies_system::get_next_free_block(void* address_block)
{
    log_with_guard_my("start get_next_free_block", logger::severity::trace);
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(address_block) + sizeof(short) + sizeof(bool) + sizeof(void*));
}

void* allocator_buddies_system::get_prev_free_block(void* address_block)
{
    log_with_guard_my("start get_prev_free_block", logger::severity::trace);
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(address_block) + sizeof(short) + sizeof(bool));
}

inline allocator *allocator_buddies_system::get_allocator() const
{
    log_with_guard_my("start get_allocator", logger::severity::debug);
    return reinterpret_cast<allocator*>(_trusted_memory);
}

inline logger *allocator_buddies_system::get_logger() const
{
    return *reinterpret_cast<logger**>(reinterpret_cast<allocator**>(_trusted_memory) + 1);
}

inline std::string allocator_buddies_system::get_typename() const noexcept
{
}


sem_t *allocator_buddies_system::get_sem() const noexcept
{
    log_with_guard_my("get_sem start", logger::severity::trace);
    return *reinterpret_cast<sem_t **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t)
                             + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(void*));;
}

void allocator_buddies_system::log_with_guard_my(
    std::string const &message,
    logger::severity severity) const
{
    logger *got_logger = get_logger(); 
    if (got_logger != nullptr)
    {
        got_logger->log(message, severity);
    }
}