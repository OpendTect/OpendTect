/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
    static PluginInfo retpi(
	"GMT Link (GUI)",
	"User Interface for the link to the GMT mapping tool."
	    "\nSee https://www.generic-mapping-tools.org for info on GMT" );
    return &retpi;
}


class uiGMTIntro : public uiDialog
{ mODTextTranslationClass(uiGMTIntro);
public:

uiGMTIntro( uiParent* p )
    : uiDialog(p,Setup(tr("GMT Mapping Tool"),mNoHelpKey))
{
    setOkText( uiStrings::sContinue() );

    uiString msg = tr("You need to install the GMT mapping tool package\n"
			"before you can use this utility\nAlso make sure that "
			"the environment variable GMT5_SHAREDIR is set "
			"to the GMT installation 'share' directory\n"
		       "and your PATH variable includes the GMT bin directory");

    auto* lbl = new uiLabel( this, msg );
    lbl->setAlignment( Alignment::HCenter );

    auto* gmtbut = new uiPushButton( this, tr("Download GMT"),
					     mCB(this,uiGMTIntro,gmtPush),
					     true );
    gmtbut->setToolTip( tr("Click to go to the Download center") );
    gmtbut->attach( centeredBelow, lbl );
}

protected:

void gmtPush( CallBacker* )
{
    uiDesktopServices::openUrl(
	    "https://www.generic-mapping-tools.org/download/" );
}

bool acceptOK( CallBacker* ) override
{
    return true;
}

};


class uiGMTMgr :  public uiPluginInitMgr
{ mODTextTranslationClass(uiGMTMgr);
public:
			uiGMTMgr();

private:

    uiDialog*		dlg_ = nullptr;

    void		dTectMenuChanged() override;
    void		cleanup() override;

    void		showDlgCB( CallBacker* );
};


uiGMTMgr::uiGMTMgr()
    : uiPluginInitMgr()
{
    init();
}


void uiGMTMgr::dTectMenuChanged()
{
    auto* action = new uiAction( m3Dots(tr("GMT Mapping Tool")),
			         mCB(this,uiGMTMgr,showDlgCB), "gmt_logo" );
    appl().menuMgr().procMnu()->insertAction( action );
}


void uiGMTMgr::cleanup()
{
    closeAndNullPtr( dlg_ );
    uiPluginInitMgr::cleanup();
}


void uiGMTMgr::showDlgCB( CallBacker* )
{
    if ( !dlg_ )
    {
	const char* gmtsharedir = GetEnvVar("GMT5_SHAREDIR");
	if ( !gmtsharedir )
	    gmtsharedir = GetEnvVar("GMT_SHAREDIR");

	if ( !GMT::hasGMT() || !gmtsharedir )
	{
	    uiGMTIntro introdlg( &appl() );
	    if ( !introdlg.go() )
		return;
	}

	dlg_ = new uiGMTMainWin( &appl() );
    }

    dlg_->show();
}


mDefODInitPlugin(uiGMT)
{
    mDefineStaticLocalObject( PtrMan<uiGMTMgr>, theinst_, = new uiGMTMgr() );
    if ( !theinst_ )
	return "Cannot instantiate the GMT plugin";

    IOMan::CustomDirData cdd( ODGMT::sKeyGMTSelKey(), ODGMT::sKeyGMT(),
			      "GMT data" );
    MultiID id = IOMan::addCustomDataDir( cdd );
    if ( id.groupID() != ODGMT::sKeyGMTSelKey() )
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

    return nullptr;
}
