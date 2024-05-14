#include <not_implemented.h>

#include "../include/allocator_red_black_tree.h"
#include <vector>
#include <iostream>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

allocator_red_black_tree::~allocator_red_black_tree()
{
    sem_t* sem = get_sem();
    logger* log = get_logger();
    log_with_guard_my("allocator destructor started", logger::severity::debug);
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
}

allocator_red_black_tree::allocator_red_black_tree(
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
            logger->error("Size of the requested memory is too small\n");
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

    sem_t** sem = reinterpret_cast<sem_t** >(fit_mode_space_address + 1);
    *sem = sem_open("/my_semaphore", O_CREAT | O_EXCL, 0666, 1); // Создаем или открываем семафор
    if (*sem == SEM_FAILED) {
        perror("sem_open");
        sem_unlink("/my_semaphore"); 
        sem_close(*sem);
        if (parent_allocator) parent_allocator->deallocate(_trusted_memory);
        else delete _trusted_memory;
        throw std::logic_error("all is bed");
       //// exit(1); 
    }
    void **root = reinterpret_cast<void **>(sem + 1);
    *root = reinterpret_cast<void *>(root + 1);

    void** ptr_to_prev =  reinterpret_cast<void**>(*root);
    *ptr_to_prev = nullptr;

    void** ptr_to_next = reinterpret_cast<void**>(ptr_to_prev + 1);
    *ptr_to_next = nullptr;

    bool* is_occupied = reinterpret_cast<bool*>(ptr_to_next + 1);
    *is_occupied = false;

    size_t* size_block = reinterpret_cast<size_t*>(is_occupied + 1);
    *size_block = space_size - get_meta_size_aviable_block();

    bool* is_red = reinterpret_cast<bool*>(size_block + 1);
    *is_red = 0;

    void** ptr_to_parent = reinterpret_cast<void**>(is_red + 1);
    *ptr_to_parent = nullptr;

    void** ptr_to_left_child = reinterpret_cast<void**>(ptr_to_parent + 1);
    *ptr_to_left_child = nullptr;

    void** ptr_to_right_child = reinterpret_cast<void**>(ptr_to_left_child + 1);
    *ptr_to_right_child = nullptr;


    if (logger != nullptr)
    {
        logger->debug("allocator created.");
    }
}

[[nodiscard]] void *allocator_red_black_tree::allocate(
    size_t value_size,
    size_t values_count)
{
    sem_t* sem = get_sem();
    while (sem_wait(sem) == -1) {
        sleep(1);
    }
    log_with_guard_my("start allocate memory", logger::severity::debug);
    auto requested_size = value_size * values_count;
    if (requested_size < get_meta_size_occupied_block())
    {
        requested_size = get_meta_size_occupied_block();
        log_with_guard_my("requested size was changed", logger::severity::warning);
    } 
    allocator_with_fit_mode::fit_mode fit_mode = get_fit_mode();
    void* target_block;
    switch(fit_mode)
    {
        case allocator_with_fit_mode::fit_mode::the_worst_fit:
        target_block = get_worst_fit(requested_size);
        break;

        case allocator_with_fit_mode::fit_mode::the_best_fit:
        target_block = get_best_fit(requested_size);
        break;

        case allocator_with_fit_mode::fit_mode::first_fit:
        target_block = get_first_fit(requested_size);
        break;
    }
    if (target_block == nullptr)
    {
        std::__throw_logic_error("nullptr");
    }
    auto real_size = get_size_aviable_block(target_block) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool);
    auto difference = real_size - requested_size;
    void* next_to_target = get_next_block(target_block);
    void* prev_to_target = get_previous_block(target_block);
    if (difference < get_meta_size_aviable_block())
    {
        log_with_guard_my("change size" , logger::severity::debug);
        requested_size = real_size;
    }
    if (requested_size != real_size)
    {
        void* new_block = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(target_block) + get_meta_size_occupied_block() + requested_size);
        void **ptr_to_prev = reinterpret_cast<void **>(new_block);
        *ptr_to_prev = target_block;

        void **ptr_to_next = reinterpret_cast<void **>(ptr_to_prev + 1);
        *ptr_to_next = next_to_target;

        void** this_to_next = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*));
        *this_to_next = new_block;

        if (next_to_target != nullptr)
        {
            void** ptr_next_to_this = reinterpret_cast<void**>(next_to_target);
            *ptr_next_to_this = new_block;
        }

        insert_tree(new_block, difference - get_meta_size_aviable_block());
    }
    delete_from_tree(target_block);
    std::cout << "here" << std::endl;
    //
    bool* is_occup = reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*) + sizeof(void*));
    *is_occup = 1;
    void** ptr_to_trusted = reinterpret_cast<void**>(is_occup + 1);
    *ptr_to_trusted = this->_trusted_memory;
    std::vector<allocator_test_utils::block_info> data = get_blocks_info();
    std::string data_str;
    for (block_info value : data)
    {
        std::string is_oc = value.is_block_occupied ? "YES" : "NO";
        data_str += (is_oc + "  " + std::to_string(value.block_size) + " | ");
    }
    log_with_guard_my("state blocks: " + data_str, logger::severity::debug);
    sem_post(sem);
    return reinterpret_cast<unsigned char*>(target_block) + sizeof(void*) * 3 + sizeof(bool);
}

