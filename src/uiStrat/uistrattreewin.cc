/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene
 Date:          July 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uistrattreewin.cc,v 1.78 2012-07-04 11:14:47 cvsbruno Exp $";

#include "uistrattreewin.h"

#include "compoundkey.h"
#include "ioman.h"
#include "stratunitrepos.h"
#include "uitoolbutton.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilistview.h"
#include "uimain.h"
#include "uimenu.h"
#include "uiselsimple.h"
#include "uimsg.h"
#include "uiobjdisposer.h"
#include "stratreftree.h"
#include "uiparent.h"
#include "uisplitter.h"
#include "uistratreftree.h"
#include "uistratlvllist.h"
#include "uistratutildlgs.h"
#include "uistratdisplay.h"
#include "uiselsimple.h"
#include "uitoolbar.h"

#define	mExpandTxt(domenu)	domenu ? "&Expand all" : "Expand all"
#define	mCollapseTxt(domenu)	domenu ? "&Collapse all" : "Collapse all"

//tricky: want to show action in menu and status on button
#define	mEditTxt(domenu)	domenu ? "&Unlock" : "Toggle read only: locked"
#define	mLockTxt(domenu)	domenu ? "&Lock" : "Toggle read only: editable"

using namespace Strat;

ObjectSet<uiToolButtonSetup> uiStratTreeWin::tbsetups_;

static uiStratTreeWin* stratwin = 0;
const uiStratTreeWin& StratTWin()
{
    if ( !stratwin )
    {
	uiMain& app = uiMain::theMain();
	stratwin = new uiStratTreeWin( app.topLevel() );
    }

    return *stratwin;
}
uiStratTreeWin& StratTreeWin()
{
    return const_cast<uiStratTreeWin&>( StratTWin() );
}


uiStratTreeWin::uiStratTreeWin( uiParent* p )
    : uiMainWin(p,"Manage Stratigraphy", 0, true)
    , needsave_(false)
    , istreedisp_(false)	
    , repos_(*new Strat::RepositoryAccess())
    , tb_(0)
{
    IOM().surveyChanged.notify( mCB(this,uiStratTreeWin,forceCloseCB ) );
    IOM().applicationClosing.notify( mCB(this,uiStratTreeWin,forceCloseCB ) );
    if ( RT().isEmpty() )
	setNewRT();

    createMenu();
    createToolBar();
    createGroups();
    setExpCB(0);
    editCB(0);
    moveUnitCB(0);
}


uiStratTreeWin::~uiStratTreeWin()
{
    delete &repos_;
}


void uiStratTreeWin::setNewRT()
{
    BufferStringSet opts;
    opts.add( "<Build from scratch>" );
    Strat::RefTree::getStdNames( opts );

    const bool nortpresent = RT().isEmpty();
    BufferString dlgmsg( "Stratigraphy: " ); 
    dlgmsg += nortpresent ? "select initial" : "select new"; 
    uiSelectFromList::Setup su( dlgmsg, opts );
    uiSelectFromList dlg( this, su );
    if ( nortpresent )
	dlg.setButtonText( uiDialog::CANCEL, "" );
    if ( dlg.go() )
    {
	const char* nm = opts.get( dlg.selection() );
	Strat::LevelSet* ls = 0;
	Strat::RefTree* rt = 0;

	if ( dlg.selection() > 0 )
	{
	    ls = Strat::LevelSet::createStd( nm );
	    if ( !ls )
		{ pErrMsg( "Cannot read LevelSet from Std!" ); return; }
	    else
	    {
		rt = Strat::RefTree::createStd( nm );
		if ( !rt )
		    { pErrMsg( "Cannot read RefTree from Std!" ); return; }
	    }
	}
	else
	{
	    rt = new RefTree(); 
	    ls = new LevelSet();
	}
	Strat::setLVLS( ls );
	Strat::setRT( rt );
	const Repos::Source dest = Repos::Survey;
	Strat::LVLS().store( dest );
	Strat::RepositoryAccess().writeTree( Strat::RT(), dest );
	if ( tb_ )
	    resetCB( 0 );
    }
}


