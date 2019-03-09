#pragma once

#ifndef assert
#include <cassert>
#endif
#include <cstdint>
#include <cstddef>
#include <utility>
#include <type_traits>
#include <iterator>
#include <memory>
#include <algorithm>
#include <limits>
#include <optional>
#include <stdexcept>
#include <initializer_list>

struct static_vector_full : std::logic_error
{
    using std::logic_error::logic_error;
};

template< typename T, size_t Capacity >
class static_vector
{
    using internal_size = std::conditional_t<
                              (Capacity <= std::numeric_limits< uint32_t >::max()),
                              uint32_t, size_t >;
    alignas( alignof( T ) ) char _data[ Capacity * sizeof( T ) ];
    internal_size _size = 0;

  public:
    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using reference = T &;
    using const_reference = const T &;
    using iterator = T *;
    using const_iterator = const T *;
    using reverse_iterator = std::reverse_iterator< iterator >;
    using const_reverse_iterator = std::reverse_iterator< const_iterator >;

    static_vector() noexcept = default;

    explicit static_vector( size_type count )
        : _size( count )
    {
        _check_count_ctor( count );
        for ( auto &loc : *this )
            new ( &loc ) T();
    }

    static_vector( size_type count, const T &value )
        : _size( count )
    {
        _check_count_ctor( count );
        std::uninitialized_fill( begin(), end(), value );
    }

    static_vector( const static_vector &other )
        noexcept( std::is_nothrow_copy_constructible_v< T > )
        : _size( other._size )
    {
        std::uninitialized_copy( other.begin(), other.end(), begin() );
    }

    static_vector( static_vector &&other )
        noexcept( std::is_nothrow_move_constructible_v< T > )
        : _size( other._size )
    {
        std::uninitialized_move( other.begin(), other.end(), begin() );
        other.clear();
    }

    static_vector( std::initializer_list< T > init ) // NOLINT
        : static_vector( init.begin(), init.end() )
    { }

    template< typename InputIt, typename = typename std::iterator_traits< InputIt >::value_type >
    static_vector( InputIt first, InputIt last ) // NOLINT
    {
        for ( ; first != last; ++first )
            push_back( *first );
    }

    ~static_vector()
        noexcept( std::is_nothrow_destructible_v< T > )
    {
        _destroy_elems();
    }

    static_vector &operator=( const static_vector &o )
        noexcept( std::is_nothrow_copy_constructible_v< T >
                  && std::is_nothrow_destructible_v< T > )
    {
        if ( &o != this ) {
            _set_size( o._size );
            std::copy( o.begin(), o.end(), begin() );
        }
        return *this;
    }

    static_vector &operator=( static_vector &&o )
        noexcept( std::is_nothrow_move_constructible_v< T >
                  && std::is_nothrow_destructible_v< T > )
    {
        if ( &o != this ) {
            _set_size( o._size );
            std::move( o.begin(), o.end(), begin() );
            o.clear();
        }
        return *this;
    }

    static_vector &operator=( std::initializer_list< T > init )
    {
        if ( init.size() > Capacity )
            throw static_vector_full( "static_vector: attempt to assign from too large initializer_list" );
        _set_size( init.size() );
        std::copy( init.begin(), init.end(), begin() );
        return *this;
    }


    iterator begin() noexcept { return reinterpret_cast< T * >( _data ); } // NOLINT
    const_iterator begin() const noexcept { return reinterpret_cast< const T * >( _data ); } // NOLINT
    const_iterator cbegin() const noexcept { return reinterpret_cast< const T * >( _data ); } // NOLINT

    iterator end() noexcept { return begin() + _size; }
    const_iterator end() const noexcept { return begin() + _size; }
    const_iterator cend() const noexcept { return begin() + _size; }

    reverse_iterator rbegin() noexcept { return std::reverse_iterator( end() ); }
    const_reverse_iterator rbegin() const noexcept { return std::reverse_iterator( end() ); }
    const_reverse_iterator crbegin() const noexcept { return std::reverse_iterator( end() ); }

    reverse_iterator rend() noexcept { return std::reverse_iterator( begin() ); }
    const_reverse_iterator rend() const noexcept { return std::reverse_iterator( begin() ); }
    const_reverse_iterator crend() const noexcept { return std::reverse_iterator( begin() ); }

    reference at( size_type pos ) {
        return _at( *this, pos );
    }

    const_reference at( size_type pos ) const {
        return _at( *this, pos );
    }

    reference operator[]( size_type pos ) noexcept { return begin()[ pos ]; }
    const_reference operator[]( size_type pos ) const noexcept { return begin()[ pos ]; }

    reference front() noexcept { return *begin(); }
    const_reference front() const noexcept { return *begin(); }

    reference back() noexcept { return *(end() - 1); }
    const_reference back() const noexcept { return *(end() - 1); }

    T *data() noexcept { return begin(); }
    const T *data() const noexcept { return begin(); }

