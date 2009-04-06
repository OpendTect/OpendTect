/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Raman Singh
 * DATE     : June 2008
-*/

static const char* rcsID = "$Id: uigmtpi.cc,v 1.19 2009-04-06 07:29:08 cvsranojay Exp $";

#include "gmtdef.h"
#include "ioman.h"
#include "pixmap.h"
#include "uibutton.h"
#include "uidesktopservices.h"
#include "uigmtadv.h"
#include "uigmtcoastline.h"
#include "uigmtcontour.h"
#include "uigmtlocations.h"
#include "uigmtmainwin.h"
#include "uigmtpolyline.h"
#include "uigmtwells.h"
#include "uigmt2dlines.h"
#include "uigmtrandlines.h"
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
    msg += "Also make sure that your PATH variable includes\n";
    msg += "the GMT bin directory";

    uiLabel* lbl = new uiLabel( this, msg );
    lbl->setAlignment( Alignment::HCenter );

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


void uiGMTMgr::updateToolBar( CallBacker* )
{
    appl_->menuMgr().dtectTB()->addButton( "gmt_logo.png",
	    				   mCB(this,uiGMTMgr,createMap),
					   "GMT Mapping Tool" );
}


void uiGMTMgr::updateMenu( CallBacker* )
{
    delete dlg_; dlg_ = 0;
    uiMenuItem* newitem = new uiMenuItem( "GMT Mapping Tool ...",
	    				  mCB(this,uiGMTMgr,createMap) );
    appl_->menuMgr().procMnu()->insertItem( newitem );
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
    uiGMTWellsGrp::initClass();
    uiGMTLocationsGrp::initClass();
    uiGMTPolylineGrp::initClass();
    uiGMTRandLinesGrp::initClass();
    uiGMT2DLinesGrp::initClass();
    uiGMTCoastlineGrp::initClass();
    uiGMTAdvGrp::initClass();
    return 0;
}
