/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : July 2007
-*/
static const char* rcsID = "$Id$";

#include "prog.h"
#include "seisimpbpsif.h"
#include "multiid.h"


static int doWork( int argc, char** argv )
{
    if ( argc < 3 )
    {
	od_cout() << "Usage: " << argv[0]
	    << " inp_BPSIF_file output_id [max_inl_offs]\n";
	return 1;
    }

    SeisImpBPSIF imp( argv[1], MultiID(argv[2]) );
    const int maxinloffs = argc < 4 ? -1 : toInt(argv[3]);
    if ( maxinloffs > 0 )
	od_cout() << "Max inl offset: " << maxinloffs << od_endl;
    imp.setMaxInlOffset( argc < 4 ? -1 : toInt(argv[3]) );

    return imp.execute( &od_cout().stdStream() ) ? 0 : 1;
}


int main( int argc, char** argv )
{
    return ExitProgram( doWork(argc,argv) );
}
