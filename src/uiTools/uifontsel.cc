/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          April 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uifontsel.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uifont.h"
#include "uilabel.h"
#include "uimain.h"
#include "od_helpids.h"
#include "settings.h"


uiFontSettingsGroup::uiFontSettingsGroup( uiParent* p, Settings& setts )
    : uiSettingsGroup(p,tr("Fonts"),setts)
{
    FontList().initialise();
    const ObjectSet<uiFont>& fonts = FontList().fonts();
    uiButtonGroup* butgrp = new uiButtonGroup( this, "", OD::Vertical );
    butgrp->setPrefWidthInChar( 25 );
    for ( int idx=0; idx<fonts.size(); idx++ )
    {
	uiButton* but = new uiPushButton( butgrp, fonts[idx]->key(), false );
        but->activated.notify( mCB(this,uiFontSettingsGroup,butPushed) );
	buttons += but;
    }

    butgrp->attach( hCentered );
}


void uiFontSettingsGroup::butPushed( CallBacker* obj )
{
    mDynamicCastGet(uiButton*,sender,obj)
    int idx = buttons.indexOf( sender );
    if ( idx < 0 ) { pErrMsg("idx < 0. Why?"); return; }

    if ( select(*FontList().fonts()[idx],sender->parent()) )
    {
	FontList().update( Settings::common() );
	if ( !idx ) uiMain::theMain().setFont( FontList().get(), true );
    }
}


bool uiFontSettingsGroup::acceptOK()
{ return true; }


HelpKey uiFontSettingsGroup::helpKey() const
{ return mODHelpKey(mSetFontsHelpID); }


// uiSelFonts
uiSelFonts::uiSelFonts( uiParent* p, const uiString& title,
			const HelpKey& helpkey )
	: uiDialog(p,uiDialog::Setup("Fonts",title,helpkey))
{
    FontList().listKeys( ids );
}


uiSelFonts::~uiSelFonts()
{
}


void uiSelFonts::add( const char* nm, const char* stdfontkey )
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, ids, nm );
    if ( sels.size() )
	lcb->attach( alignedBelow, sels[sels.size()-1] );
    lcb->box()->setCurrentItem( stdfontkey );
    sels += lcb;
}


const char* uiSelFonts::resultFor( const char* str )
{
    for ( int idx=0; idx<sels.size(); idx++ )
    {
	if ( sels[idx]->label()->name() == str )
	    return sels[idx]->box()->text();
    }

    return FontList().key(0);
}