void uiStratTreeWin::addTool( uiToolButtonSetup* su )
{
    if ( !su ) return;
    tbsetups_ += su;
    if ( stratwin )
	stratwin->tb_->addButton( *su );
}


void uiStratTreeWin::popUp() const
{
    uiStratTreeWin& self = *const_cast<uiStratTreeWin*>(this);
    self.show();
    self.raise();
}

    
void uiStratTreeWin::createMenu()
{
    uiMenuBar* menubar = menuBar();
    uiPopupMenu* mnu = new uiPopupMenu( this, "&Actions" );
    expandmnuitem_ = new uiMenuItem( mExpandTxt(true),
				     mCB(this,uiStratTreeWin,setExpCB) );
    mnu->insertItem( expandmnuitem_ );
    expandmnuitem_->setPixmap( ioPixmap("collapse_tree") );
    mnu->insertSeparator();
    editmnuitem_ = new uiMenuItem( mEditTxt(true),
	    			   mCB(this,uiStratTreeWin,editCB) );
    mnu->insertItem( editmnuitem_ );
    editmnuitem_->setPixmap( ioPixmap("unlock") );
    savemnuitem_ = new uiMenuItem( "&Save", mCB(this,uiStratTreeWin,saveCB) );
    mnu->insertItem( savemnuitem_ );
    savemnuitem_->setPixmap( ioPixmap("save") );
    resetmnuitem_ = new uiMenuItem( "&Reset to last saved",
	    			    mCB(this,uiStratTreeWin,resetCB));
    mnu->insertItem( resetmnuitem_ );
    resetmnuitem_->setPixmap( ioPixmap("undo") );
    mnu->insertSeparator();
    
    openmnuitem_ = new uiMenuItem( "&Open...", mCB(this,uiStratTreeWin,openCB));
    mnu->insertItem( openmnuitem_ );
    openmnuitem_->setPixmap( ioPixmap("openset") );
    saveasmnuitem_ = new uiMenuItem( "Save&As...",
	    			     mCB(this,uiStratTreeWin,saveAsCB) );
    mnu->insertItem( saveasmnuitem_ );
    saveasmnuitem_->setPixmap( ioPixmap("saveas") );
    menubar->insertItem( mnu );	    
}

#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton( tb_, fnm, tt, mCB(this,uiStratTreeWin,cbnm) ); \
    tb_->addButton( but );


void uiStratTreeWin::createToolBar()
{
    tb_ = new uiToolBar( this, "Stratigraphy Manager Tools" );
    mDefBut(colexpbut_,"collapse_tree",setExpCB,mCollapseTxt(false));
    tb_->addSeparator();
    mDefBut(moveunitupbut_,"uparrow",moveUnitCB,"Move unit up");
    mDefBut(moveunitdownbut_,"downarrow",moveUnitCB,"Move unit down");
    tb_->addSeparator();
    mDefBut(newbut_,"newset",newCB,"New");
    mDefBut(lockbut_,"unlock",editCB,mEditTxt(false));
    lockbut_->setToggleButton( true );
    uiToolButton* uitb;
    mDefBut(uitb,"save",saveCB,"Save");
    mDefBut(uitb,"contexthelp",helpCB,"Help on this window");
    tb_->addSeparator();
    mDefBut( switchviewbut_, "strat_tree", switchViewCB, "Switch View" );
    mDefBut( lithobut_, "lithologies", manLiths, "Manage Lithologies" );
    mDefBut( contentsbut_, "contents", manConts, "Manage Content Types" );

    for ( int idx=0; idx<tbsetups_.size(); idx++ )
	tb_->addButton( *tbsetups_[idx] );
}


