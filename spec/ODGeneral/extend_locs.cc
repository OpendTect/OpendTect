/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 1-12-1999
 * FUNCTION : Create extra picks from locations file
-*/

static const char* rcsID = "$Id: extend_locs.cc,v 1.1 2000-01-12 16:51:35 bert Exp $";

#include "prog.h"
#include "strmprov.h"
#include "survinfo.h"
#include "stats.h"
#include <stdlib.h>
#include <iostream.h>


int main( int argc, char** argv )
{
    if ( argc != 3 )
    {
	cerr << "Usage: " << argv[0] << " input_locs output_locs" << endl;
	return 1;
    }
    StreamProvider spin( argv[1] );
    StreamData sdin = spin.makeIStream();
    if ( !sdin.istrm )
    {
	cerr << argv[0] << ": Cannot open input stream" << endl;
	return 1;
    }
    else if ( sdin.istrm == &cin )
	cout << "Using standard input." << endl;
    istream& instrm = *sdin.istrm;

    Stat_initRandom(0);
 
    if ( !instrm )
    {
	cerr << "Bad locations file" << endl;
	return 1;
    }
    StreamProvider spout( argv[2] );
    StreamData sdout = spout.makeOStream();
    if ( !sdout.ostrm )
    {
	cerr << argv[0] << ": Cannot open output stream" << endl;
	return 1;
    }
    ostream& outstrm = *sdout.ostrm;

    BinID bid; float timeval;
    while ( instrm )
    {
	instrm >> bid.inl >> bid.crl >> timeval;
	if ( instrm.eof() || !bid.inl ) break;
	SI().snap( bid, BinID(0,0) );
	outstrm << bid.inl << '\t' << bid.crl << '\t' << timeval <<'\n';
	bid.inl -= SI().step().inl;
	outstrm << bid.inl << '\t' << bid.crl << '\t'
		<< timeval + (Stat_getRandom()-.5)*20 <<'\n';
	bid.inl += SI().step().inl;
	bid.crl -= SI().step().crl;
	outstrm << bid.inl << '\t' << bid.crl << '\t'
		<< timeval + (Stat_getRandom()-.5)*20 <<'\n';
	bid.inl += SI().step().inl;
	bid.crl += SI().step().crl;
	outstrm << bid.inl << '\t' << bid.crl << '\t'
		<< timeval + (Stat_getRandom()-.5)*20 <<'\n';
	bid.inl -= SI().step().inl;
	bid.crl += SI().step().crl;
	outstrm << bid.inl << '\t' << bid.crl << '\t'
		<< timeval + (Stat_getRandom()-.5)*20 <<'\n';
    }

    sdin.close(); sdout.close();
    return 0;
}
