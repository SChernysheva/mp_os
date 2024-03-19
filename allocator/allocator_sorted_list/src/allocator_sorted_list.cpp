#include <not_implemented.h>

#include "../include/allocator_sorted_list.h"
#include <vector>
#include <iostream>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
//tests + list bytes + loggs  + synchro
allocator_sorted_list::~allocator_sorted_list()
{
    sem_t* sem = get_sem();
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->debug("allocator destructor started");
    }
    sem_unlink("/my_semaphore"); 
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
    if (log != nullptr)
    {
        log->debug("allocator destructor finished");
    }
}

allocator_sorted_list::allocator_sorted_list(
    size_t space_size,
    allocator *parent_allocator,
    logger *logger,
    allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    if (logger != nullptr)
    {
        logger->debug("start creating allocator");
    }
    if (space_size < sizeof(block_pointer_t) + sizeof(block_size_t))
    {
        if (logger != nullptr)
        {
            logger->error("Size ofthe requested memory is too small\n");
        }
        throw std::logic_error("size too small");
    }

    auto common_size = space_size + get_ancillary_space_size(logger); //запрашиваемый размер

    try
    {
        _trusted_memory = parent_allocator == nullptr
                          ? ::operator new(common_size)
                          : parent_allocator->allocate(common_size, 1);
    }
    catch (std::bad_alloc const &ex)
    {
        logger->error("can't get memory");
        sem_t *sem = get_sem();
        sem_unlink("/my_semaphore");
        sem_close(sem);
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
    //sem_after_fit_mode
    sem_t** sem = reinterpret_cast<sem_t** >(fit_mode_space_address + 1);
    *sem = sem_open("/my_semaphore", O_CREAT | O_EXCL, 0666, 1); // Создаем или открываем семафор
    if (*sem == SEM_FAILED) {
        perror("sem_open");
        sem_unlink("/my_semaphore"); 
        sem_close(*sem);
        exit(1);
    }
    void **first_block_address_space_address = reinterpret_cast<void **>(sem + 1);
    *first_block_address_space_address = reinterpret_cast<void *>(first_block_address_space_address + 1);

    *reinterpret_cast<size_t *>(*first_block_address_space_address) = space_size - sizeof(size_t) - sizeof(void*);
    *reinterpret_cast<void** >(reinterpret_cast<size_t *>(*first_block_address_space_address) + 1) = nullptr;

    if (logger != nullptr)
    {
        logger->debug("allocator created.");
    }
}

[[nodiscard]] void *allocator_sorted_list::allocate(
    size_t value_size,
    size_t values_count)
{
    sem_t* sem = get_sem();
    while (sem_wait(sem) == -1) {
        sleep(1);
    }
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->debug("start allocate memory");
    }
    auto requested_size = value_size * values_count;
    if (requested_size < sizeof(block_pointer_t) + sizeof(block_size_t))
    {
        requested_size = sizeof(block_pointer_t) + sizeof(block_size_t);
        if (log != nullptr)
        {
            log->warning("requested space size was changed");
        }
    }   
    allocator_with_fit_mode::fit_mode fit_mode = get_fit_mode();
    void *target_block = nullptr;
    void *previous_to_target_block = nullptr;
    void *next_to_target_block = nullptr;

    { 
        void *previous_block = nullptr;
        void *current_block = get_first_aviable_block();
        while (current_block != nullptr)
        {
            size_t current_block_size = get_aviable_block_size(current_block);
            if (current_block_size >= requested_size &&
                (fit_mode == allocator_with_fit_mode::fit_mode::first_fit ||
                 fit_mode == allocator_with_fit_mode::fit_mode::the_best_fit &&
                 (target_block == nullptr || get_aviable_block_size(target_block) > current_block_size) ||
                 fit_mode == allocator_with_fit_mode::fit_mode::the_worst_fit && (target_block == nullptr ||
                                                                                  get_aviable_block_size(target_block) <
                                                                                  current_block_size)))
            {
                previous_to_target_block = previous_block;
                target_block = current_block;
                next_to_target_block = get_aviable_block_next_block_address(current_block);
            }
            previous_block = current_block;
            current_block = get_aviable_block_next_block_address(current_block);
        }
    }
    if (target_block == nullptr)
    {
        if (log != nullptr)
        {
            log->error("can't allocate block!");
        }
        sem_unlink("/my_semaphore");
        sem_post(sem);
        //sem_close(sem);
        throw std::bad_alloc();
    }
    if (previous_to_target_block == nullptr)
    {
        auto blocks_sizes_difference = *reinterpret_cast<size_t*>(target_block) - requested_size;
        void **ptr_to_next_free_block = reinterpret_cast<void**>(reinterpret_cast<size_t *>(target_block) + 1);
        if (blocks_sizes_difference > 0 && blocks_sizes_difference < sizeof(block_pointer_t) + sizeof(block_size_t))
        {
            if (log != nullptr)
            {
                log->warning("requested size was changed");
            }
            requested_size = get_aviable_block_size(target_block);
            void *next_free_block = get_aviable_block_next_block_address(target_block);
            void** first_avail_block = reinterpret_cast<void **>
                                (reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(sem_t*) + sizeof(allocator_with_fit_mode::fit_mode));
            *first_avail_block = next_free_block;

        }
        else
        {//если чтото осталось в таргет блоке после выделения памяти
            auto size_target_block = get_aviable_block_size(target_block);      // размер изначально свободного блока
            *reinterpret_cast<size_t *>(target_block) = requested_size;                 // поменяли размер на новый(как бы новый блок с новым размером который отдадим юзеру)
            void *next_free_block = get_aviable_block_next_block_address(target_block); // сохранили указатель на след свободный блок

            void *new_block = reinterpret_cast<unsigned char *>(target_block) + sizeof(size_t) + sizeof(void*) + requested_size; // новый блок - тот что остался в памяти после выделения
            size_t *size_space_new_block = reinterpret_cast<size_t *>(new_block);
            auto **next_free_new_block = reinterpret_cast<void **>(size_space_new_block + 1);

            *size_space_new_block = size_target_block - requested_size - sizeof(size_t) - sizeof(void*);
            *next_free_new_block = next_free_block;

            void** first_avail_block = reinterpret_cast<void **>
                                (reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(sem_t*) + sizeof(allocator_with_fit_mode::fit_mode));
            *first_avail_block = new_block;
        }

        *ptr_to_next_free_block = _trusted_memory;
    }
    else
    {
        void **ptr_to_next_free_block = reinterpret_cast<void**>(reinterpret_cast<size_t *>(target_block) + 1);
        auto blocks_sizes_difference = get_aviable_block_size(target_block) - requested_size;
        if (blocks_sizes_difference > 0 && blocks_sizes_difference < sizeof(block_pointer_t) + sizeof(block_size_t))
        {
            if (log != nullptr)
            {
                log->warning("requested space size was changed");
            }
            requested_size = get_aviable_block_size(target_block);
            void** ptr_prev_block_to_this = reinterpret_cast<void**>(reinterpret_cast<size_t*>(previous_to_target_block) + 1); 
            void* ptr_this_next = get_aviable_block_next_block_address(target_block);
            *ptr_prev_block_to_this = ptr_this_next;
        }
        else
        {
            void** ptr_prev_block_to_this = reinterpret_cast<void**>(reinterpret_cast<size_t*>(previous_to_target_block) + 1); 
            void* ptr_this_next = get_aviable_block_next_block_address(target_block);
            auto size_target_block = *reinterpret_cast<size_t*>(target_block); 
            size_t* size_target_block_ptr = reinterpret_cast<size_t*>(target_block);
            *size_target_block_ptr = requested_size;
            void *new_block = reinterpret_cast<char *>(target_block) + sizeof(size_t) + sizeof(void*) + requested_size;
            *reinterpret_cast<size_t*>(new_block) = size_target_block - requested_size - sizeof(size_t) - sizeof(void*);
            *reinterpret_cast<void**>(reinterpret_cast<size_t*>(new_block) + 1) = ptr_this_next;
            *ptr_prev_block_to_this = new_block;
        }
        *ptr_to_next_free_block = _trusted_memory;
    }
    size_t size_before = *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *));
    size_t* size_space = reinterpret_cast<size_t*>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *));
    *size_space = size_before - requested_size - sizeof(size_t) - sizeof(void*);
    if (log != nullptr)
    {
        log->information("allocate memory. free summ size:" + std::to_string(*size_space));
        log->debug("finish allocate memory");
        std::vector<allocator_test_utils::block_info> data = get_blocks_info();
        std::string data_str;
        for (block_info value : data)
        {
            std::string is_oc = value.is_block_occupied ? "YES" : "NO";
            data_str += (is_oc + "  " + std::to_string(value.block_size) + " | ");
        }
        log->debug("state blocks: " + data_str);
    }
    sem_post(sem);
    return reinterpret_cast<char *>(target_block) + sizeof(size_t) + sizeof(void*); 
}

