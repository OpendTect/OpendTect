/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene
 Date:          July 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistrattreewin.cc,v 1.40 2010-04-29 05:40:34 cvsranojay Exp $";

#include "uistrattreewin.h"

#include "compoundkey.h"
#include "ioman.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilistbox.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uisplitter.h"
#include "uistratmgr.h"
#include "uistratreftree.h"
#include "uistratutildlgs.h"
#include "uitoolbar.h"

#define	mExpandTxt(domenu)	domenu ? "&Expand all" : "Expand all"
#define	mCollapseTxt(domenu)	domenu ? "&Collapse all" : "Collapse all"

//tricky: want to show action in menu and status on button
#define	mEditTxt(domenu)	domenu ? "&Unlock" : "Toggle read only: locked"
#define	mLockTxt(domenu)	domenu ? "&Lock" : "Toggle read only: editable"

static const char* sNoLevelTxt      = "--- Empty ---";

using namespace Strat;


static uiStratTreeWin* stratwin = 0;
const uiStratTreeWin& StratTWin()
{
    if ( !stratwin )
	stratwin = new uiStratTreeWin(0);

    return *stratwin;
}

#define mAskStratMgrNotif(nm) \
    CallBack nm##cb = mCB( this,uiStratTreeWin,nm##CB );\
    uistratmgr_.nm.notify( nm##cb );

uiStratTreeWin::uiStratTreeWin( uiParent* p )
    : uiMainWin(p,"Manage Stratigraphy", 0, true)
    , uistratmgr_(*new uiStratMgr(this))
    , levelCreated(this)
    , levelChanged(this)
    , levelRemoved(this)
    , newLevelSelected(this)
    , unitCreated(this)		//TODO support
    , unitChanged(this)		//TODO support
    , unitRemoved(this)		//TODO support
    , newUnitSelected(this)
    , lithCreated(this)		//TODO support
    , lithChanged(this)		//TODO support
    , lithRemoved(this)		//TODO support
    , needsave_(false)
    , needcloseok_(true)
{
    IOM().surveyChanged.notify( mCB(this,uiStratTreeWin,forceCloseCB ) );
    mAskStratMgrNotif(unitCreated)
    mAskStratMgrNotif(unitChanged)
    mAskStratMgrNotif(unitRemoved)
    mAskStratMgrNotif(lithCreated)
    mAskStratMgrNotif(lithChanged)
    mAskStratMgrNotif(lithRemoved)
    createMenu();
    createToolBar();
    createGroups();
    setExpCB(0);
    editCB(0);
    moveUnitCB(0);
}


uiStratTreeWin::~uiStratTreeWin()
{
}

#define mImplCBFunctions(nm)\
    void uiStratTreeWin::nm##CB(CallBacker*) { nm.trigger(); }

mImplCBFunctions(unitCreated)
mImplCBFunctions(unitChanged)
mImplCBFunctions(unitRemoved)
mImplCBFunctions(lithCreated)
mImplCBFunctions(lithChanged)

void uiStratTreeWin::popUp() const
{
    uiStratTreeWin& self = *const_cast<uiStratTreeWin*>(this);
    self.show();
    self.raise();
}

    
void uiStratTreeWin::createMenu()
{
    uiMenuBar* menubar = menuBar();
    uiPopupMenu* mnu = new uiPopupMenu( this, "&Menu" );
    expandmnuitem_ = new uiMenuItem( mExpandTxt(true),
				     mCB(this,uiStratTreeWin,setExpCB) );
    mnu->insertItem( expandmnuitem_ );
    expandmnuitem_->setPixmap( ioPixmap("collapse_tree.png") );
    mnu->insertSeparator();
    editmnuitem_ = new uiMenuItem( mEditTxt(true),
	    			   mCB(this,uiStratTreeWin,editCB) );
    mnu->insertItem( editmnuitem_ );
    editmnuitem_->setPixmap( ioPixmap("unlock.png") );
    savemnuitem_ = new uiMenuItem( "&Save", mCB(this,uiStratTreeWin,saveCB) );
    mnu->insertItem( savemnuitem_ );
    savemnuitem_->setPixmap( ioPixmap("save.png") );
    resetmnuitem_ = new uiMenuItem( "&Reset to last saved",
	    			    mCB(this,uiStratTreeWin,resetCB));
    mnu->insertItem( resetmnuitem_ );
    resetmnuitem_->setPixmap( ioPixmap("undo.png") );
    mnu->insertSeparator();
    
    openmnuitem_ = new uiMenuItem( "&Open...", mCB(this,uiStratTreeWin,openCB));
    mnu->insertItem( openmnuitem_ );
    openmnuitem_->setPixmap( ioPixmap("openset.png") );
    saveasmnuitem_ = new uiMenuItem( "Save&As...",
	    			     mCB(this,uiStratTreeWin,saveAsCB) );
    mnu->insertItem( saveasmnuitem_ );
    saveasmnuitem_->setPixmap( ioPixmap("saveas.png") );
    menubar->insertItem( mnu );	    
}

#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton( tb_, 0, ioPixmap(fnm), \
			    mCB(this,uiStratTreeWin,cbnm) ); \
    but->setToolTip( tt ); \
    tb_->addObject( but );


