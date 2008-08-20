/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Raman Singh
 * DATE     : June 2008
-*/

static const char* rcsID = "$Id: uigmtpi.cc,v 1.7 2008-08-20 08:37:51 cvsraman Exp $";

#include "gmtdef.h"
#include "pixmap.h"
#include "uibutton.h"
#include "uidesktopservices.h"
#include "uigmtcoastline.h"
#include "uigmtcontour.h"
#include "uigmtlocations.h"
#include "uigmtmainwin.h"
#include "uigmtpolyline.h"
#include "uigmtwells.h"
#include "uigmt2dlines.h"
#include "uilabel.h"
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


class uiGMTIntro : public uiDialog
{
public:

uiGMTIntro( uiParent* p )
    : uiDialog(p,uiDialog::Setup("GMT Mapping Tool","",""))
{
    setOkText( "Continue" );

    BufferString msg = "You need to install the GMT mapping tool package\n";
    msg += "before you can use this utility\n";
    msg += "Also make sure that your PATH variable includes\n";
    msg += "the GMT bin directory";

    uiLabel* lbl = new uiLabel( this, msg );
    lbl->setAlignment( uiLabel::AlignHCenter );

    uiToolButton* gmtbut = new uiToolButton( this, "GMT Home",
	    				     ioPixmap("gmt_logo.png"),
					     mCB(this,uiGMTIntro,gmtPush) );
    gmtbut->setToolTip( "Go to GMT Home page" );
    gmtbut->attach( centeredBelow, lbl );

    skipfld_ = new uiCheckBox( this, "Don't show this message again" );
    skipfld_->attach( centeredBelow, gmtbut );
}

protected:

void gmtPush( CallBacker* )
{
    uiDesktopServices::openUrl( "http://www.soest.hawaii.edu/gmt" );
}

bool acceptOK( CallBacker* )
{
    mSetDefault( ODGMT::sKeySkipWarning, setYN, skipfld_->isChecked() );
    return true;
}

    uiCheckBox*		skipfld_;
};


class uiGMTMgr :  public CallBacker
{
public:
			uiGMTMgr(uiODMain*);
			~uiGMTMgr();

    uiODMain*		appl_;
    uiGMTMainWin*	dlg_;

    void		updateMenu(CallBacker* cb=0);
    void		createMap(CallBacker*);
};


uiGMTMgr::uiGMTMgr( uiODMain* a )
	: appl_(a)
	, dlg_(0)
{
    appl_->menuMgr().dTectTBChanged.notify( mCB(this,uiGMTMgr,updateMenu) );
    updateMenu();
}


void uiGMTMgr::updateMenu( CallBacker* )
{
    delete dlg_; dlg_ = 0;
    appl_->menuMgr().dtectTB()->addButton( "gmt_logo.png",
	    				  mCB(this,uiGMTMgr,createMap),
					  "GMT Mapping Tool" );
}


uiGMTMgr::~uiGMTMgr()
{
    delete dlg_;
}


void uiGMTMgr::createMap( CallBacker* )
{
    if ( !dlg_ )
    {
	bool skipwarning = false;
	mGetDefault( ODGMT::sKeySkipWarning, getYN, skipwarning );
	if ( !skipwarning )
	{
	    uiGMTIntro introdlg( appl_ );
	    if ( !introdlg.go() )
		return;
	}

	dlg_ = new uiGMTMainWin( 0 );
    }

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
    uiGMT2DLinesGrp::initClass();
    return 0;
}
