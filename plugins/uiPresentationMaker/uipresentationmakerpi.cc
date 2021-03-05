
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/


#include "uimenu.h"
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


class uiPresMakerPIMgr	: public uiPluginInitMgr
{ mODTextTranslationClass(uiPresMakerPIMgr)
public:
				uiPresMakerPIMgr();

private:

    uiDialog*			dlg_ = nullptr;

    void			dTectMenuChanged() override;
    void			dTectToolbarChanged() override;
    void			cleanup() override;

    void			showDlgCB(CallBacker*);
};


uiPresMakerPIMgr::uiPresMakerPIMgr()
    : uiPluginInitMgr()
{
    init();
}


void uiPresMakerPIMgr::dTectMenuChanged()
{
    appl().menuMgr().toolsMnu()->insertAction(
	new uiAction( m3Dots( tr("Presentation Maker") ),
		      mCB(this,uiPresMakerPIMgr,showDlgCB), "ppt" ) );
}


void uiPresMakerPIMgr::dTectToolbarChanged()
{
    appl().menuMgr().dtectTB()->addButton( "ppt", tr("Presentation Maker"),
				mCB(this,uiPresMakerPIMgr,showDlgCB) );
}


void uiPresMakerPIMgr::cleanup()
{
    closeAndZeroPtr( dlg_ );
    uiPluginInitMgr::cleanup();
}


void uiPresMakerPIMgr::showDlgCB( CallBacker* )
{
    if ( !dlg_ )
	dlg_ = new uiPresentationMakerDlg( &appl() );

    dlg_->show();
}


mDefODInitPlugin(uiPresentationMaker)
{
    mDefineStaticLocalObject( PtrMan<uiPresMakerPIMgr>, theinst_,
				= new uiPresMakerPIMgr() );
    if ( !theinst_ )
	return "Cannot instantiate uiPresentationMaker plugin";

    return nullptr;
}
