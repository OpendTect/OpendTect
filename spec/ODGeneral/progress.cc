
#include <iostream>
#include "progressmeter.h"
#include "prog.h"


int mProgMainFnName( int argc, char** argv )
{
    int delayms = 0;
    int curarg = 1;
    int chunksz = 1024;
    if ( argc > curarg )
    {
	if ( !strcmp(argv[curarg],"--delay") )
	    { curarg++; delayms = toInt( argv[curarg] ); curarg++; }
	if ( !strcmp(argv[curarg],"--blocksz") )
	    { curarg++; chunksz = toInt( argv[curarg] ); curarg++; }
    }
    int totnr = argc > curarg ? toInt(argv[curarg]) : 0;
    if ( totnr < 0 ) totnr = 0;

    if ( delayms ) std::cerr << "Delay in ms: " << delayms << std::endl;
    std::cerr << "Blocks of " << chunksz << " bytes\n" << std::endl;

    TextStreamProgressMeter progressmeter( std::cerr );
    char* buf = new char [chunksz];
    while ( true )
    {
	std::cin.read( buf, chunksz );
	int bytesread = std::cin.gcount();
	std::cout.write( buf, bytesread );
	if ( bytesread < chunksz ) break;

	if ( totnr )	progressmeter.setNrDone( totnr );
	else		++progressmeter;

	if ( delayms )
	    sleepSeconds( delayms*0.001 );
    }

    return 0;
}
