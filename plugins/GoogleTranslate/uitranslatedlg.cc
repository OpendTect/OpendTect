/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uitranslatedlg.cc,v 1.1 2010-10-26 06:41:37 cvsnanne Exp $";


#include "uitranslatedlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uimsg.h"

#include "odhttp.h"
#include "googletranslator.h"
#include "settings.h"


uiTranslateDlg::uiTranslateDlg( uiParent* p )
    : uiDialog(p,Setup("Google Translate","",""))
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
    fillBox();
}


uiTranslateDlg::~uiTranslateDlg()
{
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
}


bool uiTranslateDlg::acceptOK( CallBacker* )
{
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