void uiStratTreeWin::createToolBar()
{
    tb_ = new uiToolBar( this, "Stratigraphy Manager Tools" );
    mDefBut(colexpbut_,"collapse_tree.png",setExpCB,mCollapseTxt(false));
    tb_->addSeparator();
    mDefBut(moveunitupbut_,"uparrow.png",moveUnitCB,"Move unit up");
    mDefBut(moveunitdownbut_,"downarrow.png",moveUnitCB,"Move unit down");
    tb_->addSeparator();
    mDefBut(lockbut_,"unlock.png",editCB,mEditTxt(false));
    lockbut_->setToggleButton( true );
//    mDefBut(openbut_,"openset.png",openCB,"Open"); not implemented yet
    mDefBut(savebut_,"save.png",saveCB,"Save");
    uiToolButton* helpbut = new uiToolButton( tb_, 0,
					      ioPixmap("contexthelp.png"),
					      mCB(this,uiStratTreeWin,helpCB) );
    helpbut->setToolTip( "Help" );
    tb_->addObject( helpbut );
}


void uiStratTreeWin::createGroups()
{
    uiGroup* leftgrp = new uiGroup( this, "LeftGroup" );
    leftgrp->setStretch( 1, 1 );
    uiGroup* rightgrp = new uiGroup( this, "RightGroup" );
    rightgrp->setStretch( 1, 1 );
    
    uitree_ = new uiStratRefTree( leftgrp, &uistratmgr_ );
    CallBack selcb = mCB( this,uiStratTreeWin,unitSelCB );
    CallBack renmcb = mCB(this,uiStratTreeWin,unitRenamedCB);
    uitree_->listView()->selectionChanged.notify( selcb );
    uitree_->listView()->itemRenamed.notify( renmcb );
    
    uiLabeledListBox* llb = new uiLabeledListBox( rightgrp, "Levels", false,
						  uiLabeledListBox::AboveMid );
    lvllistfld_ = llb->box();
    lvllistfld_->setStretch( 2, 2 );
    lvllistfld_->setFieldWidth( 12 );
    lvllistfld_->selectionChanged.notify( mCB( this, uiStratTreeWin,
						      selLvlChgCB ) );
    lvllistfld_->rightButtonClicked.notify( mCB( this, uiStratTreeWin,
						      rClickLvlCB ) );
    fillLvlList();
    
    uiSplitter* splitter = new uiSplitter( this, "Splitter", true );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );
}


void uiStratTreeWin::setExpCB( CallBacker* )
{
    bool expand = !strcmp( expandmnuitem_->text(), mExpandTxt(true) );
    uitree_->expand( expand );
    expandmnuitem_->setText( expand ? mCollapseTxt(true) : mExpandTxt(true) );
    expandmnuitem_->setPixmap( expand ? ioPixmap("collapse_tree.png")
				      : ioPixmap("expand_tree.png") );
    colexpbut_->setPixmap( expand ? ioPixmap("collapse_tree.png")
	    			  : ioPixmap("expand_tree.png") );
    colexpbut_->setToolTip( expand ? mCollapseTxt(false) : mExpandTxt(false) );
}