void allocator_red_black_tree::deallocate(
    void *at)
{
    sem_t* sem = get_sem();
    log_with_guard_my("start deallocate memory", logger::severity::debug);
    while (sem_wait(sem) == -1) {
        sleep(1);
    }
    if (at == nullptr)
    {
        sem_post(sem);
        return;
    }
    void *target_block = reinterpret_cast<void*>(reinterpret_cast<char *>(at) - sizeof(void*) * 3 - sizeof(bool)); 
    void* pointer = *reinterpret_cast<void**>(reinterpret_cast<char *>(target_block) + sizeof(void*) + sizeof(void*) + sizeof(bool));
    if (pointer != _trusted_memory)
    {
        log_with_guard_my("block doesn't belong this allocator!", logger::severity::error);
        sem_unlink("/my_semaphore");
        sem_close(sem);
        throw std::logic_error("block doesn't belong this allocator!");
    }
    size_t size_target_block;
    void* next_block = get_next_block(target_block);
    void* prev_block = get_previous_block(target_block);
    if (next_block == nullptr)
    {
        size_target_block = reinterpret_cast<char*>(_trusted_memory) + get_ancillary_space_size(nullptr) + get_all_size() - reinterpret_cast<char*>(at);
    }
    else size_target_block = reinterpret_cast<char*>(next_block) - reinterpret_cast<char*>(at);
    std::string result;
    auto* bytes = reinterpret_cast<unsigned char*>(at);

    for (int i = 0; i < size_target_block; i++)
    {
        result += std::to_string(static_cast<int>(bytes[i])) + " ";
    }
    size_t clear_size_target_without_meta_for_free = size_target_block + get_meta_size_occupied_block() - get_meta_size_aviable_block();
    bool done = 0;
    if (next_block && !is_occupied_block(next_block))
    {
        size_t size_next_block = get_size_aviable_block(next_block);
        size_t size_next_with_meta = size_next_block + get_meta_size_aviable_block();
        size_t new_size_target = clear_size_target_without_meta_for_free + size_next_with_meta;
        delete_from_tree(next_block);
        insert_tree(target_block, new_size_target);

        void* next_next_block = get_next_block(next_block);
        void** ptr_this_next = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*));
        *ptr_this_next = next_next_block;
        if (next_next_block != nullptr)
        {
            void **ptr_next_to_this = reinterpret_cast<void**>(next_next_block);
            *ptr_next_to_this = target_block;
        }
        done = 1;
        clear_size_target_without_meta_for_free = new_size_target;

    }
    if (prev_block && !is_occupied_block(prev_block))
    {
        size_t size_prev_block = get_size_aviable_block(prev_block);

        size_t new_size_target = clear_size_target_without_meta_for_free + size_prev_block + get_meta_size_aviable_block();
        if (!is_occupied_block(target_block)) delete_from_tree(target_block);
        delete_from_tree(prev_block);
        insert_tree(prev_block, new_size_target);

        next_block = get_next_block(target_block);

        void** ptr_this_next = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(prev_block) + sizeof(void*));
        *ptr_this_next = next_block;
        if (next_block != nullptr)
        {
            void **ptr_next_to_this = reinterpret_cast<void**>(next_block);
            *ptr_next_to_this = prev_block;
        }
        log_with_guard_my("finish dellocate memory", logger::severity::debug);
        std::vector<allocator_test_utils::block_info> data = get_blocks_info();
        std::string data_str;
        for (block_info const &value : data)
        {
            std::string is_oc = value.is_block_occupied ? "YES" : "NO";
            data_str += (is_oc + "  " + std::to_string(value.block_size) + " | ");
        }
        log_with_guard_my("state blocks: " + data_str, logger::severity::debug);
        sem_post(sem);
        return;
    }

    if (!done)
    {
        insert_tree(target_block, clear_size_target_without_meta_for_free);
    }
    log_with_guard_my("finish dellocate memory", logger::severity::debug);
    std::vector<allocator_test_utils::block_info> data = get_blocks_info();
    std::string data_str;
    for (block_info value : data)
    {
        std::string is_oc = value.is_block_occupied ? "YES" : "NO";
        data_str += (is_oc + "  " + std::to_string(value.block_size) + " | ");
    }
    log_with_guard_my("state blocks: " + data_str, logger::severity::debug);
    sem_post(sem);
    return;
}

