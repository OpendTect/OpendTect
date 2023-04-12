/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv, false );
    uiMain app( argc, argv );

    OD::ModDeps().ensureLoaded( "Network" );
    OD::ModDeps().ensureLoaded( "uiBase" );

    const CommandLineParser clp( argc, argv );
    BufferStringSet args;
    clp.getNormalArguments( args );

    if ( args.isEmpty() || clp.hasKey("help") || clp.hasKey("h") )
    {
	od_cout() << "Usage: " << argv[0]
		<< " filename [title]\nNote: filename has to be with FULL path."
		<< od_endl;
	return 1;
    }

    BufferString& fnm = args.get( 0 );
#ifdef __win__
    if ( File::isLink(fnm) )
	fnm = File::linkTarget( fnm );
#endif
    if ( !File::exists(fnm.buf()) )
    {
	od_cerr() << "File name does not exist." << od_endl;
	return 1;
    }

    const BufferString title = args.size() > 1 ? args.get(1).buf() : fnm.buf();

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiTools" );
    PtrMan<uiMainWin> topmw = new uiMainWin( nullptr, toUiString(title.buf()) );
    uiPixmap pm( fnm );
    PtrMan<uiGraphicsView> view = new uiGraphicsView( topmw, "Graphics Viewer");
    view->setPrefWidth( pm.width() );
    view->setPrefHeight( pm.height() );
    view->scene().addItem( new uiPixmapItem(pm) );
    app.setTopLevel( topmw );
    PIM().loadAuto( true );
    topmw->show();

    return app.exec();
}
