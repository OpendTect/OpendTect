/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		Nov 2017
________________________________________________________________________

-*/

#include "uiodlangmenumgr.h"

#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uiodstdmenu.h"
#include "texttranslator.h"

uiODLangMenuMgr::uiODLangMenuMgr( uiODMenuMgr& mm )
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
	langmnu_ = mnumgr_.addSubMenu( mnumgr_.settMnu(), tr("Language"),
				       "language" );
	for ( int idx=0; idx<TrMgr().nrSupportedLanguages(); idx++ )
	{
	    uiAction* itm = new uiAction( TrMgr().getLanguageUserName(idx),
				 mCB(this,uiODLangMenuMgr,languageSelectedCB));
	    itm->setCheckable( true );
	    langmnu_->insertAction( itm, mSettLanguageMnu+idx );
	}
    }
}


void uiODLangMenuMgr::updateLanguageMenu()
{
    for ( int idx=0; langmnu_ && idx<langmnu_->actions().size(); idx++ )
    {
	uiAction* itm = const_cast<uiAction*>(langmnu_->actions()[idx]);
	const int trmgridx = idx - mSettLanguageMnu;
	itm->setChecked( trmgridx == TrMgr().currentLanguage() );
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
    if ( !itm )
	{ pErrMsg("Huh"); return; }

    const int trmgridx = itm->getID() - mSettLanguageMnu;
    uiRetVal uirv = TrMgr().setLanguage( trmgridx );
    if ( !uirv.isOK() )
	uiMSG().error( uirv );
}