inline void allocator_red_black_tree::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    sem_t* sem = get_sem();
    while (sem_wait(sem) == -1) {
        sleep(1);
    }
    log_with_guard_my("set_fit_mode start", logger::severity::trace);
    *reinterpret_cast<allocator_with_fit_mode::fit_mode *>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t)) = mode;
    sem_post(sem);
}

inline allocator *allocator_red_black_tree::get_allocator() const
{
    log_with_guard_my("get_allocator start", logger::severity::trace);

    return *reinterpret_cast<allocator**>(_trusted_memory);
}

std::vector<allocator_test_utils::block_info> allocator_red_black_tree::get_blocks_info() const noexcept
{
    log_with_guard_my("get_blocks_info start", logger::severity::trace);

    std::vector<allocator_test_utils::block_info> data;
    void** ans = reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(sem_t*) + sizeof(allocator_with_fit_mode::fit_mode));
    void* current = reinterpret_cast<void **>(ans + 1);

    while(current != nullptr)
    {
        allocator_test_utils::block_info value;
        value.is_block_occupied = is_occupied_block(current);
        if (value.is_block_occupied)
        {
            if (get_next_block(current) == nullptr)
            {
                value.block_size = reinterpret_cast<char *>(_trusted_memory) + get_ancillary_space_size(nullptr) + get_all_size() - reinterpret_cast<char *>(current) - get_meta_size_occupied_block();
            }
            else value.block_size = reinterpret_cast<char *>(get_next_block(current)) - reinterpret_cast<char *>(current) - get_meta_size_occupied_block();
        }
        else
        {
            value.block_size = get_size_aviable_block(current);
        }
        data.push_back(value);
        //std::cout << std::to_string(value.block_size) << std::endl;
        current = get_next_block(current);
    }
    return data;
}

inline logger *allocator_red_black_tree::get_logger() const
{
    return *reinterpret_cast<logger **>(reinterpret_cast<allocator **>(_trusted_memory) + 1);
}

inline std::string allocator_red_black_tree::get_typename() const noexcept
{
}
size_t allocator_red_black_tree::get_ancillary_space_size(logger* log) const noexcept
{
    if (log != nullptr) log->trace("get_ancillary_space_size start");
    return sizeof(logger *) + sizeof(allocator *) + sizeof(size_t) +  sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(sem_t*) + sizeof(void *); //sizeof(sem_t*)
}
allocator_with_fit_mode::fit_mode allocator_red_black_tree::get_fit_mode() const noexcept
{
    log_with_guard_my("get_fit_mode start", logger::severity::trace);
    return *reinterpret_cast<allocator_with_fit_mode::fit_mode *>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t));
}
void *allocator_red_black_tree::get_root() const noexcept
{
    log_with_guard_my("get_root start", logger::severity::trace);
    return *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(sem_t*));
}

sem_t *allocator_red_black_tree::get_sem() const noexcept
{
    log_with_guard_my("get_sem start", logger::severity::trace);
    return *reinterpret_cast<sem_t **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t)
                             + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode));;
}

