
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/

static const char* rcsID = "$Id: uimadagascarmain.cc,v 1.11 2007-11-13 16:21:27 cvsbert Exp $";

#include "uimadagascarmain.h"
#include "uimadiosel.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uimenu.h"
#include "uitoolbar.h"
#include "uiseparator.h"
#include "uimsg.h"
#include "pixmap.h"


uiMadagascarMain::uiMadagascarMain( uiParent* p )
	: uiDialog( p, Setup( "Madagascar processing",
			      "Processing flow",
			      "0.0.0").menubar(true) )
{
    setCtrlStyle( uiDialog::DoAndStay );
    createMenus();

    infld_ = new uiMadIOSel( this, true );

    uiGroup* procgrp = crProcGroup();
    procgrp->attach( alignedBelow, infld_ );

    outfld_ = new uiMadIOSel( this, false );
    outfld_->attach( alignedBelow, procgrp );

    finaliseDone.notify( mCB(this,uiMadagascarMain,setButStates) );
}


uiMadagascarMain::~uiMadagascarMain()
{
}


#define mInsertItem( txt, func ) \
    mnu->insertItem( new uiMenuItem(txt,mCB(this,uiMadagascarMain,func)) )
#define mAddButton(pm,func,tip) \
    toolbar->addButton( pm, mCB(this,uiMadagascarMain,func), tip )

void uiMadagascarMain::createMenus()
{
    uiMenuBar* menubar = menuBar();
    if ( !menubar ) { pErrMsg("huh?"); return; }

    uiPopupMenu* mnu = new uiPopupMenu( this, "&File" );
    mInsertItem( "&New flow ...", newFlow );
    mInsertItem( "&Open flow ...", openFlow );
    mInsertItem( "&Save flow ...", saveFlow );
    mnu->insertSeparator();
    mInsertItem( "&Import flow ...", importFlow );
    mInsertItem( "&Export flow ...", exportFlow );
    menubar->insertItem( mnu );

    uiToolBar* toolbar = new uiToolBar( this, "Flow tools" );
    mAddButton( "newflow.png", newFlow, "Empty this flow" );
    mAddButton( "openflow.png", openFlow, "Open saved flow" );
    mAddButton( "saveflow.png", saveFlow, "Save flow" );
}


uiGroup* uiMadagascarMain::crProcGroup()
{
    uiGroup* procgrp = new uiGroup( this, "Proc group" );
    const CallBack butpushcb( mCB(this,uiMadagascarMain,butPush) );

    uiLabeledListBox* pfld = new uiLabeledListBox( procgrp, "WORK", false,
						   uiLabeledListBox::LeftMid );
    procsfld_ = pfld->box();
    procsfld_->setPrefHeightInChar( 8 );
    procsfld_->setPrefWidthInChar( 40 );
    procsfld_->selectionChanged.notify( mCB(this,uiMadagascarMain,selChg) );

    uiButtonGroup* bgrp = new uiButtonGroup( procgrp, "" );
    bgrp->displayFrame( true );

    upbut_ = new uiToolButton( bgrp, "Up button", butpushcb );
    upbut_->setArrowType( uiToolButton::UpArrow );
    upbut_->setToolTip( "Move current command up" );
    downbut_ = new uiToolButton( bgrp, "Down button", butpushcb );
    downbut_->setArrowType( uiToolButton::DownArrow );
    downbut_->setToolTip( "Move current command down" );

    rmbut_ = new uiToolButton( bgrp, "Remove button", ioPixmap("trashcan.png"),
	    			butpushcb );
    rmbut_->setToolTip( "Remove current command from flow" );
    bgrp->attach( centeredRightOf, pfld );

    procgrp->setHAlignObj( pfld );
    return procgrp;
}


void uiMadagascarMain::butPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb)
    int curidx = procsfld_->currentItem();
    const int sz = procsfld_->size();

    if ( tb == rmbut_ )
    {
	if ( curidx < 0 ) return;
	procsfld_->removeItem( curidx );
	if ( curidx >= procsfld_->size() )
	    curidx--;
    }
    else if ( tb == upbut_ || tb == downbut_ )
    {
	if ( curidx < 0 ) return;
	const bool isup = tb == upbut_;
	const int newcur = curidx + (isup ? -1 : 1);
	if ( newcur >= 0 && newcur < sz )
	{
	    BufferString tmp( procsfld_->textOfItem(newcur) );
	    procsfld_->setItemText( newcur, procsfld_->getText() );
	    procsfld_->setItemText( curidx, tmp );
	    curidx = newcur;
	}
    }

    if ( curidx >= 0 )
	procsfld_->setCurrentItem( curidx );

    setButStates();
}


void uiMadagascarMain::setButStates( CallBacker* )
{
    const bool havesel = !procsfld_->isEmpty();
    rmbut_->setSensitive( havesel );
    selChg( 0 );
}


void uiMadagascarMain::selChg( CallBacker* )
{
    const int curidx = procsfld_->isEmpty() ? -1 : procsfld_->currentItem();
    const int sz = procsfld_->size();
    upbut_->setSensitive( sz > 1 && curidx > 0 );
    downbut_->setSensitive( sz > 1 && curidx >= 0 && curidx < sz-1 );
}


void uiMadagascarMain::newFlow( CallBacker* )
{
}


void uiMadagascarMain::openFlow( CallBacker* )
{
}


void uiMadagascarMain::saveFlow( CallBacker* )
{
}


void uiMadagascarMain::importFlow( CallBacker* )
{
}


void uiMadagascarMain::exportFlow( CallBacker* )
{
}


bool uiMadagascarMain::acceptOK( CallBacker* )
{
    return false;
}
