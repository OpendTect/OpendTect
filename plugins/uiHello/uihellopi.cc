/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: uihellopi.cc,v 1.1 2003-10-30 13:07:42 bert Exp $";

#include "uimsg.h"

extern "C" const char* InituiHelloPlugin( int*, char** )
{
    uiMSG().message( "Hello world" );
    return 0; // All OK - no error messages
}
