/*+
________________________________________________________________________

 Copyright:	(C) 1995-2025 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitutmod.h"

#include "uidialog.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimsg.h"
#include "uisurvey.h"

#include "applicationdata.h"
#include "commandlineparser.h"
#include "ioman.h"
#include "moddepmgr.h"
#include "oddirs.h"
#include "prog.h"

// make an xpm icon file defining a tutorialapp_xpm_data const char* function
#include "odlogo64x64.xpm"

class uiTutAppDlg : public uiDialog
{
public:
    uiTutAppDlg( uiParent* p )
	: uiDialog(p,Setup(toUiString("Tutorial application"),
			   mNoHelpKey))
    {
	setCtrlStyle( CloseOnly );
	new uiLabel( this, toUiString("Hello World!") );
    }

    ~uiTutAppDlg()
    {
	detachAllNotifiers();
    }
};


static bool loadOpendTectPlugins( const char* piname )
{
    BufferString libnm; libnm.setMinBufSize( 32 );
    SharedLibAccess::getLibName( piname, libnm.getCStr(), libnm.bufSize() );
    const FilePath libfp( GetLibPlfDir(), libnm );
    return libfp.exists()
	? PIM().load( libfp.fullPath(), PluginManager::Data::AppDir,
	    PI_AUTO_INIT_EARLY )
	: false;
}


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::RunCtxt::StandAloneCtxt );
    const bool needprojectdata = true;
    SetProgramArgs( argc, argv, needprojectdata );
    uiMain::setXpmIconData( od_logo_64x64 );
    uiMain app( argc, argv );
    app.setIcon( "opendtect" );
    ApplicationData::setApplicationName( "OpendTect - Tutorial app" );

    OD::ModDeps().ensureLoaded( "uiIo" );

    const CommandLineParser parser( argc, argv );
    if ( needprojectdata )
    {
	uiRetVal uirv = IOMan::setDataSource( parser );
	mIfIOMNotOK( return 1 )
    }

    /* Plugins from this (Tutorial) project are loaded from the alo files
	using PIM().loadAuto, plugins from the OpendTect SDK must be
	loaded explicitely using a function like loadOpendTectPlugins */
    PIM().loadAuto( false );
    loadOpendTectPlugins( "ODHDF5" );
    OD::ModDeps().ensureLoaded( "uiWellAttrib" );
    PtrMan<uiDialog> topdlg = new uiTutAppDlg( nullptr );
    app.setTopLevel( topdlg.ptr() );
    PIM().loadAuto( true );
    topdlg->show();

    return app.exec();
}
