/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 1-12-1999
 * FUNCTION : Create extra picks from locations file
-*/

static const char* rcsID = "$Id: extend_locs.cc,v 1.5 2004-04-28 21:30:58 bert Exp $";

#include "prog.h"
#include "strmprov.h"
#include "survinfo.h"
#include "stats.h"
#include <stdlib.h>
#include <iostream>


int main( int argc, char** argv )
{
    if ( argc != 3 )
    {
	std::cerr << "Usage: " << argv[0] << " input_locs output_locs"
	    	  << std::endl;
	exitProgram( 1 );
    }
    StreamProvider spin( argv[1] );
    StreamData sdin = spin.makeIStream();
    if ( !sdin.istrm )
    {
	std::cerr << argv[0] << ": Cannot open input stream" << std::endl;
	exitProgram( 1 );
    }
    else if ( sdin.istrm == &std::cin )
	std::cout << "Using standard input." << std::endl;
    std::istream& instrm = *sdin.istrm;

    Stat_initRandom(0);
 
    if ( !instrm )
    {
	std::cerr << "Bad locations file" << std::endl;
	exitProgram( 1 );
    }
    StreamProvider spout( argv[2] );
    StreamData sdout = spout.makeOStream();
    if ( !sdout.ostrm )
    {
	std::cerr << argv[0] << ": Cannot open output stream" << std::endl;
	exitProgram( 1 );
    }
    std::ostream& outstrm = *sdout.ostrm;

    BinID bid; float timeval;
    while ( instrm )
    {
	instrm >> bid.inl >> bid.crl >> timeval;
	if ( instrm.eof() || !bid.inl ) break;
	SI().snap( bid, BinID(0,0) );
	outstrm << bid.inl << '\t' << bid.crl << '\t' << timeval <<'\n';
	bid.inl -= SI().inlStep();
	outstrm << bid.inl << '\t' << bid.crl << '\t'
		<< timeval + (Stat_getRandom()-.5)*20 <<'\n';
	bid.inl += SI().inlStep();
	bid.crl -= SI().crlStep();
	outstrm << bid.inl << '\t' << bid.crl << '\t'
		<< timeval + (Stat_getRandom()-.5)*20 <<'\n';
	bid.inl += SI().inlStep();
	bid.crl += SI().crlStep();
	outstrm << bid.inl << '\t' << bid.crl << '\t'
		<< timeval + (Stat_getRandom()-.5)*20 <<'\n';
	bid.inl -= SI().inlStep();
	bid.crl += SI().crlStep();
	outstrm << bid.inl << '\t' << bid.crl << '\t'
		<< timeval + (Stat_getRandom()-.5)*20 <<'\n';
    }

    sdin.close(); sdout.close();
    exitProgram( 0 ); return 0;
}
