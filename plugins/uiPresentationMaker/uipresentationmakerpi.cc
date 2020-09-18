
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


class uiPresMakerPIMgr	: public uiPluginInitMgr
{ mODTextTranslationClass(uiPresMakerPIMgr)
public:
				uiPresMakerPIMgr();

    uiPresentationMakerDlg*	dlg_;

    void			dTectMenuChanged() override;
    void			beforeSurveyChange() override { cleanup(); }
    void			applicationClosing() override { cleanup(); }

    void			cleanup();
    void			mnuCB(CallBacker*);
};


uiPresMakerPIMgr::uiPresMakerPIMgr()
    : uiPluginInitMgr()
    , dlg_(nullptr)
{
    uiAction* action = new uiAction( m3Dots(tr("Presentation Maker")),
			mCB(this,uiPresMakerPIMgr,mnuCB), "ppt" );
    appl().menuMgr().toolsMnu()->insertAction( action );

    init();
}


void uiPresMakerPIMgr::dTectMenuChanged()
{
    appl().menuMgr().dtectTB()->addButton( "ppt", tr("Presentation Maker"),
					   mCB(this,uiPresMakerPIMgr,mnuCB) );
}


void uiPresMakerPIMgr::cleanup()
{
    if ( dlg_ )
    {
	dlg_->close();
	deleteAndZeroPtr( dlg_ );
    }
}


void uiPresMakerPIMgr::mnuCB( CallBacker* )
{
    if ( !dlg_ )
	dlg_ = new uiPresentationMakerDlg( &appl() );

    dlg_->show();
}


mDefODInitPlugin(uiPresentationMaker)
{
    mDefineStaticLocalObject( uiPresMakerPIMgr*, mgr, = nullptr );
    if ( mgr ) return nullptr;
    mgr = new uiPresMakerPIMgr();
    return nullptr;
}
