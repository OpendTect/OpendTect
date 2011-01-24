/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uitranslatedlg.cc,v 1.3 2011-01-24 04:41:03 cvsnanne Exp $";


#include "uitranslatedlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uidesktopservices.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"

#include "odhttp.h"
#include "pixmap.h"
#include "googletranslator.h"
#include "settings.h"

bool sUseKeyFld = false;

uiTranslateDlg::uiTranslateDlg( uiParent* p )
    : uiDialog(p,Setup("Google Translate","",""))
    , keyfld_(0)
{
    uiLabel* lbl = new uiLabel( this, "Translation into the selected language\n"
	    "will start on OK.\n"
	    "In other windows, press 'F12' to start the translation" );

    const bool enabled = TrMgr().tr()->enabled();
    enabbut_ = new uiCheckBox( this, "Enable" );
    enabbut_->setChecked( enabled );
    enabbut_->attach( alignedBelow, lbl );

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "Language" );
    lcb->attach( alignedBelow, enabbut_ );
    languagefld_ = lcb->box();


    if ( sUseKeyFld )
    {
	keyfld_ = new uiGenInput( this, "Google API Key", StringInpSpec() );
	keyfld_->attach( alignedBelow, lcb );
    }

    ioPixmap pm( "logo-powered-by-google.png" );
    googlebut_ = new uiPushButton( this, "", pm,
	    mCB(this,uiTranslateDlg,googleButPushCB), true );
    googlebut_->setMinimumHeight( 60 );
    googlebut_->setMinimumWidth( 140 );
    googlebut_->attach( rightTo, enabbut_ );
    googlebut_->attach( ensureRightOf, lcb );

    fillBox();
}


uiTranslateDlg::~uiTranslateDlg()
{
}


bool uiTranslateDlg::enabled() const
{ return enabbut_->isChecked(); }


void uiTranslateDlg::googleButPushCB( CallBacker* )
{
    const char* url = keyfld_
	? "https://code.google.com/apis/console/?api=translate"
	: "http://translate.google.com";
    uiDesktopServices::openUrl( url );
}


void uiTranslateDlg::fillBox()
{
    BufferString curlang;
    Settings::common().get( "Translator.Language", curlang );
    const bool res = !curlang.isEmpty()
		? TrMgr().tr()->setToLanguage( curlang ) : false;

    const int nrlangs = TrMgr().tr()->nrSupportedLanguages();
    for ( int idx=0; idx<nrlangs; idx++ )
    {
	languagefld_->addItem( TrMgr().tr()->getLanguageUserName(idx) );
	if ( curlang == TrMgr().tr()->getLanguageName(idx) )
	    languagefld_->setCurrentItem( idx );
    }

    if ( keyfld_ )
    {
	BufferString key;
	Settings::common().get( "Translator.Key", key );
	keyfld_->setText( key );
    }
}


static const char* sAPIKey()
{ return "AIzaSyBDR4RWX27WpkutU0olXxAl1-9BkaIp-EI"; }

bool uiTranslateDlg::acceptOK( CallBacker* )
{
    mDynamicCastGet(GoogleTranslator*,gtr,TrMgr().tr())
    if ( !gtr ) return false;

    ODHttp& http = gtr->http();
    if ( http.hasPendingRequests() )
	http.clearPendingRequests();

    BufferString key = sAPIKey();
    if ( keyfld_ )
    {
	key = keyfld_->text();
	Settings::common().set( "Translator.Key", key );
    }

    gtr->setAPIKey( key );

    const bool enabtrl = enabbut_->isChecked();
    if ( enabtrl )
    {
	const int langidx = languagefld_->currentItem();
	const char* lang = TrMgr().tr()->getLanguageName( langidx );
	TrMgr().tr()->setToLanguage( lang );
	Settings::common().set( "Translator.Language", lang );
	Settings::common().write();
    }

    if ( enabtrl && !TrMgr().tr()->enabled() )
	TrMgr().tr()->enable();
    else if ( !enabtrl )
	TrMgr().tr()->disable();

    return true;
}
