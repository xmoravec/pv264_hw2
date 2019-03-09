#include <rapidcheck.h>
#undef assert
#define assert(X) RC_ASSERT(X)

#include "static_vector.hpp"
#include <deque>
#include <variant>
#include <cstring>

template class static_vector< int, 128 >;

struct InstanceCounter {
    InstanceCounter() { ++ctor_cnt; }
    ~InstanceCounter() { ++dtor_cnt; }

    struct Guard {
        Guard() { reset(); }
        ~Guard() { reset(); };
        void reset() { ctor_cnt = dtor_cnt = 0; }
    };

    static inline int ctor_cnt = 0;
    static inline int dtor_cnt = 0;
};

struct TractingInt {
    explicit TractingInt( int v ) : v( v ) { construct(); }

    TractingInt( const TractingInt &o ) : v( o.v ) { construct(); }
    TractingInt( TractingInt &&o ) : v( o.v ) { construct(); }
    ~TractingInt() { ++destructed; RC_ASSERT( constructed == destructed ); }

    void construct() {
        RC_ASSERT( constructed == destructed );
        ++constructed;
    }

    TractingInt &operator=( int v ) {
        this->v = v;
        return *this;
    }

    TractingInt &operator=( const TractingInt &o ) {
        v = o.v;
        ++copied_to;
        return *this;
    }

    TractingInt &operator=( TractingInt &&o ) {
        v = o.v;
        ++moved_to;
        ++o.moved_from;
        return *this;
    }

    int v;
    int constructed;
    int moved_to;
    int moved_from;
    int copied_to;
    int destructed;
};

template< typename T >
struct CADeleter {
    void operator()( T *v ) {
        std::destroy_at( v );
        std::free( v ); // NOLINT
    }
};

