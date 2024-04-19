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


        public:

        iterator_data(
             AVL_tree<tkey, tvalue>::iterator_data const &other) : iterator_data(other.depth, other.get_key(), other.get_value(), other.subtree_height)
        {
        }

        iterator_data &operator=(
            AVL_tree<tkey, tvalue>::iterator_data const &other)
        {
            if (this != &other)
            {
                if (this->is_state_initialized())
                {
                    this->_key = other.key;
                    this->_value = other.value;
                }
                else
                {
                    allocator::construct(this->_key, other._key);
                    allocator::construct(this->_value, other._value);
                    this->_is_state_initialized = true;
                }
                this->depth = other.depth;
                this->subtree_height = other.subtree_height;
            }

            return *this;
        }

        iterator_data(
            AVL_tree<tkey, tvalue>::iterator_data &&other) noexcept : iterator_data(other.depth, other.get_key(), other.get_value(), other.subtree_height)
        {
            allocator::destruct(other._key);
            allocator::destruct(other._value);
            other._is_state_initialized = false;
            this->_is_state_initialized = true;
        }

        iterator_data &operator=(
            AVL_tree<tkey, tvalue>::iterator_data &&other) noexcept 
        {
            if (this != &other)
            {
                if (this->_is_state_initialized)
                {
                     _key(std::move(other.key));
                    _value(std::move(other.value));
                }
                else
                {
                    allocator::construct(this->_key, std::move(other._key));
                    allocator::construct(this->_value, std::move(other._value));
                    this->_is_state_initialized = true;
                }
                this->depth = other.depth;
                this->subtree_height = other.subtree_height;
                other.is_state_initialized = false;
            }

            return *this;
        }

        virtual ~iterator_data() noexcept 
        {
        }
        
    };

private:

    friend class balancer;

    class balancer
    {

    public:
        void balance1(
            std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path, AVL_tree<tkey, tvalue> const *tree)
        {
            if (path.empty())
            {
                // todo throw
                return;
            }
            if ((*path.top()) == nullptr)
            {
                path.pop();
                if (!path.empty()) balance1(path, tree);
            }
            AVL_tree<tkey, tvalue>::node *current = reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(*(path.top()));
            path.pop();
            int balance = get_balance_node(current);
            if (balance == 2)
            {
                auto *left_child = reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(current->left_subtree);

                typename binary_search_tree<tkey, tvalue>::node **p;//двойной указатель на current
                if (!path.empty())
                {
                    if (reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>( (*path.top())->left_subtree) == current)
                        p = reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>(&(*path.top())->left_subtree);
                    else
                        p = reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>(&(*path.top())->right_subtree);
                }
                else
                    p = const_cast<typename binary_search_tree<tkey, tvalue>::node **>(&(tree->_root));

                if (get_balance_node(left_child) == -1)
                {
                    auto *right_left_child = reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(left_child->right_subtree);
                    tree->small_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(&(current->left_subtree)));
                    update_height(left_child);
                    update_height(right_left_child);
                    left_child = reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(current->left_subtree);
                }
                tree->small_right_rotation(*p);
                update_height(current);
                update_height(left_child);
                if (!path.empty())
                    balance1(path, tree);
            }
            else if (balance == -2)
            {
                auto *right_child = reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(current->right_subtree);
                typename binary_search_tree<tkey, tvalue>::node **p;
                if (!path.empty())
                {
                    if (reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>( (*path.top())->left_subtree) == current)
                        p = reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>(&(*path.top())->left_subtree);
                    else
                        p = reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node **>(&(*path.top())->right_subtree);
                }
                else
                    p = const_cast<typename binary_search_tree<tkey, tvalue>::node **>(&(tree->_root));
                if (get_balance_node(right_child) == 1)
                {
                    auto *left_right_child = reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(right_child->left_subtree);
                    tree->small_right_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(&(current->right_subtree)));
                    update_height(right_child);
                    update_height(left_right_child);
                    right_child = reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(current->right_subtree);
                }
                tree->small_left_rotation(*p);
                update_height(current);
                update_height(right_child);
                if (!path.empty())
                    balance1(path, tree);
            }
            else
            {
                update_height(current);
                if (!path.empty())
                    balance1(path, tree);
            }
        }
        

        int get_balance_node(typename AVL_tree<tkey, tvalue>::node* node_)
        {
            if (node_->left_subtree != nullptr && node_->right_subtree != nullptr)
            {
                return reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(node_->left_subtree)->subtree_height - reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(node_->right_subtree)->subtree_height;
            }
            else if (node_->left_subtree == nullptr && node_->right_subtree != nullptr) return (int)-reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(node_->right_subtree)->subtree_height;
            else if (node_->right_subtree == nullptr && node_->left_subtree != nullptr) return (int)reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(node_->left_subtree)->subtree_height;
            else return 0;
        }

        int get_height(typename AVL_tree<tkey, tvalue>::node * node)
        {
            int ans = (node == nullptr) ? 0 : (int)reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(node)->subtree_height;
            return ans;
        }

        void update_height(typename AVL_tree<tkey, tvalue>::node* node){
            node->subtree_height = std::max(get_height(reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(node->left_subtree)), get_height(reinterpret_cast<typename AVL_tree<tkey, tvalue>::node *>(node->right_subtree))) + 1;
        }

    };
    
    class insertion_template_method final:
        public binary_search_tree<tkey, tvalue>::insertion_template_method,
        public balancer
    {
    
    public:
        
        explicit insertion_template_method(
            AVL_tree<tkey, tvalue> *tree,
            typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy);
    
    private:

        void balance(
            std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path) override
        {
            this->balance1(path, dynamic_cast<AVL_tree<tkey, tvalue> const *>(this->_tree));
        }

        
    };

    class obtaining_template_method final:
        public binary_search_tree<tkey, tvalue>::obtaining_template_method,
        public balancer
    {
    
    public:
        
        explicit obtaining_template_method(
            AVL_tree<tkey, tvalue> *tree);
        
    private:

        void balance(
            std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path) override
        {
            dynamic_cast<AVL_tree<tkey, tvalue> *>(this->_tree)->balance(path);
        }
        
    };
    
    class disposal_template_method final:
        public binary_search_tree<tkey, tvalue>::disposal_template_method,
        public balancer
    {

    public:
        
        explicit disposal_template_method(
            AVL_tree<tkey, tvalue> *tree,
            typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy);
        
    private:

        void balance(
            std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path) override
        {
            if (*(path.top()) == nullptr) 
            {
                path.pop();
            }
            this->balance1(path, dynamic_cast<AVL_tree<tkey, tvalue> const *>(this->_tree));
        }
        
    };

