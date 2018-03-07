
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



class uiPresMakerPIMgr	: public CallBacker
{ mODTextTranslationClass(uiPresMakerPIMgr)
public:

				uiPresMakerPIMgr(uiODMain*);

    uiODMain*			appl_;
    uiPresentationMakerDlg*	dlg_;

    void			updateMenu(CallBacker*);
    void			mnuCB(CallBacker*);

    static uiString		pkgDispNm()
				{ return tr("Powerpoint Presentation Maker"); }
};


uiPresMakerPIMgr::uiPresMakerPIMgr( uiODMain* a )
    : appl_(a)
    , dlg_(0)
{
    mAttachCB( appl_->menuMgr().dTectMnuChanged, uiPresMakerPIMgr::updateMenu );
    mAttachCB( DBM().applicationClosing, uiPresMakerPIMgr::updateMenu );

    uiAction* action = new uiAction( m3Dots(tr("Presentation Maker")),
			mCB(this,uiPresMakerPIMgr,mnuCB), "ppt" );
    appl_->menuMgr().toolsMnu()->insertAction( action );
}


void uiPresMakerPIMgr::updateMenu( CallBacker* )
{
    if ( dlg_ )
	{ dlg_->close(); delete dlg_; dlg_ = 0; }
}


void uiPresMakerPIMgr::mnuCB( CallBacker* )
{
    if ( !dlg_ )
	dlg_ = new uiPresentationMakerDlg( appl_ );

    dlg_->show();
}


mDefODPluginInfo(uiPresentationMaker)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Presentation Maker",
	"Powerpoint Presentation Maker",
	mODPluginCreator, mODPluginVersion,
	"Create Powerpoint presentations from OpendTect") );
    retpi.useronoffselectable_ = true;
    mSetPackageDisplayName( retpi, uiPresMakerPIMgr::pkgDispNm() );
    return &retpi;
}


mDefODInitPlugin(uiPresentationMaker)
{
    mDefineStaticLocalObject( uiPresMakerPIMgr*, mgr, = 0 );
    if ( mgr ) return 0;
    mgr = new uiPresMakerPIMgr( ODMainWin() );

    return 0;
}
