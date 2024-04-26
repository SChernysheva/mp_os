#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_RED_BLACK_TREE_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_RED_BLACK_TREE_H

#include <binary_search_tree.h>

template<
    typename tkey,
    typename tvalue>
class red_black_tree final:
    public binary_search_tree<tkey, tvalue>
{

public:
    
    enum class node_color
    {
        RED,
        BLACK
    };

private:
    
    struct node final:
        binary_search_tree<tkey, tvalue>::node
    {

    public:

        node_color color;

    public:

        explicit node(
            tkey const &key,
            tvalue const &value):
                binary_search_tree<tkey, tvalue>::node(key, value),
                color(node_color::RED)
        {

        }

        explicit node(
            tkey const &key,
            tvalue &&value):
                binary_search_tree<tkey, tvalue>::node(key, std::move(value)),
                color(node_color::RED)
        {

        }
        
    };

public:
    
    struct iterator_data final:
        public binary_search_tree<tkey, tvalue>::iterator_data
    {
    
    public:
        
        node_color color;
    
    public:
        
        explicit iterator_data(
            unsigned int depth,
            tkey const &key,
            tvalue const &value,
            node_color color);
        
    };

private:

    friend class balancer;

    class balancer
    {

    public:
        void balance_after_insert(std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path, red_black_tree<tkey, tvalue> const *tree)
        {
            if (path.empty())
            {
                return;
            }
            red_black_tree<tkey, tvalue>::node *current = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));
            red_black_tree<tkey, tvalue>::node **curent_ = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node **>(path.top());

            if (current == tree->_root)
            {
                current->color = node_color::BLACK;
                return;
            }
            path.pop();
            //if empty -> root
            red_black_tree<tkey, tvalue>::node *parent = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));
            red_black_tree<tkey, tvalue>::node **parent_ = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node **>(path.top());

            if (parent->color == node_color::BLACK) return;
            path.pop();

            red_black_tree<tkey, tvalue>::node *grandpa = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));
            red_black_tree<tkey, tvalue>::node **grandpa_ = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node **>(path.top());

            red_black_tree<tkey, tvalue>::node *uncle;
        
            if (grandpa->left_subtree == parent) 
            {
                uncle =  reinterpret_cast<red_black_tree<tkey, tvalue>::node *>(grandpa->right_subtree);
                if (uncle && uncle->color == node_color::RED)
                {
                    parent->color = node_color::BLACK;
                    uncle->color = node_color::BLACK;
                    grandpa->color = node_color::RED;
                    balance_after_insert(path, tree);
                }
                else
                {
                    //uncle = reinterpret_cast<red_black_tree<tkey, tvalue>::node *>(grandpa->left_subtree);
                    if (parent->right_subtree == current)
                    {
                        if (parent == tree->_root) tree->small_left_rotation(*const_cast<typename binary_search_tree<tkey, tvalue>::node**>(&(tree->_root)));
                        else tree->small_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(parent_));
                    }
                    parent->color = node_color::BLACK;//here check
                    grandpa->color = node_color::RED;
                    if (grandpa == tree->_root) tree->small_right_rotation(*const_cast<typename binary_search_tree<tkey, tvalue>::node**>(&(tree->_root)));
                    else 
                    {
                        tree->small_right_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(grandpa_));
                    }
                }
            }
            else
            {
                uncle =  reinterpret_cast<red_black_tree<tkey, tvalue>::node *>(grandpa->left_subtree);
                if (uncle && uncle->color == node_color::RED)
                {
                    parent->color = node_color::BLACK;
                    uncle->color = node_color::BLACK;
                    grandpa->color = node_color::RED;
                    balance_after_insert(path, tree);
                }
                else
                {
                    if (parent->left_subtree == current)
                    {
                        if (parent == tree->_root) tree->small_right_rotation(*const_cast<typename binary_search_tree<tkey, tvalue>::node**>(&(tree->_root)));

                        else tree->small_right_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(parent_));
                    }
                    parent->color = node_color::BLACK;
                    grandpa->color = node_color::RED;
                    if (grandpa == tree->_root) tree->small_left_rotation(*const_cast<typename binary_search_tree<tkey, tvalue>::node**>(&(tree->_root)));
                    else tree->small_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(grandpa_));
                    balance_after_insert(path, tree);
                }
            }

        }



        void balance_after_delete(std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path, red_black_tree<tkey, tvalue> const *tree, int is_left_child)
        {
            //only if delete black with 0 children
            //now top = parent
            if (path.empty())
            {
                return;
            }
            red_black_tree<tkey, tvalue>::node *brother;
            red_black_tree<tkey, tvalue>::node **brother_;
            red_black_tree<tkey, tvalue>::node *current_parent = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));
            red_black_tree<tkey, tvalue>::node **current_parent_ = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node **>(path.top());

            if (current_parent == nullptr || current_parent->color == node_color::RED) return; 

            if (current_parent->left_subtree != nullptr) 
            {
                brother = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(current_parent->left_subtree);
                brother_ = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node **>(&(current_parent->left_subtree));
            }
            else 
            {
                brother = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(current_parent->right_subtree);
                brother_ = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node **>(&(current_parent->right_subtree));
            }

            if (brother->color == node_color::BLACK)
            {
                red_black_tree<tkey, tvalue>::node *left_child_brother = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(brother->left_subtree);
                red_black_tree<tkey, tvalue>::node *right_child_brother = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(brother->right_subtree);
                if (left_child_brother && left_child_brother->color == node_color::RED || right_child_brother && right_child_brother->color == node_color::RED)
                {
                    if (is_left_child)
                    {
                        if (right_child_brother && right_child_brother->color == node_color::RED)
                        {
                            brother->color = current_parent->color;
                            current_parent->color = node_color::BLACK;
                            right_child_brother->color = node_color::BLACK;
                            
                            tree->small_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(current_parent_));
                            return;
                        }
                        else if (left_child_brother && left_child_brother->color == node_color::RED && right_child_brother && right_child_brother->color == node_color::BLACK)
                        {
                            left_child_brother->color = node_color::BLACK;
                            brother->color = node_color::RED;
                            //here
                            tree->small_right_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(brother_));
                            balance_after_delete(path, tree, is_left_child);
                        }
                    }
                    else
                    {
                        if (left_child_brother && left_child_brother->color == node_color::RED)
                        {
                            brother->color = current_parent->color;
                            current_parent->color = node_color::BLACK;
                            left_child_brother->color = node_color::BLACK;
                            tree->small_right_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(current_parent_));
                            return;
                        }
                        else if (right_child_brother && right_child_brother->color == node_color::RED && left_child_brother && left_child_brother->color == node_color::BLACK)
                        {
                            right_child_brother->color = node_color::BLACK;
                            brother->color = node_color::RED;
                            tree->small_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(brother_));
                            balance_after_delete(path, tree, is_left_child); 
                        }
                    }
                }
                else
                {
                    brother->color = node_color::RED;
                    if (current_parent->color == node_color::RED)
                    {
                        current_parent->color = node_color::BLACK;
                        return;
                    }
                    else
                    {
                        current_parent->color = node_color::BLACK;
                        if (is_left_child)
                        {
                            path.pop();
                            red_black_tree<tkey, tvalue>::node *grandpa = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));
                            if (grandpa && grandpa->left_subtree == current_parent)
                                balance_after_delete(path, tree, 1);
                            else if (grandpa)
                                balance_after_delete(path, tree, 0);
                        }
                        else
                        {
                            path.pop();
                            red_black_tree<tkey, tvalue>::node *grandpa = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));
                            if (grandpa && grandpa->right_subtree == current_parent)
                                balance_after_delete(path, tree, 1);
                            else if (grandpa)
                                balance_after_delete(path, tree, 0);
                        }
                    }
                }
            }
            else
            {
                current_parent->color = node_color::RED;
                brother->color = node_color::BLACK;
                if (is_left_child)
                {
                    tree->small_left_rotation(*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(current_parent_));
                    balance_after_delete(path, tree, 1);
                }
                else
                {
                    tree->small_right_rotation((*reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(current_parent_)));
                    balance_after_delete(path, tree, 0);
                }
            }
        }
    };




