/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Raman Singh
 * DATE     : June 2008
-*/

static const char* rcsID = "$Id: uigmtpi.cc,v 1.4 2008-08-07 12:38:39 cvsraman Exp $";

#include "uigmtcoastline.h"
#include "uigmtcontour.h"
#include "uigmtlocations.h"
#include "uigmtmainwin.h"
#include "uigmtpolyline.h"
#include "uigmtwells.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uitoolbar.h"

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
    uiGMTMainWin*	dlg_;

    void		createMap(CallBacker*);
};


uiGMTMgr::uiGMTMgr( uiODMain* a )
	: appl_(a)
	, dlg_(0)
{
    appl_->menuMgr().dtectTB()->addButton( "gmt_logo.png",
	    				  mCB(this,uiGMTMgr,createMap),
					  "Create PostScript Map" );
}


uiGMTMgr::~uiGMTMgr()
{
    delete dlg_;
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
    uiGMTContourGrp::initClass();
    uiGMTCoastlineGrp::initClass();
    uiGMTWellsGrp::initClass();
    return 0;
}
