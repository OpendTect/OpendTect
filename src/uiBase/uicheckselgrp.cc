/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicheckselgrp.h"

#include "uibutton.h"
#include "uimenu.h"
#include "uitoolbutton.h"

static const int cIconSz = 16;


uiCheckSelGrp::uiCheckSelGrp( uiParent* p, const char* nm )
    : uiGroup(p,nm)
    , beforeShowMenu(this)
    , checkAllClicked(this)
    , invertSelection(this)
    , showCheckedOnly(this)
    , menu_(*new uiMenu(p))
{
    menubut_ = new uiPushButton( this, uiStrings::sEmptyString() );
    menubut_->setName( "Selection menu" );
    menubut_->setIcon( "menu-arrow" );
    menubut_->setPrefWidth( 40 );
    menubut_->setMinimumWidth( 40 );
    menubut_->setMaximumWidth( 40 );
    menubut_->setFlat( true );
#ifdef __win__
    menubut_->setStyleSheet( ":pressed { background: transparent; }" );
#else
    menubut_->setStyleSheet(
		":pressed { border: 0; background: transparent }" );
#endif
    mAttachCB( menubut_->activated, uiCheckSelGrp::menuCB );

    cb_ = new uiCheckBox( this, uiStrings::sEmptyString() );
    cb_->setName( "Check-all box" );
    cb_->setMaximumWidth( 20 );
    mAttachCB( cb_->activated, uiCheckSelGrp::checkCB );

    openbut_ = new uiToolButton( this, "open", tr("Read selection"),
				 mCB(this,uiCheckSelGrp,retrieveCB) );
    openbut_->setPrefHeight( cIconSz );
    openbut_->setPrefWidth( cIconSz );
    openbut_->attach( centeredRightOf, menubut_, 1 );
    openbut_->display( false );

    savebut_ = new uiToolButton( this, "save", tr("Save selection"),
				 mCB(this,uiCheckSelGrp,saveCB) );
    savebut_->setPrefHeight( cIconSz );
    savebut_->setPrefWidth( cIconSz );
    savebut_->attach( rightTo, openbut_, 2 );
    savebut_->display( false );

    setHAlignObj( menubut_ );
}


uiCheckSelGrp::~uiCheckSelGrp()
{
    detachAllNotifiers();
    delete &menu_;
}


void uiCheckSelGrp::setPrimaryCheckBox( bool yn )
{
    cb_->setChecked( yn );
}


void uiCheckSelGrp::offerReadWriteSelection( const CallBack& readcb,
					    const CallBack& writecb )
{
    opencb_ = readcb;
    savecb_ = writecb;
    openbut_->display( opencb_.willCall() );
    savebut_->display( savecb_.willCall() );
    updateButtons();
}


void uiCheckSelGrp::updateState( int nrchecked, int nritems )
{
    nrchecked_ = nrchecked;
    nritems_ = nritems;

    NotifyStopper ns( cb_->activated );
    CmdRecStopper cmdrecstopper( cb_ );
    if ( nrchecked==0 )
	cb_->setCheckState( OD::Unchecked );
    else if ( nritems>0 && nrchecked==nritems )
	cb_->setCheckState( OD::Checked );
    else
	cb_->setCheckState( OD::PartiallyChecked );

    updateButtons();
}


void uiCheckSelGrp::updateButtons()
{
    openbut_->setSensitive( nritems_>0 );
    savebut_->setSensitive( nrchecked_>0 );
}


void uiCheckSelGrp::showMenu()
{
    beforeShowMenu.trigger();
    if ( nritems_ < 1 )
	return;

    menu_.clear();
    if ( nrchecked_ < nritems_ )
	menu_.insertAction( new uiAction(tr("Check all (Ctrl-A)")), 0 );

    if ( nrchecked_ > 0 )
	menu_.insertAction( new uiAction(tr("Uncheck all (Ctrl-Z)")), 1 );

    menu_.insertAction( new uiAction(tr("Invert selection")), 2 );

    if ( nrchecked_ > 0 )
    {
	menu_.insertSeparator();
	if ( allshown_ )
	    menu_.insertAction( new uiAction(tr("Show checked only")), 5 );
	else
	    menu_.insertAction( new uiAction(tr("Show all")), 6 );
    }

    const bool needretrieve = opencb_.willCall();
    const bool needsave = savecb_.willCall() && nrchecked_>0;
    if ( needretrieve || needsave )
    {
	menu_.insertSeparator();
	if ( needretrieve )
	    menu_.insertAction( new uiAction(tr("Read selection")), 3 );

	if ( needsave )
	    menu_.insertAction( new uiAction(tr("Save selection")), 4 );
    }

    const int res = menu_.exec();
    if ( res==0 || res==1 )
	checkAllClicked.trigger( res==0 );
    else if ( res == 2 )
	invertSelection.trigger();
    else if ( res == 3 )
	retrieveCB( nullptr );
    else if ( res == 4 )
	saveCB( nullptr );
    else if ( res==5 || res==6 )
    {
	allshown_ = res==6;
	showCheckedOnly.trigger( !allshown_ );
    }
}


bool uiCheckSelGrp::handleShortcut( OD::KeyboardKey key, bool ctrl )
{
    if ( !ctrl || !attachObj()->isDisplayed() )
	return false;

    if ( key == OD::KB_A )
    {
	checkAllClicked.trigger( true );
	return true;
    }

    if ( key == OD::KB_Z )
    {
	checkAllClicked.trigger( false );
	return true;
    }

    if ( key == OD::KB_M )
    {
	showMenu();
	return true;
    }

    return false;
}


void uiCheckSelGrp::menuCB( CallBacker* )
{
    showMenu();
}


void uiCheckSelGrp::checkCB( CallBacker* )
{
    const bool checkall = cb_->getCheckState()==OD::Checked;
    checkAllClicked.trigger( checkall );
}


void uiCheckSelGrp::retrieveCB( CallBacker* )
{
    if ( opencb_.willCall() )
	opencb_.doCall( this );
}


void uiCheckSelGrp::saveCB( CallBacker* )
{
    if ( savecb_.willCall() )
	savecb_.doCall( this );
}
