/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: hellopi.cc,v 1.6 2009-04-16 10:33:26 cvsranojay Exp $";

#include <iostream>

extern "C"  __declspec( dllexport ) const char* InitHelloPlugin( int, char** )
{
    std::cout << "Hello world" << std::endl;
    return 0; // All OK - no error messages
}
