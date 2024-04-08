#include <not_implemented.h>

#include "../include/allocator_boundary_tags.h"
#include <vector>
#include <iostream>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#define NAME_SEM \
const char* id = ("/" + std::to_string(getpid())).c_str();
//generate id, five roolslslslslslslsls roys

allocator_boundary_tags::~allocator_boundary_tags()
{
    NAME_SEM
    sem_t* sem = get_sem();
    logger* log = get_logger();
    log_with_guard_my("allocator destructor started", logger::severity::debug);
    sem_unlink(id); 
    sem_close(sem); 
    auto* parent_allocator = get_allocator();
    if (parent_allocator == nullptr)
    {
        ::operator delete(_trusted_memory);
    }
    else
    {
        parent_allocator->deallocate(_trusted_memory);
    }
}

allocator_boundary_tags::allocator_boundary_tags(
    allocator_boundary_tags &&other) noexcept
{
   this->_trusted_memory  = other._trusted_memory;
   other._trusted_memory = nullptr;
}

allocator_boundary_tags &allocator_boundary_tags::operator=(
    allocator_boundary_tags &&other) noexcept
{
    if (this != &other)
    {
        this->~allocator_boundary_tags();
        this->_trusted_memory = other._trusted_memory;
        other._trusted_memory = nullptr;
    }
}

allocator_boundary_tags::allocator_boundary_tags(
    size_t space_size,
    allocator *parent_allocator,
    logger *logger,
    allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (logger != nullptr)
    {
        logger->debug("start construct");
    }
    size_t common_size = space_size + get_ancillary_size();
    try
    {
        _trusted_memory = parent_allocator == nullptr
                          ? ::operator new(common_size)
                          : parent_allocator->allocate(common_size, 1);
    }
    catch (std::bad_alloc const &ex)
    {
        logger->error("can't get memory");
        throw std::bad_alloc();
    }
    
    allocator** parent_allocator_space_address = reinterpret_cast<allocator **>(_trusted_memory);
    *parent_allocator_space_address = parent_allocator;

    class logger** logger_space_address = reinterpret_cast<class logger**>(parent_allocator_space_address + 1);
    *logger_space_address = logger;

    size_t *space_size_space_address = reinterpret_cast<size_t *>(logger_space_address + 1);
    *space_size_space_address = space_size;

    size_t *size_memory = reinterpret_cast<size_t *>(space_size_space_address + 1);
    *size_memory = space_size;

    allocator_with_fit_mode::fit_mode *fit_mode_space_address = reinterpret_cast<allocator_with_fit_mode::fit_mode *>(size_memory + 1);
    *fit_mode_space_address = allocate_fit_mode;

    sem_t** sem = reinterpret_cast<sem_t** >(fit_mode_space_address + 1);
    NAME_SEM
    *sem = sem_open(id, O_CREAT | O_EXCL, 0666, 1); // Создаем или открываем семафор
    if (*sem == SEM_FAILED) {
        perror("sem_open");
        sem_unlink(id); 
        sem_close(*sem);
        if (parent_allocator == nullptr)
        {
            ::operator delete(_trusted_memory);
        }
        else
        {
            parent_allocator->deallocate(_trusted_memory);
        }
        throw std::error_code();
    }
    void **first_block_address_space_address = reinterpret_cast<void **>(sem + 1);
    *first_block_address_space_address = nullptr;
    if (logger != nullptr)
    {
        logger->debug("allocator created.");
    }

}

