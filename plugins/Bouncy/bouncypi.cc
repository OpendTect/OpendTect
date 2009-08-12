/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : NOv 2003
-*/

static const char* rcsID = "$Id: bouncypi.cc,v 1.1 2009-08-12 09:53:01 cvskarthika Exp $";

#include "bouncymain.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"

#include "plugins.h"


mExternC mGlobal int GetBouncyPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC mGlobal PluginInfo* GetBouncyPluginInfo()
{
    static PluginInfo retpi = {
	"Bouncy thingy",
	"dGB (Karthika)",
	"4.0",
    	"Having some fun in OpendTect." };
    return &retpi;
}


class BouncyMgr :  public CallBacker
{
public:
			BouncyMgr(uiODMain*);

    uiODMain*		appl_;

    void		doWork(CallBacker*);
};


BouncyMgr::BouncyMgr( uiODMain* a )
	: appl_(a)
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    mnumgr.toolsMnu()->insertItem(
	    new uiMenuItem("&Bouncy",mCB(this,BouncyMgr,doWork)) );
}


void BouncyMgr::doWork( CallBacker* cb )
{
    BouncyMain bm( appl_ );
    bm.go();
}


mExternC mGlobal const char* InitBouncyPlugin( int, char** )
{
    static BouncyMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new BouncyMgr( ODMainWin() );
    return 0;
}
