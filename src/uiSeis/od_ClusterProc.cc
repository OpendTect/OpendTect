/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Raman Singh
 Date:          April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: od_ClusterProc.cc,v 1.2 2009-05-25 10:16:06 cvsraman Exp $";

#include "uiclusterproc.h"
#include "uimain.h"

#include "envvars.h"
#include "filepath.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "plugins.h"
#include "prog.h"
#include "strmprov.h"
#include "strmdata.h"
#include "survinfo.h"

#include <iostream>
#ifndef __msvc__
# include <unistd.h>
#endif


int main( int argc, char ** argv )
{
    od_putProgInfo( argc, argv );

    const int bgadd = argc > 1 && !strcmp(argv[1],"-bg") ? 1 : 0;
    if ( argc+bgadd < 2 )
    {
	std::cerr << "Usage: " << argv[0] << " parfile" << std::endl;
	ExitProgram( 1 );
    }

    FilePath fp( argv[ 1 + bgadd ] );
    const BufferString parfnm( fp.fullPath() );
    StreamProvider spin( parfnm );
    StreamData sdin = spin.makeIStream();
    if ( !sdin.usable() )
    {
	std::cerr << argv[0] << ": Cannot open parameter file" << std::endl;
	ExitProgram( 1 );
    }
    IOPar iop; iop.read( *sdin.istrm, sKey::Pars );
    if ( iop.size() == 0 )
    {
	std::cerr << argv[0] << ": Invalid parameter file" << std::endl;
	ExitProgram( 1 );
    }

    sdin.close();

    const char* res = iop.find( sKey::Survey );
    if ( res && *res && SI().getDirName() != res )
	IOMan::setSurvey( res );

    PIM().setArgs( argc, argv );
    PIM().loadAuto( false );

    uiMain app( argc, argv );
    uiClusterProc* cp = new uiClusterProc( 0, iop );

    app.setTopLevel( cp );
    cp->show();

    int ret = app.exec();
    ExitProgram( ret );
    return ret;
}
