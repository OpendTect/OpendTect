/*+
 * AUTHOR   : A.H. Lammertink
 * DATE     : October 2003
 * FUNCTION : get special folder location
-*/

static const char* rcsID = "$Id: SearchODFile.cc,v 1.3 2004-11-15 15:46:28 dgb Exp $";


#include "prog.h"
#include "genc.h"

#include <iostream>

using namespace std;

int main( int argc, char** argv )
{
    char* debugenv = "DTECT_DEBUG=";
    if ( !getenv("DTECT_NO_IGNORE_DEBUG") )
	putenv( debugenv );

    if ( argc < 2 )
    {
	cerr << "Usage: " << argv[0]
	     << " <filename> "<< endl;
	return 1;
    }

    const char* result = SearchODFile( argv[1] );
    if ( !result || !*result ) return 1;


    cout <<  result << endl;
    return 0;
}