size_t allocator_red_black_tree::get_meta_size_aviable_block() const noexcept
{
    log_with_guard_my("get_meta_size_aviable_block start", logger::severity::trace);
    return sizeof(void*) * 2 + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) * 3;
}
size_t allocator_red_black_tree::get_meta_size_occupied_block() const noexcept
{
    log_with_guard_my("get_meta_size_occupied_block start", logger::severity::trace);
    return sizeof(void*) * 3 + sizeof(bool);
}
void allocator_red_black_tree::log_with_guard_my(
    std::string const &message,
    logger::severity severity) const
{
    logger *got_logger = get_logger(); 
    if (got_logger != nullptr)
    {
        got_logger->log(message, severity);
    }
}
void* allocator_red_black_tree::get_previous_block(void* block) const
{
    log_with_guard_my("get_previous_block start", logger::severity::trace);
    return *reinterpret_cast<void**>(block);
}
void** allocator_red_black_tree::get_ptr_previous_block(void* block) const
{
    log_with_guard_my("get_ptr_previous_block start", logger::severity::trace);
    return reinterpret_cast<void**>(block);
}

void* allocator_red_black_tree::get_next_block(void* block) const
{
    log_with_guard_my("get_next_block start", logger::severity::trace);
    return *reinterpret_cast<void**>(reinterpret_cast<void**>(block) + 1);
}

void** allocator_red_black_tree::get_ptr_next_block(void* block) const
{
    log_with_guard_my("get_ptr_next_block start", logger::severity::trace);
    return reinterpret_cast<void**>(reinterpret_cast<void**>(block) + 1);
}

bool allocator_red_black_tree::is_occupied_block(void* block) const
{
    log_with_guard_my("is_occupied_block start", logger::severity::trace);
    return *reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) * 2);
}

size_t allocator_red_black_tree::get_size_aviable_block(void* block) const
{
    log_with_guard_my("get_size_aviable_block start", logger::severity::trace);
    return *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) * 2 + sizeof(bool));
}

bool allocator_red_black_tree::is_color_red(void* block) const
{
    log_with_guard_my("is_color_red start", logger::severity::trace);
    return *reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t));
}

void* allocator_red_black_tree::get_parent(void* block) const
{
    log_with_guard_my("get_parent start", logger::severity::trace);
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool));
}

void* allocator_red_black_tree::get_left_child(void* block) const
{
    log_with_guard_my("get_left_child start", logger::severity::trace);
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*));
}

void* allocator_red_black_tree::get_right_child(void* block) const
{
    log_with_guard_my("get_right_child start", logger::severity::trace);
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
}

void allocator_red_black_tree::change_color(void* block)
{
    log_with_guard_my("change_color start", logger::severity::trace);
    bool* is_red = reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t));
    (*is_red) ? *is_red = 0 : *is_red = 1;
}

void* allocator_red_black_tree::get_best_fit(size_t size)
{//check
    void* current = get_root();
    if (!current) return nullptr;
    auto current_block_size = get_size_aviable_block(current) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool);
    while (current_block_size < size)
    {
        //std::cout << std::to_string(get_size_aviable_block(current)) << std::endl;
        current = get_right_child(current);
        if (current) current_block_size = get_size_aviable_block(current) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool);
        else break;
    }
    if (!current) return nullptr;
    while(get_left_child(current) != nullptr)
    {
        void* left = get_left_child(current);
        auto size_left =  get_size_aviable_block(left) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool);
        if (size_left >= size)
        {
            current = left;
        }
        else
        {
            break;
        }
    }
    return current;
}

void* allocator_red_black_tree::get_worst_fit(size_t size)
{
    log_with_guard_my("get_worst_fit start", logger::severity::trace);
    void* current = get_root();
    while (get_right_child(current))
    {
        current = get_right_child(current);
    }
    size_t current_block_size = reinterpret_cast<size_t>(reinterpret_cast<unsigned char*>(current) + get_size_aviable_block(current) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool));
    return (current_block_size >= size) ? current : nullptr;
}
void* allocator_red_black_tree::get_first_fit(size_t size)
{
    log_with_guard_my("get_first_fit start", logger::severity::trace);
    void* current = get_root();
    size_t current_block_size = reinterpret_cast<size_t>(reinterpret_cast<unsigned char*>(current) + get_size_aviable_block(current) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool));
    while (current && current_block_size < size)
    {
        current = get_right_child(current);
        if (current) current_block_size = reinterpret_cast<size_t>(reinterpret_cast<unsigned char*>(current) + get_size_aviable_block(current) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool));
    }
    return current;
}
void* allocator_red_black_tree::get_uncle(void* block)
{
    log_with_guard_my("get_uncle start", logger::severity::trace);
    void* parent = get_parent(block);
    if (!parent) return nullptr;
    void *grandpa = get_parent(parent);
    if (!grandpa)
        return nullptr;
    if (get_left_child(grandpa) == parent)
    {
        return get_right_child(grandpa);
    }
    return get_left_child(grandpa);
}


