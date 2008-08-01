/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Raman Singh
 * DATE     : June 2008
-*/

static const char* rcsID = "$Id: uigmtpi.cc,v 1.2 2008-08-01 08:31:21 cvsraman Exp $";

#include "uigmtlocations.h"
#include "uigmtmainwin.h"
#include "uigmtplotter.h"
#include "uigmtpolyline.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "vishorizondisplay.h"
#include "uitoolbar.h"
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
			~uiGMTMgr();

    uiODMain*		appl_;
    uiVisMenuItemHandler	mnuitmhandler_;
    uiGMTMainWin*	dlg_;

    void 		insertSubMenu();

    void		doWork(CallBacker*);
    void		createMap(CallBacker*);
};


uiGMTMgr::uiGMTMgr( uiODMain* a )
	: appl_(a)
	, dlg_(0)
	, mnuitmhandler_(visSurvey::HorizonDisplay::getStaticClassName(),
		  	 *a->applMgr().visServer(),"Create Contour &Map ...",
			 mCB(this,uiGMTMgr,doWork))
{
    appl_->menuMgr().coinTB()->addButton( "gmt_logo.png",
	    				  mCB(this,uiGMTMgr,createMap),
					  "Create PostScript Map" );
}


uiGMTMgr::~uiGMTMgr()
{
    delete dlg_;
}


void uiGMTMgr::doWork( CallBacker* )
{
    const int displayid = mnuitmhandler_.getDisplayID();
    uiGMTPlotter dlg( appl_, displayid );
    dlg.go();
}


void uiGMTMgr::createMap( CallBacker* )
{
    if ( !dlg_ )
	dlg_ = new uiGMTMainWin( 0 );

    dlg_->show();
}


extern "C" const char* InituiGMTPlugin( int, char** )
{
    static uiGMTMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiGMTMgr( ODMainWin() );

    uiGMTLocationsGrp::initClass();
    uiGMTPolylineGrp::initClass();

    return 0;
}