private:
    
    class insertion_template_method final:
        public binary_search_tree<tkey, tvalue>::insertion_template_method,
        public balancer
    {
    
    public:
        
        explicit insertion_template_method(
            red_black_tree<tkey, tvalue> *tree,
            typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy);
    
    private:
        
        void balance(
            std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path) override
        {
            if (path.size() == 1)
            {
                red_black_tree<tkey, tvalue>::node *current = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));
                current->color = node_color::BLACK;
            }
            else
            {
                red_black_tree<tkey, tvalue>::node *current = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));
                current->color = node_color::RED;
            }
            this->balance_after_insert(path, dynamic_cast<red_black_tree<tkey, tvalue> const *>(this->_tree));
        }


        template<
            typename T>
        inline void swap(
            T &&one,
            T &&another)
        {
            T temp = std::move(one);
            one = std::move(another);
            another = std::move(temp);
        }
        
    };

    
    class disposal_template_method final:
        public binary_search_tree<tkey, tvalue>::disposal_template_method,
        public balancer
    {
    
    public:
        
        explicit disposal_template_method(
            red_black_tree<tkey, tvalue> *tree,
            typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy);
    private:
        void balance(
            std::stack<typename binary_search_tree<tkey, tvalue>::node **> &path) override
        {
            path.pop();
            red_black_tree<tkey, tvalue>::node *parent = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node *>(*(path.top()));
            if (parent && parent->right_subtree == nullptr)
                this->balance_after_delete(path, dynamic_cast<red_black_tree<tkey, tvalue> const *>(this->_tree), 0);
            else 
                this->balance_after_delete(path, dynamic_cast<red_black_tree<tkey, tvalue> const *>(this->_tree), 1);
        }

        template<
            typename T>
        inline void swap(
            T &&one,
            T &&another)
        {
            T temp = std::move(one);
            one = std::move(another);
            another = std::move(temp);
        }

    public:

        tvalue dispose(tkey const &key) override
        {
            auto path = this->find_path(key);
            if (*(path.top()) == nullptr) //todo
            {
                std::cout << "case path is empty" << std::endl;
                
                return tvalue();
            }
            if ((*(path.top()))->left_subtree != nullptr && (*(path.top()))->right_subtree != nullptr)
            {
                std::cout << "case node have 2 subtrees" << std::endl;
                auto *target_to_swap = *(path.top());
                auto **current =reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(&((*(path.top()))->left_subtree));

                while (*current != nullptr)
                {
                    path.push(current);
                    current = reinterpret_cast<typename binary_search_tree<tkey, tvalue>::node**>(&((*current)->right_subtree));
                }

                swap(std::move(target_to_swap->key), std::move((*(path.top()))->key));
                swap(std::move(target_to_swap->value), std::move((*(path.top()))->value));
            }
            tvalue value = (*(path.top()))->value;

            
            red_black_tree<tkey, tvalue>::node **current_ = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node **>(&(*(path.top())));

            if ((*current_)->left_subtree != nullptr || (*current_)->right_subtree != nullptr)
            {//черная - 1 ребенок

                std::cout << "case node have 1 subtree" << std::endl;
                typename binary_search_tree<tkey, tvalue>::node** target_to_swap;
                target_to_swap = ( (*current_)->left_subtree == nullptr ) ?  &(*current_)->right_subtree : &(*current_)->left_subtree;
                
                swap(std::move((*target_to_swap)->key), std::move((*(path.top()))->key));
                swap(std::move((*target_to_swap)->value), std::move((*(path.top()))->value));

                path.push(target_to_swap);
            }

            current_ = reinterpret_cast<typename red_black_tree<tkey, tvalue>::node **>(&(*(path.top())));

            if ((*current_)->left_subtree == nullptr && (*current_)->right_subtree == nullptr && (*current_)->color == node_color::BLACK)
            {//черная 0 детей
                std::cout << "case black node have 0 subtrees" << std::endl;
                allocator::destruct(*(path.top()));
                this->deallocate_with_guard(*(path.top()));
                *(path.top()) = nullptr;
                this->balance(path);
                return value;
            }
            if ((*current_)->left_subtree == nullptr && (*current_)->right_subtree == nullptr && (*current_)->color == node_color::RED)
            {//красная 0 детей
                std::cout << "case red node have 0 subtrees" << std::endl;
                allocator::destruct(*(path.top()));
                this->deallocate_with_guard(*(path.top()));
                *(path.top()) = nullptr;
                return value;
            }
        }
        };

    public:
        explicit red_black_tree(
            std::function<int(tkey const &, tkey const &)> comparer,
            allocator *allocator = nullptr,
            logger *logger = nullptr,
            typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy = binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy::throw_an_exception,
            typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy = binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy::throw_an_exception);

    public:
        ~red_black_tree() noexcept final;

        red_black_tree(
            red_black_tree<tkey, tvalue> const &other);

        red_black_tree<tkey, tvalue> &operator=(
            red_black_tree<tkey, tvalue> const &other);

        red_black_tree(
            red_black_tree<tkey, tvalue> &&other) noexcept;

        red_black_tree<tkey, tvalue> &operator=(
            red_black_tree<tkey, tvalue> &&other) noexcept;
    
};