void uiStratTreeWin::createGroups()
{
    uiGroup* leftgrp = new uiGroup( this, "LeftGroup" );
    leftgrp->setStretch( 1, 1 );
    uiGroup* rightgrp = new uiGroup( this, "RightGroup" );
    rightgrp->setStretch( 1, 1 );

    uitree_ = new uiStratRefTree( leftgrp );
    CallBack selcb = mCB( this,uiStratTreeWin,unitSelCB );
    CallBack renmcb = mCB(this,uiStratTreeWin,unitRenamedCB);
    uitree_->listView()->selectionChanged.notify( selcb );
    uitree_->listView()->itemRenamed.notify( renmcb );
    uitree_->listView()->display( false );

    if ( !uitree_->haveTimes() )
	uitree_->setEntranceDefaultTimes();

    uistratdisp_ = new uiStratDisplay( leftgrp, *uitree_ );
    uistratdisp_->addControl( tb_ );

    lvllist_ = new uiStratLvlList( rightgrp );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", true );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );
}


void uiStratTreeWin::setExpCB( CallBacker* )
{
    bool expand = !strcmp( expandmnuitem_->text(), mExpandTxt(true) );
    uitree_->expand( expand );
    expandmnuitem_->setText( expand ? mCollapseTxt(true) : mExpandTxt(true) );
    expandmnuitem_->setPixmap( expand ? ioPixmap("collapse_tree")
				      : ioPixmap("expand_tree") );
    colexpbut_->setPixmap( expand ? ioPixmap("collapse_tree")
	    			  : ioPixmap("expand_tree") );
    colexpbut_->setToolTip( expand ? mCollapseTxt(false) : mExpandTxt(false) );
}


void uiStratTreeWin::unitSelCB(CallBacker*)
{
    moveUnitCB(0);
}


void uiStratTreeWin::newCB( CallBacker* )
{
    BufferString msg( "This will overwrite the current tree. \n" );
    msg += "Your work will be lost. Continue anyway ?";
    if ( RT().isEmpty() || uiMSG().askGoOn( msg ) )
	setNewRT();
}


void uiStratTreeWin::editCB( CallBacker* )
{
    bool doedit = !strcmp( editmnuitem_->text(), mEditTxt(true) );
    uitree_->makeTreeEditable( doedit );
    editmnuitem_->setText( doedit ? mLockTxt(true) : mEditTxt(true) );
    editmnuitem_->setPixmap( doedit ? ioPixmap("unlock")
				    : ioPixmap("readonly") );
    lockbut_->setPixmap( doedit ? ioPixmap("unlock")
	    			: ioPixmap("readonly") );
    lockbut_->setToolTip( doedit ? mLockTxt(false) : mEditTxt(false) );
    lockbut_->setOn( !doedit );
    setIsLocked( !doedit );
}


void uiStratTreeWin::setIsLocked( bool yn )
{
    uistratdisp_->setIsLocked( yn );
    lvllist_->setIsLocked( yn );
    lithobut_->setSensitive( !yn );
    contentsbut_->setSensitive( !yn );
    newbut_->setSensitive( !yn );
    openmnuitem_->setEnabled( !yn );
    saveasmnuitem_->setEnabled( !yn );
    resetmnuitem_->setEnabled( !yn );
}


void uiStratTreeWin::resetCB( CallBacker* )
{
    Strat::RefTree& bcktree = Strat::eRT(); 
    //for the time beeing, get back the global tree, but we may want to have 
    //a snapshot copy of the actual tree we are working on...
    bool iseditmode = !strcmp( editmnuitem_->text(), mEditTxt(true) );
    uitree_->setTree( bcktree, true );
    uitree_->expand( true );
    uistratdisp_->setTree();
    lvllist_->setLevels();
}


void uiStratTreeWin::saveCB( CallBacker* )
{
    Strat::eLVLS().store( Repos::Survey );
    if ( uitree_->tree() )
    {
	repos_.writeTree( *uitree_->tree() );
	needsave_ = false;
    }
    else
	uiMSG().error( "Can not find tree" ); 
}


