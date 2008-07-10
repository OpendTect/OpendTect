/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Raman Singh
 * DATE     : June 2008
-*/

static const char* rcsID = "$Id: uigmtpi.cc,v 1.1 2008-07-10 11:10:58 cvsraman Exp $";

#include "uigmtplotter.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "vishorizondisplay.h"
#include "uivismenuitemhandler.h"
#include "uivispartserv.h"

#include "plugins.h"

extern "C" int GetuiGMTPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiGMTPluginInfo()
{
    static PluginInfo retpi = {
	"GMT link",
	"dGB (Raman)",
	"3.2",
    	"Plots Surface data using GMT mapping tool" };
    return &retpi;
}


class uiGMTMgr :  public CallBacker
{
public:
			uiGMTMgr(uiODMain*);

    uiODMain*		appl_;
    uiVisMenuItemHandler	mnuitmhandler_;

    void 		insertSubMenu();

    void		doWork(CallBacker*);
};


uiGMTMgr::uiGMTMgr( uiODMain* a )
	: appl_(a)
	, mnuitmhandler_(visSurvey::HorizonDisplay::getStaticClassName(),
		  	 *a->applMgr().visServer(),"Create Contour &Map ...",
			 mCB(this,uiGMTMgr,doWork))
{}


void uiGMTMgr::doWork( CallBacker* )
{
    const int displayid = mnuitmhandler_.getDisplayID();
    uiGMTPlotter dlg( appl_, displayid );
    dlg.go();
}


extern "C" const char* InituiGMTPlugin( int, char** )
{
    static uiGMTMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiGMTMgr( ODMainWin() );

    return 0;
}
