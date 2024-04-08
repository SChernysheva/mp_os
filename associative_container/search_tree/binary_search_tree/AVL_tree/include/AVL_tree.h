#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_AVL_TREE_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_AVL_TREE_H

#include <binary_search_tree.h>

template<
    typename tkey,
    typename tvalue>
class AVL_tree final:
    public binary_search_tree<tkey, tvalue>
{

private:
    
    struct node final:
        binary_search_tree<tkey, tvalue>::node
    {

    public:

        size_t subtree_height;

    public:

        explicit node(
            tkey const &key,
            tvalue const &value):
                binary_search_tree<tkey, tvalue>::node(key, value),
                subtree_height(1)
        {

        }

        explicit node(
            tkey const &key,
            tvalue &&value):
                binary_search_tree<tkey, tvalue>::node(key, std::move(value)),
                subtree_height(1)
        {

        }
        
    };

public:
    
    struct iterator_data final:
        public binary_search_tree<tkey, tvalue>::iterator_data
    {
    
    public:
        
        size_t subtree_height;
    
    public:
        
        explicit iterator_data(
            unsigned int depth,
            tkey const &key,
            tvalue const &value,
            size_t subtree_height);
        
    };

private:

    class balancer
    {

    public:

        void balance(
            std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path)
        {
            // TODO: logic
        }

    };

    void balance(
        std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path)
    {
        // TODO: logic
    }
    
    class insertion_template_method final:
        public binary_search_tree<tkey, tvalue>::insertion_template_method
    {
    
    public:
        
        explicit insertion_template_method(
            AVL_tree<tkey, tvalue> *tree,
            typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy);
    
    private:

        void balance(
            std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path) override
        {
            dynamic_cast<AVL_tree<tkey, tvalue> *>(this->_tree)->balance(path);
        }
        
    };
    
    class disposal_template_method final:
        public binary_search_tree<tkey, tvalue>::disposal_template_method
    {
    
    public:
        
        explicit disposal_template_method(
            AVL_tree<tkey, tvalue> *tree,
            typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy);
        
    private:

        void balance(
            std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path) override
        {
            dynamic_cast<AVL_tree<tkey, tvalue> *>(this->_tree)->balance(path);
        }
        
    };

public:
    
    explicit AVL_tree(
        allocator *allocator = nullptr,
        logger *logger = nullptr,
        typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy = binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy::throw_an_exception,
        typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy = binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy::throw_an_exception);

public:
    
    ~AVL_tree() noexcept final;
    
    AVL_tree(
        AVL_tree<tkey, tvalue> const &other);
    
    AVL_tree<tkey, tvalue> &operator=(
        AVL_tree<tkey, tvalue> const &other);
    
    AVL_tree(
        AVL_tree<tkey, tvalue> &&other) noexcept;
    
    AVL_tree<tkey, tvalue> &operator=(
        AVL_tree<tkey, tvalue> &&other) noexcept;

private:

    size_t get_node_size() const noexcept override
    {
        return sizeof(typename AVL_tree<tkey, tvalue>::node);
    }

    void call_node_constructor(
        typename binary_search_tree<tkey, tvalue>::node *raw_space,
        tkey const &key,
        tvalue const &value) override
    {
        allocator::construct(dynamic_cast<typename AVL_tree<tkey, tvalue>::node>(raw_space), key, value);
    }

    void call_node_constructor(
        typename binary_search_tree<tkey, tvalue>::node *raw_space,
        tkey const &key,
        tvalue &&value) override
    {
        allocator::construct(dynamic_cast<typename AVL_tree<tkey, tvalue>::node>(raw_space), key, std::move(value));
    }
    
};

template<
    typename tkey,
    typename tvalue>
AVL_tree<tkey, tvalue>::iterator_data::iterator_data(
    unsigned int depth,
    tkey const &key,
    tvalue const &value,
    size_t subtree_height):
    binary_search_tree<tkey, tvalue>::iterator_data(depth, key, value)
{
    throw not_implemented("template<typename tkey, typename tvalue> AVL_tree<tkey, tvalue>::iterator_data::iterator_data(unsigned int, tkey const &, tvalue const &, size_t)", "your code should be here...");
}

template<
    typename tkey,
    typename tvalue>
AVL_tree<tkey, tvalue>::insertion_template_method::insertion_template_method(
    AVL_tree<tkey, tvalue> *tree,
    typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy):
    binary_search_tree<tkey, tvalue>::insertion_template_method(tree, insertion_strategy)
{
    throw not_implemented("template<typename tkey, typename tvalue> AVL_tree<tkey, tvalue>::insertion_template_method::insertion_template_method(AVL_tree<tkey, tvalue> *, typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy)", "your code should be here...");
}

// template<
//     typename tkey,
//     typename tvalue>
// AVL_tree<tkey, tvalue>::obtaining_template_method::obtaining_template_method(
//     AVL_tree<tkey, tvalue> *tree)
// {
//     throw not_implemented("template<typename tkey, typename tvalue> AVL_tree<tkey, tvalue>::obtaining_template_method::obtaining_template_method(AVL_tree<tkey, tvalue> *)", "your code should be here...");
// }

template<
    typename tkey,
    typename tvalue>
AVL_tree<tkey, tvalue>::disposal_template_method::disposal_template_method(
    AVL_tree<tkey, tvalue> *tree,
    typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy)
{
    throw not_implemented("template<typename tkey, typename tvalue> AVL_tree<tkey, tvalue>::disposal_template_method::disposal_template_method(AVL_tree<tkey, tvalue> *, typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy)", "your code should be here...");
}

template<
    typename tkey,
    typename tvalue>
AVL_tree<tkey, tvalue>::AVL_tree(
    allocator *allocator,
    logger *logger,
    typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy,
    typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy):
    binary_search_tree<tkey, tvalue>()
{

}

template<
    typename tkey,
    typename tvalue>
AVL_tree<tkey, tvalue>::~AVL_tree() noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue> AVL_tree<tkey, tvalue>::~AVL_tree() noexcept", "your code should be here...");
}

template<
    typename tkey,
    typename tvalue>
AVL_tree<tkey, tvalue>::AVL_tree(
    AVL_tree<tkey, tvalue> const &other)
{
    throw not_implemented("template<typename tkey, typename tvalue> AVL_tree<tkey, tvalue>::AVL_tree(AVL_tree<tkey, tvalue> const &)", "your code should be here...");
}

template<
    typename tkey,
    typename tvalue>
AVL_tree<tkey, tvalue> &AVL_tree<tkey, tvalue>::operator=(
    AVL_tree<tkey, tvalue> const &other)
{
    throw not_implemented("template<typename tkey, typename tvalue> AVL_tree<tkey, tvalue> &AVL_tree<tkey, tvalue>::operator=(AVL_tree<tkey, tvalue> const &)", "your code should be here...");
}

template<
    typename tkey,
    typename tvalue>
AVL_tree<tkey, tvalue>::AVL_tree(
    AVL_tree<tkey, tvalue> &&other) noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue> AVL_tree<tkey, tvalue>::AVL_tree(AVL_tree<tkey, tvalue> &&) noexcept", "your code should be here...");
}

template<
    typename tkey,
    typename tvalue>
AVL_tree<tkey, tvalue> &AVL_tree<tkey, tvalue>::operator=(
    AVL_tree<tkey, tvalue> &&other) noexcept
{
    throw not_implemented("template<typename tkey, typename tvalue> AVL_tree<tkey, tvalue> &AVL_tree<tkey, tvalue>::operator=(AVL_tree<tkey, tvalue> &&) noexcept", "your code should be here...");
}

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_AVL_TREE_H