/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "prog.h"

#include "uimain.h"
#include "uiseismmproc.h"

#include "filepath.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "plugins.h"
#include "strmprov.h"
#include "survinfo.h"
#include "od_istream.h"


int main( int argc, char ** argv )
{
    SetProgramArgs( argc, argv );

    OD::ModDeps().ensureLoaded( "uiSeis" );
    const FixedString arg1( argv[1] );
    const int bgadd = arg1 == "-bg" ? 1 : 0;
    if ( argc+bgadd < 3 )
    {
	od_cout() << "Usage: " << argv[0] << " program parfile" << od_endl;
	return ExitProgram( 1 );
    }

    FilePath fp( argv[ 2 + bgadd ] );
    const BufferString parfnm( fp.fullPath() );
    od_istream strm( parfnm );
    if ( !strm.isOK() )
    {
	od_cout() << argv[0] << ": Cannot open parameter file" << od_endl;
	return ExitProgram( 1 );
    }
    IOPar iop; iop.read( strm, sKey::Pars() );
    if ( iop.size() == 0 )
    {
	od_cout() << argv[0] << ": Invalid parameter file" << od_endl;
	return ExitProgram( 1 );
    }
    strm.close();

    if ( bgadd )
	ForkProcess();

    const char* res = iop.find( sKey::Survey() );
    if ( res && *res && SI().getDirName() != res )
	IOMan::setSurvey( res );

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "Seis" );

    uiMain app( argc, argv );
    uiSeisMMProc* smmp = new uiSeisMMProc( 0, iop, argv[1+bgadd], parfnm );

    app.setTopLevel( smmp );
    smmp->show();

    return ExitProgram( app.exec() );
}