void allocator_red_black_tree::balance_rb_height(void* block)
{
    log_with_guard_my("balance_rb_height start", logger::severity::trace);
    //now red
    void* parent = get_parent(block);
    if (!parent) 
    {
        if (is_color_red(block)) change_color(block);
        return; 
    }//todo if root
    if (!is_color_red(parent)) 
    {
        return;
    }
    void* uncle = get_uncle(block);
    void* grandpa = get_parent(parent);
    if (get_left_child(grandpa) == parent)
    {
        if (uncle && is_color_red(uncle))
        {
            change_color(parent);
            change_color(uncle);
            change_color(grandpa);
            balance_rb_height(grandpa);
        }
        else
        {
            change_color(block);
            change_color(grandpa);
            if (get_right_child(parent) == block)
            {
                left_rotate(parent);
            }
            right_rotate(grandpa); 
        }
    }
    else
    {
        if (uncle && is_color_red(uncle))
        {
            change_color(parent);
            change_color(uncle);
            change_color(grandpa);
            balance_rb_height(grandpa);
        }
        else
        {
            change_color(block);
            change_color(grandpa);
            if (get_left_child(parent) == block)
            {
                right_rotate(parent);
            }
            left_rotate(grandpa);
        }
    }
}

void allocator_red_black_tree::left_rotate(void* block)
{
    log_with_guard_my("left_rotate start", logger::severity::debug);
    void* parent_old_root = get_parent(block);
    void *new_root =  get_right_child(block); // subtree_root->right_subtree;
    void* left_root_child = get_left_child(new_root);
    void** right_child_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
    *right_child_block = left_root_child;

    if (left_root_child)
    {
        *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(left_root_child) + sizeof(void *) + sizeof(void *) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = block;
    }

    
    void** left_child_new_root = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*));
    *left_child_new_root = block;
    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = new_root;



    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(new_root) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = parent_old_root;
    if (!parent_old_root)
    {
        *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(sem_t*)) = new_root;
    }
    else
    {
        if (block == get_right_child(parent_old_root))
        {
            void** right_child_parent = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_old_root) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
            *right_child_parent = new_root;
        }
        else
        {
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_old_root) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*)) = new_root;
        }
    }
}

void allocator_red_black_tree::right_rotate(void* block)
{
    log_with_guard_my("right_rotate start", logger::severity::debug);
    void* parent_old_root = get_parent(block);
    void *new_root =  get_left_child(block);
    void* right_root_child = get_right_child(new_root); 
    void** left_child_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*));
    *left_child_block = right_root_child;


     if (right_root_child)
    {
        *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(right_root_child) + sizeof(void *) + sizeof(void *) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = block;
    }

    void** right_child_new_root = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
    *right_child_new_root = block;
    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = new_root;

    
    *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(new_root) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = parent_old_root;

    if (!parent_old_root)
    {
        *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(sem_t*)) = new_root;
    }
    else
    {
        if (block == get_right_child(parent_old_root))
        {
            void** right_child_parent = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_old_root) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
            *right_child_parent = new_root;
        }
        else
        {
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_old_root) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*)) = new_root;
        }
    }
}

void allocator_red_black_tree::insert_tree(void* block, size_t size)
{
    log_with_guard_my("insert_tree start", logger::severity::trace);
    void* parent = nullptr;
    void* current = get_root();
    while(true)
    {
        parent = current;
        if (size < get_size_aviable_block(current))
        {
            current = get_left_child(current);
        }
        else 
        {
            current = get_right_child(current);
        }
        if (current == nullptr) break;
    }
    void** ptr_to_prev = reinterpret_cast<void**>(block);

    void** ptr_to_next = reinterpret_cast<void**>(ptr_to_prev + 1);

    bool* is_occupied = reinterpret_cast<bool*>(ptr_to_next + 1);
    *is_occupied = false;

    size_t* size_block = reinterpret_cast<size_t*>(is_occupied + 1);
    *size_block = size;

    bool* is_red = reinterpret_cast<bool*>(size_block + 1);
    *is_red = 1;

    void** ptr_to_parent = reinterpret_cast<void**>(is_red + 1);
    *ptr_to_parent = parent;

    void** ptr_to_left_child = reinterpret_cast<void**>(ptr_to_parent + 1);
    *ptr_to_left_child = nullptr;

    void** ptr_to_right_child = reinterpret_cast<void**>(ptr_to_left_child + 1);
    *ptr_to_right_child = nullptr;

    if (size < get_size_aviable_block(parent))
    {
        void** left_child = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*));
        *left_child = block;
    }
    else
    {
        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*)) = block;
    }
    balance_rb_height(block);
}

