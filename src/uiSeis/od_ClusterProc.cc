/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          April 2009
________________________________________________________________________

-*/

#include "prog.h"
#include "uiclusterproc.h"
#include "uimain.h"

#include "commandlineparser.h"
#include "envvars.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "moddepmgr.h"
#include "keystrs.h"
#include "survinfo.h"
#include "od_iostream.h"


#define mPrintHelpMsg \
    od_cout() << "Usage: " << argv[0] << " --parfile <parfile>" \
	      << " [--dosubmit] [--nodelete]" << od_endl;

int mProgMainFnName( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    OD::ModDeps().ensureLoaded( "General" );

    CommandLineParser parser;

    const bool withdelete = !parser.hasKey( "nodelete" );
    const bool dosubmit = parser.hasKey( "dosubmit" );
    mInitProg( dosubmit ? OD::UiProgCtxt : OD::BatchProgCtxt )

    BufferString parfilenm;
    if ( !parser.getVal("parfile",parfilenm) )
    {
	BufferStringSet normalargs;
	parser.getNormalArguments( normalargs );
	if ( normalargs.isEmpty() )
	{
	    mPrintHelpMsg;
	    return 1;
	}

	parfilenm = normalargs.get(0);
    }

    od_istream strm( parfilenm );
    if ( !strm.isOK() )
    {
	od_cout() << argv[0] << ": Cannot open parameter file" << od_endl;
	return 1;
    }

    IOPar iop; iop.read( strm, sKey::Pars() );
    if ( iop.size() == 0 )
    {
	od_cout() << argv[0] << ": Invalid parameter file" << od_endl;
	return 1;
    }

    const char* res = iop.find( sKey::Survey() );
    if ( res && *res && SI().getDirName() != res )
	IOMan::setSurvey( res );

    PIM().loadAuto( false );

    if ( dosubmit )
    {
	uiMain app( argc, argv );
	OD::ModDeps().ensureLoaded( "uiSeis" );
	PtrMan<uiDialog> cp = new uiClusterProc( nullptr, iop );

	app.setTopLevel( cp );
	cp->setActivateOnFirstShow();
	cp->show();

	return app.exec();
    }

    OD::ModDeps().ensureLoaded( "uiSeis" );

    od_cout() << "Merging output ..." << od_endl;
    TextTaskRunner taskrunner( od_cout() );
    BufferString msg;
    const bool result =
	uiClusterProc::mergeOutput( iop, &taskrunner, msg, withdelete );
    od_cout() << msg << od_endl;

    return result ? 0 : 1;
}