static const char* infolvltrs[] =
{
    "Survey level",
    "OpendTect data level",
    "User level",
    "Global level",
    0
};

void uiStratTreeWin::saveAsCB( CallBacker* )
{
    const char* dlgtit = "Save the stratigraphy at:";
    const char* helpid = 0;
    uiDialog savedlg( this, uiDialog::Setup( "Save Stratigraphy",
		    dlgtit, helpid ) );
    BufferStringSet bfset( infolvltrs );
    uiListBox saveloclist( &savedlg, bfset );
    savedlg.go();
    if ( savedlg.uiResult() == 1 )
    {
	const char* savetxt = saveloclist.getText();
	Repos::Source src = Repos::Survey;
	if ( !strcmp( savetxt, infolvltrs[1] ) )
	    src = Repos::Data;
	else if ( !strcmp( savetxt, infolvltrs[2] ) )
	    src = Repos::User;
	else if ( !strcmp( savetxt, infolvltrs[3] ) )
	    src = Repos::ApplSetup;

	repos_.writeTree( Strat::RT(), src );
    }
}


void uiStratTreeWin::switchViewCB( CallBacker* )
{
    istreedisp_ = istreedisp_ ? false : true;
    uistratdisp_->display( !istreedisp_ );
    if ( uistratdisp_->control() )
	uistratdisp_->control()->setSensitive( !istreedisp_ );
    uitree_->listView()->display( istreedisp_ );
    switchviewbut_->setPixmap( istreedisp_ ? "stratframeworkgraph"
					   : "strat_tree" );
}


void uiStratTreeWin::openCB( CallBacker* )
{
    uiMSG().error( "Not Implemented yet" );
    return;
    //TODO
}


void uiStratTreeWin::unitRenamedCB( CallBacker* )
{
    needsave_ = true;
    //TODO requires Qt4 to approve/cancel renaming ( name already in use...)
}


bool uiStratTreeWin::closeOK()
{
    if ( needsave_ )
    {
	int res = uiMSG().askSave( 
			"Do you want to save this stratigraphic framework?" );
	if ( res == 1 )
	    saveCB( 0 );
	else if ( res == 0 )
	{
	    resetCB( 0 );
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

    uiOBJDISP()->go( uistratdisp_ );

    delete lvllist_;
    delete uitree_;
    stratwin = 0;
}


void uiStratTreeWin::moveUnitCB( CallBacker* cb )
{
    if ( cb) 
	uitree_->moveUnit( cb == moveunitupbut_ );

    moveunitupbut_->setSensitive( uitree_->canMoveUnit( true ) );
    moveunitdownbut_->setSensitive( uitree_->canMoveUnit( false ) );
}


void uiStratTreeWin::helpCB( CallBacker* )
{
    uiMainWin::provideHelp( "110.0.0" );
}


void uiStratTreeWin::manLiths( CallBacker* )
{
    uiStratLithoDlg dlg( this );
    dlg.go();
    uitree_->updateLithoCol();
}


void uiStratTreeWin::manConts( CallBacker* )
{
    uiStratContentsDlg dlg( this );
    dlg.go();
}


void uiStratTreeWin::changeLayerModelNumber( bool add )
{
    static int nrlayermodelwin = 0;
    bool haschged = false;
    if ( add )
    {
	nrlayermodelwin ++;
	if ( nrlayermodelwin == 1 )
	    haschged = true;
    }
    else 
    {
	nrlayermodelwin --;
	if ( nrlayermodelwin == 0 )
	    haschged = true;
    }
    if ( haschged )
    {
	const bool islocked = lockbut_->isOn();
	if ( (!islocked && nrlayermodelwin) || (islocked && !nrlayermodelwin) )
	    { lockbut_->setOn( nrlayermodelwin ); editCB(0); }
	lockbut_->setSensitive( !nrlayermodelwin );
	editmnuitem_->setEnabled( !nrlayermodelwin );
    }
}
