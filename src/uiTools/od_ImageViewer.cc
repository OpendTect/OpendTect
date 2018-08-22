/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2003
________________________________________________________________________

-*/

#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uipixmap.h"

#include "commandlineparser.h"
#include "file.h"
#include "moddepmgr.h"
#include "prog.h"


int main( int argc, char** argv )
{
    OD::SetRunContext( OD::UiProgCtxt );
    SetProgramArgs( argc, argv );
    uiMain app;

    OD::ModDeps().ensureLoaded( "uiTools" );

    auto& clp = app.commandLineParser();
    BufferStringSet args;
    clp.getNormalArguments( args );

    if ( args.isEmpty() || clp.hasKey("help") || clp.hasKey("h") )
    {
	od_cout() << "Usage: " << argv[0]
		<< " filename [title]\nNote: filename has to be with FULL path."
		<< od_endl;
	return ExitProgram( 0 );
    }

    if ( clp.hasKey("bg") )
	ForkProcess();

    BufferString& fnm = args.get( 0 );
#ifdef __win__
    fnm = File::linkEnd( fnm );
#endif
    if ( !File::exists(fnm.buf()) )
    {
	od_cerr() << "File name does not exist." << od_endl;
	return ExitProgram( 0 );
    }

    const BufferString title = args.size() > 1 ? args.get(1).buf() : fnm.buf();

    uiMainWin* mw = new uiMainWin( 0, toUiString(title.buf()) );
    uiPixmap pm( fnm );
    uiGraphicsView* view = new uiGraphicsView( mw, "Graphics Viewer" );
    view->setPrefWidth( pm.width() );
    view->setPrefHeight( pm.height() );
    view->scene().addItem( new uiPixmapItem(pm) );
    app.setTopLevel( mw );
    mw->show();

    const int ret = app.exec();
    delete mw;
    return ExitProgram( ret );
}
