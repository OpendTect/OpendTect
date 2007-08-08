/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene
 Date:          July 2007
 RCS:		$Id: uistrattreewin.cc,v 1.5 2007-08-08 14:55:46 cvshelene Exp $
________________________________________________________________________

-*/

#include "uistrattreewin.h"

#include "compoundkey.h"
#include "stratlevel.h"
#include "stratunitrepos.h"
#include "uigroup.h"
#include "uilistbox.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uisplitter.h"
#include "uistratreftree.h"

#define	mExpandTxt	"&Expand all"
#define	mCollapseTxt	"&Collapse all"
#define	mEditTxt	"&Edit"
#define	mLockTxt	"&Lock"


using namespace Strat;

uiStratTreeWin::uiStratTreeWin( uiParent* p )
    : uiMainWin(p,"Manage Stratigraphy", 0, true, true)
{
    createMenus();
    createGroups();
    setExpCB(0);
}


uiStratTreeWin::~uiStratTreeWin()
{
    delete uitree_;
}

    
void uiStratTreeWin::createMenus()
{
    uiMenuBar* menubar =  menuBar();
    uiPopupMenu* viewmnu = new uiPopupMenu( this, "&View" );
    expandmnuitem_ = new uiMenuItem( mExpandTxt,
				     mCB(this, uiStratTreeWin, setExpCB ) );
    viewmnu->insertItem( expandmnuitem_ );
    menubar->insertItem( viewmnu );

    uiPopupMenu* actmnu = new uiPopupMenu( this, "&Action" );
    editmnuitem_ = new uiMenuItem( mEditTxt, mCB(this,uiStratTreeWin,editCB) );
    actmnu->insertItem( editmnuitem_ );
    saveasmnuitem_ = new uiMenuItem( "Save &As",
				     mCB( this, uiStratTreeWin, saveAsCB ) );
    actmnu->insertItem( saveasmnuitem_ );
    savemnuitem_ = new uiMenuItem( "&Save", mCB(this,uiStratTreeWin,saveCB) );
    actmnu->insertItem( savemnuitem_ );
    resetmnuitem_ = new uiMenuItem( "&Reset", mCB(this,uiStratTreeWin,resetCB));
    actmnu->insertItem( resetmnuitem_ );
    menubar->insertItem( actmnu );	    
}


void uiStratTreeWin::createGroups()
{
    uiGroup* leftgrp = new uiGroup( this, "LeftGroup" );
    leftgrp->setStretch( 1, 1 );
    uiGroup* rightgrp = new uiGroup( this, "RightGroup" );
    rightgrp->setStretch( 1, 1 );
    uitree_ = new uiStratRefTree( leftgrp, &Strat::RT() );
    const_cast<uiStratRefTree*>(uitree_)->listView()->selectionChanged.notify( 
	    				mCB( this,uiStratTreeWin,unitSelCB ) );
    lvllistfld_ = new uiListBox( rightgrp, "Existing Levels" );
    lvllistfld_->setStretch( 2, 2 );
    lvllistfld_->selectionChanged.notify( mCB(this,uiStratTreeWin,selLvlChgCB));
    fillLvlList();
    
    uiSplitter* splitter = new uiSplitter( this, "Splitter", true );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );
}


void uiStratTreeWin::setExpCB( CallBacker* )
{
    bool expand = !strcmp( expandmnuitem_->text(), mExpandTxt );
    uitree_->expand( expand );
    expandmnuitem_->setText( expand ? mCollapseTxt : mExpandTxt );
}


void uiStratTreeWin::unitSelCB(CallBacker*)
{
/*    uistrattab_->clearLevDesc();
    uiListViewItem* item = uitree_->listView()->selectedItem();
    BufferString bs = item->text();
    int itemdepth = item->depth();
    for ( int idx=itemdepth-1; idx>=0; idx-- )
    {
	item = item->parent();
	CompoundKey kc( item->text() );
	kc += bs.buf();
	bs = kc.buf();
    }
    const Strat::UnitRef* ur = uitree_->findUnit( bs.buf() ); 
    const Strat::Level* toplvl = Strat::RT().getLevel( ur, true );
    const Strat::Level* botlvl = Strat::RT().getLevel( ur, false );
    if ( toplvl )
	uistrattab_->setLevDesc( true, toplvl->name_, toplvl->color() );
    if ( botlvl )
	uistrattab_->setLevDesc( false, botlvl->name_, botlvl->color() );
    uistrattab_->setUnitRef( ur );
*/}


void uiStratTreeWin::editCB( CallBacker* )
{
    bool doedit = !strcmp( editmnuitem_->text(), mEditTxt );
//    uitree_->listView()->setRenameEnabled( doedit );
//    uitree_->listView()->setDragEnabled( doedit );
//    uitree_->listView()->setDropEnabled( doedit );
    editmnuitem_->setText( doedit ? mLockTxt : mEditTxt );
}


void uiStratTreeWin::saveAsCB( CallBacker* )
{
    pErrMsg("Not implemented yet: uiStratTreeWin::saveAsCB");
}


void uiStratTreeWin::saveCB( CallBacker* )
{
    pErrMsg("Not implemented yet: uiStratTreeWin::saveCB");
}


void uiStratTreeWin::resetCB( CallBacker* )
{
    pErrMsg("Not implemented yet: uiStratTreeWin::resetCB");
}


void uiStratTreeWin::selLvlChgCB( CallBacker* )
{
    pErrMsg("Not implemented yet: uiStratTreeWin::selLvlChgCB");
}


void uiStratTreeWin::fillLvlList()
{
    lvllistfld_->empty();
    int nrlevels = RT().nrLevels();
    for ( int idx=0; idx<nrlevels; idx++ )
    {
	const Level* lvl = RT().level( idx );
	if ( !lvl ) return;
	lvllistfld_->addItem( lvl->name_, lvl->color() );
    }
}
