/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: hellopi.cc,v 1.3 2003-10-30 12:48:07 bert Exp $";

#include <iostream>

extern "C" const char* InitHelloPlugin( int*, char** )
{
    std::cout << "Hello world" << std::endl;
    return 0; // All OK - no error messages
}
