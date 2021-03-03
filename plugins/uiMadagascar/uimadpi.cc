
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/


#include "uimadagascarmain.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uitoolbar.h"

#include "envvars.h"
#include "file.h"
#include "dbman.h"
#include "maddefs.h"
#include "madio.h"
#include "odplugin.h"
#include "separstr.h"
#include "staticstring.h"


mDefODPluginInfo(uiMadagascar)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Madagascar Link (GUI)",
	"OpendTect",
	"dGB (Bert Bril)",
	"=od",
	"A link to the Madagascar system."
	    "\nSee http://opendtect.org/links/madagascar.html"
	    " for info on Madagascar."));
    return &retpi;
}


static bool checkEnvVars( uiString& msg )
{
    BufferString rsfdir = GetEnvVar( "RSFROOT" );
    if ( rsfdir.isEmpty() || !File::isDirectory(rsfdir.buf()) )
    {
	msg = od_static_tr("Madagascarlink_checkEnvVars",
					"RSFROOT is either not set or invalid");
	return false;
    }

    return true;
}


class uiMadagascarLinkMgr : public uiPluginInitMgr
{ mODTextTranslationClass(uiMadagascarLinkMgr);
public:

			uiMadagascarLinkMgr();

private:

    uiMadagascarMain*	madwin_ = nullptr;
    bool		ishidden_ = false;

    void		beforeSurveyChange() override { cleanup(); }
    void		dTectMenuChanged() override;
    void		cleanup();

    void		doMain(CallBacker*);
    void		winHide(CallBacker*);

};


uiMadagascarLinkMgr::uiMadagascarLinkMgr()
    : uiPluginInitMgr()
{
    init();
}


void uiMadagascarLinkMgr::dTectMenuChanged()
{
    auto* action = new uiAction( m3Dots(toUiString("Madagascar")),
				      mCB(this,uiMadagascarLinkMgr,doMain),
				      "madagascar" );
    appl().menuMgr().procMnu()->insertAction( action );
}


void uiMadagascarLinkMgr::cleanup()
{
    if ( madwin_ )
	madwin_->askSave( false );
    closeAndZeroPtr( madwin_ );
    uiPluginInitMgr::cleanup();
}


void uiMadagascarLinkMgr::winHide( CallBacker* )
{
    ishidden_ = true;
}


void uiMadagascarLinkMgr::doMain( CallBacker* )
{
    uiString errmsg;
    if ( !checkEnvVars(errmsg) )
    {
	gUiMsg().error( errmsg );
	return;
    }

    if ( !madwin_ )
    {
	madwin_ = new uiMadagascarMain( &appl() );
	mAttachCB( madwin_->windowHide, uiMadagascarLinkMgr::winHide );
    }

    ishidden_ = false;
    madwin_->show();
    madwin_->raise();
}


mDefODInitPlugin(uiMadagascar)
{
    mDefineStaticLocalObject( PtrMan<uiMadagascarLinkMgr>, theinst_,
				= new uiMadagascarLinkMgr() );

    DBMan::CustomDirData cdd( ODMad::cMadDirIDNr(), ODMad::sKeyMadagascar(),
			      "Madagascar data" );
    uiRetVal uirv = DBMan::addCustomDataDir( cdd );
    if ( !uirv.isOK() )
    {
	mDeclStaticString(ret);
	ret = uirv.getText();
	return ret.str();
    }

#ifdef MAD_UIMSG_IF_FAIL
    if ( ODMad::PI().errMsg().isSet() )
	gUiMsg().error( ODMad::PI().errMsg() );
#endif

    return nullptr;
}
