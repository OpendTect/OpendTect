/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimenu.h"
#include "uiodmenumgr.h"
#include "uimsg.h"
#include "uipresentationmaker.h"
#include "uitoolbar.h"

#include "ioman.h"
#include "odplugin.h"


mDefODPluginInfo(uiPresentationMaker)
{
    static PluginInfo retpi(
	"Presentation Maker (GUI)",
	"Create PowerPoint presentations from OpendTect" );
    return &retpi;
}


class uiPresMakerPIMgr	: public uiPluginInitMgr
{ mODTextTranslationClass(uiPresMakerPIMgr)
public:
				uiPresMakerPIMgr();

private:

    uiDialog*			dlg_ = nullptr;

    void			init() override;
    void			dTectToolbarChanged() override;
    void			cleanup() override;

    void			showDlgCB(CallBacker*);
};


uiPresMakerPIMgr::uiPresMakerPIMgr()
    : uiPluginInitMgr()
{
    init();
}


void uiPresMakerPIMgr::init()
{
    uiPluginInitMgr::init();
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
    closeAndNullPtr( dlg_ );
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
	return "Cannot instantiate the Presentation Maker plugin";

    return nullptr;
}