[[nodiscard]] void *allocator_boundary_tags::allocate(
    size_t value_size,
    size_t values_count)
{
    sem_t* sem = get_sem();
    while (sem_wait(sem) == -1) {
        sleep(1);
    }
    NAME_SEM
    log_with_guard_my("start allocate memory", logger::severity::debug);
    size_t* free_size = reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t));
    auto requested_size = value_size * values_count + get_meta_size();
    allocator_with_fit_mode::fit_mode fit_mode = get_fit_mode();
    void *target_block = nullptr;
    void *previous_to_target_block = nullptr;
    void *next_to_target_block = nullptr;
    size_t current_size = 0;

    { 
        void *previous_block = nullptr;
        void *current_block = get_first_occupied_block();
        if (current_block == nullptr)
        {
            unsigned char* start = reinterpret_cast<unsigned char*>(_trusted_memory) + get_ancillary_size();
            unsigned char* end = reinterpret_cast<unsigned char*>(_trusted_memory) + get_ancillary_size() + get_full_size();
            size_t aviable_size = end - start;
            //get_full_size?
            if (aviable_size >= requested_size) 
            {
                target_block = reinterpret_cast<void*>(start);
                current_size = aviable_size;
            }
        }
        else
        {
            while (current_block != nullptr)
            {
                unsigned char* start = nullptr;
                unsigned char* end = reinterpret_cast<unsigned char*>(current_block);

                if (previous_block == nullptr)
                {
                    start = reinterpret_cast<unsigned char*>(_trusted_memory) + get_ancillary_size();
                }
                else
                {
                    start = reinterpret_cast<unsigned char*>(previous_block) + get_meta_size() + *reinterpret_cast<size_t*>(previous_block);
                }
                size_t current_block_size = end - start;
                if (current_block_size >= requested_size &&
                    (fit_mode == allocator_with_fit_mode::fit_mode::first_fit ||
                     fit_mode == allocator_with_fit_mode::fit_mode::the_best_fit &&
                         (target_block == nullptr || current_size > current_block_size) ||
                     fit_mode == allocator_with_fit_mode::fit_mode::the_worst_fit && (target_block == nullptr ||
                                                                                      current_size <
                                                                                          current_block_size)))
                {
                    previous_to_target_block = previous_block;
                    target_block = reinterpret_cast<void*>(start);
                    current_size = current_block_size;
                    next_to_target_block = get_next_occupied_block(current_block); //todo
                }
                previous_block = current_block;
                current_block = get_next_occupied_block(current_block);
            }
            unsigned char* start = reinterpret_cast<unsigned char*>(previous_block) + get_meta_size() + *reinterpret_cast<size_t*>(previous_block);
            unsigned char* end = reinterpret_cast<unsigned char*>(_trusted_memory) + get_ancillary_size() + get_full_size();
            size_t current_block_size = end - start;
            if (current_block_size >= requested_size &&
                    (fit_mode == allocator_with_fit_mode::fit_mode::first_fit ||
                     fit_mode == allocator_with_fit_mode::fit_mode::the_best_fit &&
                         (target_block == nullptr || current_size > current_block_size) ||
                     fit_mode == allocator_with_fit_mode::fit_mode::the_worst_fit && (target_block == nullptr ||
                                                                                      current_size <
                                                                                          current_block_size)))
                {
                    previous_to_target_block = previous_block;
                    target_block = reinterpret_cast<void*>(start);
                    current_size = current_block_size;
                    next_to_target_block = nullptr;
                }
                //previous_block = current_block;
                //current_block = get_next_occupied_block(current_block);

        }
    }
    if (target_block == nullptr)
    {
        log_with_guard_my("can't allocate block", logger::severity::error);
        sem_unlink(id);
        sem_post(sem);
        //sem_close(sem);
        throw std::bad_alloc();
    }

    if (get_first_occupied_block() == nullptr)
    {
        size_t* size_block = reinterpret_cast<size_t*>(target_block);
        size_t difference = current_size - requested_size;
        if (difference < get_meta_size())
        {
            requested_size = current_size;
        }
        *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(sem_t*)) = target_block;

        *size_block = requested_size - get_meta_size();
        void** ptr_prev = reinterpret_cast<void**>(size_block + 1);
        void** ptr_next = reinterpret_cast<void**>(ptr_prev + 1);
        void** ptr_start_memory = reinterpret_cast<void**>(ptr_next + 1);

        *ptr_prev = nullptr;
        *ptr_next = nullptr;
        *ptr_start_memory = _trusted_memory;
    }
    else
    {
        size_t difference = current_size - requested_size;
        if (difference < get_meta_size())
        {
            requested_size = current_size;
        }
        size_t* size_block = reinterpret_cast<size_t*>(target_block);
        *size_block = requested_size - get_meta_size();

        void** ptr_prev = reinterpret_cast<void**>(size_block + 1);
        void** ptr_next = reinterpret_cast<void**>(ptr_prev + 1);
        void** ptr_memory = reinterpret_cast<void**>(ptr_next + 1); 

        *ptr_prev = previous_to_target_block;
        *ptr_next = next_to_target_block;
        *ptr_memory = _trusted_memory;

        if(next_to_target_block != nullptr)
        {
            void** ptr_next_to_prev = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(next_to_target_block) + sizeof(size_t));
            *ptr_next_to_prev = target_block;
        }
        if (previous_to_target_block != nullptr)
        {
            void** ptr_prev_to_next = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(previous_to_target_block) + sizeof(size_t) + sizeof(void*));
            *ptr_prev_to_next = target_block;
        }

    }
    *free_size -=  requested_size;
    std::vector<allocator_test_utils::block_info> data = get_blocks_info();
    std::string data_str;
    for (block_info value : data)
    {
        std::string is_oc = value.is_block_occupied ? "YES" : "NO";
        data_str += (is_oc + "  " + std::to_string(value.block_size) + " | ");
    }
    log_with_guard_my("finish allocate, state blocks: " + data_str, logger::severity::debug);
    log_with_guard_my("free size:" + std::to_string(*free_size), logger::severity::information); 
    sem_post(sem);
    return reinterpret_cast<unsigned char*>(target_block) + get_meta_size();
}

