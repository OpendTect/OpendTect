/*+
 * AUTHOR   : A.H. Lammertink
 * DATE     : October 2003
 * FUNCTION : get special folder location
-*/

static const char* rcsID = "$Id: SearchODFile.cc,v 1.1 2004-10-25 11:01:56 dgb Exp $";


#include "prog.h"
#include "genc.h"

#include <iostream>

using namespace std;

int main( int argc, char** argv )
{
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