void allocator_sorted_list::deallocate(
    void *deallocated_block)
{
    sem_t* sem = get_sem();
    while (sem_wait(sem) == -1) {
        sleep(1);
    }
    if (deallocated_block == nullptr)
    {
        return;
    }
    logger* log = get_logger();
    void *target_block = reinterpret_cast<char *>(deallocated_block) - sizeof(void*) - sizeof(size_t); 
    auto size_target_block = *reinterpret_cast<size_t*>(target_block);
    if (log != nullptr)
    {
        log->debug("start deallocate memory");
        std::string result;
        auto *size_space = reinterpret_cast<size_t *>(target_block);
        auto *bytes = reinterpret_cast<unsigned char *>(size_space + 1);

        size_t size_object = get_occupied_block_size(target_block);

        for (int i = 0; i < size_object; i++)
        {
            result += std::to_string(static_cast<int>(bytes[i])) + " ";
        }
        log->debug("state block:" + result);
    }

    size_t size_before = *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *));
    size_t* size_space = reinterpret_cast<size_t*>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *));
    if (*reinterpret_cast<void**>(reinterpret_cast<size_t*>(target_block) + 1) != _trusted_memory)
    {
        if (log != nullptr)
        {
            log->error("can't allocate block!");
        }
        sem_unlink("/my_semaphore");
        sem_close(sem);
        throw std::logic_error("block doesn't belong this allocator!");
    }
    void *current_free_block = get_first_aviable_block();
    void *prev_free_block = nullptr;
    while (current_free_block != nullptr &&
           current_free_block < target_block)
    {
        prev_free_block = current_free_block;
        current_free_block = get_aviable_block_next_block_address(current_free_block);
    }

    if (prev_free_block == nullptr && current_free_block != nullptr)
    {
        void **ptr_to_first_avail_block = reinterpret_cast<void**>( reinterpret_cast<unsigned char*>(_trusted_memory) 
                             + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(sem_t*) + sizeof(allocator_with_fit_mode::fit_mode));
        *ptr_to_first_avail_block = target_block;
        void** ptr_target_block_to_next = reinterpret_cast<void**>(reinterpret_cast<size_t*>(target_block) + 1);
        *ptr_target_block_to_next = current_free_block;
        *size_space = size_before + get_occupied_block_size(target_block);
        size_before = *size_space;
        if (reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(target_block) + size_target_block + sizeof(size_t) + sizeof(void*)) == current_free_block )
        {
            *reinterpret_cast<size_t*>(target_block) = size_target_block + *reinterpret_cast<size_t*>(current_free_block) + sizeof(size_t) + sizeof(void*);
            *reinterpret_cast<void**>(reinterpret_cast<size_t*>(target_block) + 1) = *reinterpret_cast<void**>(reinterpret_cast<size_t*>(current_free_block) + 1);
            *size_space = size_before + sizeof(size_t) + sizeof(void*);
        }
    }
    else if (prev_free_block != nullptr && current_free_block != nullptr)
    {
        void** ptr_prev_to_next = reinterpret_cast<void**>(reinterpret_cast<size_t*>(prev_free_block) + 1);
        *ptr_prev_to_next = target_block;
        void** ptr_target_block_to_next = reinterpret_cast<void**>(reinterpret_cast<size_t*>(target_block) + 1);
        *ptr_target_block_to_next = current_free_block;
        *size_space = size_before + get_occupied_block_size(target_block);
        size_before = *size_space;
        if (reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(target_block) + size_target_block + sizeof(size_t) + sizeof(void*)) == current_free_block )
        {
            *reinterpret_cast<size_t*>(target_block) = size_target_block + *reinterpret_cast<size_t*>(current_free_block) + sizeof(size_t) + sizeof(void*);
            *reinterpret_cast<void**>(reinterpret_cast<size_t*>(target_block) + 1) = *reinterpret_cast<void**>(reinterpret_cast<size_t*>(current_free_block) + 1);
            *size_space = size_before + sizeof(size_t) + sizeof(void*);
        }
        auto size_prev_block = get_aviable_block_size(prev_free_block);
        if ( reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(prev_free_block) + size_prev_block + sizeof(size_t) + sizeof(void*)) == target_block )
        {
            *reinterpret_cast<size_t*>(prev_free_block) = size_prev_block + size_target_block + sizeof(size_t) + sizeof(void*);
            *reinterpret_cast<void**>(reinterpret_cast<size_t*>(prev_free_block) + 1) = get_aviable_block_next_block_address(target_block);
            *size_space = size_before + sizeof(size_t) + sizeof(void*);
        }
    }
    else if (prev_free_block != nullptr && current_free_block == nullptr)
    {
        void** ptr_prev_to_next = reinterpret_cast<void**>(reinterpret_cast<size_t*>(prev_free_block) + 1);
        *ptr_prev_to_next = target_block;
        void** ptr_target_block_to_next = reinterpret_cast<void**>(reinterpret_cast<size_t*>(target_block) + 1);
        *ptr_target_block_to_next = nullptr;

        auto size_prev_block = get_aviable_block_size(prev_free_block);
        *size_space = size_before + get_occupied_block_size(target_block);
        size_before = *size_space;
        if(reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(prev_free_block) + size_prev_block + sizeof(size_t) + sizeof(void*)) == target_block)
        {
            *reinterpret_cast<size_t*>(prev_free_block) = size_prev_block + size_target_block + sizeof(size_t) + sizeof(void*);
            *reinterpret_cast<void**>(reinterpret_cast<size_t*>(prev_free_block) + 1) = *reinterpret_cast<void**>(reinterpret_cast<size_t*>(target_block) + 1);
            *size_space = size_before + sizeof(size_t) + sizeof(void*);
        }
    }
    else
    {
        void **ptr_to_first_avail_block = reinterpret_cast<void**>( reinterpret_cast<unsigned char*>(_trusted_memory) 
                             + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(sem_t*) + sizeof(allocator_with_fit_mode::fit_mode));
        *ptr_to_first_avail_block = target_block;
        void** ptr_target_block_to_next = reinterpret_cast<void**>(reinterpret_cast<size_t*>(target_block) + 1);
        *ptr_target_block_to_next = nullptr;
        *size_space = size_before + get_occupied_block_size(target_block);
    } 
    if (log != nullptr)
    {
        log->information("dellocate memory. free summ size:" + std::to_string(*size_space));

        log->debug("finish deallocate memory");
        std::vector<allocator_test_utils::block_info> data = get_blocks_info();
        std::string data_str;
        for (block_info value : data)
        {
            std::string is_oc = value.is_block_occupied ? "YES" : "NO";
            data_str += (is_oc + "  " + std::to_string(value.block_size) + " | ");
        }
        log->debug("state blocks: " + data_str);

    }
    sem_post(sem);
}

