static const char* rcsID = "$Id: progress.cc,v 1.11 2010/10/14 09:58:06 cvsbert Exp $";

#include <iostream>
#include <stdlib.h>
#include "progressmeter.h"
#include "thread.h"
#include "prog.h"


int main( int argc, char** argv )
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
	    Threads::sleep( delayms*0.001 );
    }

    ExitProgram( 0 ); return 0;
}
