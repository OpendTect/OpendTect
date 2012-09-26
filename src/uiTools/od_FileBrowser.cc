/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Jan 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uitextfile.h"
#include "uimain.h"

#include "prog.h"
#include <iostream>

#ifdef __win__
# include "file.h"
#endif

int main( int argc, char ** argv )
{
    int argidx = 1;
    bool edit = false, table = false, dofork = true, logview = false;
    int maxlines = mUdf(int);

    while ( argc > argidx )
    {
	if ( !strcmp(argv[argidx],"--edit") )
	    edit = true;
	else if ( !strcmp(argv[argidx],"--table") )
	    table = true;
	else if ( !strcmp(argv[argidx],"--maxlines") )
	    { argidx++; maxlines = toInt(argv[argidx]); }
	else if ( !strcmp(argv[argidx],"--nofork") )
	    dofork = false;
	else if ( !strcmp(argv[argidx],"--log") )
	    logview = true;
	else if ( !strcmp(argv[argidx],"--help") )
	{
	    std::cerr << "Usage: " << argv[0]
		<< " [--edit|--table|--log|--maxlines nrlines] [filename]\n"
		<< "Note: filename must be with FULL path." << std::endl;
	    ExitProgram( 0 );
	}
	argidx++;
    }
    argidx--;

    if ( dofork )
	forkProcess();

    BufferString fnm = argidx > 0 ? argv[argidx] : "";
    replaceCharacter( fnm.buf(), (char)128, ' ' );

    uiMain app( argc, argv );

#ifdef __win__
    if ( File::isLink( fnm ) )
	fnm = const_cast<char*>(File::linkTarget(fnm));
#endif

    uiTextFile::Setup tfsetup( !edit, table, fnm );
    tfsetup.maxlines( maxlines );
    tfsetup.logviewmode( logview );
    uiTextFileDlg::Setup fdsetup( fnm );
    fdsetup.allowopen(edit).allowsave(edit);
    uiTextFileDlg* dlg = new uiTextFileDlg( 0, tfsetup, fdsetup );
    app.setTopLevel( dlg );
    dlg->show();
    ExitProgram( app.exec() ); return 0;
}
