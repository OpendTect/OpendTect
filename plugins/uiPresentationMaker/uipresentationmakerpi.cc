
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uimenu.h"
#include "uiodmenumgr.h"
#include "uimsg.h"
#include "uipresentationmaker.h"
#include "uitoolbar.h"

#include "ioman.h"
#include "odplugin.h"


mDefODPluginInfo(uiPresentationMaker)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Presentation Maker",
	"OpendTect",
	"dGB",
	"1.0",
	"Create PowerPoint presentations from OpendTect") );
    return &retpi;
}


class uiPresMakerPIMgr	: public CallBacker
{ mODTextTranslationClass(uiPresMakerPIMgr)
public:

				uiPresMakerPIMgr(uiODMain*);

    uiODMain*			appl_;
    uiPresentationMakerDlg*	dlg_;

    void			updateMenu(CallBacker*);
    void			mnuCB(CallBacker*);
};


uiPresMakerPIMgr::uiPresMakerPIMgr( uiODMain* a )
    : appl_(a)
    , dlg_(0)
{
    mAttachCB( appl_->menuMgr().dTectMnuChanged, uiPresMakerPIMgr::updateMenu );
    mAttachCB( IOM().applicationClosing, uiPresMakerPIMgr::updateMenu );

    uiAction* action = new uiAction( m3Dots(tr("Presentation Maker")),
			mCB(this,uiPresMakerPIMgr,mnuCB), "ppt" );
    appl_->menuMgr().toolsMnu()->insertAction( action );

    updateMenu( nullptr );
}


void uiPresMakerPIMgr::updateMenu( CallBacker* )
{
    if ( dlg_ )
    {
	dlg_->close();
	deleteAndZeroPtr( dlg_ );
    }

    appl_->menuMgr().dtectTB()->addButton( "ppt", tr("Presentation Maker"),
					   mCB(this,uiPresMakerPIMgr,mnuCB) );
}


void uiPresMakerPIMgr::mnuCB( CallBacker* )
{
    if ( !dlg_ )
	dlg_ = new uiPresentationMakerDlg( appl_ );

    dlg_->show();
}


mDefODInitPlugin(uiPresentationMaker)
{
    mDefineStaticLocalObject( uiPresMakerPIMgr*, mgr, = 0 );
    if ( mgr ) return 0;
    mgr = new uiPresMakerPIMgr( ODMainWin() );

    return 0;
}