inline void allocator_sorted_list::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    sem_t* sem = get_sem();
    while (sem_wait(sem) == -1) {
        sleep(1);
    }
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->debug("fit mode changing/adding start");
    }
    *reinterpret_cast<allocator_with_fit_mode::fit_mode *>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t)) = mode;
    if (log != nullptr)
    {
        log->debug("fit mode was changed");
    }
    sem_post(sem);
}

size_t allocator_sorted_list::get_ancillary_space_size(logger* log) const noexcept
{
    if (log != nullptr)
    {
        log->trace("get_ancillary_space_size start");
    }
    size_t ans = sizeof(logger *) + sizeof(allocator *) + sizeof(size_t) +  sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(void *);
    if (log != nullptr)
    {
        log->trace("get_ancillary_space_size finish");
    }
    return ans;
}

allocator_with_fit_mode::fit_mode allocator_sorted_list::get_fit_mode() const noexcept
{
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->trace("get_fit_mode start");
    }
    allocator_with_fit_mode::fit_mode ans = *reinterpret_cast<allocator_with_fit_mode::fit_mode *>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t));
    if (log != nullptr)
    {
        log->trace("get_fit_mode finish");
    }
    return ans;
}

void *allocator_sorted_list::get_first_aviable_block() const noexcept
{
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->trace("get_first_aviable_block start");
    }
    void* ans = *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(sem_t*));
    if (log != nullptr)
    {
        log->trace("get_first_aviable_block finish");
    }
    return ans;
}

