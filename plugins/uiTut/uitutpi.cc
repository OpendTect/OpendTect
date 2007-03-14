
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : NOv 2003
-*/

static const char* rcsID = "$Id: uitutpi.cc,v 1.2 2007-03-14 09:00:47 cvsraman Exp $";

#include "uitutseistools.h"
#include "uiodmenumgr.h"
#include "uimenu.h"
#include "uimsg.h"
#include "plugins.h"

extern "C" int GetuiTutPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiTutPluginInfo()
{
    static PluginInfo retpi = {
	"Tutorial plugin development",
	"dGB (Raman)",
	"3.0",
    	"Shows some simple plugin basics using a seismic example." };
    return &retpi;
}


class uiTutMgr :  public CallBacker
{
public:
			uiTutMgr(uiODMain*);

    uiODMain*		appl;

    void		doSeis(CallBacker*);
    void		doHor(CallBacker*);
};


uiTutMgr::uiTutMgr( uiODMain* a )
	: appl(a)
{
    uiODMenuMgr& mnumgr = appl->menuMgr();
    uiPopupMenu* mnu = new uiPopupMenu( appl, "&Tut Tools" );
    mnu->insertItem( new uiMenuItem("&Seismic ...",
			mCB(this,uiTutMgr,doSeis)) );
    mnu->insertItem( new uiMenuItem("&Horizon ...",
			mCB(this,uiTutMgr,doHor)) );
    mnumgr.utilMnu()->insertItem( mnu );
}


void uiTutMgr::doSeis( CallBacker* )
{
    uiTutSeisTools dlg( appl );
    dlg.go();
}


void uiTutMgr::doHor( CallBacker* )
{
    uiMSG().message( "Horizontools not yet implemented" );
}


extern "C" const char* InituiTutPlugin( int, char** )
{
    static uiTutMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiTutMgr( ODMainWin() );
    return 0;
}
