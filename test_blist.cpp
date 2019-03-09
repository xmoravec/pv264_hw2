#include <rapidcheck.h>
// for the sake of test clarity we re-define the standard assert macro to use
// rapidcheck
#undef assert
#define assert(X) RC_ASSERT(X)

#include "blist.hpp"
#include <deque>
#include <variant>
#include <cstring>

template class blist< int >;

template< typename T >
struct PushFront {
    T val;
};

template< typename T >
std::ostream &operator<<( std::ostream &os, const PushFront< T > &p ) {
    return os << "push_front( " << p.val << " )";
}

template< typename T >
struct PushBack {
    T val;
};

template< typename T >
std::ostream &operator<<( std::ostream &os, const PushBack< T > &p ) {
    return os << "push_back( " << p.val << " )";
}

template< typename Container, typename T >
static void run( Container &c, PushFront< T > &x ) {
    c.push_front( x.val );
}

template< typename Container, typename T >
static void run( Container &c, PushBack< T > &x ) {
    c.push_back( x.val );
}

template< typename... Ops >
struct CheckOpts {
    void operator()( std::vector< std::variant< Ops... > > ops ) const {
        blist< int, 8 > bl;
        std::deque< int > deq;
        size_t depth = 0;
        for ( auto &v : ops ) {
            std::visit( [&]( auto op ) {
                    run( bl, op );
                    run( deq, op );
                }, v );
            RC_ASSERT( bl.empty() == deq.empty() );
            RC_ASSERT( bl.size() == deq.size() );
            bl.validate();
            RC_ASSERT( std::equal( bl.begin(), bl.end(), deq.begin(), deq.end() ) );
            depth = std::max( bl.depth(), depth );
        }
        RC_TAG( "depth " + std::to_string( depth ) );
    }
};

namespace rc {

    template< typename T >
    struct Arbitrary< PushBack< T > > {
        static Gen< PushBack< T > > arbitrary() {
            return gen::build< PushBack< T > >(
                gen::set( &PushBack< T >::val ) );
        }
    };

    template< typename T >
    struct Arbitrary< PushFront< T > > {
        static Gen< PushFront< T > > arbitrary() {
            return gen::build< PushFront< T > >(
                gen::set( &PushFront< T >::val ) );
        }
    };

} // namespace rc

