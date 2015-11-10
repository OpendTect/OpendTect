
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uimenu.h"
#include "uiodmenumgr.h"
#include "uimsg.h"
#include "uipresentationmakermod.h"
#include "uitoolbar.h"

#include "odplugin.h"


mDefODPluginInfo(uiPresentationMaker)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Presentation Maker",
	"OpendTect",
	"dGB",
	"1.0",
	"Create Powerpoint presentations from OpendTect") );
    return &retpi;
}


class uiPresMakerPIMgr	: public CallBacker
{ mODTextTranslationClass(uiPresMakerPIMgr)
public:

			uiPresMakerPIMgr(uiODMain*);

    uiODMain*		appl_;
    uiODMenuMgr&	mnumgr_;

    void		updateToolBar(CallBacker*);
};


uiPresMakerPIMgr::uiPresMakerPIMgr( uiODMain* a )
    : mnumgr_(a->menuMgr())
    , appl_(a)
{
    mAttachCB( mnumgr_.dTectTBChanged, uiPresMakerPIMgr::updateToolBar );

    updateToolBar(0);
}


void uiPresMakerPIMgr::updateToolBar( CallBacker* )
{
}


mDefODInitPlugin(uiPresentationMaker)
{
    return 0;
}

