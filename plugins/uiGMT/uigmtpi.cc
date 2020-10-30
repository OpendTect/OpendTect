/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : June 2008
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uigmtmod.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "gmtdef.h"
#include "initgmtplugin.h"
#include "ioman.h"
#include "uibutton.h"
#include "uidesktopservices.h"
#include "uigmtadv.h"
#include "uigmtarray2dinterpol.h"
#include "uigmtclip.h"
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

#include "odplugin.h"


mDefODPluginInfo(uiGMT)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"GMT Link (GUI)",
	"OpendTect",
	"dGB (Raman)",
	"3.2",
	"A link to the GMT mapping tool."
	    "\nThis is the User interface of the link."
	    "\nSee http://opendtect.org/links/gmt.html for info on GMT" ));
    return &retpi;
}


class uiGMTIntro : public uiDialog
{ mODTextTranslationClass(uiGMTIntro);
public:

uiGMTIntro( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("GMT Mapping Tool"),mNoDlgTitle,mNoHelpKey))
{
    setOkText( uiStrings::sContinue() );

    uiString msg = tr("You need to install the GMT mapping tool package\n"
			"before you can use this utility\nAlso make sure that "
			"the environment variable GMT5_SHAREDIR is set "
			"to the GMT installation 'share' directory\n"
		       "and your PATH variable includes the GMT bin directory");

    uiLabel* lbl = new uiLabel( this, msg );
    lbl->setAlignment( Alignment::HCenter );

    uiPushButton* gmtbut = new uiPushButton( this, tr("Download GMT"),
					     mCB(this,uiGMTIntro,gmtPush),
					     true );
    gmtbut->setToolTip( tr("Click to go to the Download center") );
    gmtbut->attach( centeredBelow, lbl );
}

protected:

void gmtPush( CallBacker* )
{
    uiDesktopServices::openUrl( __islinux__
	? "https://dgbes.com/index.php/download"
	: "http://gmt.soest.hawaii.edu/projects/gmt/wiki/Download" );
}

bool acceptOK( CallBacker* )
{
    return true;
}

};


class uiGMTMgr :  public CallBacker
{ mODTextTranslationClass(uiGMTMgr);
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
    mAttachCB( appl_->menuMgr().dTectTBChanged, uiGMTMgr::updateToolBar );
    mAttachCB( appl_->menuMgr().dTectMnuChanged, uiGMTMgr::updateMenu );
    updateToolBar(0);
    updateMenu(0);
}


uiGMTMgr::~uiGMTMgr()
{
    detachAllNotifiers();
}


void uiGMTMgr::updateToolBar( CallBacker* )
{
}


void uiGMTMgr::updateMenu( CallBacker* )
{
    deleteAndZeroPtr( dlg_ );
    auto* act = new uiAction( m3Dots(tr("GMT Mapping Tool")),
			      mCB(this,uiGMTMgr,createMap), "gmt_logo" );
    appl_->menuMgr().procMnu()->insertAction( act );
}


void uiGMTMgr::createMap( CallBacker* )
{
    if ( !dlg_ )
    {
	const char* gmtsharedir = GetEnvVar("GMT5_SHAREDIR");
	if ( !gmtsharedir )
	    gmtsharedir = GetEnvVar("GMT_SHAREDIR");

	if ( !GMT::hasGMT() || !gmtsharedir )
	{
	    uiGMTIntro introdlg( appl_ );
	    if ( !introdlg.go() )
		return;
	}

	dlg_ = new uiGMTMainWin( appl_ );
    }

    dlg_->show();
    dlg_->raise();
}


mDefODInitPlugin(uiGMT)
{
    mDefineStaticLocalObject( PtrMan<uiGMTMgr>, theinst_, = 0 );
    if ( theinst_ ) return 0;

    theinst_ = new uiGMTMgr( ODMainWin() );
    if ( !theinst_ )
	return "Cannot instantiate GMT plugin";

    IOMan::CustomDirData cdd( ODGMT::sKeyGMTSelKey(), ODGMT::sKeyGMT(),
			      "GMT data" );
    MultiID id = IOMan::addCustomDataDir( cdd );
    if ( id != ODGMT::sKeyGMTSelKey() )
	return "Cannot create 'GMT' directory in survey";

    uiGMTContourGrp::initClass();
    uiGMTFaultsGrp::initClass();
    uiGMTWellsGrp::initClass();
    uiGMTLocationsGrp::initClass();
    uiGMTPolylineGrp::initClass();
    uiGMTRandLinesGrp::initClass();
    uiGMT2DLinesGrp::initClass();
    uiGMTCoastlineGrp::initClass();
    uiGMTClipGrp::initClass();
    uiGMTAdvGrp::initClass();
    uiGMTSurfaceGrid::initClass();
    uiGMTNearNeighborGrid::initClass();

    return 0;
}
