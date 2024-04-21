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
    throw not_implemented("allocator_red_black_tree::~allocator_red_black_tree()", "your code should be here...");
}

allocator_red_black_tree::allocator_red_black_tree(
    allocator_red_black_tree const &other)
{
    throw not_implemented("allocator_red_black_tree::allocator_red_black_tree(allocator_red_black_tree const &)", "your code should be here...");
}

allocator_red_black_tree &allocator_red_black_tree::operator=(
    allocator_red_black_tree const &other)
{
    throw not_implemented("allocator_red_black_tree &allocator_red_black_tree::operator=(allocator_red_black_tree const &)", "your code should be here...");
}

allocator_red_black_tree::allocator_red_black_tree(
    allocator_red_black_tree &&other) noexcept
{
    throw not_implemented("allocator_red_black_tree::allocator_red_black_tree(allocator_red_black_tree &&) noexcept", "your code should be here...");
}

allocator_red_black_tree &allocator_red_black_tree::operator=(
    allocator_red_black_tree &&other) noexcept
{
    throw not_implemented("allocator_red_black_tree &allocator_red_black_tree::operator=(allocator_red_black_tree &&) noexcept", "your code should be here...");
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
        logger->error("can't get memory");///!!!!!!!
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
        exit(1);
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
    // sem_t* sem = get_sem();
    // while (sem_wait(sem) == -1) {
    //     sleep(1);
    // }
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

    size_t real_size = reinterpret_cast<size_t>(reinterpret_cast<unsigned char*>(target_block) + get_size_aviable_block(target_block) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool));
    size_t difference = real_size - requested_size;
    void* next_to_target = get_next_block(target_block);
    void* prev_to_target = get_previous_block(target_block);

    if (difference < get_meta_size_aviable_block())
    {
        requested_size = real_size;
    }
    if (requested_size != real_size)
    {
        void* new_block = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(target_block + get_meta_size_occupied_block() + requested_size));
        void **ptr_to_prev = reinterpret_cast<void **>(new_block);
        *ptr_to_prev = prev_to_target;

        void **ptr_to_next = reinterpret_cast<void **>(ptr_to_prev + 1);
        *ptr_to_next = next_to_target;

        if (prev_to_target != nullptr)
        {
            void** ptr_prev_to_this = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(prev_to_target) + sizeof(void*));
            *ptr_prev_to_this = new_block;
        }

        if (next_to_target != nullptr)
        {
            void** ptr_next_to_this = reinterpret_cast<void**>(next_to_target);
            *ptr_next_to_this = new_block;
        }
        insert_tree(new_block, difference);
    }
    //todo delete target 
    bool* is_occup = reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(target_block) + sizeof(void*) + sizeof(void*));
    *is_occup = 1;
    void** ptr_to_trusted = reinterpret_cast<void**>(is_occup + 1);
    *ptr_to_trusted = this->_trusted_memory;

    return reinterpret_cast<unsigned char*>(target_block) + sizeof(void*) * 3 + sizeof(bool);
}

void allocator_red_black_tree::deallocate(
    void *at)
{
    throw not_implemented("void allocator_red_black_tree::deallocate(void *)", "your code should be here...");
}

inline void allocator_red_black_tree::set_fit_mode(
    allocator_with_fit_mode::fit_mode mode)
{
    throw not_implemented("inline void allocator_red_black_tree::set_fit_mode(allocator_with_fit_mode::fit_mode)", "your code should be here...");
}

inline allocator *allocator_red_black_tree::get_allocator() const
{
    throw not_implemented("inline allocator *allocator_red_black_tree::get_allocator() const", "your code should be here...");
}

std::vector<allocator_test_utils::block_info> allocator_red_black_tree::get_blocks_info() const noexcept
{
    throw not_implemented("std::vector<allocator_test_utils::block_info> allocator_red_black_tree::get_blocks_info() const noexcept", "your code should be here...");
}

