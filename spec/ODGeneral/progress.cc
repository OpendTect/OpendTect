#include <iostream>
#include <stdlib.h>
#include "progressmeter.h"
#include "prog.h"

const int chunksz = 1024;

int main( int argc, char** argv )
{
    const int totnr = argc > 1 ? atoi(argv[1]) : 0;
    ProgressMeter progressmeter( std::cerr );

    char buf[chunksz];
    while ( 1 )
    {
	std::cin.read( buf, chunksz );
	int bytesread = std::cin.gcount();
	std::cout.write( buf, bytesread );
	if ( bytesread < chunksz ) break;

	if ( totnr )	progressmeter.update( totnr );
	else		++progressmeter;
    }

    exitProgram( 0 ); return 0;
}
