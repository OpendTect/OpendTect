/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene
 Date:          July 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistrattreewin.cc,v 1.22 2008-11-25 15:35:26 cvsbert Exp $";

#include "uistrattreewin.h"

#include "compoundkey.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilistbox.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uisplitter.h"
#include "uistratmgr.h"
#include "uistratreftree.h"
#include "uistratutildlgs.h"

#define	mExpandTxt	"&Expand all"
#define	mCollapseTxt	"&Collapse all"
#define	mEditTxt	"&Edit"
#define	mLockTxt	"&Lock"


static const char* sNoLevelTxt      = "--- Empty ---";

using namespace Strat;


const uiStratTreeWin& StratTWin()
{
    static uiStratTreeWin* stratwin = 0;
    if ( !stratwin )
	stratwin = new uiStratTreeWin(0);

    return *stratwin;
}


uiStratTreeWin::uiStratTreeWin( uiParent* p )
    : uiMainWin(p,"Manage Stratigraphy", 0, true, false)
{
    uistratmgr_ = new uiStratMgr( this );
    createMenus();
    createGroups();
    setExpCB(0);
    editCB(0);
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
    
    uitree_ = new uiStratRefTree( leftgrp, uistratmgr_ );
    CallBack selcb = mCB( this,uiStratTreeWin,unitSelCB );
    CallBack renmcb = mCB(this,uiStratTreeWin,unitRenamedCB);
    uitree_->listView()->selectionChanged.notify( selcb );
    uitree_->listView()->itemRenamed.notify( renmcb );
    
    lvllistfld_ = new uiLabeledListBox( rightgrp, "Existing Levels", false,
	    				uiLabeledListBox::AboveMid );
    lvllistfld_->box()->setStretch( 2, 2 );
    lvllistfld_->box()->setFieldWidth( 12 );
    lvllistfld_->box()->selectionChanged.notify( mCB( this, uiStratTreeWin,
						      selLvlChgCB ) );
    lvllistfld_->box()->rightButtonClicked.notify( mCB( this, uiStratTreeWin,
						      rClickLvlCB ) );
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
{ /*
    uiListViewItem* item = uitree_->listView()->selectedItem();
    BufferString bs;
    
    if ( item )
    {
	bs = item->text();

	while ( item->parent() )
	{
	    item = item->parent();
	    CompoundKey kc( item->text() );
	    kc += bs.buf();
	    bs = kc.buf();
	}
    }

    const Strat::UnitRef* ur = uitree_->findUnit( bs.buf() ); 
*/ }


void uiStratTreeWin::editCB( CallBacker* )
{
    bool doedit = !strcmp( editmnuitem_->text(), mEditTxt );
    uitree_->makeTreeEditable( doedit );
    editmnuitem_->setText( doedit ? mLockTxt : mEditTxt );
    if ( doedit )
	uistratmgr_->createTmpTree( false );
}


void uiStratTreeWin::resetCB( CallBacker* )
{
    const Strat::RefTree* bcktree = uistratmgr_->getBackupTree();
    if ( !bcktree ) return;
    bool iseditmode = !strcmp( editmnuitem_->text(), mEditTxt );
    uistratmgr_->reset( iseditmode );
    uitree_->setTree( bcktree, true );
    uitree_->expand( true );
}


void uiStratTreeWin::saveCB( CallBacker* )
{
    uistratmgr_->save();
}


void uiStratTreeWin::saveAsCB( CallBacker* )
{
    uistratmgr_->saveAs();
}


void uiStratTreeWin::openCB( CallBacker* )
{
    pErrMsg("Not implemented yet: uiStratTreeWin::openCB");
}


void uiStratTreeWin::selLvlChgCB( CallBacker* )
{
    pErrMsg("Not implemented yet: uiStratTreeWin::selLvlChgCB");
}


void uiStratTreeWin::rClickLvlCB( CallBacker* )
{
    if ( strcmp( editmnuitem_->text(), mLockTxt ) ) return;
    int curit = lvllistfld_->box()->currentItem();
    uiPopupMenu mnu( this, "Action" );
    mnu.insertItem( new uiMenuItem("Create &New ..."), 0 );
    if ( curit>-1 && !lvllistfld_->box()->isPresent( sNoLevelTxt ) )
    {
	mnu.insertItem( new uiMenuItem("&Edit ..."), 1 );
	mnu.insertItem( new uiMenuItem("&Remove"), 2 );
    }
    const int mnuid = mnu.exec();
    if ( mnuid<0 || mnuid>2 ) return;
    if ( mnuid == 2 )
    {
	uistratmgr_->removeLevel( lvllistfld_->box()->getText() );
	lvllistfld_->box()->removeItem( lvllistfld_->box()->currentItem() );
	uitree_->updateLvlsPixmaps();
	uitree_->listView()->triggerUpdate();
	if ( lvllistfld_->box()->isEmpty() )
	    lvllistfld_->box()->addItem( sNoLevelTxt );
	return;
    }

    editLevel( mnuid ? false : true );
}


void uiStratTreeWin::fillLvlList()
{
    lvllistfld_->box()->empty();
    BufferStringSet lvlnms;
    TypeSet<Color> lvlcolors;
    uistratmgr_->getLvlsTxtAndCol( lvlnms, lvlcolors );
    for ( int idx=0; idx<lvlnms.size(); idx++ )
	lvllistfld_->box()->addItem( lvlnms[idx]->buf(), lvlcolors[idx] );
    
    if ( !lvlnms.size() )
	lvllistfld_->box()->addItem( sNoLevelTxt );
}


void uiStratTreeWin::editLevel( bool create )
{
    uiStratLevelDlg newlvldlg( this, uistratmgr_ );
    if ( !create )
	newlvldlg.setLvlInfo( lvllistfld_->box()->getText() );
    if ( newlvldlg.go() )
	updateLvlList( create );
}


void uiStratTreeWin::updateLvlList( bool create )
{
    if ( create && lvllistfld_->box()->isPresent( sNoLevelTxt ) )
	lvllistfld_->box()->removeItem( 0 );
    
    BufferString lvlnm;
    Color lvlcol;
    int lvlidx = create ? lvllistfld_->box()->size()
			: lvllistfld_->box()->currentItem();
    uistratmgr_->getLvlTxtAndCol( lvlidx, lvlnm, lvlcol );
    if ( create )
	lvllistfld_->box()->addItem( lvlnm, lvlcol );
    else
    {
	lvllistfld_->box()->setItemText( lvlidx, lvlnm );
	lvllistfld_->box()->setPixmap( lvlidx, lvlcol );
    }
}


void uiStratTreeWin::unitRenamedCB( CallBacker* )
{
    //TODO requires Qt4 to approve/cancel renaming ( name already in use...)
}