inline logger *allocator_red_black_tree::get_logger() const
{
    return *reinterpret_cast<logger **>(reinterpret_cast<allocator **>(_trusted_memory) + 1);
}

inline std::string allocator_red_black_tree::get_typename() const noexcept
{
    throw not_implemented("inline std::string allocator_red_black_tree::get_typename() const noexcept", "your code should be here...");
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
    return sizeof(void*) * 2 + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) * 3;
}
size_t allocator_red_black_tree::get_meta_size_occupied_block() const noexcept
{
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
    return *reinterpret_cast<void**>(block);
}
void** allocator_red_black_tree::get_ptr_previous_block(void* block) const
{
    return reinterpret_cast<void**>(block);
}

void* allocator_red_black_tree::get_next_block(void* block) const
{
    return *reinterpret_cast<void**>(reinterpret_cast<void**>(block) + 1);
}

void** allocator_red_black_tree::get_ptr_next_block(void* block) const
{
    return reinterpret_cast<void**>(reinterpret_cast<void**>(block) + 1);
}

bool allocator_red_black_tree::is_occupied_block(void* block) const
{
    return *reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) * 2);
}

size_t allocator_red_black_tree::get_size_aviable_block(void* block) const
{
    return *reinterpret_cast<size_t*>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) * 2 + sizeof(bool));
}

bool allocator_red_black_tree::is_color_red(void* block) const
{
    return *reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t));
}

void* allocator_red_black_tree::get_parent(void* block) const
{
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool));
}

void* allocator_red_black_tree::get_left_child(void* block) const
{
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*));
}

void* allocator_red_black_tree::get_right_child(void* block) const
{
    return *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
}

void allocator_red_black_tree::change_color(void* block)
{
    bool* is_red = reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t));
    (*is_red) ? *is_red = 0 : *is_red = 1;
}

void* allocator_red_black_tree::get_best_fit(size_t size)
{
    void* current = get_root();
    if (!current) return nullptr;
    size_t current_block_size = reinterpret_cast<size_t>(reinterpret_cast<unsigned char*>(current) + get_size_aviable_block(current) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool));
    while (current && current_block_size < size)
    {
        current = get_right_child(current);
        if (current) current_block_size = reinterpret_cast<size_t>(reinterpret_cast<unsigned char*>(current) + get_size_aviable_block(current) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool));
    }
    if (!current) return nullptr;
    
    while(get_left_child(current))
    {
        void* left = get_left_child(current);
        size_t size_left =  reinterpret_cast<size_t>(reinterpret_cast<unsigned char*>(left) + get_size_aviable_block(left) + get_meta_size_aviable_block() - sizeof(void*) * 3 - sizeof(bool));
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
    //now red
    void* parent = get_parent(block);
    if (!parent) return; //todo if root
    if (!is_color_red(parent)) return;
    void* uncle = get_uncle(block);
    if (is_color_red(uncle))
    {
        change_color(uncle);
        change_color(parent);
        void* grandpa = get_parent(parent);
        if (get_parent(grandpa)) change_color(grandpa);
        else return;
        balance_rb_height(grandpa);
    }
    else
    {
        if (get_left_child(parent) == block)
        {
            left_rotate(parent);
        }
        else
        {
            right_rotate(parent);
        }
    }
}

void allocator_red_black_tree::left_rotate(void* block)
{
    void *new_root =  get_right_child(block); // subtree_root->right_subtree;
    void** right_child_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
    *right_child_block = get_left_child(new_root);
    void** left_child_new_root = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*));
    *left_child_new_root = block;
    void* parent_old_root = get_parent(block);
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
    void *new_root =  get_left_child(block); 
    void** left_child_block = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*));
    *left_child_block = get_right_child(new_root);

    void** right_child_new_root = reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(block) + sizeof(void*) + sizeof(void*) + sizeof(bool) + sizeof(size_t) + sizeof(bool) + sizeof(void*) + sizeof(void*));
    *right_child_new_root = block;
    void* parent_old_root = get_parent(block);
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