void allocator_boundary_tags::deallocate(
    void *at)
{
    sem_t* sem = get_sem();
    while (sem_wait(sem) == -1) {
        sleep(1);
    }
    NAME_SEM
    log_with_guard_my("start deallocate", logger::severity::debug);
    size_t* free_size = reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t));
    void* ptr_to_start = *reinterpret_cast<void**>(reinterpret_cast<char*>(at) - sizeof(void*));
    if (ptr_to_start != _trusted_memory)
    {
        log_with_guard_my("block doesnt belong to this instance", logger::severity::error);
        sem_unlink(id);
        sem_post(sem);
        //sem_close(sem);
        throw std::logic_error("block doesnt belong to this instance");
    }

    void *target_block = reinterpret_cast<void*>(reinterpret_cast<char *>(at) - sizeof(void*) - sizeof(void*) - sizeof(void*) - sizeof(size_t));

    std::string result;
    auto* bytes = reinterpret_cast<unsigned char*>(at);

    size_t size_object = *reinterpret_cast<size_t*>(target_block);

// TODO: wtf?!
    for (int i = 0; i < size_object; i++)
    {
        result += std::to_string(static_cast<int>(bytes[i])) + " ";
    }
    log_with_guard_my("state block:" + result, logger::severity::debug);

    
    //void *current_occup = get_first_occupied_block();

    void* prev_occup_block = *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(size_t));
    void* current_occup = *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(size_t) + sizeof(void*));
    //void *prev_occup_block = nullptr;
    // while (current_occup != nullptr &&
    //        current_occup <= target_block)
    // {
    //     if (current_occup == target_block)
    //     {
    //         current_occup = get_next_occupied_block(target_block);
    //         break;
    //     }
    //     prev_occup_block = current_occup;
    //     current_occup = get_next_occupied_block(current_occup);
    // }

    if (get_first_occupied_block() == target_block)
    {
        *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(sem_t*)) = current_occup;
    }
    if (prev_occup_block != nullptr)
    {
        size_t* size_ = reinterpret_cast<size_t*>(prev_occup_block);
        void** ptr_prev_ = reinterpret_cast<void**>(size_ + 1);
        void** ptr_prev_to_next = reinterpret_cast<void**>(ptr_prev_ + 1);
        *ptr_prev_to_next = current_occup;
    }
    if (current_occup != nullptr)
    {   
        void** ptr_next_to_prev = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(current_occup) + sizeof(size_t)); 
        *ptr_next_to_prev = prev_occup_block;
    }
    std::vector<allocator_test_utils::block_info> data = get_blocks_info();
    std::string data_str;
    for (block_info value : data)
    {
        std::string is_oc = value.is_block_occupied ? "YES" : "NO";
        data_str += (is_oc + "  " + std::to_string(value.block_size) + " | ");
    }
    log_with_guard_my("finish deallocate, state blocks: " + data_str, logger::severity::debug);
    (*free_size) += (size_object + get_meta_size());
    log_with_guard_my("free size:" + std::to_string(*free_size), logger::severity::information); 
    sem_post(sem);
}

inline void allocator_boundary_tags::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    sem_t* sem = get_sem();
    while (sem_wait(sem) == -1) {
        sleep(1);
    }
    log_with_guard_my("set_fit_mode start", logger::severity::trace);
    *reinterpret_cast<allocator_with_fit_mode::fit_mode *>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t)) = mode;
    log_with_guard_my("set_fit_mode finish", logger::severity::trace);
    sem_post(sem);
}

inline allocator *allocator_boundary_tags::get_allocator() const
{
    log_with_guard_my("get_allocator start", logger::severity::trace);
    return *reinterpret_cast<allocator**>(_trusted_memory);
}

