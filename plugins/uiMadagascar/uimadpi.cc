
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : NOv 2003
-*/

static const char* rcsID = "$Id: uimadpi.cc,v 1.6 2007-12-18 16:21:00 cvsbert Exp $";

#include "uimadagascarmain.h"
#include "uiodmenumgr.h"
#include "uimenu.h"
#include "uitoolbar.h"
#include "maddefs.h"
#include "uimsg.h"
#include "plugins.h"

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
    uiMadagascarMain dlg( &appl );
    dlg.go();
}


extern "C" const char* InituiMadagascarPlugin( int, char** )
{
    static uiMadagascarLink* lnk = 0;
    if ( lnk ) return 0;

#ifdef MAD_UIMSG_IF_FAIL
    if ( !ODMad::PI().errMsg().isEmpty() )
	uiMSG().error( ODMad::PI().errMsg() );
#endif

    lnk = new uiMadagascarLink( *ODMainWin() );
    return lnk ? 0 : ODMad::PI().errMsg().buf();
}