    bool empty() const noexcept { return _size == 0; }
    bool full() const noexcept { return _size == Capacity; }
    size_type size() const noexcept { return _size; }
    size_type max_size() const noexcept { return Capacity; }
    size_type capacity() const noexcept { return Capacity; }

    void clear()
        noexcept( std::is_nothrow_destructible_v< T > )
    {
        _destroy_elems();
        _size = 0;
    }

    // returns nullopt if static_vector is full, iterator to inserted element otherwise
    template< typename... Args >
    std::optional< iterator > try_emplace( iterator pos, Args &&...args ) {
        if ( _size == Capacity )
            return std::nullopt;
        if ( pos != end() && _size ) {
            std::uninitialized_move_n( end() - 1, 1, end() );
            if ( end() - 1 > pos )
                std::move_backward( pos, end() - 1, end() );
            std::destroy_at( pos );
        }
        ++_size;
        new ( pos ) T( std::forward< Args >( args )... );
        return pos;
    }

    template< typename... Args >
    iterator emplace( iterator pos, Args &&...args ) {
        if ( auto r = try_emplace( pos, std::forward< Args >( args )... ) )
            return r.value();
        throw static_vector_full( "static_vector: insertion into full static_vector failed" );
    }

    iterator insert( iterator pos, const T &value ) { return emplace( pos, value ); }
    iterator insert( iterator pos, T &&value ) { return emplace( pos, std::move( value ) ); }

    template< typename It, typename = typename std::iterator_traits< It >::value_type >
    iterator insert( iterator pos, It first, It last ) {
        auto dist = std::distance( first, last );
        if ( dist <= 0 )
            return pos;
        if ( _size + size_t( dist ) > Capacity )
            throw static_vector_full( "static_vector: range insertion into full static_vector failed" );

        size_t to_end = std::distance( pos, end() );
        if ( to_end >= size_t( dist ) ) {
            std::uninitialized_move_n( end() - dist, dist, end() );
            std::move_backward( pos, end() - dist, end() );
            std::copy( first, last, pos );
        } else {
            std::uninitialized_move_n( end() - to_end, to_end, end() + dist - to_end );
            auto dst = pos;
            // note: not using algorithms as copy_n does not return iterator to
            // the last copied element in the original range and we don't want
            // to re-iterate to that position
            for ( size_t i = 0; first != last; ++i, ++first, ++dst ) {
                if ( i < to_end )
                    *dst = *first;
                else
                    new ( dst ) T( *first );
            }
        }
        _size += dist;
        return pos;
    }

    template< typename... Args >
    void emplace_back( Args &&...args ) { emplace( end(), std::forward< Args >( args )... ); }

    void push_back( const T &val ) { emplace_back( val ); }
    void push_back( T &&val ) { emplace_back( std::move( val ) ); }

    void pop_back() {
        std::destroy_at( &back() );
        --_size;
    }

    void resize( size_type count ) {
        auto it = _resize( count );
        for ( auto e = end(); it < e; ++it )
            new ( it ) T();
    }

    void resize( size_type count, const T &value ) {
        auto it = _resize( count );
        if ( it < end() )
            std::fill( it, end(), value );
    }

    iterator erase( iterator pos ) {
        return erase( pos, pos + 1 );
    }

    iterator erase( iterator first, iterator last ) {
        auto it = std::move( last, end(), first );
        std::destroy( it, end() );
        _size -= last - first;
        return first;
    }

    bool operator==( const static_vector &o ) const noexcept {
        return std::equal( begin(), end(), o.begin(), o.end() );
    }

    bool operator!=( const static_vector &o ) const noexcept { return !(*this == o); }

    bool operator<( const static_vector &o ) const noexcept {
        return std::lexicographical_compare( begin(), end(), o.begin(), o.end() );
    }

    bool operator>( const static_vector &o ) const noexcept { return o < *this; }
    bool operator<=( const static_vector &o ) const noexcept { return !(*this > o); }
    bool operator>=( const static_vector &o ) const noexcept { return !(*this < o); }

  private:
    template< typename Self >
    static auto& _at( Self& self, size_type pos )
    {
        if ( pos >= self._size )
            throw std::out_of_range( "static_vector: index out of range" );
        return self.begin()[ pos ];
    }

    void _check_count_ctor( size_type count ) {
        if ( count > Capacity )
            throw static_vector_full( "static_vector: attempt to construct vector with count > capacity" );
    }

    iterator _resize( size_type count ) {
        if ( count > Capacity )
            throw static_vector_full( "static_vector: attempt to resize vector with count > capacity" );
        auto it = end();
        _set_size( count );
        return it;
    }

    void _destroy_elems() { std::destroy( begin(), end() ); }

    void _set_size( size_type size )
    {
        if ( size < _size )
            std::destroy( begin() + size, end() );
        _size = size;
    }
};
