
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : NOv 2003
-*/

static const char* rcsID = "$Id: uimadpi.cc,v 1.2 2007-06-29 11:58:53 cvsbert Exp $";

#include "uimadagascarmain.h"
#include "uiodmenumgr.h"
#include "uimenu.h"
#include "uitoolbar.h"
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
			uiMadagascarLink(uiODMain*);

    uiODMain*		appl;

    void		doMain(CallBacker*);
};


uiMadagascarLink::uiMadagascarLink( uiODMain* a )
	: appl(a)
{
    uiODMenuMgr& mnumgr = appl->menuMgr();
    const CallBack cb( mCB(this,uiMadagascarLink,doMain) );
    mnumgr.utilMnu()->insertItem( new uiMenuItem("&Madagascar ...",cb) );
    mnumgr.dtectTB()->addButton( "madagascar.png", cb, "Madagascar link" );
}


void uiMadagascarLink::doMain( CallBacker* )
{
    uiMadagascarMain dlg( appl );
    dlg.go();
}


extern "C" const char* InituiMadagascarPlugin( int, char** )
{
    static uiMadagascarLink* lnk = 0; if ( lnk ) return 0;
    lnk = new uiMadagascarLink( ODMainWin() );
    return 0;
}