void uiStratTreeWin::unitSelCB(CallBacker*)
{
    moveUnitCB(0);
    newUnitSelected.trigger();
    /*
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
    bool doedit = !strcmp( editmnuitem_->text(), mEditTxt(true) );
    uitree_->makeTreeEditable( doedit );
    editmnuitem_->setText( doedit ? mLockTxt(true) : mEditTxt(true) );
    editmnuitem_->setPixmap( doedit ? ioPixmap("unlock.png")
				    : ioPixmap("readonly.png") );
    lockbut_->setPixmap( doedit ? ioPixmap("unlock.png")
	    			: ioPixmap("readonly.png") );
    lockbut_->setToolTip( doedit ? mLockTxt(false) : mEditTxt(false) );
    lockbut_->setOn( !doedit );
    if ( doedit )
	uistratmgr_.createTmpTree( false );
}


void uiStratTreeWin::resetCB( CallBacker* )
{
    const Strat::RefTree* bcktree = uistratmgr_.getBackupTree();
    if ( !bcktree ) return;
    bool iseditmode = !strcmp( editmnuitem_->text(), mEditTxt(true) );
    uistratmgr_.reset( iseditmode );
    uitree_->setTree( bcktree, true );
    uitree_->expand( true );
}


void uiStratTreeWin::saveCB( CallBacker* )
{
    uistratmgr_.save();
    needsave_ = false;
}


void uiStratTreeWin::saveAsCB( CallBacker* )
{
    uistratmgr_.saveAs();
    needsave_ = false;
}


void uiStratTreeWin::openCB( CallBacker* )
{
    pErrMsg("Not implemented yet: uiStratTreeWin::openCB");
}


void uiStratTreeWin::selLvlChgCB( CallBacker* )
{
    newLevelSelected.trigger();
    needsave_ = true;
}


void uiStratTreeWin::rClickLvlCB( CallBacker* )
{
    if ( strcmp( editmnuitem_->text(), mLockTxt(true) ) ) return;
    int curit = lvllistfld_->currentItem();
    uiPopupMenu mnu( this, "Action" );
    mnu.insertItem( new uiMenuItem("Create &New ..."), 0 );
    if ( curit>-1 && !lvllistfld_->isPresent( sNoLevelTxt ) )
    {
	mnu.insertItem( new uiMenuItem("&Edit ..."), 1 );
	mnu.insertItem( new uiMenuItem("&Remove"), 2 );
    }
    const int mnuid = mnu.exec();
    if ( mnuid<0 || mnuid>2 ) return;
    if ( mnuid != 2 )
	editLevel( mnuid ? false : true );
    else
    {
	uistratmgr_.removeLevel( lvllistfld_->getText() );
	lvllistfld_->removeItem( lvllistfld_->currentItem() );
	uitree_->updateLvlsPixmaps();
	uitree_->listView()->triggerUpdate();
	if ( lvllistfld_->isEmpty() )
	    lvllistfld_->addItem( sNoLevelTxt );
	levelRemoved.trigger();
	needsave_ = true;
    }
}


void uiStratTreeWin::fillLvlList()
{
    lvllistfld_->empty();
    BufferStringSet lvlnms;
    TypeSet<Color> lvlcolors;
    uistratmgr_.getLvlsTxtAndCol( lvlnms, lvlcolors );
    for ( int idx=0; idx<lvlnms.size(); idx++ )
	lvllistfld_->addItem( lvlnms[idx]->buf(), lvlcolors[idx] );
    
    if ( !lvlnms.size() )
	lvllistfld_->addItem( sNoLevelTxt );
}


void uiStratTreeWin::editLevel( bool create )
{
    uiStratLevelDlg newlvldlg( this, &uistratmgr_ );
    if ( !create )
	newlvldlg.setLvlInfo( lvllistfld_->getText() );
    if ( newlvldlg.go() )
    {
	updateLvlList( create );
	create ? levelCreated.trigger() : levelChanged.trigger();
    }

    needsave_ = true;
}


void uiStratTreeWin::updateLvlList( bool create )
{
    if ( create && lvllistfld_->isPresent( sNoLevelTxt ) )
	lvllistfld_->removeItem( 0 );
    
    BufferString lvlnm;
    Color lvlcol;
    int lvlidx = create ? lvllistfld_->size()
			: lvllistfld_->currentItem();
    uistratmgr_.getLvlTxtAndCol( lvlidx, lvlnm, lvlcol );
    if ( create )
	lvllistfld_->addItem( lvlnm, lvlcol );
    else
    {
	lvllistfld_->setItemText( lvlidx, lvlnm );
	lvllistfld_->setPixmap( lvlidx, lvlcol );
    }
}


void uiStratTreeWin::unitRenamedCB( CallBacker* )
{
    needsave_ = true;
    //TODO requires Qt4 to approve/cancel renaming ( name already in use...)
}


bool uiStratTreeWin::closeOK()
{
    if ( !needcloseok_ )
	return true;
    if ( needsave_ || uistratmgr_.needSave() )
    {
	int res = uiMSG().askSave( 
			"Do you want to save this stratigraphic framework?" );
	if ( res == 1 )
	    uistratmgr_.save();
	else if ( res == 0 )
	{
	    resetCB( 0 );
	    uistratmgr_.createTmpTree( true );
	    return true;
	}
	else if ( res == -1 )
	    return false;
    }

    return true;
}


void uiStratTreeWin::forceCloseCB( CallBacker* )
{
    IOM().surveyChanged.remove( mCB(this,uiStratTreeWin,forceCloseCB ) );
    needcloseok_ = false;
    if ( stratwin )
	stratwin->close();
    needcloseok_ = true;
    stratwin = 0;
}


void uiStratTreeWin::moveUnitCB( CallBacker* cb )
{
    if ( cb) 
	uitree_->moveUnit( cb == moveunitupbut_ );

    moveunitupbut_->setSensitive( uitree_->canMoveUnit( true ) );
    moveunitdownbut_->setSensitive( uitree_->canMoveUnit( false ) );
}


void uiStratTreeWin::lithRemovedCB( CallBacker* cb )
{
    uitree_->updateLithoCol();
    lithRemoved.trigger();
}


void uiStratTreeWin::helpCB( CallBacker* )
{
    uiMainWin::provideHelp( "110.0.0" );
}

