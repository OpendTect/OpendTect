
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/


#include "uimenu.h"
#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uimsg.h"
#include "uipresentationmaker.h"
#include "uitoolbar.h"

#include "dbman.h"
#include "odplugin.h"


mDefODPluginInfo(uiPresentationMaker)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Presentation Maker",
	"OpendTect",
	"dGB (Bert Bril)",
	"=od",
	"Create Powerpoint presentations from OpendTect") );
    return &retpi;
}


class uiPresMakerPIMgr: public uiPluginInitMgr
{ mODTextTranslationClass(uiPresMakerPIMgr)
public:

				uiPresMakerPIMgr();

private:

    uiPresentationMakerDlg*	dlg_ = nullptr;

    void			beforeSurveyChange() override;
    void			applicationClosing() override;
    void			doCleanup();
    void			mnuCB(CallBacker*);

};


uiPresMakerPIMgr::uiPresMakerPIMgr()
    : uiPluginInitMgr()
{
    init();
    uiAction* action = new uiAction( m3Dots(tr("Presentation Maker")),
			mCB(this,uiPresMakerPIMgr,mnuCB), "ppt" );
    appl_.menuMgr().toolsMnu()->insertAction( action );
}


void uiPresMakerPIMgr::beforeSurveyChange()
{
    doCleanup();
}


void uiPresMakerPIMgr::applicationClosing()
{
    doCleanup();
}


void uiPresMakerPIMgr::doCleanup()
{
    if ( dlg_ )
	{ dlg_->close(); deleteAndZeroPtr( dlg_ ); }
}


void uiPresMakerPIMgr::mnuCB( CallBacker* )
{
    if ( !dlg_ )
	dlg_ = new uiPresentationMakerDlg( &appl_ );

    dlg_->show();
}



mDefODInitPlugin(uiPresentationMaker)
{
    mDefineStaticLocalObject( PtrMan<uiPresMakerPIMgr>, presmgr,
				= new uiPresMakerPIMgr() );

    return nullptr;
}
