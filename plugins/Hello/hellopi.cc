/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id";

#include <iostream>

extern "C" const char* HelloInitPlugin( int*, char** )
{
    std::cout << "Hello world" << std::endl;
    return 0; // All OK - no error messages
}
