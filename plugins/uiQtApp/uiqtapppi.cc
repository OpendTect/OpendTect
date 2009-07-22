
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2009
-*/

static const char* rcsID = "$Id: uiqtapppi.cc,v 1.2 2009-07-22 16:01:28 cvsbert Exp $";

#include "qtclss.h"

#include "uimenu.h"
#include "uiodmenumgr.h"

#include "plugins.h"


extern "C" int GetuiQtAppPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiQtAppPluginInfo()
{
    static PluginInfo retpi = {
	"QT Application plugin",
	"dGB (Bert)",
	"0.001",
    	"Example of how to call a Qt something from a plugin." };
    return &retpi;
}


class uiQtAppMgr :  public CallBacker
{
public:
			uiQtAppMgr(uiODMain*);

    uiODMain*		appl_;
    QtClss*		qtclss_;

    void		doStuff(CallBacker*);
};


uiQtAppMgr::uiQtAppMgr( uiODMain* a )
    : appl_(a)
    , qtclss_(0)
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    mnumgr.utilMnu()->insertItem(
	    new uiMenuItem( "&Qt Thing", mCB(this,uiQtAppMgr,doStuff) ) );
}


void uiQtAppMgr::doStuff( CallBacker* )
{
    if ( qtclss_ ) delete qtclss_;
    qtclss_ = new QtClss( appl_->qWidget() );
    qtclss_->go();
}


extern "C" const char* InituiQtAppPlugin( int, char** )
{
    static uiQtAppMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiQtAppMgr( ODMainWin() );
    return 0;
}
