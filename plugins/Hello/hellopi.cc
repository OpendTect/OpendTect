/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: hellopi.cc,v 1.8 2009-07-22 16:01:27 cvsbert Exp $";

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