void allocator_red_black_tree::delete_from_tree(void* block)
{
    log_with_guard_my("delete_from_tree start", logger::severity::trace);
    bool is_red = is_color_red(block);
    int count_childs;
    if (get_left_child(block) != nullptr && get_right_child(block) != nullptr) count_childs = 2;
    else if (get_right_child(block) != nullptr || get_left_child(block) != nullptr)
    {
        count_childs = 1;
    }
    else count_childs = 0;

    //red
    if(is_red && count_childs == 0)
    {
        std::cout << "case1" << std::endl;
        void* parent = get_parent(block);
        if (get_left_child(parent) == block) 
        {
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*)) = nullptr;
        }
        else
        {
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*)) = nullptr;
        }
        return;
    }
    if (count_childs == 2)
    {//check, parent
        std::cout << "case2" << std::endl;
        void* target_to_change_block = get_right_child(block);
        while(get_left_child(target_to_change_block) != nullptr)
        {
            target_to_change_block = get_left_child(target_to_change_block);
        }
        void* parent_block = get_parent(block);
        void* parent_target = get_parent(target_to_change_block);
        if (is_color_red(target_to_change_block) != is_color_red(block))
        {
            change_color(block);
            change_color(target_to_change_block);
        }
        void* copy_block = block;
        void* copy_target = target_to_change_block;

        void* left_child_block = get_left_child(block);
        void* right_child_block = get_right_child(block);

        void* right_child_target = get_right_child(target_to_change_block);
        if (parent_block != nullptr)
        {
            if (get_left_child(parent_block) == block)
            {
                *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(parent_block) + sizeof(void *) + sizeof(void *) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void *)) = target_to_change_block;
            }
            else
            {
                *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(parent_block) + sizeof(void *) + sizeof(void *) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void *) + sizeof(void *)) = target_to_change_block;
            }
        }
        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_to_change_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = parent_block;
        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_to_change_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*)) = left_child_block;//here
        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_to_change_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*)) = right_child_block;




        if (get_left_child(parent_target) == target_to_change_block)
        {
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(parent_target) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*)) = block;
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = parent_target;
        }
        else
        {
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(target_to_change_block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*)) = block;
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = target_to_change_block;
        }
        //*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = parent_target;

        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*)) = nullptr;
        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*)) = right_child_target;
        if (parent_block == nullptr)
        {
            *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(sem_t*)) = target_to_change_block;
        }
        delete_from_tree(block);
    }
    if(!is_color_red(block) && count_childs == 1)
    {
        std::cout << "case3" << std::endl;
        void* child = (get_left_child(block) != nullptr) ?  get_left_child(block) : get_right_child(block);
        if (is_color_red(block) != is_color_red(child))
        {
            change_color(block);
            change_color(child);
        }
        void* parent_block = get_parent(block);
        void* left_child_child = get_left_child(child);
        void* right_child_child = get_right_child(child);
        if (parent_block != nullptr)
        {
            if (get_left_child(parent_block) == block)
            {
                *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(parent_block) + sizeof(void *) + sizeof(void *) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void *)) = child;
            }
            else
            {
                *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(parent_block) + sizeof(void *) + sizeof(void *) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void *) + sizeof(void *)) = child;
            }
        }
        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(child) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = parent_block;


        if (get_right_child(block) == child)
        {
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(child) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*)) = block;
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(child) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*)) = nullptr;   
        }
        else
        {
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(child) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*)) = block; 
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(child) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*)) = nullptr;     
        }

        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = child;

        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*)) = left_child_child;
        *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*)) = right_child_child;
        if (left_child_child)
        {
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(left_child_child) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = block;
        }
        if (right_child_child)
        {
            *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(right_child_child) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool)) = block;
        }
        if (parent_block == nullptr)
        {
            *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(_trusted_memory) + sizeof(allocator *) + sizeof(logger *) + sizeof(size_t) + sizeof(size_t) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(sem_t*)) = child;
        }
        delete_from_tree(block);
    }
    if (!is_color_red(block) && count_childs == 0)
    {
        std::cout << "case4" << std::endl;
        void* parent = get_parent(block);
        bool is_left_child = 0;
        if (parent != nullptr)
        {
            if (get_left_child(parent) == block)
            {
                *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(parent) + sizeof(void *) + sizeof(void *) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void *)) = nullptr;
            }
            else
            {
                *reinterpret_cast<void **>(reinterpret_cast<unsigned char *>(parent) + sizeof(void *) + sizeof(void *) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void *) + sizeof(void *)) = nullptr;
            }
            bool is_left_child = (get_left_child(parent) == block) ? 1 : 0;
        }
        rebalance_after_delete(block, is_left_child);
    }
}

