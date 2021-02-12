/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2001
________________________________________________________________________

-*/

#include "uisegyexamine.h"

#include "uimain.h"
#include "prog.h"
#include "moddepmgr.h"
#include "commandlineparser.h"

#ifdef __win__
#include "file.h"
#endif


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv, false );

    uiMain app;
    CommandLineParser& clp = app.commandLineParser();

    OD::ModDeps().ensureLoaded( "uiSeis" );

    BufferStringSet normargs;
    clp.getNormalArguments( normargs );
    if ( normargs.size() != 1 )
    {
	od_cout() << "Usage: " << clp.getExecutableName()
		  << "\n\t[--ns #samples]""\n\t[--nrtrcs #traces]"
		     "\n\t[--fmt segy_format_number]"
		     "\n\t[--filenrs start`stop`step[`nrzeropad]]"
		     "\n\t[--swapbytes 0_1_or_2]"
		     "\n\tfilename\n"
		  << "Note: filename must be with FULL path." << od_endl;
	return 1;
    }

    const char* key_ns = "ns";
    const char* key_fmt = "fmt";
    const char* key_nrtrcs = "nrtrcs";
    const char* key_filenrs = "filenrs";
    const char* key_swapbytes = "swapbytes";

    uiSEGYExamine::Setup su;
    clp.setKeyHasValue( key_ns );
    clp.setKeyHasValue( key_fmt );
    clp.setKeyHasValue( key_nrtrcs );
    clp.setKeyHasValue( key_filenrs );
    clp.setKeyHasValue( key_swapbytes );

#define mHaveArg(ky) clp.hasKey( key_##ky )
    if ( mHaveArg(ns) )
	clp.getKeyedInfo( key_ns, su.fp_.ns_ );
    else if ( mHaveArg(fmt) )
	clp.getKeyedInfo( key_fmt, su.fp_.fmt_ );
    else if ( mHaveArg(nrtrcs) )
	clp.getKeyedInfo( key_nrtrcs, su.nrtrcs_ );
    else if ( mHaveArg(swapbytes) )
	clp.getKeyedInfo( key_swapbytes, su.fp_.byteswap_ );
    else if ( mHaveArg(filenrs) )
    {
	BufferString nrstr;
	clp.getKeyedInfo( key_filenrs, nrstr );
	su.fs_.getMultiFromString( nrstr );
    }

    BufferString fnm( normargs.get(0) );
    fnm.replace( "+x+", "*" );
    su.fs_.setFileName( fnm );

#ifdef __win__
    if ( File::isLink( su.fs_.fileName(0) ) )
	su.fs_.setFileName( File::linkEnd( su.fs_.fileName(0) ) );
#endif

    PtrMan<uiDialog> sgyex = new uiSEGYExamine( nullptr, su );
    app.setTopLevel( sgyex );
    sgyex->show();

    return app.exec();
}
