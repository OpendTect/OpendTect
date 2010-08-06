/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene
 Date:          July 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistrattreewin.cc,v 1.48 2010-08-06 07:52:33 cvsbruno Exp $";

#include "uistrattreewin.h"

#include "compoundkey.h"
#include "ioman.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiparent.h"
#include "uisplitter.h"
#include "uistratmgr.h"
#include "uistratreftree.h"
#include "uistratlvllist.h"
#include "uistratutildlgs.h"
#include "uistratdisplay.h"
#include "uitoolbar.h"

#define	mExpandTxt(domenu)	domenu ? "&Expand all" : "Expand all"
#define	mCollapseTxt(domenu)	domenu ? "&Collapse all" : "Collapse all"

//tricky: want to show action in menu and status on button
#define	mEditTxt(domenu)	domenu ? "&Unlock" : "Toggle read only: locked"
#define	mLockTxt(domenu)	domenu ? "&Lock" : "Toggle read only: editable"

using namespace Strat;


static uiStratTreeWin* stratwin = 0;
const uiStratTreeWin& StratTWin()
{
    if ( !stratwin )
	stratwin = new uiStratTreeWin(0);

    return *stratwin;
}
uiStratTreeWin& StratTreeWin()
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
    , newLevelSelected(this)
    , unitCreated(this)		//TODO support
    , unitChanged(this)		//TODO support
    , unitRemoved(this)		//TODO support
    , newUnitSelected(this)
    , lithCreated(this)		//TODO support
    , lithChanged(this)		//TODO support
    , lithRemoved(this)		//TODO support
    , needsave_(false)
    , istreedisp_(false)		
{
    IOM().surveyChanged.notify( mCB(this,uiStratTreeWin,forceCloseCB ) );
    IOM().applicationClosing.notify( mCB(this,uiStratTreeWin,forceCloseCB ) );
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
    tb_->addSeparator();
    mDefBut(switchviewbut_,"stratframeworkgraph.png",
	    				switchViewCB,"Switch View..." );
    switchviewbut_->setToggleButton( true );
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
    uitree_->listView()->display( istreedisp_ );

    uistratdisp_ = new uiStratDisplay( leftgrp, *uitree_ );

    uiStratLvlList* lvllist = new uiStratLvlList( rightgrp, uistratmgr_ );

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
}


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


void uiStratTreeWin::switchViewCB( CallBacker* )
{
    istreedisp_ = istreedisp_ ? false : true;
    uistratdisp_->display( !istreedisp_ );
    uitree_->listView()->display( istreedisp_ );
    switchviewbut_->setPixmap( istreedisp_ ? "strat_tree.png" 
	    				   : "stratframeworkgraph.png" ); 
}



#define mErrRet(s) { uiMSG().error(s); return false; }
class openDlg : public uiDialog
{
public:
    openDlg( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Select Stratigraphy file","",mTODOHelpID))
    {
	fnmfld_ = new uiFileInput( this, "Input file", uiFileInput::Setup("")
				    .forread( true )
				    .withexamine( true ) );
    }

    bool acceptOK(CallBacker*)
    {
	filenm_ = fnmfld_->fileName();
	return true;
    }

    BufferString 	filenm_;

protected:

    uiFileInput*        fnmfld_;
};


void uiStratTreeWin::openCB( CallBacker* )
{
    openDlg dlg( this );
    if ( dlg.go() )
	uistratmgr_.openStratFile( dlg.filenm_ );
}


void uiStratTreeWin::unitRenamedCB( CallBacker* )
{
    needsave_ = true;
    //TODO requires Qt4 to approve/cancel renaming ( name already in use...)
}


bool uiStratTreeWin::closeOK()
{
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
    IOM().applicationClosing.remove( mCB(this,uiStratTreeWin,forceCloseCB ) );
    if ( stratwin )
	stratwin->close();
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

