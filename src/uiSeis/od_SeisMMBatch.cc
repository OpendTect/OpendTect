/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2002
 RCS:           $Id: od_SeisMMBatch.cc,v 1.12 2004-01-21 13:46:25 bert Exp $
________________________________________________________________________

-*/

#include "uiseismmproc.h"
#include "uimain.h"
#include "plugins.h"

#include "prog.h"
#include "strmprov.h"
#include "strmdata.h"
#include "ioparlist.h"
#ifdef __msvc__
#else
#include <unistd.h>
#endif


int main( int argc, char ** argv )
{
    PIM().setArgs( argc, argv );
    PIM().loadAuto( false );

    int bgadd = argc > 1 && !strcmp(argv[1],"-bg") ? 1 : 0;
	
    if ( argc+bgadd < 3 )
    {
	cerr << "Usage: " << argv[0] << " program parfile" << endl;
	exitProgram( 1 );
    }
    const char* fnm = argv[ 2 + bgadd ];
    StreamProvider spin( fnm );
    StreamData sdin = spin.makeIStream();
    if ( !sdin.usable() )
    {
	cerr << argv[0] << ": Cannot open parameter file" << endl;
	exitProgram( 1 );
    }
    IOParList parlist( *sdin.istrm );
    if ( parlist.size() == 0 || parlist[0]->size() == 0 )
    {
	cerr << argv[0] << ": Invalid parameter file" << endl;
	exitProgram( 1 );
    }
    sdin.close();
    parlist.setFileName( fnm );

#ifndef __win__
    if ( bgadd )
    {
	switch ( fork() )
	{
	case -1:
	    cerr << argv[0] << ": cannot fork: " << errno_message() << endl;
		    exitProgram( 1 );
	case 0:		break;
	default:	return 0;
	}
    }
#endif 

    uiMain app( argc, argv );
    uiSeisMMProc* smmp = new uiSeisMMProc( 0, argv[1+bgadd], parlist );

    app.setTopLevel( smmp );
    smmp->show();

    exitProgram( app.exec() ); return 0;
}