public:
    
    explicit AVL_tree(
        std::function<int(tkey const &, tkey const &)> comparer,
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
        allocator::construct(reinterpret_cast<typename AVL_tree<tkey, tvalue>::node*>(raw_space), key, value);
    }

    void call_node_constructor(
        typename binary_search_tree<tkey, tvalue>::node *raw_space,
        tkey const &key,
        tvalue &&value) override
    {
        allocator::construct(dynamic_cast<typename AVL_tree<tkey, tvalue>::node*>(raw_space), key, std::move(value));
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
    this->subtree_height = subtree_height;
}





template<
    typename tkey,
    typename tvalue>
AVL_tree<tkey, tvalue>::insertion_template_method::insertion_template_method(
    AVL_tree<tkey, tvalue> *tree,
    typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy):
    binary_search_tree<tkey, tvalue>::insertion_template_method(tree, insertion_strategy)
{

}

template<
    typename tkey,
    typename tvalue>
AVL_tree<tkey, tvalue>::disposal_template_method::disposal_template_method(
    AVL_tree<tkey, tvalue> *tree,
    typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy):
        binary_search_tree<tkey, tvalue>::disposal_template_method(tree, disposal_strategy)
{
    
}

template<
    typename tkey,
    typename tvalue>
AVL_tree<tkey, tvalue>::AVL_tree(
    std::function<int(tkey const &, tkey const &)> comparer,
    allocator *allocator,
    logger *logger,
    typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy,
    typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy):
    binary_search_tree<tkey, tvalue>(new AVL_tree<tkey, tvalue>::insertion_template_method(this, insertion_strategy), new typename binary_search_tree<tkey, tvalue>::obtaining_template_method(dynamic_cast<binary_search_tree<tkey, tvalue> *>(this)), new AVL_tree<tkey, tvalue>::disposal_template_method(this, disposal_strategy), comparer, allocator, logger)
{

}

template<
    typename tkey,
    typename tvalue>
AVL_tree<tkey, tvalue>::~AVL_tree() noexcept
{
}

template<
    typename tkey,
    typename tvalue>
AVL_tree<tkey, tvalue>::AVL_tree(
    AVL_tree<tkey, tvalue> const &other) : AVL_tree<tkey, tvalue>(other._keys_comparer, other.get_allocator(), other.get_logger(),
     binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy::throw_an_exception,
     binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy::throw_an_exception)
{

}

template<
    typename tkey,
    typename tvalue>
AVL_tree<tkey, tvalue> &AVL_tree<tkey, tvalue>::operator=(
    AVL_tree<tkey, tvalue> const &other)
{
    if (this != &other)
    {
        clear(this->_root);
        this->_allocator = other._allocator;
        this->_logger = other._logger;
        this->_keys_comparer = other._keys_comparer;
        this->_root = copy(other._root);
    }

    return *this;
}

template<
    typename tkey,
    typename tvalue>
AVL_tree<tkey, tvalue>::AVL_tree(
    AVL_tree<tkey, tvalue> &&other) noexcept : AVL_tree<tkey, tvalue>(other._keys_comparer, other.get_allocator(), other.get_logger())
{
    this->_insertion_template = other._insertion_template;
    other._insertion_template = nullptr;

    this->_obtaining_template = other._obtaining_template;
    other._obtaining_template = nullptr;

    this->_disposal_template = other._disposal_template;
    other._disposal_template = nullptr;

    this->_root = other._root;
    other._root = nullptr;
}

template<
    typename tkey,
    typename tvalue>
AVL_tree<tkey, tvalue> &AVL_tree<tkey, tvalue>::operator=(
    AVL_tree<tkey, tvalue> &&other) noexcept
{
    if (this != &other)
    {
        clear(this->_root);
        this->_allocator = other._allocator;
        other._allocator = nullptr;
        this->_logger = other._logger;
        other._logger = nullptr;
        this->_keys_comparer = other._keys_comparer;
        this->_root =other._root;
        other._root = nullptr;
    }

    return *this;
}



#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_AVL_TREE_H