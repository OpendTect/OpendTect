/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          April 2002
 RCS:           $Id: od_SeisMMBatch.cc,v 1.2 2002-05-08 12:17:06 bert Exp $
________________________________________________________________________

-*/

#include "uiseismmproc.h"
#include "uimain.h"

#include "prog.h"
#include "strmprov.h"
#include "strmdata.h"
#include "ioparlist.h"
#define NO_FS_TRANSL 1
#include "seisfact.h"
#ifdef __msvc__
#else
#include <unistd.h>
#endif

extern "C" { extern void SetDgbApplicationCode(int); }

int main( int argc, char ** argv )
{
    SetDgbApplicationCode( mDgbApplCodeDTECT );

    int bgadd = argc > 1 && !strcmp(argv[1],"-bg") ? 1 : 0;
	
    if ( argc+bgadd < 3 )
    {
	cerr << "Usage: " << argv[0] << " program parfile" << endl;
	return 1;
    }
    StreamProvider spin( argv[2+bgadd] );
    StreamData sdin = spin.makeIStream();
    if ( !sdin.usable() )
    {
	cerr << argv[0] << ": Cannot open parameter file" << endl;
	return 1;
    }
    IOParList parlist( *sdin.istrm );
    if ( parlist.size() == 0 || parlist[0]->size() == 0 )
    {
	cerr << argv[0] << ": Invalid parameter file" << endl;
	return 1;
    }
    sdin.close();

#ifndef __msvc__
    if ( bgadd )
    {
	switch ( fork() )
	{
	case -1:
	    cerr << argv[0] << ": cannot fork: " << errno_message() << endl;
		    return 1;
	case 0:		break;
	default:	return 0;
	}
    }
#endif 

    uiMain app( argc, argv );
    uiSeisMMProc* smmp = new uiSeisMMProc( 0, argv[1+bgadd], *parlist[0] );

    app.setTopLevel( smmp );
    smmp->show();
    return app.exec();
}
