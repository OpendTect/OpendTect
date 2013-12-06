/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Aug 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisegyexamine.h"

#include "uimain.h"
#include "prog.h"

#ifdef __win__
#include "file.h"
#endif


int main( int argc, char ** argv )
{
    SetProgramArgs( argc, argv );

    bool dofork = true;
    uiSEGYExamine::Setup su;
    int argidx = 1;
    while ( argc > argidx
	 && *argv[argidx] == '-' && *(argv[argidx]+1) == '-' )
    {
	const FixedString arg( argv[argidx]+2 );
#define mArgIs(s) arg == s
	if ( mArgIs("ns") )
	    { argidx++; su.fp_.ns_ = toInt( argv[argidx] ); }
	else if ( mArgIs("fmt") )
	    { argidx++; su.fp_.fmt_ = toInt( argv[argidx] ); }
	else if ( mArgIs("nrtrcs") )
	    { argidx++; su.nrtrcs_ = toInt( argv[argidx] ); }
	else if ( mArgIs("filenrs") )
	    { argidx++; su.fs_.getMultiFromString( argv[argidx] ); }
	else if ( mArgIs("swapbytes") )
	    { argidx++; su.fp_.byteswap_ = toInt( argv[argidx] ); }
	else if ( mArgIs("fg") )
	    dofork = false;
	else
	    { od_cout() << "Ignoring option: " << argv[argidx] << od_endl; }

	argidx++;
    }

    if ( argc <= argidx )
    {
	od_cout() << "Usage: " << argv[0]
		  << "\n\t[--ns #samples]""\n\t[--nrtrcs #traces]"
		     "\n\t[--fmt segy_format_number]"
		     "\n\t[--filenrs start`stop`step[`nrzeropad]]"
		     "\n\t[--swapbytes 0_1_or_2]"
		     "\n\tfilename\n"
		  << "Note: filename must be with FULL path." << od_endl;
	return ExitProgram( 1 );
    }

#ifdef __debug__
    od_cout() << argv[0] << " started with args:";
    for ( int idx=1; idx<argc; idx++ )
	od_cout() << ' ' << argv[idx];
    od_cout() << od_endl;
#endif

    if ( dofork )
	ForkProcess();

    su.fs_.fname_ = argv[argidx];
    su.fs_.fname_.replace( "+x+", "*" );

    uiMain app( argc, argv );

#ifdef __win__
    if ( File::isLink( su.fs_.fname_.buf() ) )
	su.fs_.fname_ = File::linkTarget( su.fs_.fname_.buf() );
#endif

    uiSEGYExamine* sgyex = new uiSEGYExamine( 0, su );
    app.setTopLevel( sgyex );
    sgyex->show();

    return ExitProgram( app.exec() );
}
