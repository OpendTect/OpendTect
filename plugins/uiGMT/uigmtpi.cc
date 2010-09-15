/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : June 2008
-*/

static const char* rcsID = "$Id: uigmtpi.cc,v 1.30 2010-09-15 12:06:09 cvsnageswara Exp $";

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "gmtdef.h"
#include "ioman.h"
#include "pixmap.h"
#include "uibutton.h"
#include "uidesktopservices.h"
#include "uigmtadv.h"
#include "uigmtarray2dinterpol.h"
#include "uigmtcoastline.h"
#include "uigmtcontour.h"
#include "uigmtfaults.h"
#include "uigmtlocations.h"
#include "uigmtmainwin.h"
#include "uigmtpolyline.h"
#include "uigmtrandlines.h"
#include "uigmtwells.h"
#include "uigmt2dlines.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uitoolbar.h"

#include "plugins.h"


mExternC int GetuiGMTPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetuiGMTPluginInfo()
{
    static PluginInfo retpi = {
	"GMT link",
	"dGB (Raman)",
	"3.2",
    	"Plots Surface data using GMT mapping tool" };
    return &retpi;
}


class uiGMTIntro : public uiDialog
{
public:

uiGMTIntro( uiParent* p )
    : uiDialog(p,uiDialog::Setup("GMT Mapping Tool",mNoDlgTitle,mNoHelpID))
{
    setOkText( "Continue" );

    BufferString msg = "You need to install the GMT mapping tool package\n";
    msg += "before you can use this utility\n";
    msg += "Also make sure that the environment variable GMTROOT is set ";
    msg += "to the GMT installation directory\n and your PATH variable ";
    msg += "includes the GMT bin directory";

    uiLabel* lbl = new uiLabel( this, msg );
    lbl->setAlignment( Alignment::HCenter );

    uiPushButton* gmtbut = new uiPushButton( this, "Download GMT",
					     mCB(this,uiGMTIntro,gmtPush),
					     true );
    gmtbut->setToolTip( "Click to go to the Download centre" );
    gmtbut->attach( centeredBelow, lbl );
}

protected:

void gmtPush( CallBacker* )
{
    uiDesktopServices::openUrl(
	    __islinux__ ? "http://www.opendtect.org/index.php/download.html"
	    		: "http://www.soest.hawaii.edu/gmt" );
}

bool acceptOK( CallBacker* )
{
    return true;
}

};


class uiGMTMgr :  public CallBacker
{
public:
			uiGMTMgr(uiODMain*);
			~uiGMTMgr();

    uiODMain*		appl_;
    uiGMTMainWin*	dlg_;

    void		updateToolBar(CallBacker*);
    void		updateMenu(CallBacker*);
    void		createMap(CallBacker*);
};


uiGMTMgr::uiGMTMgr( uiODMain* a )
	: appl_(a)
	, dlg_(0)
{
    appl_->menuMgr().dTectTBChanged.notify( mCB(this,uiGMTMgr,updateToolBar) );
    appl_->menuMgr().dTectMnuChanged.notify( mCB(this,uiGMTMgr,updateMenu) );
    updateToolBar(0);
    updateMenu(0);
}


uiGMTMgr::~uiGMTMgr()
{
    delete dlg_;
}


void uiGMTMgr::updateToolBar( CallBacker* )
{
    appl_->menuMgr().dtectTB()->addButton( "gmt_logo.png",
	    				   mCB(this,uiGMTMgr,createMap),
					   "GMT Mapping Tool" );
}


void uiGMTMgr::updateMenu( CallBacker* )
{
    delete dlg_; dlg_ = 0;
    const ioPixmap gmtpm( "gmt_logo.png" );
    uiMenuItem* newitem = new uiMenuItem( "GMT Mapping Tool ...",
	    				  mCB(this,uiGMTMgr,createMap),
	   				  &gmtpm );
    appl_->menuMgr().procMnu()->insertItem( newitem );
}


void uiGMTMgr::createMap( CallBacker* )
{
    if ( !dlg_ )
    {
	FilePath gmtroot( GetEnvVar("GMTROOT") );
	if ( !File::isDirectory(gmtroot.fullPath()) )
	{
	    uiGMTIntro introdlg( appl_ );
	    if ( !introdlg.go() )
		return;
	}

	dlg_ = new uiGMTMainWin( 0 );
    }

    dlg_->show();
    dlg_->raise();
}


mExternC const char* InituiGMTPlugin( int, char** )
{
    static uiGMTMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiGMTMgr( ODMainWin() );

    IOMan::CustomDirData cdd( ODGMT::sKeyGMTSelKey, ODGMT::sKeyGMT,
	    		      "GMT data" );
    MultiID id = IOMan::addCustomDataDir( cdd );
    if ( id != ODGMT::sKeyGMTSelKey )
	return "Cannot create 'GMT' directory in survey";

    uiGMTContourGrp::initClass();
    uiGMTFaultsGrp::initClass();
    uiGMTWellsGrp::initClass();
    uiGMTLocationsGrp::initClass();
    uiGMTPolylineGrp::initClass();
    uiGMTRandLinesGrp::initClass();
    uiGMT2DLinesGrp::initClass();
    uiGMTCoastlineGrp::initClass();
    uiGMTAdvGrp::initClass();
    uiGMTSurfaceGrid::initClass();
    uiGMTNearNeighborGrid::initClass();

    return 0;
}
