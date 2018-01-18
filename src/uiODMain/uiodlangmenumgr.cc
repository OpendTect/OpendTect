/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		Nov 2017
________________________________________________________________________

-*/

#include "uiodlangmenumgr.h"

#include "uicombobox.h"
#include "uilanguagesel.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uiodstdmenu.h"
#include "uistatusbar.h"
#include "uistrings.h"
#include "texttranslator.h"

uiODLangMenuMgr::uiODLangMenuMgr( uiODMenuMgr& mm )
    : mnumgr_(mm)
    , langmnu_(0)
    , selfld_(0)
{
    setLanguageMenu();

    if ( uiLanguageSel::haveMultipleLanguages() )
    {
	selfld_ = new uiLanguageSel( &mnumgr_.appl_, false );
	selfld_->setAutoCommit( true );
	mnumgr_.appl_.statusBar()->addObject( selfld_->selFld() );
    }

    mAttachCB( TrMgr().languageChange, uiODLangMenuMgr::languageChangeCB );
}


uiODLangMenuMgr::~uiODLangMenuMgr()
{
    detachAllNotifiers();
}


void uiODLangMenuMgr::setLanguageMenu()
{
    if ( langmnu_ )
	langmnu_->removeAllActions();

    const int nrlang = TrMgr().nrSupportedLanguages();
    if ( nrlang < 2 )
	{ delete langmnu_; langmnu_ = 0; return; }

    if ( !langmnu_ )
	langmnu_ = mnumgr_.addSubMenu( mnumgr_.settMnu(),
			uiStrings::sLanguage(), "language" );

    const int trmgridx = TrMgr().currentLanguage();
    for ( int idx=0; idx<nrlang; idx++ )
    {
	uiAction* itm = new uiAction( TrMgr().getLanguageUserName(idx),
			     mCB(this,uiODLangMenuMgr,languageSelectedCB) );
	itm->setCheckable( true );
	itm->setChecked( idx == trmgridx );
	langmnu_->insertAction( itm, mSettLanguageMnu+idx );
    }
}


void uiODLangMenuMgr::languageChangeCB(CallBacker*)
{
    setLanguageMenu();
}


void uiODLangMenuMgr::languageSelectedCB( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm )
	{ pErrMsg("Huh"); return; }

    const int trmgridx = itm->getID() - mSettLanguageMnu;
    if ( trmgridx == TrMgr().currentLanguage() )
	return;

    uiRetVal uirv = TrMgr().setLanguage( trmgridx );
    if ( !uirv.isOK() )
	uiMSG().error( uirv );

    TrMgr().storeToUserSettings();
}
