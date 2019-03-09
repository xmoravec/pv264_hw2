// conditionally define assert so we can override it with RC_ASSERT for tests
#ifndef assert
#include <cassert>
#endif
#include <type_traits>
#include "static_vector.hpp"

template< typename T, uint32_t NodeSize = 128 >
class blist
{
    static_assert( NodeSize >= 4, "node size must be at least 4 elements" );
    static_assert( NodeSize % 2 == 0, "node size must be an even number" );
    static constexpr size_t node_size = NodeSize;
    static constexpr size_t half_size = node_size / 2;

    // **suggested**
    // Can be used to set one type to const if the other type is const.
    // CopyConst< const int, long > == const long
    // CopyConst< int, long > = long
    //
    // Typically used for templated functions (or classes) to make the result
    // (or member) have same constness as one of the template arguments. E.g.
    // in the following example if `val` is mutable (non-const) reference, then
    // result will be int &, otherwise it will be const int &
    //
    // template< typename T >
    // CopyConst< T, int > &foo( T &val );
    template< typename From, typename To >
    using CopyConst = std::conditional_t< std::is_const_v< From >, const To, To >;

    // **suggested**
    // We suggest that you define the iterator using this templated class which
    // will be instantiated to either non-const or const tree node to get you
    // const or non-const iterator.
    template< typename Node >
    class base_iterator;

  public:
    using value_type = T;
    using reference = T &;
    using const_reference = const T &;
    using pointer = T *;
    using const_pointer = const T *;
    using iterator = base_iterator< /* ... */ >; // **suggested implementation**
    using const_iterator = base_iterator< const /* ... */ >; // **suggested implementation**
    using reverse_iterator = std::reverse_iterator< iterator >;
    using const_reverse_iterator = std::reverse_iterator< const_iterator >;

    // **required: define these constructors & assignment operators as needed,
    // they can (and should) be omitted or defaulted if that results in a
    // correct implementation (do not re-implement what compiler can do for you)
    blist();
    blist( blist && );
    blist( const blist &o );

    // note: this constructor can be called only if It is an iterator as seen by C++ <= 17
    template< typename It, typename = typename std::iterator_traits< It >::value_type >
    blist( It first, It last );

    blist( std::initializer_list< T > ilist );

    blist &operator=( blist && );
    blist &operator=( const blist &o );


    bool empty() const;
    size_t size() const;

    // Returns the number of nodes on the path from root to leaf (i.e.
    // if root is the only leaf then depth() == 1)
    size_t depth() const;

    iterator begin();
    const_iterator begin() const;
    const_iterator cbegin() const;

    iterator end();
    const_iterator end() const;
    const_iterator cend() const;

    reverse_iterator rbegin();
    const_reverse_iterator rbegin() const;
    const_reverse_iterator crbegin() const;

    reverse_iterator rend();
    const_reverse_iterator rend() const;
    const_reverse_iterator crend() const;

    reference front();
    const_reference front() const;

    reference back();
    const_reference back() const;

    template< typename... Args >
    void emplace_back( Args &&...args );

    void push_back( const T &x );
    void push_back( T &&x );

    template< typename... Args >
    void emplace_front( Args &&...args );

    void push_front( const T &x );
    void push_front( T &&x );

    // NOTE: signature changed compared to std, where the iterator would be const
    template< typename... Args >
    iterator emplace( iterator pos, Args &&...args );

    iterator insert( iterator pos, const T &value );
    iterator insert( iterator pos, T &&value );

    iterator erase( iterator pos );

    T &operator[]( size_t idx );
    const T &operator[]( size_t idx ) const;

    // **suggested**, implement this to check in the unit test that the blist
    // is consistent.
    // e.g. you can check that parent pointers are correct, sizes of internal nodes are correct, ...
    // This might help you with the debugging.
    void validate() const { }
};