void test_static_vector() {
    rc::Config single;
    single.max_success = 1;

    rc::check( "static_vector ctor default", single, [] {
        InstanceCounter::Guard g;
        static_vector< int, 16 > sv;
        RC_ASSERT( sv.size() == 0u );
        RC_ASSERT( sv.empty() );
        RC_ASSERT( !sv.full() );
        RC_ASSERT( sv.capacity() == 16u );
        RC_ASSERT( sv.max_size() == 16u );

        static_vector< InstanceCounter, 16 > svc;
        RC_ASSERT( InstanceCounter::ctor_cnt == 0 );
        RC_ASSERT( svc.size() == 0u );
        RC_ASSERT( svc.empty() );
        RC_ASSERT( !svc.full() );
    } );

    rc::check( "static_vector ctor count", single, [] {
        InstanceCounter::Guard g;
        static_vector< InstanceCounter, 16 > svc( 8 );
        RC_ASSERT( InstanceCounter::ctor_cnt == 8 );
        RC_ASSERT( InstanceCounter::dtor_cnt == 0 );

        static_vector< int, 16 > svi( 8, 42 );
        for ( int i = 0; i < 8; ++i )
            RC_ASSERT( svi[i] == 42 );

        RC_ASSERT_THROWS( (static_vector< int, 16 >( 32 )) );

        g.reset();
        RC_ASSERT_THROWS( (static_vector< InstanceCounter, 16 >( 32 )) );
        RC_ASSERT( InstanceCounter::ctor_cnt == 0 );
    } );

    rc::check( "static_vector ctor ilist", single, [] {
        static_vector< int, 16 > sv{ 0, 1, 2, 3, 4 };
        for ( int i = 0; i <= 4; ++i )
            RC_ASSERT( sv[ i ] == i );
        RC_ASSERT( sv.size() == 5u );

        RC_ASSERT_THROWS( (static_vector< int, 2 >{ 1, 2, 3, 4 }) );
    } );

    rc::check( "static_vector ctor iterator", single, [] {
        int array[] = { 0, 1, 2, 3, 4 };
        static_vector< int, 16 > sv{ std::begin( array ), std::end( array ) };
        for ( int i = 0; i <= 4; ++i )
            RC_ASSERT( sv[ i ] == i );
        RC_ASSERT( sv.size() == 5u );

        RC_ASSERT_THROWS( (static_vector< int, 2 > { std::begin( array ), std::end( array ) }) );
    } );

    rc::check( "static_vector ctor copy", single, [] {
        static_vector< int, 16 > a{ 0, 1, 2, 3, 4 };
        static_vector< int, 16 > b( a );
        for ( int i = 0; i <= 4; ++i ) {
            RC_ASSERT( a[ i ] == i );
            RC_ASSERT( b[ i ] == i );
        }
        RC_ASSERT( a.size() == 5u );
        RC_ASSERT( b.size() == 5u );
    } );

    rc::check( "static_vector ctor move", single, [] {
        static_vector< int, 16 > a{ 0, 1, 2, 3, 4 };
        static_vector< int, 16 > b( std::move( a ) );
        for ( int i = 0; i <= 4; ++i ) {
            RC_ASSERT( b[ i ] == i );
        }
        RC_ASSERT( a.size() == 0u );
        RC_ASSERT( a.empty() );
        RC_ASSERT( b.size() == 5u );
    } );

    rc::check( "static_vector assign copy", single, [] {
        static_vector< int, 16 > a{ 0, 1, 2, 3, 4 };
        static_vector< int, 16 > b{ 0, 1 };
        b = a;
        for ( int i = 0; i <= 4; ++i ) {
            RC_ASSERT( a[ i ] == i );
            RC_ASSERT( b[ i ] == i );
        }
        RC_ASSERT( a.size() == 5u );
        RC_ASSERT( b.size() == 5u );
    } );

    rc::check( "static_vector assign move", single, [] {
        static_vector< int, 16 > a{ 0, 1, 2, 3, 4 };
        static_vector< int, 16 > b{ 0, 1 };
        b = std::move( a );
        for ( int i = 0; i <= 4; ++i ) {
            RC_ASSERT( b[ i ] == i );
        }
        RC_ASSERT( a.size() == 0u );
        RC_ASSERT( a.empty() );
        RC_ASSERT( b.size() == 5u );
    } );

    rc::check( "static_vector assign ilist", single, [] {
        static_vector< int, 16 > b{ 0, 1 };
        b = { 0, 1, 2, 3, 4 };
        for ( int i = 0; i <= 4; ++i ) {
            RC_ASSERT( b[ i ] == i );
        }
        RC_ASSERT( b.size() == 5u );

        static_vector< int, 2 > sv2;
        RC_ASSERT_THROWS( (sv2 = { 1, 2, 3 }) );
    } );

    rc::check( "static_vector range for", single, [] {
        static_vector< int, 16 > sv{ 0, 1, 2, 3, 4 };
        int i = 0;
        for ( int v : sv )
            RC_ASSERT( i++ == v );
        i = 0;
        for ( auto &v : sv )
            RC_ASSERT( i++ == v );
        i = 0;
        for ( const auto &v : sv )
            RC_ASSERT( i++ == v );
        i = 0;
        const auto &csv = sv;
        for ( auto &v : csv )
            RC_ASSERT( i++ == v );
    } );

    rc::check( "static_vector iterator", single, [] {
        static_vector< int, 16 > sv{ 0, 1, 2, 3, 4 };
        auto it = sv.begin();
        RC_ASSERT( it[0] == 0 );
        RC_ASSERT( it[4] == 4 );
        RC_ASSERT( it + 5 == sv.end() );

        *it = 42;
        RC_ASSERT( *it == 42 );
        RC_ASSERT( sv[0] == 42 );
        static_vector< int, 16 >::const_iterator cit = it;
        RC_ASSERT( *cit == 42 );

        const auto &csv = sv;
        cit = csv.begin();
        RC_ASSERT( *cit == 42 );
        cit = csv.end();
        RC_ASSERT( cit[-1] == 4 );
        cit = sv.cbegin();
        RC_ASSERT( *cit == 42 );
        cit = sv.cend();
        RC_ASSERT( cit[-1] == 4 );
    } );

    rc::check( "static_vector reverse iterator", single, [] {
        static_vector< int, 16 > sv{ 0, 1, 2, 3, 4 };
        auto it = sv.rbegin();
        RC_ASSERT( it[0] == 4 );
        RC_ASSERT( it[4] == 0 );
        RC_ASSERT( it + 5 == sv.rend() );

        *it = 42;
        RC_ASSERT( *it == 42 );
        RC_ASSERT( sv[4] == 42 );
        static_vector< int, 16 >::const_reverse_iterator cit = it;
        RC_ASSERT( *cit == 42 );

        const auto &csv = sv;
        cit = csv.rbegin();
        RC_ASSERT( *cit == 42 );
        cit = csv.rend();
        RC_ASSERT( cit[-1] == 0 );
        cit = sv.crbegin();
        RC_ASSERT( *cit == 42 );
        cit = sv.crend();
        RC_ASSERT( cit[-1] == 0 );
    } );

    rc::check( "static_vector emplace_back counting", single, [] {
        InstanceCounter::Guard g;
        static_vector< InstanceCounter, 16 > sv;
        for ( size_t i = 0; i < 16; ++i ) {
            sv.emplace_back();
            RC_ASSERT( sv.size() == i + 1 );
        }
        RC_ASSERT( sv.full() );
        RC_ASSERT_THROWS( sv.emplace_back() );
    } );

    rc::check( "static_vector emplace_back random", []( std::vector< int > vals ) {
        static_vector< int, 16 > sv;
        const auto &csv = sv;
        if ( vals.size() > 16 )
            vals.resize( 16 );
        for ( auto v : vals ) {
            sv.emplace_back( v );
            RC_ASSERT( sv.front() == vals.front() );
            RC_ASSERT( sv.back() == v );
            RC_ASSERT( csv.front() == vals.front() );
            RC_ASSERT( csv.back() == v );
        }
        RC_ASSERT( vals.size() == sv.size() );
        for ( size_t i = 0; i < vals.size(); ++i )
            RC_ASSERT( vals[ i ] == sv[ i ] );
    } );

    rc::check( "static_vector push_back random", []( std::vector< int > vals ) {
        static_vector< int, 16 > sv;
        if ( vals.size() > 16 )
            vals.resize( 16 );
        std::copy( vals.begin(), vals.end(), std::back_inserter( sv ) );
        RC_ASSERT( vals.size() == sv.size() );
        for ( size_t i = 0; i < vals.size(); ++i )
            RC_ASSERT( vals[ i ] == sv[ i ] );
    } );

    rc::check( "static_vector push_back rvalue random", []( std::vector< int > vals ) {
        static_vector< int, 16 > sv;
        if ( vals.size() > 16 )
            vals.resize( 16 );
        std::copy( std::move_iterator( vals.begin() ),
                   std::move_iterator( vals.end() ), std::back_inserter( sv ) );
        RC_ASSERT( vals.size() == sv.size() );
        for ( size_t i = 0; i < vals.size(); ++i )
            RC_ASSERT( vals[ i ] == sv[ i ] );
    } );

    rc::check( "static_vector data random", []( std::vector< int > vals ) {
        if ( vals.size() > 16 )
            vals.resize( 16 );
        static_vector< int, 16 > sv{ vals.begin(), vals.end() };
        const auto &csv = sv;

        RC_ASSERT( sv.size() == vals.size() );
        for ( size_t i = 0; i < vals.size(); ++i ) {
            RC_ASSERT( vals[ i ] == sv.data()[ i ] );
            RC_ASSERT( vals[ i ] == csv.data()[ i ] );
        }
    } );

    rc::check( "static_vector indexing random", []( std::vector< int > vals ) {
        if ( vals.size() > 16 )
            vals.resize( 16 );
        static_vector< int, 16 > sv{ vals.begin(), vals.end() };
        const auto &csv = sv;

        for ( size_t i = 0; i < vals.size(); ++i ) {
            RC_ASSERT( vals[ i ] == sv[ i ] );
            RC_ASSERT( vals[ i ] == csv[ i ] );
            RC_ASSERT( vals[ i ] == sv.at( i ) );
            RC_ASSERT( vals[ i ] == csv.at( i ) );
        }
        RC_ASSERT_THROWS( sv.at( 16 ) );
        RC_ASSERT_THROWS( sv.at( vals.size() ) );
        RC_ASSERT_THROWS( csv.at( 16 ) );
        RC_ASSERT_THROWS( csv.at( vals.size() ) );
    } );

    rc::check( "static_vector dtor", single, [] {
        InstanceCounter::Guard g;
        {
            static_vector< InstanceCounter, 16 > sv( 8 );
            RC_ASSERT( InstanceCounter::ctor_cnt == 8 );
            sv.emplace_back();
            RC_ASSERT( InstanceCounter::ctor_cnt == 9 );
        }
        RC_ASSERT( InstanceCounter::dtor_cnt == 9 );
    } );

#define EMPLACE_INIT \
        if ( vals.size() > 16 ) \
            vals.resize( 16 ); \
        idx %= vals.size() + 1; \
        static_vector< int, 16 > sv{ vals.begin(), vals.end() }; \
        \
        bool pre_full = sv.full(); \
        auto pos = sv.begin() + idx; \
        RC_CLASSIFY( idx == sv.size(), "at end" )

    rc::check( "static_vector try_emplace", []( std::vector< int > vals, unsigned idx, int val ) {
        EMPLACE_INIT;

        auto r = sv.try_emplace( pos, val );
        RC_ASSERT( r.has_value() == !pre_full );
        if ( r.has_value() ) {
            RC_ASSERT( **r == val );
            RC_ASSERT( &**r == &*pos );
            RC_TAG( "try_emplace - success" );
        } else {
            RC_TAG( "try_emplace - full" );
        }
    } );

    rc::check( "static_vector emplace", []( std::vector< int > vals, unsigned idx, int val ) {
        EMPLACE_INIT;

        if ( pre_full ) {
            RC_ASSERT_THROWS( sv.emplace( pos, val ) );
            RC_TAG( "emplace - full" );
        }
        else {
            auto r = sv.emplace( pos, val );
            RC_ASSERT( *r == val );
            RC_ASSERT( &*r == &*pos );
            RC_TAG( "emplace - success" );
        }
    } );

    rc::check( "static_vector insert", []( std::vector< int > vals, unsigned idx, int val ) {
        EMPLACE_INIT;

        if ( pre_full ) {
            RC_ASSERT_THROWS( sv.insert( pos, val ) );
            RC_TAG( "insert - full" );
        }
        else {
            auto r = sv.insert( pos, val );
            RC_ASSERT( *r == val );
            RC_ASSERT( &*r == &*pos );
            RC_TAG( "insert - success" );
        }
    } );

    rc::check( "static_vector insert rvalue", []( std::vector< int > vals, unsigned idx, int val ) {
        EMPLACE_INIT;

        if ( pre_full ) {
            RC_ASSERT_THROWS( sv.insert( pos, std::move( val ) ) );
            RC_TAG( "insert - full" );
        }
        else {
            auto r = sv.insert( pos, std::move( val ) );
            RC_ASSERT( *r == val );
            RC_ASSERT( &*r == &*pos );
            RC_TAG( "insert - success" );
        }
    } );

    rc::check( "static_vector insert range", single, [] {
        auto ins = []( auto &sv, auto pos, std::initializer_list< int > ilist ) {
            sv.insert( pos, ilist.begin(), ilist.end() );
        };
        auto eq = []( const auto &sv, std::initializer_list< int > ilist ) {
            RC_ASSERT( !sv.empty() );
            RC_ASSERT( sv.size() == ilist.size() );
            for ( size_t i = 0; i < sv.size(); ++i ) {
                RC_ASSERT( sv[ i ].v == ilist.begin()[ i ] );
                RC_ASSERT( sv[ i ].constructed == sv[ i ].destructed + 1 );
            }
        };
        auto mk = []() {
            using V = static_vector< TractingInt, 16 >;
            auto ptr = std::calloc( 1, sizeof( V ) ); // NOLINT
            auto sv = std::unique_ptr< V, CADeleter< V > >( new ( ptr ) V{} );
            for ( int i : { 0, 1, 2, 3, 4 } )
                sv->emplace_back( i );
            return sv;
        };

        auto sv1 = mk();
        ins( *sv1, sv1->begin(), { -3, -2, -1 } );
        eq( *sv1, { -3, -2, -1, 0, 1, 2, 3, 4 } );

        auto sv2 = mk();
        ins( *sv2, sv2->end(), { 5, 6, 7 } );
        eq( *sv2, { 0, 1, 2, 3, 4, 5, 6, 7 } );

        auto sv3 = mk();
        ins( *sv3, std::next( sv3->begin(), 2 ), { 11, 12, 13 } );
        eq( *sv3, { 0, 1, 11, 12, 13, 2, 3, 4 } );

        auto sv4 = mk();
        ins( *sv4, std::next( sv4->begin(), 4 ), { 31, 32, 33 } );
        eq( *sv4, { 0, 1, 2, 3, 31, 32, 33, 4 } );

        auto sv5 = mk();
        ins( *sv5, std::next( sv5->begin(), 3 ), { 21, 22, 23, 24, 25, 26 } );
        eq( *sv5, { 0, 1, 2, 21, 22, 23, 24, 25, 26, 3, 4 } );

        static_vector< int, 4 > sv6{ 1, 2, 3 };
        auto to_ins = { 21, 22 };
        RC_ASSERT_THROWS( sv6.insert( std::next( sv6.begin(), 2 ), to_ins.begin(), to_ins.end() ) );
    } );

    rc::check( "static_vector insert range random",
        []( std::vector< int > vals, std::vector< int > to_ins, int pos )
    {
        pos %= vals.size() + 1;
        static_vector< int, 128 > sv{ vals.begin(), vals.end() };
        vals.insert( std::next( vals.begin(), pos ), to_ins.begin(), to_ins.end() );
        try {
            sv.insert( std::next( sv.begin(), pos ), to_ins.begin(), to_ins.end() );
            RC_ASSERT( sv.empty() == vals.empty() );
            RC_ASSERT( sv.size() == vals.size() );
            RC_ASSERT( std::equal( sv.begin(), sv.end(), vals.begin(), vals.end() ) );
        } catch (...) {
            RC_ASSERT( vals.size() > sv.capacity() );
            RC_TAG( "overfull" );
        }
    } );

    rc::check( "static_vector push/pop", []( std::vector< std::pair< bool, int > > vals ) {
        std::vector< int > stdvec;
        static_vector< int, 16 > sv;
        int poe = false, pue = false;
        for ( auto [ pop, v ] : vals ) {
            if ( pop ) {
                if ( sv.empty() )
                    RC_CLASSIFY( !poe++, "pop - empty" );
                else {
                    sv.pop_back();
                    stdvec.pop_back();
                }
            } else {
                if ( sv.full() )
                    RC_CLASSIFY( !pue++, "push - full" );
                else {
                    sv.push_back( v );
                    stdvec.push_back( v );
                }
            }
            RC_ASSERT( sv.size() == stdvec.size() );
            RC_ASSERT( std::equal( stdvec.begin(), stdvec.end(),
                                   sv.begin(), sv.end() ) );
        }
    } );

    rc::check( "static_vector resize counter", []( unsigned original, unsigned res ) {
        original %= 17;
        res %= 18;
        InstanceCounter::Guard g;
        static_vector< InstanceCounter, 16 > sv( original );
        RC_ASSERT( InstanceCounter::ctor_cnt == int( original ) );

        if ( res > 16 ) {
            RC_TAG( "too large" );
            RC_ASSERT_THROWS( sv.resize( res ) );
            return;
        }

        sv.resize( res );
        RC_ASSERT( sv.size() == res );
        if ( res < original ) {
            RC_TAG( "resize smaller" );
            RC_ASSERT( InstanceCounter::dtor_cnt == (int( original ) - int( res )) );
            RC_ASSERT( InstanceCounter::ctor_cnt == int( original ) );
        } else {
            RC_TAG( "resize larger" );
            RC_ASSERT( InstanceCounter::dtor_cnt == 0 );
            RC_ASSERT( InstanceCounter::ctor_cnt == int( res ) );
        }
    } );

    rc::check( "static_vector resize int", []( unsigned original, unsigned res ) {
        original %= 17;
        res %= 17;
        static_vector< int, 16 > sv( original, 42 );

        if ( res > 16 ) {
            RC_TAG( "too large" );
            RC_ASSERT_THROWS( sv.resize( res ) );
            return;
        }

        sv.resize( res );
        RC_ASSERT( sv.size() == res );
        if ( res < original ) {
            RC_TAG( "resize smaller" );
            for ( unsigned i = 0; i < res; ++i )
                RC_ASSERT( sv[ i ] == 42 );
        } else {
            RC_TAG( "resize larger" );
            for ( unsigned i = 0; i < res; ++i )
                if ( i < original )
                    RC_ASSERT( sv[ i ] == 42 );
                else
                    RC_ASSERT( sv[ i ] == 0 );
        }
    } );

    rc::check( "static_vector resize int val", []( unsigned original, unsigned res ) {
        original %= 17;
        res %= 18;
        static_vector< int, 16 > sv( original, 42 );

        if ( res > 16 ) {
            RC_TAG( "too large" );
            RC_ASSERT_THROWS( sv.resize( res ) );
            return;
        }

        sv.resize( res, 16 );
        RC_ASSERT( sv.size() == res );
        if ( res < original ) {
            RC_TAG( "resize smaller" );
            for ( unsigned i = 0; i < res; ++i )
                RC_ASSERT( sv[ i ] == 42 );
        } else {
            RC_TAG( "resize larger" );
            for ( unsigned i = 0; i < res; ++i )
                if ( i < original )
                    RC_ASSERT( sv[ i ] == 42 );
                else
                    RC_ASSERT( sv[ i ] == 16 );
        }
    } );

    rc::check( "static_vector push/erase", []( std::vector< std::pair< bool, int > > vals ) {
        std::vector< std::unique_ptr< int > > stdvec;
        static_vector< std::unique_ptr< int >, 16 > sv;
        int poe = false, pue = false;
        for ( auto [ pop, v ] : vals ) {
            if ( pop ) {
                if ( sv.empty() )
                    RC_CLASSIFY( !poe++, "erase - empty" );
                else {
                    int idx = std::abs( v ) % stdvec.size();
                    sv.erase( sv.begin() + idx );
                    stdvec.erase( stdvec.begin() + idx );
                }
            } else {
                if ( sv.full() )
                    RC_CLASSIFY( !pue++, "push - full" );
                else {
                    sv.push_back( std::make_unique< int >( v ) );
                    stdvec.push_back( std::make_unique< int >( v ) );
                }
            }
            RC_ASSERT( sv.size() == stdvec.size() );
            RC_ASSERT( std::equal( stdvec.begin(), stdvec.end(),
                                   sv.begin(), sv.end(),
                                   []( auto &a, auto &b ) { return *a == *b; } ) );
        }
    } );

    rc::check( "static_vector insert/erase range", []( std::vector< std::tuple< bool, int, int > > vals ) {
        std::vector< std::unique_ptr< int > > stdvec;
        static_vector< std::unique_ptr< int >, 16 > sv;
        int poe = false, pue = false;
        for ( auto [ pop, v1, v2 ] : vals ) {
            if ( pop ) {
                if ( sv.empty() )
                    RC_CLASSIFY( !poe++, "erase - empty" );
                else {
                    int idx1 = std::abs( v1 ) % stdvec.size();
                    int idx2 = idx1 + (std::abs( v2 ) % (stdvec.size() - idx1)) + 1;
                    sv.erase( sv.begin() + idx1, sv.begin() + idx2 );
                    stdvec.erase( stdvec.begin() + idx1, stdvec.begin() + idx2 );
                }
            } else {
                if ( sv.full() )
                    RC_CLASSIFY( !pue++, "push - full" );
                else {
                    int idx1 = stdvec.empty() ? 0 : std::abs( v1 ) % stdvec.size();
                    sv.insert( sv.begin() + idx1, std::make_unique< int >( v2 ) );
                    stdvec.insert( stdvec.begin() + idx1, std::make_unique< int >( v2 ) );
                }
            }
            RC_ASSERT( sv.size() == stdvec.size() );
            RC_ASSERT( std::equal( stdvec.begin(), stdvec.end(),
                                   sv.begin(), sv.end(),
                                   []( auto &a, auto &b ) { return *a == *b; } ) );
        }
    } );

    rc::check( "static_vector operator==", []( std::vector< int > vals ) {
        if ( vals.size() > 16 )
            vals.resize( 16 );
        static_vector< int, 16 > sv1{ vals.begin(), vals.end() };
        static_vector< int, 16 > sv2{ vals.begin(), vals.end() };
        RC_ASSERT( sv1 == sv2 );

        if ( !vals.empty() ) {
            ++sv1[0];
            RC_ASSERT( !(sv1 == sv2 ) );
        }
    } );

    rc::check( "static_vector operator!=", []( std::vector< int > vals ) {
        if ( vals.size() > 16 )
            vals.resize( 16 );
        static_vector< int, 16 > sv1{ vals.begin(), vals.end() };
        static_vector< int, 16 > sv2{ vals.begin(), vals.end() };
        RC_ASSERT( !(sv1 != sv2) );

        if ( !vals.empty() ) {
            ++sv1[0];
            RC_ASSERT( sv1 != sv2 );
        }
    } );

    auto check_operator = []( std::string op, std::string name, auto cmp ) {
        rc::check( "static_vector operator" + op, [&]( std::vector< int > vals1, std::vector< int > vals2 ) {
            if ( vals1.size() > 4 )
                vals1.resize( 4 );
            if ( vals2.size() > 4 )
                vals2.resize( 4 );

            static_vector< int, 4 > sv1{ vals1.begin(), vals1.end() };
            static_vector< int, 4 > sv2{ vals2.begin(), vals2.end() };
            if ( cmp( vals1, vals2 ) ) {
                RC_TAG( name );
                RC_ASSERT( cmp( sv1, sv2 ) );
            } else {
                RC_TAG( "!" + name );
                RC_ASSERT( !cmp( sv1, sv2 ) );
            }
        } );
    };

    check_operator( "==", "equal", []( auto a, auto b ) { return a == b; } );
    check_operator( "!=", "not equal", []( auto a, auto b ) { return a != b; } );
    check_operator( "<", "less", []( auto a, auto b ) { return a < b; } );
    check_operator( "<=", "less equal", []( auto a, auto b ) { return a <= b; } );
    check_operator( ">", "greater", []( auto a, auto b ) { return a > b; } );
    check_operator( ">=", "greater equal", []( auto a, auto b ) { return a >= b; } );
}
