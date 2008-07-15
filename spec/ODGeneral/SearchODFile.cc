/*+
 * AUTHOR   : A.H. Lammertink
 * DATE     : October 2003
 * FUNCTION : get special folder location
-*/

static const char* rcsID = "$Id: SearchODFile.cc,v 1.7 2008-07-15 12:21:31 cvsbert Exp $";


#include "prog.h"
#include "oddirs.h"
#include "envvars.h"

#include <iostream>


int main( int argc, char** argv )
{
    SetEnvVar( "DTECT_DEBUG", "" );
    if ( argc < 2 )
    {
	std::cerr << "Usage: " << argv[0] << " [filename] "<< std::endl;
	return 1;
    }

    const char* result = SearchODFile( argv[1] );
    if ( !result || !*result ) return 1;

    std::cout << result << std::endl;
    return 0;
}
