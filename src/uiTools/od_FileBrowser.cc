/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Jan 2003
 RCS:           $Id: od_FileBrowser.cc,v 1.1 2004-03-02 13:39:00 bert Exp $
________________________________________________________________________

-*/

#include "uifilebrowser.h"
#include "uimain.h"

#include "prog.h"
#include <unistd.h>

#ifdef __win__
#include "filegen.h"
#endif

int main( int argc, char ** argv )
{
    bool inpok = argc > 1;

    int argidx = 1;
    bool editable = false;
    if ( !strcmp(argv[argidx],"--edit") )
    {
	editable = true;
	argidx++;
	inpok = argc > 2;
    }

    if ( !inpok )
    {
	cerr << "Usage: " << argv[0] << " [--edit] filename\n"
	     << "Note: filename must be with FULL path." << endl;
	exitProgram( 1 );
    }

#ifndef __win__

    switch ( fork() )
    {
    case -1:
	cerr << argv[0] << ": cannot fork: " << errno_message() << endl;
	exitProgram( 1 );
    case 0:	break;
    default:	return 0;
    }

#endif

    char* fnm = argv[argidx];
    replaceCharacter( fnm, '%', ' ' );

    uiMain app( argc, argv );

#ifdef __win__
    if ( File_isLink( url ) )
	url = File_linkTarget(url);
#endif

    uiFileBrowser* fb = new uiFileBrowser( 0,
	    		uiFileBrowser::Setup(fnm).readonly(!editable) );
    app.setTopLevel( fb );
    fb->show();
    exitProgram( app.exec() ); return 0;
}