void allocator_red_black_tree::rebalance_after_delete(void* block, bool is_left_child)
{
    log_with_guard_my("rebalance_after_delete start", logger::severity::trace);
    if (block == nullptr || get_parent(block) == nullptr || is_color_red(block)) return; //check
    void* parent = get_parent(block);
    void* brother = (is_left_child) ? get_right_child(parent) : get_left_child(parent);
    if (brother == nullptr) {};//todo

    if (!is_color_red(brother))
    {
        void* left_child_brother = get_left_child(brother);
        void* right_child_brother = get_right_child(brother);
        if ((left_child_brother != nullptr && is_color_red(left_child_brother) || (right_child_brother != nullptr && is_color_red(right_child_brother))))
        {
            if (is_left_child)
            {
                if (right_child_brother != nullptr && is_color_red(right_child_brother))
                { // parent = null ?
                    if (is_color_red(brother) != is_color_red(parent))
                        change_color(brother);
                    change_color(right_child_brother);
                    change_color(parent);
                    left_rotate(parent);
                    return;
                }
                if ((left_child_brother != nullptr && is_color_red(left_child_brother) && (right_child_brother != nullptr && !is_color_red(right_child_brother))))
                {
                    change_color(brother);
                    change_color(left_child_brother);
                    right_rotate(brother);
                    rebalance_after_delete(block, is_left_child);
                }
            }
            else
            {
                if (left_child_brother != nullptr && is_color_red(left_child_brother))
                { // parent = null ?
                    if (is_color_red(brother) != is_color_red(parent))
                        change_color(brother);
                    change_color(left_child_brother);
                    change_color(parent);
                    right_rotate(parent);
                    return;
                }
                if ((right_child_brother != nullptr && is_color_red(right_child_brother) && (left_child_brother != nullptr && !is_color_red(left_child_brother))))
                {
                    change_color(brother);
                    change_color(right_child_brother);
                    left_rotate(brother);
                    rebalance_after_delete(block, is_left_child);
                }
            }
        }
        if ((left_child_brother != nullptr && !is_color_red(left_child_brother) && (right_child_brother != nullptr && !is_color_red(right_child_brother))))
        {//parent = null?
            change_color(brother);
            if (is_color_red(parent)) 
            {
                change_color(parent);
                return;
            }
            else
            {
                if (is_left_child)
                {
                    void *grandpa = get_parent(parent);
                    if (grandpa && get_left_child(grandpa) == parent)
                        rebalance_after_delete(parent, 1);
                    else if (grandpa)
                        rebalance_after_delete(parent, 0);
                }
                else
                {
                    void *grandpa = get_parent(parent);
                    if (grandpa && get_right_child(grandpa) == parent)
                        rebalance_after_delete(parent, 1);
                    else if (grandpa)
                        rebalance_after_delete(parent, 0);
                }
            }
        }
    }
    else
    {
        change_color(parent);
        change_color(brother);
        if (is_left_child) 
        {
            left_rotate(parent);
            rebalance_after_delete(block, 1);
        }
        else {
            right_rotate(parent);
            rebalance_after_delete(block, 0);
        }
    }
}

size_t allocator_red_black_tree::get_all_size() const
{//a l s
    log_with_guard_my("get_all_size start", logger::severity::trace);
    return *reinterpret_cast<size_t*>(reinterpret_cast<char *>(_trusted_memory) + sizeof(allocator*) + sizeof(logger*) + sizeof(size_t));
}

