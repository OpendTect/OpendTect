/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene
 Date:          July 2007
 RCS:		$Id: uistrattreewin.cc,v 1.7 2007-08-13 15:16:39 cvshelene Exp $
________________________________________________________________________

-*/

#include "uistrattreewin.h"

#include "compoundkey.h"
#include "stratlevel.h"
#include "stratunitrepos.h"
#include "uidialog.h"
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


static const char* infolvltrs[] =
{
    "Survey level",
    "OpendTect data level",
    "User level",
    "Global level",
    0
};

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
    savemnuitem_ = new uiMenuItem( "&Save", mCB(this,uiStratTreeWin,saveCB) );
    actmnu->insertItem( savemnuitem_ );
    resetmnuitem_ = new uiMenuItem( "&Reset", mCB(this,uiStratTreeWin,resetCB));
    actmnu->insertItem( resetmnuitem_ );
    actmnu->insertSeparator();
    openmnuitem_ = new uiMenuItem( "&Open...", mCB(this,uiStratTreeWin,openCB));
    actmnu->insertItem( openmnuitem_ );
    saveasmnuitem_ = new uiMenuItem( "Save&As...",
	    			     mCB(this,uiStratTreeWin,saveAsCB) );
    actmnu->insertItem( saveasmnuitem_ );
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
    lvllistfld_ = new uiLabeledListBox( rightgrp, "Existing Levels", false,
	    				uiLabeledListBox::AboveMid );
    lvllistfld_->box()->setStretch( 2, 2 );
    lvllistfld_->box()->setFieldWidth( 12 );
    lvllistfld_->box()->selectionChanged.notify( mCB( this, uiStratTreeWin,
						      selLvlChgCB ) );
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
    /*uiListViewItem* item = uitree_->listView()->selectedItem();
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
*/}


void uiStratTreeWin::editCB( CallBacker* )
{
    bool doedit = !strcmp( editmnuitem_->text(), mEditTxt );
    uitree_->makeTreeEditable( doedit );
    editmnuitem_->setText( doedit ? mLockTxt : mEditTxt );
}


void uiStratTreeWin::saveCB( CallBacker* )
{
    const_cast<UnitRepository*>(&UnRepo())->copyCurTreeAtLoc( Repos::Survey );
    UnRepo().write( Repos::Survey );
}


void uiStratTreeWin::saveAsCB( CallBacker* )
{
    const char* dlgtit = "Save the stratigraphy at:";
    const char* helpid = 0;
    uiDialog savedlg( this, uiDialog::Setup( "Save Stratigraphy",
					     dlgtit, helpid ) );
    BufferStringSet bfset( infolvltrs );
    uiListBox saveloclist( savedlg.parent(), bfset );
    savedlg.go();
    pErrMsg("Not implemented yet: uiStratTreeWin::saveAsCB");
}


void uiStratTreeWin::resetCB( CallBacker* )
{
    pErrMsg("Not implemented yet: uiStratTreeWin::resetCB");
}


void uiStratTreeWin::openCB( CallBacker* )
{
    pErrMsg("Not implemented yet: uiStratTreeWin::openCB");
}


void uiStratTreeWin::selLvlChgCB( CallBacker* )
{
    pErrMsg("Not implemented yet: uiStratTreeWin::selLvlChgCB");
}


void uiStratTreeWin::fillLvlList()
{
    lvllistfld_->box()->empty();
    int nrlevels = RT().nrLevels();
    for ( int idx=0; idx<nrlevels; idx++ )
    {
	const Level* lvl = RT().level( idx );
	if ( !lvl ) return;
	lvllistfld_->box()->addItem( lvl->name_, lvl->color() );
    }
}
