/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodlangmenumgr.h"

#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "texttranslator.h"

uiODLangMenuMgr::uiODLangMenuMgr( uiODMenuMgr* mm )
    : mnumgr_(mm)
    , langmnu_(0)
{
    mAttachCB( TrMgr().languageChange, uiODLangMenuMgr::languageChangeCB );
    languageChangeCB( 0 );
}


uiODLangMenuMgr::~uiODLangMenuMgr()
{
    detachAllNotifiers();
}


void uiODLangMenuMgr::initLanguageMenu()
{
    if ( TrMgr().nrSupportedLanguages() > 1 && !langmnu_ )
    {
	langmnu_ = new uiMenu( tr("Language") );
	mnumgr_->settMnu()->addMenu( langmnu_ );
	for ( int idx=0; idx<TrMgr().nrSupportedLanguages(); idx++ )
	{
	    uiAction* itm = new uiAction( TrMgr().getLanguageUserName(idx),
				 mCB(this,uiODLangMenuMgr,languageSelectedCB));
	    itm->setCheckable( true );
	    langmnu_->insertAction( itm, idx );
	}
    }
}


void uiODLangMenuMgr::updateLanguageMenu()
{
    for ( int idx=0; langmnu_ && idx<langmnu_->actions().size(); idx++ )
    {
	uiAction* itm = const_cast<uiAction*>(langmnu_->actions()[idx]);
	itm->setChecked( idx==TrMgr().currentLanguage() );
    }
}


void uiODLangMenuMgr::languageChangeCB(CallBacker*)
{
    initLanguageMenu();
    updateLanguageMenu();
}


void uiODLangMenuMgr::languageSelectedCB( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm ) return; // Huh?

    const int idx = itm->getID();
    uiString err;
    if ( !TrMgr().setLanguage(idx,err) )
    {
	uiMSG().error( err );
    }
}
