/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          April 2014
________________________________________________________________________

-*/

#include "uifontsel.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uifont.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uimain.h"
#include "od_helpids.h"
#include "settings.h"


uiFontSettingsGroup::uiFontSettingsGroup( uiParent* p, Settings& setts )
    : uiSettingsGroup(p,tr("Fonts"),setts)
{
    FontList().initialise();
    butgrp_ = new uiButtonGroup( this, "", OD::Vertical );
    butgrp_->attach( hCentered );

    addButton( FontData::Control, tr("General User Interface") );
    addButton( FontData::Graphics2D, tr("Used by 2D Graphics") );
    addButton( FontData::Graphics3D, tr("Used in 3D Scenes") );
    addButton( FontData::Fixed, tr("Information, Notes and Progress") );
    addResetButton();
}


void uiFontSettingsGroup::addButton( FontData::StdSz tp, uiString infotxt )
{
    uiButton* but = new uiPushButton( butgrp_,
				    mToUiStringTodo(FontData::key(tp)), false );
    but->setPrefWidthInChar( 25 );
    but->activated.notify( mCB(this,uiFontSettingsGroup,butPushed) );
    buttons_ += but;

    uiLabel* lbl = new uiLabel( butgrp_, infotxt );
    lbl->setStretch( 2, 1 );
    lbl->attach( rightTo, but );
    lbl->setFont( FontList().get(tp) );
    lbls_ += lbl;

    types_ += tp;
}


void uiFontSettingsGroup::addResetButton()
{
    uiButton* but = new uiPushButton( this,
				    tr("Reset to default fonts"),
				    true );
    but->setPrefWidthInChar( 25 );
    but->activated.notify( mCB(this,uiFontSettingsGroup,resetCB) );
    but->attach( centeredBelow, butgrp_ );
}


void uiFontSettingsGroup::butPushed( CallBacker* obj )
{
    mDynamicCastGet(uiButton*,sender,obj)
    const int idx = buttons_.indexOf( sender );
    if ( idx < 0 ) { pErrMsg("idx < 0. Why?"); return; }

    uiFont& selfont = FontList().get( types_[idx] );
    if ( !selectFont(selfont,sender->parent()) )
	return;

    FontData fd = selfont.fontData();
    const int ptsz = fd.pointSize();
    const int smallsz = ptsz - 2;
    const int largesz = ptsz + 2;
    if ( types_[idx] == FontData::Control )
    {
	fd.setPointSize( smallsz );
	FontList().get( FontData::ControlSmall ).setFontData( fd );
	fd.setPointSize( largesz );
	FontList().get( FontData::ControlLarge ).setFontData( fd );
    }
    else if ( types_[idx] == FontData::Graphics2D )
    {
	fd.setPointSize( smallsz );
	FontList().get( FontData::Graphics2DSmall ).setFontData( fd );
	fd.setPointSize( largesz );
	FontList().get( FontData::Graphics2DLarge ).setFontData( fd );
    }

    FontList().update( Settings::common() );
    if ( !idx ) uiMain::theMain().setFont( FontList().get(), true );
    lbls_[idx]->setFont( selfont );
}


void uiFontSettingsGroup::resetCB( CallBacker* )
{
    if ( uiMSG().askGoOn(tr("Reset to application defaults?")) )
    {
	FontList().setDefaults();
	uiMain::theMain().setFont( FontList().get(), true );
	for ( int idx=0; idx<lbls_.size(); idx++ )
	{
	    const uiFont& selfont = FontList().get( types_[idx] );
	    lbls_[idx]->setFont( selfont );
	}
    }
}


bool uiFontSettingsGroup::acceptOK()
{ return true; }


HelpKey uiFontSettingsGroup::helpKey() const
{ return mODHelpKey(mSetFontsHelpID); }


// uiSelFonts
uiSelFonts::uiSelFonts( uiParent* p, const uiString& title,
			const HelpKey& helpkey )
	: uiDialog(p,uiDialog::Setup(tr("Fonts"),title,helpkey))
{
    FontList().listKeys( ids_ );
}


uiSelFonts::~uiSelFonts()
{
}


void uiSelFonts::add( const char* nm, const char* stdfontkey )
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, ids_,
							 mToUiStringTodo(nm) );
    if ( !sels_.isEmpty() )
	lcb->attach( alignedBelow, sels_.last() );
    lcb->box()->setCurrentItem( stdfontkey );
    sels_ += lcb;
}


const char* uiSelFonts::resultFor( const char* str )
{
    for ( int idx=0; idx<sels_.size(); idx++ )
    {
	if ( sels_[idx]->label()->name() == str )
	    return sels_[idx]->box()->text();
    }

    return FontList().key(0);
}