allocator::block_size_t allocator_sorted_list::get_aviable_block_size(
    void *block_address) noexcept
{
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->trace("get_aviable_block_size start");
    }
    size_t ans = *reinterpret_cast<size_t *>(block_address);
    if (log != nullptr)
    {
        log->trace("get_aviable_block_size has been gone");
    }
    return ans;
}

void *allocator_sorted_list::get_aviable_block_next_block_address(
    void *block_address) noexcept
{
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->trace("get_aviable_block_next_block_address start");
    }
    void* ans =*reinterpret_cast<void**>(reinterpret_cast<size_t *>(block_address) + 1);
    if (log != nullptr)
    {
        log->trace("get_aviable_block_next_block_address has been gone");
    }
    return ans;
}

allocator::block_size_t allocator_sorted_list::get_occupied_block_size(
    void *block_address) noexcept
{
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->trace("get_occupied_block_size start");
    }
    size_t ans = *reinterpret_cast<allocator::block_size_t *>(block_address);
    if (log != nullptr)
    {
        log->trace("get_occupied_block_size has been gone");
    }
    return ans;
    
}

inline allocator *allocator_sorted_list::get_allocator() const
{
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->debug("get allocator start");
    }
    allocator* alloc = *reinterpret_cast<allocator**>(_trusted_memory);
    if (log != nullptr) 
    {
        log->debug("allocator has been gone");
    }
    return alloc;
}

