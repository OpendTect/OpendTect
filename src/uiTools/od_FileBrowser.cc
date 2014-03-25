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
    File::ViewPars vp;
    bool dofork = true;

    while ( argc > argidx )
    {
	const FixedString arg( argv[argidx]+2 );
#define mArgIs(s) arg == #s
	if ( mArgIs(edit) )
	    vp.editable_ = true;
	else if ( mArgIs(maxlines) )
	    { argidx++; vp.maxnrlines_ = toInt(argv[argidx]); }
	else if ( mArgIs(style) )
	{
	    argidx++; const BufferString stl( argv[argidx] );
	    if ( stl == "table" )
		vp.style_ = File::Table;
	    else if ( stl == "log" )
		vp.style_ = File::Log;
	    else if ( stl == "bin" )
		vp.style_ = File::Bin;
	}
	else if ( mArgIs(nofork) || mArgIs(fg) )
	    dofork = false;
	else if ( mArgIs(h) || mArgIs(help) )
	{
	    od_cout() << "Usage: " << argv[0]
		<< " [--readonly|--maxlines nrlines|--style table|log|bin]"
		   " [filename]\nNote: filename has to be with FULL path."
		<< od_endl;
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

    uiTextFileDlg::Setup fdsetup( fnm );
    fdsetup.allowopen( vp.editable_ ).allowsave( true );
    uiTextFileDlg* dlg = new uiTextFileDlg( 0, vp, fdsetup, fnm );
    dlg->showAlwaysOnTop();
    app.setTopLevel( dlg );
    dlg->show();

    return ExitProgram( app.exec() );
}
