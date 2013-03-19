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
#include "strmdata.h"
#include "survinfo.h"

#include <iostream>


int main( int argc, char ** argv )
{
    SetProgramArgs( argc, argv );

    const int bgadd = argc > 1 && !strcmp(argv[1],"-bg") ? 1 : 0;
    if ( argc+bgadd < 3 )
    {
	std::cerr << "Usage: " << argv[0] << " program parfile" << std::endl;
	ExitProgram( 1 );
    }

    FilePath fp( argv[ 2 + bgadd ] );
    const BufferString parfnm( fp.fullPath() );
    StreamProvider spin( parfnm );
    StreamData sdin = spin.makeIStream();
    if ( !sdin.usable() )
    {
	std::cerr << argv[0] << ": Cannot open parameter file" << std::endl;
	ExitProgram( 1 );
    }
    IOPar iop; iop.read( *sdin.istrm, sKey::Pars() );
    if ( iop.size() == 0 )
    {
	std::cerr << argv[0] << ": Invalid parameter file" << std::endl;
	ExitProgram( 1 );
    }
    sdin.close();

    if ( bgadd )
	forkProcess();

    const char* res = iop.find( sKey::Survey() );
    if ( res && *res && SI().getDirName() != res )
	IOMan::setSurvey( res );

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "Seis" );

    uiMain app( argc, argv );
    uiSeisMMProc* smmp = new uiSeisMMProc( 0, iop, argv[1+bgadd], parfnm );

    app.setTopLevel( smmp );
    smmp->show();

    ExitProgram( app.exec() ); return 0;
}
