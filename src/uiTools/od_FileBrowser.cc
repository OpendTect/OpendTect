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

#ifdef __win__
# include "file.h"
#endif

int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );

    int argidx = 1;
    bool edit = false, table = false, dofork = true, logview = false;
    int maxlines = mUdf(int);

    while ( argc > argidx )
    {
	const FixedString arg( argv[argidx]+2 );
#define mArgIs(s) arg == #s
	if ( mArgIs(edit) )
	    edit = true;
	else if ( mArgIs(table) )
	    table = true;
	else if ( mArgIs(maxlines) )
	    { argidx++; maxlines = toInt(argv[argidx]); }
	else if ( mArgIs(nofork) || mArgIs(fg) )
	    dofork = false;
	else if ( mArgIs(log) )
	    logview = true;
	else if ( mArgIs(h) || mArgIs(help) )
	{
	    od_cout() << "Usage: " << argv[0]
		<< " [--edit|--table|--log|--maxlines nrlines] [filename]\n"
		<< "Note: filename must be with FULL path." << od_endl;
	    return ExitProgram( 0 );
	}
	argidx++;
    }
    argidx--;

    if ( dofork )
	ForkProcess();

    BufferString fnm = argidx > 0 ? argv[argidx] : "";

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

    return ExitProgram( app.exec() );
}