std::vector<allocator_test_utils::block_info> allocator_boundary_tags::get_blocks_info() const noexcept
{
    log_with_guard_my("get_blocks_info start", logger::severity::trace);

    std::vector<allocator_test_utils::block_info> data;
    void** ans = reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(sem_t*) + sizeof(allocator_with_fit_mode::fit_mode));
    void* first_block = reinterpret_cast<void **>(ans + 1);

    //void* prev_occup = nullptr;
    void* current_occup = get_first_occupied_block();
    if (current_occup == nullptr)
    {
        allocator_test_utils::block_info obj;
        obj.is_block_occupied = 0;
        obj.block_size = get_full_size();
        data.push_back(obj);
        return data;
    }
    else
    {
        void* prev_block_occup = nullptr;
        if (current_occup == first_block)
        {
            allocator_test_utils::block_info obj;
            obj.block_size = *reinterpret_cast<size_t*>(current_occup);
            obj.is_block_occupied = 1;
            prev_block_occup = current_occup;
            current_occup = get_next_occupied_block(current_occup);
            data.push_back(obj);
        }
        else
        {
            allocator_test_utils::block_info obj1;
            unsigned char* start = reinterpret_cast<unsigned char*>(_trusted_memory) + get_ancillary_size();
            unsigned char* end = reinterpret_cast<unsigned char*>(current_occup);
            if (end - start > 0)
            {
                obj1.is_block_occupied = 0;
                obj1.block_size = end - start;
                data.push_back(obj1);
            }
            allocator_test_utils::block_info obj2;
            obj2.block_size = *reinterpret_cast<size_t*>(current_occup);
            obj2.is_block_occupied = 1;
            data.push_back(obj2);
            prev_block_occup = current_occup;
            current_occup = get_next_occupied_block(current_occup);
        }
        //void* next_block = nullptr;
        while (current_occup != nullptr)
        {
            allocator_test_utils::block_info obj1;
            unsigned char* start = reinterpret_cast<unsigned char*>(prev_block_occup) + get_meta_size() + *reinterpret_cast<size_t*>(prev_block_occup);
            unsigned char* end = reinterpret_cast<unsigned char*>(current_occup);
            if (end - start > 0)
            {
                obj1.is_block_occupied = 0;
                obj1.block_size = end - start;
                data.push_back(obj1);
            }
            allocator_test_utils::block_info obj2;
            obj2.block_size = *reinterpret_cast<size_t*>(current_occup);
            obj2.is_block_occupied = 1;
            data.push_back(obj2);
            prev_block_occup = current_occup;
            current_occup = get_next_occupied_block(current_occup);
        }
        unsigned char* end =  reinterpret_cast<unsigned char*>(_trusted_memory) + get_ancillary_size() + get_full_size();
        unsigned char* start = reinterpret_cast<unsigned char*>(prev_block_occup) + get_meta_size() + *reinterpret_cast<size_t*>(prev_block_occup);
        if (start != end)
        {
            allocator_test_utils::block_info obj;
            obj.block_size = end - start;
            obj.is_block_occupied = 0;
            data.push_back(obj);
        }
    }
    log_with_guard_my("get_blocks_info finish", logger::severity::trace);
    return data;
}

inline logger *allocator_boundary_tags::get_logger() const
{
    return *reinterpret_cast<logger **>(reinterpret_cast<allocator **>(_trusted_memory) + 1);
}

inline std::string allocator_boundary_tags::get_typename() const noexcept
{//aaaaa krokodil v vannoi
}

void allocator_boundary_tags::log_with_guard_my(std::string const &message,
    logger::severity severity) const
{
    logger *got_logger = get_logger(); 
    if (got_logger != nullptr)
    {
        got_logger->log(message, severity);
    }
}

size_t allocator_boundary_tags::get_ancillary_size() const
{
    return sizeof(size_t) + sizeof(allocator*) + sizeof(logger*) + sizeof(void*) + sizeof(size_t) + sizeof(sem_t*) + sizeof(allocator_with_fit_mode::fit_mode);
}

size_t allocator_boundary_tags::get_meta_size() const
{
    log_with_guard_my("get_meta_size start", logger::severity::trace);
    return sizeof(size_t) + 3 * sizeof(void*);
}

sem_t* allocator_boundary_tags::get_sem()
{
    log_with_guard_my("get_sem start", logger::severity::trace);
    return *reinterpret_cast<sem_t **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t)
                             + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode));;
}

void* allocator_boundary_tags::get_first_occupied_block() const
{
     log_with_guard_my("get_first_occupied_block start", logger::severity::trace);
    return *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(sem_t*));
}

allocator_with_fit_mode::fit_mode allocator_boundary_tags::get_fit_mode()
{
    log_with_guard_my("get_fit_mode start", logger::severity::trace);
    return *reinterpret_cast<allocator_with_fit_mode::fit_mode *>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t));
}

size_t allocator_boundary_tags::get_full_size() const
{
    log_with_guard_my("get_full_size start", logger::severity::trace);
    return *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*));
}

void* allocator_boundary_tags::get_next_occupied_block(void* block_address) const
{
    log_with_guard_my("get_next_occupied_block start", logger::severity::trace);
    return  *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block_address) + sizeof(size_t) + sizeof(void*));
}