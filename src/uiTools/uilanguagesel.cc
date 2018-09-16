/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2018
________________________________________________________________________

-*/

#include "uilanguagesel.h"
#include "settings.h"
#include "texttranslation.h"
#include "uistrings.h"

#include "uicombobox.h"
#include "uilabel.h"
#include "uimsg.h"


uiLanguageSel::uiLanguageSel( uiParent* p, bool withtext )
    : uiGroup(p,"Language selector")
    , selfld_(0)
    , autocommit_(0)
{
    const int nrlang = TrMgr().nrSupportedLanguages();
    if ( nrlang < 2 )
    {
	uiString txt;
	if ( nrlang < 1 )
	    txt = tr("Internationalisation unavailable").embedFinalState();
	else
	    txt = TrMgr().languageUserName(0).embed("* "," *");
	new uiLabel( this, txt );
	return;
    }

    if ( withtext )
    {
	uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
						    uiStrings::sLanguage() );
	setHAlignObj( lcb );
	selfld_ = lcb->box();
    }
    else
    {
	selfld_ = new uiComboBox( this, "Language" );
	setHAlignObj( selfld_ );
    }

    for ( int idx=0; idx<nrlang; idx++ )
	selfld_->addItem( TrMgr().languageUserName(idx) );

    const int curidx = TrMgr().currentLanguageIdx();
    selfld_->setCurrentItem( curidx<0 ? 0 : curidx );

    selfld_->setToolTip( tr("Select a supported language") );

    mAttachCB( selfld_->selectionChanged, uiLanguageSel::langSelCB );
    mAttachCB( TrMgr().languageChange, uiLanguageSel::extLangChgCB );
}


uiLanguageSel::~uiLanguageSel()
{
    detachAllNotifiers();
}


bool uiLanguageSel::haveMultipleLanguages()
{
    return TrMgr().nrSupportedLanguages() > 1;
}


void uiLanguageSel::extLangChgCB( CallBacker* )
{
    NotifyStopper ns( selfld_->selectionChanged );
    const int curidx = TrMgr().currentLanguageIdx();
    if ( curidx >= 0 )
	selfld_->setCurrentItem( curidx );
}


void uiLanguageSel::langSelCB( CallBacker* )
{
    if ( !selfld_ )
	return;

    const int selidx = selfld_->currentItem();
    if ( selidx >= 0 )
    {
	const uiRetVal uirv = TrMgr().setLanguage( selidx );
	if ( uirv.isError() )
	    uiMSG().error( uirv );
	if ( autocommit_ )
	    commit( true );
    }
}


bool uiLanguageSel::commit( bool writesettings )
{
    if ( !selfld_ )
	return false;
    const int selidx = selfld_->currentItem();
    if ( selidx < 0 )
	return false;

    return setODLocale( TrMgr().localeKey(selidx), writesettings );
}


bool uiLanguageSel::setODLocale( const char* nm, bool mkpermanent )
{
    const uiRetVal uirv = TrMgr().setLanguageByLocaleKey( nm );
    if ( uirv.isError() )
	gUiMsg().error( uirv );
    else
	TrMgr().storeToUserSettings();
    return uirv.isOK();
}