std::vector<allocator_test_utils::block_info> allocator_sorted_list::get_blocks_info() const noexcept
{
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->debug("blocks info start");
    }
    std::vector<allocator_test_utils::block_info> data;
    void** ans = reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(sem_t*) + sizeof(allocator_with_fit_mode::fit_mode));
    void* cur_block = reinterpret_cast<void **>(ans + 1);

    size_t current_size = 0;
    size_t size_trusted_memory = *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t));

    while(current_size < size_trusted_memory)
    {
        allocator_test_utils::block_info value;
        value.block_size = *reinterpret_cast<size_t*>(cur_block); 
        void* ptr_next_or_memory =*reinterpret_cast<void**>(reinterpret_cast<size_t *>(cur_block) + 1);
        value.is_block_occupied = (ptr_next_or_memory == _trusted_memory) ? true : false;
        data.push_back(value);
        cur_block = reinterpret_cast<unsigned char *>(cur_block) + sizeof(size_t) + sizeof(void*) + value.block_size;
        current_size = current_size + sizeof(size_t) + sizeof(void* ) + value.block_size;
    }

    if (log != nullptr)
    {
        log->debug("block info has been gathered");
    }
    
    return data;
}


inline logger *allocator_sorted_list::get_logger() const
{
    return *reinterpret_cast<logger **>(reinterpret_cast<allocator **>(_trusted_memory) + 1);
}

sem_t *allocator_sorted_list::get_sem() const noexcept
{
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->trace("get_sem start");
    }
    sem_t* ans = *reinterpret_cast<sem_t **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t)
                             + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode));;
    if (log != nullptr)
    {
        log->trace("get_sem finish");
    }
    return ans;
}


inline std::string allocator_sorted_list::get_typename() const noexcept
{
    logger* log = get_logger();
    if (log != nullptr)
    {
        log->debug("typename start");
    }
    //here
    if (log != nullptr)
    {
        log->debug("typename has been gone");
    }
}