template<
    typename tkey,
    typename tvalue>
red_black_tree<tkey, tvalue>::iterator_data::iterator_data(
    unsigned int depth,
    tkey const &key,
    tvalue const &value,
    typename red_black_tree<tkey, tvalue>::node_color color):
    binary_search_tree<tkey, tvalue>::iterator_data(depth, key, value)
{
    this->color = color;
}

template<
    typename tkey,
    typename tvalue>
red_black_tree<tkey, tvalue>::insertion_template_method::insertion_template_method(
    red_black_tree<tkey, tvalue> *tree,
    typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy):
    binary_search_tree<tkey, tvalue>::insertion_template_method(tree, insertion_strategy)
{

}

template<
    typename tkey,
    typename tvalue>
red_black_tree<tkey, tvalue>::disposal_template_method::disposal_template_method(
    red_black_tree<tkey, tvalue> *tree,
    typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy):
    binary_search_tree<tkey, tvalue>::disposal_template_method(tree, disposal_strategy)
{

}

template<
    typename tkey,
    typename tvalue>
red_black_tree<tkey, tvalue>::red_black_tree(
    std::function<int(tkey const &, tkey const &)> comparer,
    allocator *allocator,
    logger *logger,
    typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy,
    typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy):
    binary_search_tree<tkey, tvalue>(new red_black_tree<tkey, tvalue>::insertion_template_method(this, insertion_strategy), new typename binary_search_tree<tkey, tvalue>::obtaining_template_method(dynamic_cast<binary_search_tree<tkey, tvalue> *>(this)), new red_black_tree<tkey, tvalue>::disposal_template_method(this, disposal_strategy), comparer, allocator, logger)

{
    
}

template<
    typename tkey,
    typename tvalue>
red_black_tree<tkey, tvalue>::~red_black_tree() noexcept
{

}

template<
    typename tkey,
    typename tvalue>
red_black_tree<tkey, tvalue>::red_black_tree(
    red_black_tree<tkey, tvalue> const &other) : red_black_tree<tkey, tvalue>(other._keys_comparer, other.get_allocator(), other.get_logger(),
     binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy::throw_an_exception,
     binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy::throw_an_exception)
{

}

template<
    typename tkey,
    typename tvalue>
red_black_tree<tkey, tvalue> &red_black_tree<tkey, tvalue>::operator=(
    red_black_tree<tkey, tvalue> const &other)
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
red_black_tree<tkey, tvalue>::red_black_tree(
    red_black_tree<tkey, tvalue> &&other) noexcept : red_black_tree<tkey, tvalue>(other._keys_comparer, other.get_allocator(), other.get_logger())
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
red_black_tree<tkey, tvalue> &red_black_tree<tkey, tvalue>::operator=(
    red_black_tree<tkey, tvalue> &&other) noexcept
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

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_RED_BLACK_TREE_H
