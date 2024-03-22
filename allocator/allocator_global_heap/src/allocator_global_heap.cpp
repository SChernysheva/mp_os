#include <not_implemented.h>

#include "../include/allocator_global_heap.h"

allocator_global_heap::allocator_global_heap(
    logger *logger)
{
    if (logger != nullptr) logger->log("start constructing allocator", logger::severity::debug);//todo
    this->_logger = logger;
}

allocator_global_heap::~allocator_global_heap()
{
    log_with_guard_my("start destructor", logger::severity::debug);
}



[[nodiscard]] void *allocator_global_heap::allocate(
    size_t value_size,
    size_t values_count)
{
    log_with_guard_my("start allocate memory", logger::severity::debug);
    size_t requested_size = value_size * values_count;
    if (requested_size < sizeof(size_t) + sizeof(allocator*))
    {
        requested_size = sizeof(size_t) + sizeof(allocator*);
        log_with_guard_my("change requested size", logger::severity::warning);
    }
    auto common_size = requested_size + sizeof(size_t) + sizeof(allocator*);
    void* memory;
    try
    {
        memory = ::operator new(common_size);
    }
    catch(const std::exception& e)
    {
        log_with_guard_my("can't get memory", logger::severity::error);
        throw std::bad_alloc();
    }
    allocator** alloc = reinterpret_cast<allocator**>(memory);
    size_t* size_block = reinterpret_cast<size_t*>(alloc + 1);
    *size_block = requested_size;
    *alloc = this;
    return reinterpret_cast<unsigned char*>(memory) + sizeof(size_t) + sizeof(allocator*);
}

void allocator_global_heap::deallocate(
    void *at)
{
    log_with_guard_my("start deallocate block", logger::severity::debug);
    void *target_block_size = reinterpret_cast<void*>(reinterpret_cast<char *>(at) - sizeof(size_t));
    size_t* size_block = reinterpret_cast<size_t*>(target_block_size);
    std::string result;
    auto *bytes = reinterpret_cast<unsigned char *>(at);

    size_t size_object = *size_block;
    for (int i = 0; i < size_object; i++)
    {
        result += std::to_string(static_cast<int>(bytes[i])) + " ";
    }
    log_with_guard_my("state block:" + result, logger::severity::debug);
    
    allocator* alloc;
    try
    {
        alloc = reinterpret_cast<allocator*>(size_block - 1);
        // if (alloc != this)
        // {
        //     log_with_guard_my("block doesn't belong this allocate!", logger::severity::error);
        //     throw std::logic_error("block doesn't belong this allocate!");
        // } 
    }
    catch(const std::exception& e)
    {
        log_with_guard_my("block doesn't belong this allocate!", logger::severity::error);
        throw std::logic_error("block doesn't belong this allocate!");
    }
    ::operator delete(alloc);
    
}

inline logger *allocator_global_heap::get_logger() const
{//my
    return this->_logger;
}

inline std::string allocator_global_heap::get_typename() const noexcept
{
}
void allocator_global_heap::log_with_guard_my(
    std::string const &message,
    logger::severity severity) 
{//log
    logger *got_logger = this->get_logger(); 
    if (got_logger != nullptr)
    {
        got_logger->log(message, severity);
    }
}