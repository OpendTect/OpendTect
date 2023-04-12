/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegyexamine.h"

#include "commandlineparser.h"
#include "file.h"
#include "moddepmgr.h"
#include "prog.h"
#include "seistype.h"
#include "uimain.h"


static int printUsage( const char* cmd )
{
    od_cout() << "Usage: " << cmd
	      << "\n\t[--geomtype #Seis::nameOf(Seis::GeomType), mandatory]"
		 "\n\t[--ns #samples]"
		 "\n\t[--nrtrcs #traces]"
		 "\n\t[--fmt segy_format_number]"
		 "\n\t[--filenrs start`stop`step[`nrzeropad]]"
		 "\n\t[--swapbytes 0_1_or_2]"
		 "\n\tfilename\n"
	      << "Note: filename must be with FULL path." << od_endl;
    return 1;
}


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv );
    uiMain app( argc, argv );

    OD::ModDeps().ensureLoaded( "uiIo" );

    CommandLineParser clp( argc, argv );
    clp.setKeyHasValue( "geomtype" );
    clp.setKeyHasValue( "nrtrcs" );
    clp.setKeyHasValue( "ns" );
    clp.setKeyHasValue( "fmt" );
    clp.setKeyHasValue( "swapbytes" );
    clp.setKeyHasValue( "filenrs" );

    BufferStringSet normargs;
    clp.getNormalArguments( normargs );
    if ( normargs.size() < 1 || !File::exists(normargs.first()->buf()) )
	return printUsage( clp.getExecutable() );

    Seis::GeomType gt;
    getFromCLP( clp, gt );

    uiSEGYExamine::Setup su( gt );
    BufferString tmpstr;
    if ( clp.getVal("nrtrcs",tmpstr) && !tmpstr.isEmpty() )
	su.nrtrcs( tmpstr.toInt() );

    if ( clp.getVal("ns",tmpstr) && !tmpstr.isEmpty() )
	su.fp_.ns_ = tmpstr.toInt();

    if ( clp.getVal("fmt",tmpstr) && !tmpstr.isEmpty() )
	su.fp_.fmt_ = tmpstr.toInt();

    if ( clp.getVal("filenrs",tmpstr) && !tmpstr.isEmpty() )
	su.fs_.getMultiFromString( tmpstr.str() );

    if ( clp.getVal("swapbytes",tmpstr) && !tmpstr.isEmpty() )
	su.fp_.byteswap_ = tmpstr.toInt();

#ifdef __debug__
    od_cout() << argv[0] << " started with args:";
    for ( int idx=1; idx<argc; idx++ )
	od_cout() << ' ' << argv[idx];
    od_cout() << od_endl;
#endif

    BufferString& fnm = *normargs.first();
    fnm.replace( "+x+", "*" );
    su.setFileName( fnm );

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiWellAttrib" );
    PtrMan<uiDialog> sgyex = new uiSEGYExamine( nullptr, su );
    app.setTopLevel( sgyex );
    PIM().loadAuto( true );
    sgyex->show();

    return app.exec();
}
