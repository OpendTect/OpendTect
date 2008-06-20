
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uimadpi.cc,v 1.8 2008-06-20 11:41:41 cvsraman Exp $";

#include "uimadagascarmain.h"
#include "uiodmenumgr.h"
#include "uimenu.h"
#include "uitoolbar.h"
#include "envvars.h"
#include "filegen.h"
#include "filepath.h"
#include "separstr.h"
#include "maddefs.h"
#include "madio.h"
#include "uimsg.h"
#include "plugins.h"
#include "ioman.h"

extern "C" int GetuiMadagascarPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiMadagascarPluginInfo()
{
    static PluginInfo retpi = {
	"Madagascar link",
	"dGB (Bert)",
	"3.0",
    	"Enables the Madagascar link." };
    return &retpi;
}


bool checkEnvVars( BufferString& msg )
{
    BufferString rsfdir = GetEnvVar( "RSFROOT" );
    if ( rsfdir.isEmpty() || !File_isDirectory(rsfdir.buf()) )
    {
	msg = "RSFROOT is either not set or invalid";
	return false;
    }

    FilePath fp( rsfdir );
    fp.add( "bin" );
    rsfdir = fp.fullPath();
    BufferString pathvar = GetEnvVar( "PATH" );
    if ( pathvar.isEmpty() )
    {
	msg = "Could not retrieve environment variable PATH";
	return false;
    }

    SeparString pathstr( pathvar, SeparString::getPathSepChar() );
    const int nrpaths = pathstr.size();
    for ( int idx=0; idx<pathstr.size(); idx++ )
    {
	if ( rsfdir == pathstr[idx] )
	    return true;
    }

    msg = "RSF bin directory is not included in PATH. ";
    msg += "Please add $RSFROOT/bin to your PATH variable and try again.";
    return false;
}


class uiMadagascarLink :  public CallBacker
{
public:
			uiMadagascarLink(uiODMain&);

    uiODMain&		appl;
    uiODMenuMgr&	mnumgr;

    void		doMain(CallBacker*);
    void		insertItems(CallBacker* cb=0);

};


uiMadagascarLink::uiMadagascarLink( uiODMain& a )
	: appl(a)
    	, mnumgr(a.menuMgr())
{
    mnumgr.dTectTBChanged.notify( mCB(this,uiMadagascarLink,insertItems) );
    insertItems();
}


void uiMadagascarLink::insertItems( CallBacker* )
{
    const CallBack cb( mCB(this,uiMadagascarLink,doMain) );
    mnumgr.procMnu()->insertItem( new uiMenuItem("&Madagascar ...",cb) );
    mnumgr.dtectTB()->addButton( "madagascar.png", cb, "Madagascar link" );
}


void uiMadagascarLink::doMain( CallBacker* )
{
    BufferString errmsg;
    if ( !checkEnvVars(errmsg) )
    {
	uiMSG().error( errmsg );
	return;
    }

    uiMadagascarMain dlg( &appl );
    dlg.go();
}


extern "C" const char* InituiMadagascarPlugin( int, char** )
{
    static uiMadagascarLink* lnk = 0;
    if ( lnk ) return 0;

    IOMan::CustomDirData cdd( ODMad::sKeyMadSelKey, ODMad::sKeyMadagascar,
	    		      "Madagascar data" );
    MultiID id = IOMan::addCustomDataDir( cdd );
    if ( id != ODMad::sKeyMadSelKey )
	return "Cannot create 'Madagascar' directory in survey";

#ifdef MAD_UIMSG_IF_FAIL
    if ( !ODMad::PI().errMsg().isEmpty() )
	uiMSG().error( ODMad::PI().errMsg() );
#endif

    lnk = new uiMadagascarLink( *ODMainWin() );
    return lnk ? 0 : ODMad::PI().errMsg().buf();
}
