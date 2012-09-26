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
#include <iostream>

#ifdef __win__
#include "file.h"
#endif


int main( int argc, char ** argv )
{
    bool dofork = true;
    uiSEGYExamine::Setup su;
    int argidx = 1;
    while ( argc > argidx
	 && *argv[argidx] == '-' && *(argv[argidx]+1) == '-' )
    {
	if ( !strcmp(argv[argidx],"--ns") )
	    { argidx++; su.fp_.ns_ = toInt( argv[argidx] ); }
	else if ( !strcmp(argv[argidx],"--fmt") )
	    { argidx++; su.fp_.fmt_ = toInt( argv[argidx] ); }
	else if ( !strcmp(argv[argidx],"--nrtrcs") )
	    { argidx++; su.nrtrcs_ = toInt( argv[argidx] ); }
	else if ( !strcmp(argv[argidx],"--filenrs") )
	    { argidx++; su.fs_.getMultiFromString( argv[argidx] ); }
	else if ( !strcmp(argv[argidx],"--swapbytes") )
	    { argidx++; su.fp_.byteswap_ = toInt( argv[argidx] ); }
	else if ( !strcmp(argv[argidx],"--fg") )
	    dofork = false;
	else
	    { std::cerr << "Ignoring option: " << argv[argidx] << std::endl; }

	argidx++;
    }

    if ( argc <= argidx )
    {
	std::cerr << "Usage: " << argv[0]
		  << "\n\t[--ns #samples]""\n\t[--nrtrcs #traces]"
		     "\n\t[--fmt segy_format_number]"
	    	     "\n\t[--filenrs start`stop`step[`nrzeropad]]"
		     "\n\t[--swapbytes 0_1_or_2]"
		     "\n\tfilename\n"
	     << "Note: filename must be with FULL path." << std::endl;
	ExitProgram( 1 );
    }

#ifdef __debug__
    std::cerr << argv[0] << " started with args:";
    for ( int idx=1; idx<argc; idx++ )
	std::cerr << ' ' << argv[idx];
    std::cerr << std::endl;
#endif

    if ( dofork )
	forkProcess();

    su.fs_.fname_ = argv[argidx];
    replaceCharacter( su.fs_.fname_.buf(), (char)128, ' ' );
    replaceString( su.fs_.fname_.buf(), "+x+", "*" );

    uiMain app( argc, argv );

#ifdef __win__
    if ( File::isLink( su.fs_.fname_.buf() ) )
	su.fs_.fname_ = File::linkTarget( su.fs_.fname_.buf() );
#endif

    uiSEGYExamine* sgyex = new uiSEGYExamine( 0, su );
    app.setTopLevel( sgyex );
    sgyex->show();
    ExitProgram( app.exec() ); return 0;
}