void test_blist() {
    rc::Config single;
    single.max_success = 1;

    rc::check( "blist ctor default", single, [] {
        blist< int > bli;
        RC_ASSERT( bli.size() == 0u );
        RC_ASSERT( bli.empty() );
    } );

    rc::check( "blist push_back", []( std::vector< int > vals ) {
        blist< int, 8 > bl;
        for ( auto it = vals.begin(); it != vals.end(); ++it ) {
            RC_LOG() << *it << " ";
            bl.push_back( *it );
            RC_ASSERT( !bl.empty() );
            RC_ASSERT( bl.size() == size_t(it - vals.begin() + 1) );
            bl.validate();
            RC_ASSERT( std::equal( vals.begin(), std::next( it ), bl.begin(), bl.end() ) );
        }
        RC_TAG( "depth " + std::to_string( bl.depth() ) );
    } );

    rc::check( "blist ctor iterator", []( std::vector< int > vals ) {
        blist< int, 8 > bl( vals.begin(), vals.end() );
        RC_ASSERT( bl.size() == vals.size() );
        bl.validate();
        RC_ASSERT( std::equal( vals.begin(), vals.end(), bl.begin(), bl.end() ) );
    } );

    rc::check( "blist iterator", []( std::vector< int > vals ) {
        blist< int, 8 > bl( vals.begin(), vals.end() );
        using CIt = blist< int, 8 >::const_iterator;
        using CRIt = blist< int, 8 >::const_reverse_iterator;
        const auto &cbl = bl;

        auto fit = bl.begin();
        auto cfit = bl.cbegin();
        auto efit = bl.end();
        auto cefit = bl.cend();

        auto rit = bl.rbegin();
        auto crit = bl.crbegin();
        auto erit = bl.rend();
        auto cerit = bl.crend();

        RC_ASSERT( cfit == cbl.begin() );
        RC_ASSERT( cefit == cbl.end() );
        RC_ASSERT( crit == cbl.rbegin() );
        RC_ASSERT( cerit == cbl.rend() );

        for ( size_t i = 0; i < vals.size(); ++i ) {
            auto c_efit = efit--;
            auto c_cefit = cefit--;
            auto c_erit = erit--;
            auto c_cerit = cerit--;

            --c_efit;
            --c_cefit;
            --c_erit;
            --c_cerit;

            RC_ASSERT( c_efit == efit );
            RC_ASSERT( c_cefit == cefit );
            RC_ASSERT( c_erit == erit );
            RC_ASSERT( c_cerit == cerit );

            RC_ASSERT( std::addressof( *fit ) == std::addressof( *cfit ) );
            RC_ASSERT( std::addressof( *rit ) == std::addressof( *crit ) );

            RC_ASSERT( std::addressof( *efit ) == std::addressof( *cefit ) );
            RC_ASSERT( std::addressof( *erit ) == std::addressof( *cerit ) );

            RC_ASSERT( *fit == vals[ i ] );
            RC_ASSERT( *cfit == vals[ i ] );
            RC_ASSERT( *erit == vals[ i ] );
            RC_ASSERT( *cerit == vals[ i ] );

            RC_ASSERT( *efit == vals.rbegin()[ i ] );
            RC_ASSERT( *cefit == vals.rbegin()[ i ] );
            RC_ASSERT( *rit == vals.rbegin()[ i ] );
            RC_ASSERT( *crit == vals.rbegin()[ i ] );

            auto c_fit = fit++;
            auto c_cfit = cfit++;
            auto c_rit = rit++;
            auto c_crit = crit++;

            ++c_fit;
            ++c_cfit;
            ++c_rit;
            ++c_crit;

            RC_ASSERT( c_fit == fit );
            RC_ASSERT( c_cfit == cfit );
            RC_ASSERT( c_rit == rit );
            RC_ASSERT( c_crit == crit );

            c_cfit = fit;
            c_crit = rit;
            RC_ASSERT( c_cfit == cfit );
            RC_ASSERT( c_crit == crit );

            CIt n_cfit = fit;
            CRIt n_crit = rit;
            RC_ASSERT( n_cfit == cfit );
            RC_ASSERT( n_crit == crit );
        }
    } );

    rc::check( "blist push_{front,back} random", CheckOpts< PushBack< int >, PushFront< int > >() );

    rc::check( "blist create + erases", []( std::vector< int > vals, std::vector< unsigned > idxs ) {
        blist< int, 8 > bl( vals.begin(), vals.end() );
        RC_TAG( "depth " + std::to_string( bl.depth() ) );
        for ( auto v : idxs ) {
            if ( vals.empty() )
                break;
            v %= bl.size();
            bl.erase( std::next( bl.begin(), v ) );
            vals.erase( std::next( vals.begin(), v ) );

            RC_ASSERT( bl.empty() == vals.empty() );
            RC_ASSERT( bl.size() == vals.size() );
            bl.validate();
            RC_ASSERT( std::equal( bl.begin(), bl.end(), vals.begin(), vals.end() ) );
        }
    } );

    rc::check( "blist create + move", []( std::vector< int > vals ) {
        blist< int, 8 > bl( vals.begin(), vals.end() );
        auto it = bl.begin();
        auto rit = bl.rbegin();
        auto copy( std::move( bl ) );
        RC_ASSERT( vals.empty() == copy.empty() );
        RC_ASSERT( vals.size() == copy.size() );
        bl.validate();
        copy.validate();
        RC_ASSERT( std::equal( vals.begin(), vals.end(), copy.begin(), copy.end() ) );
        RC_ASSERT( copy.begin() == it );
        RC_ASSERT( copy.rbegin() == rit );
    } );
}
