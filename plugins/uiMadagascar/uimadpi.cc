
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : NOv 2003
-*/

static const char* rcsID = "$Id: uimadpi.cc,v 1.1 2007-05-23 17:05:28 cvsbert Exp $";

#include "uimadagascarmain.h"
#include "uiodmenumgr.h"
#include "uimenu.h"
#include "uimsg.h"
#include "plugins.h"

extern "C" int GetuiMadagascarPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiMadagascarPluginInfo()
{
    static PluginInfo retpi = {
	"Madagascar connection",
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
    mnumgr.utilMnu()->insertItem( new uiMenuItem("&Madagascar ...",
				  mCB(this,uiMadagascarLink,doMain)) );
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
