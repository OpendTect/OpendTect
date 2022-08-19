/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimadagascarmain.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uitoolbar.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "maddefs.h"
#include "madio.h"
#include "odplugin.h"
#include "separstr.h"


mDefODPluginInfo(uiMadagascar)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"Madagascar Link (GUI)",
	"OpendTect",
	"dGB Earth Sciences (Raman Singh)",
	"=od",
	"User Interface for the link to the Madagascar system."
	    "\nSee http://opendtect.org/links/madagascar.html"
	    " for info on Madagascar." ))
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


class uiMadagascarLink	: public uiPluginInitMgr
{ mODTextTranslationClass(uiMadagascarLink);
public:
			uiMadagascarLink();

private:

    uiMadagascarMain*	madwin_ = nullptr;
    bool		ishidden_ = false;

    void		dTectMenuChanged() override;
    void		cleanup() override;

    void		doMain( CallBacker* );
    void		winHide(CallBacker*);

};


uiMadagascarLink::uiMadagascarLink()
    : uiPluginInitMgr()
{
    init();
}


void uiMadagascarLink::dTectMenuChanged()
{
    auto* action = new uiAction( m3Dots(tr("Madagascar")),
				 mCB(this,uiMadagascarLink,doMain),
				 "madagascar" );
    appl().menuMgr().procMnu()->insertAction( action );
}


void uiMadagascarLink::cleanup()
{
    if ( madwin_ )
	madwin_->askSave( false );
    closeAndZeroPtr( madwin_ );
    uiPluginInitMgr::cleanup();
}


void uiMadagascarLink::winHide( CallBacker* )
{
    ishidden_ = true;
}


void uiMadagascarLink::doMain( CallBacker* )
{
    uiString errmsg;
    if ( !checkEnvVars(errmsg) )
    {
	uiMSG().error( errmsg );
	return;
    }

    if ( !madwin_ )
    {
	madwin_ = new uiMadagascarMain( &appl() );
	mAttachCB( madwin_->windowHide, uiMadagascarLink::winHide );
    }

    ishidden_ = false;
    madwin_->show();
    madwin_->raise();
}


mDefODInitPlugin(uiMadagascar)
{
    mDefineStaticLocalObject( PtrMan<uiMadagascarLink>, theinst_,
		= new uiMadagascarLink() );

    if ( !theinst_ )
	return ODMad::PI().errMsg().getFullString().str();

    IOMan::CustomDirData cdd( ODMad::sKeyMadSelKey(), ODMad::sKeyMadagascar(),
			      "Madagascar data" );
    MultiID id = IOMan::addCustomDataDir( cdd );
    if ( id.groupID() != ODMad::sKeyMadSelKey() )
	return "Cannot create 'Madagascar' directory in survey";

#ifdef MAD_UIMSG_IF_FAIL
    if ( ODMad::PI().errMsg().isSet() )
	uiMSG().error( ODMad::PI().errMsg() );
#endif

    return nullptr;
}
