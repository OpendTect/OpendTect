/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: hellopi.cc,v 1.7 2009-04-21 03:32:43 cvsnanne Exp $";

#include <iostream>

// MSVC macro's 
#ifdef HELLO_EXPORTS
# define mExternC extern "C" __declspec( dllexport )
#else
# define mExternC extern "C"
#endif

mExternC const char* InitHelloPlugin( int, char** )
{
    std::cout << "Hello world" << std::endl;
    return 0; // All OK - no error messages
}
