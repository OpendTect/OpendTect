#include <iostream.h>
#include <stdlib.h>
#include "progressmeter.h"

const int chunksz = 1024;

int main( int argc, char** argv )
{
    const int totnr = argc > 1 ? atoi(argv[1]) : 0;
    ProgressMeter progressmeter( cerr );

    unsigned char buf[chunksz];
    while ( 1 )
    {
	cin.read( buf, chunksz );
	int bytesread = cin.gcount();
	cout.write( buf, bytesread );
	if ( bytesread < chunksz ) break;

	if ( totnr )	progressmeter.update( totnr );
	else		++progressmeter;
    }

    return 0;
}
