//  (C) Copyright Gennadiy Rozental 2005-2014.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
#ifndef BOOST_TEST_DYN_LINK
#define BOOST_TEST_DYN_LINK
#endif
#include <boost/test/unit_test.hpp>
// problem building original version in boost 1.60; command line argument processing is absent or moved
// removing the #includes and declarations - Riabinin

// STL
#include <iostream>
#include <fstream>

//_________________________________________________________________//

// System API

namespace dyn_lib {

#if defined(BOOST_WINDOWS) && !defined(BOOST_DISABLE_WIN32) // WIN32 API

#include <windows.h>

typedef HINSTANCE handle;

inline handle
open( std::string const& file_name )
{
    return LoadLibrary( file_name.c_str() );
}

//_________________________________________________________________//

template<typename TargType>
inline TargType
locate_symbol( handle h, std::string const& symbol )
{
    return reinterpret_cast<TargType>( GetProcAddress( h, symbol.c_str() ) );
}

//_________________________________________________________________//

inline void
close( handle h )
{
    if( h )
        FreeLibrary( h );
}

//_________________________________________________________________//

inline std::string
error()
{
    LPTSTR msg = NULL;

    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL, 
                   GetLastError(), 
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPTSTR)&msg, 
                   0, NULL );

    std::string res( msg );

    if( msg )
        LocalFree( msg );

    return res;    
}

//_________________________________________________________________//

#elif defined(BOOST_HAS_UNISTD_H) // POSIX API

#include <dlfcn.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


typedef void* handle;

inline handle
open( std::string const& file_name )
{
    return dlopen( file_name.c_str(), RTLD_LOCAL | RTLD_LAZY );
}

//_________________________________________________________________//

template<typename TargType>
inline TargType
locate_symbol( handle h, std::string const& symbol )
{
    return reinterpret_cast<TargType>( dlsym( h, symbol.c_str() ) );
}

//_________________________________________________________________//

inline void
close( handle h )
{
    if( h )
        dlclose( h );
}

//_________________________________________________________________//

inline std::string
error()
{
    return dlerror();
}

//_________________________________________________________________//

#else

#error "Dynamic library API is unknown"

#endif

} // namespace dyn_lib

//____________________________________________________________________________//

static std::string test_lib_name;
static std::string init_func_name( "init_unit_test" );

dyn_lib::handle test_lib_handle;

bool load_test_lib()
{
    typedef bool (*init_func_ptr)();
    init_func_ptr init_func;

    test_lib_handle = dyn_lib::open( test_lib_name );
    if( !test_lib_handle )
        throw std::logic_error( std::string("Fail to load test library: ")
                                    .append( dyn_lib::error() ) );

    init_func =  dyn_lib::locate_symbol<init_func_ptr>( test_lib_handle, init_func_name );
    
    if( !init_func )
        throw std::logic_error( std::string("Can't locate test initilization function ")
                                    .append( init_func_name )
                                    .append( ": " )
                                    .append( dyn_lib::error() ) );
   
    return (*init_func)();
}

//____________________________________________________________________________//

int main( int argc, char* argv[] )
{
// problem building original version in boost 1.60; command line argument processing is absent or moved
// doing trivial initialization here - Riabinin

    if ( argc < 2 )
    {
        std::cout << "need a dll to get tests from" << std::endl;
        return 0;
    }

    test_lib_name = argv[1];

    // after main returns, memory leak report will be dumped on us, which contains too many false positives to be useful
    // It is written to cerr; I wish I could execute the main portion, and then redirect cerr to a file, to hide the report
    // But I was not able to; somehow on return from main the redirection is canceled
    // So instead I rely on the caller program to use '2> file' (which works all the time), but temporarily redirect cerr to cout for the duration of the real run


    auto cerr_buff = std::cerr.rdbuf(); 
    std::cerr.rdbuf(std::cout.rdbuf());

    int res = ::boost::unit_test::unit_test_main( &load_test_lib, argc-1, argv+1 );

    ::boost::unit_test::framework::clear();
    dyn_lib::close( test_lib_handle );
    std::cerr << std::flush;

    // looks like I don't need to restore - it will be done on returm from main
    // but I am not sure if it is guaranteed, so
    std::cerr.rdbuf(cerr_buff);

    return res;
}

//____________________________________________________________________________//

// EOF
