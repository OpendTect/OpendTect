/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene
 Date:          July 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uistrattreewin.h"

#include "compoundkey.h"
#include "ioman.h"
#include "oddirs.h"
#include "objdisposer.h"
#include "od_helpids.h"
#include "stratreftree.h"
#include "stratunitrepos.h"

#include "uicolor.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uimain.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiparent.h"
#include "uiselsimple.h"
#include "uisplitter.h"
#include "uistratdisplay.h"
#include "uistratlvllist.h"
#include "uistratreftree.h"
#include "uistratutildlgs.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"
#include "uitreeview.h"

using namespace Strat;

ManagedObjectSet<uiToolButtonSetup> uiStratTreeWin::tbsetups_;

static uiStratTreeWin* stratwin = 0;
const uiStratTreeWin& StratTWin()
{
    if ( !stratwin )
	stratwin = new uiStratTreeWin( 0 );

    return *stratwin;
}
uiStratTreeWin& StratTreeWin()
{
    return const_cast<uiStratTreeWin&>( StratTWin() );
}


uiStratTreeWin::uiStratTreeWin( uiParent* p )
    : uiMainWin(p,uiStrings::phrManage( uiStrings::sStratigraphy() ), 0, true)
    , needsave_(false)
    , istreedisp_(false)
    , repos_(*new Strat::RepositoryAccess())
    , tb_(0)
{
    IOM().surveyChanged.notify( mCB(this,uiStratTreeWin,survChgCB ) );
    IOM().applicationClosing.notify( mCB(this,uiStratTreeWin,appCloseCB ) );
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


uiString uiStratTreeWin::sExpandTxt()
{ return tr("Expand all"); }


uiString uiStratTreeWin::sCollapseTxt()
{ return tr("Collapse all"); }

//tricky: want to show action in menu and status on button
uiString uiStratTreeWin::sEditTxt(bool domenu)
{ return domenu ? tr("Unlock") : tr("Toggle read only: locked"); }


uiString uiStratTreeWin::sLockTxt(bool domenu)
{ return domenu ? tr("Lock") : tr("Toggle read only: editable"); }


void uiStratTreeWin::setNewRT()
{
    BufferStringSet opts;
    opts.add( "<Build from scratch>" );
    Strat::RefTree::getStdNames( opts );

    const bool nortpresent = RT().isEmpty();
    uiString dlgmsg = tr("Stratigraphy: %1")
		    .arg(nortpresent ? tr("select initial")
				     : tr("select new"));
    uiSelectFromList::Setup su( dlgmsg, opts );
    uiSelectFromList dlg( this, su );
    if ( nortpresent )
	dlg.setButtonText( uiDialog::CANCEL, uiStrings::sStratigraphy() );
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
	needsave_ = true;
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
    uiMenu* mnu = new uiMenu( this, uiStrings::sFile() );
    expandmnuitem_ =
	new uiAction( sExpandTxt(), mCB(this,uiStratTreeWin,setExpCB) );
    mnu->insertItem( expandmnuitem_ );
    expandmnuitem_->setIcon( "collapse_tree" );
    mnu->insertSeparator();
    editmnuitem_ =
	new uiAction( sEditTxt(true), mCB(this,uiStratTreeWin,editCB) );
    mnu->insertItem( editmnuitem_ );
    editmnuitem_->setIcon( "unlock" );
    savemnuitem_ = new uiAction( uiStrings::sSave(),
				 mCB(this,uiStratTreeWin,saveCB) );
    mnu->insertItem( savemnuitem_ );
    savemnuitem_->setIcon( "save" );
    resetmnuitem_ = new uiAction( tr("Reset to last saved"),
				  mCB(this,uiStratTreeWin,resetCB));
    mnu->insertItem( resetmnuitem_ );
    resetmnuitem_->setIcon( "undo" );
    mnu->insertSeparator();

    saveasmnuitem_ = new uiAction(m3Dots(uiStrings::sSaveAs()),
				  mCB(this,uiStratTreeWin,saveAsCB) );
    mnu->insertItem( saveasmnuitem_ );
    saveasmnuitem_->setIcon( "saveas" );
    menubar->insertItem( mnu );
}

#define mDefBut(but,fnm,cbnm,tt) \
    but = new uiToolButton( tb_, fnm, tt, mCB(this,uiStratTreeWin,cbnm) ); \
    tb_->addButton( but );


void uiStratTreeWin::createToolBar()
{
    tb_ = new uiToolBar( this, tr("Stratigraphy Manager Tools") );
    mDefBut(colexpbut_,"collapse_tree",setExpCB,sCollapseTxt());
    colexpbut_->setSensitive( istreedisp_ );
    tb_->addSeparator();
    mDefBut(moveunitupbut_,"uparrow",moveUnitCB,tr("Move unit up"));
    mDefBut(moveunitdownbut_,"downarrow",moveUnitCB,tr("Move unit down"));
    tb_->addSeparator();
    mDefBut(newbut_,"new",newCB,uiStrings::sNew());
    mDefBut(lockbut_,"unlock",editCB,sEditTxt(false));
    lockbut_->setToggleButton( true );
    uiToolButton* uitb;
    mDefBut(uitb,"save",saveCB,uiStrings::sSave());
    mDefBut(uitb,"contexthelp",helpCB,tr("Help on this window"));
    tb_->addSeparator();
    mDefBut( switchviewbut_, "strat_tree", switchViewCB, tr("Switch View") );
    mDefBut( lithobut_, "lithologies", manLiths,
            uiStrings::phrManage( uiStrings::sLithology(mPlural) ));
    mDefBut( contentsbut_, "contents", manConts,
            uiStrings::phrManage( tr("Content Types") ));

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
    uitree_->treeView()->selectionChanged.notify( selcb );
    uitree_->treeView()->itemRenamed.notify( renmcb );
    uitree_->treeView()->display( false );

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
    const bool expand =
	expandmnuitem_->text().getFullString() == sExpandTxt().getFullString();
    uitree_->expand( expand );
    expandmnuitem_->setText( expand ? sCollapseTxt() : sExpandTxt() );
    expandmnuitem_->setIcon( expand ? "collapse_tree" : "expand_tree" );
    colexpbut_->setIcon( expand ? "collapse_tree" : "expand_tree" );
    colexpbut_->setToolTip( expand ? sCollapseTxt() : sExpandTxt() );
}


void uiStratTreeWin::unitSelCB(CallBacker*)
{
    moveUnitCB(0);
}


void uiStratTreeWin::newCB( CallBacker* )
{
    uiString msg = tr("This will overwrite the current tree. \n"
		      "Your work will be lost. Continue anyway ?");
    if ( RT().isEmpty() || uiMSG().askGoOn( msg ) )
	setNewRT();
}


void uiStratTreeWin::editCB( CallBacker* )
{
    uiString edmenutxt( editmnuitem_->text() );
    uiString edtxt( sEditTxt(true) );
    const bool doedit = edmenutxt.getFullString()==edtxt.getFullString();
    setEditable( doedit );
}


void uiStratTreeWin::setEditable( bool doedit )
{
    uitree_->makeTreeEditable( doedit );
    editmnuitem_->setText( doedit ? sLockTxt(true) : sEditTxt(true) );
    editmnuitem_->setIcon( doedit ? "unlock" : "readonly" );
    lockbut_->setIcon( doedit ? "unlock" : "readonly" );
    lockbut_->setToolTip( doedit ? sLockTxt(false) : sEditTxt(false) );
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
    saveasmnuitem_->setEnabled( !yn );
    resetmnuitem_->setEnabled( !yn );
    moveunitupbut_->setSensitive(
	    !lockbut_->isOn() && uitree_->canMoveUnit( true ) );
    moveunitdownbut_->setSensitive(
	    !lockbut_->isOn()  && uitree_->canMoveUnit( false ) );
}


void uiStratTreeWin::resetCB( CallBacker* )
{
    Strat::setRT( repos_.readTree() );
    Strat::eLVLS().read( repos_.lastSource() );
    Strat::RefTree& bcktree = Strat::eRT();
    //for the time beeing, get back the global tree, but we may want to have
    //a snapshot copy of the actual tree we are working on...
    uitree_->setTree( bcktree, true );
    uitree_->expand( true );
    uistratdisp_->setTree();
    uitree_->setNoChg();
    lvllist_->setNoChg();
    needsave_ = false;
}


void uiStratTreeWin::saveCB( CallBacker* )
{
    Strat::eLVLS().store( Repos::Survey );
    if ( uitree_->tree() )
    {
	repos_.writeTree( *uitree_->tree() );
	uitree_->setNoChg();
	lvllist_->setNoChg();
	needsave_ = false;
    }
    else
	uiMSG().error( tr("Can not find tree") );
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
    const uiString dlgtit = uiStrings::phrSave(tr("the stratigraphy at:"));

    uiDialog savedlg( this, uiDialog::Setup( tr("Save Stratigraphy"),
		    dlgtit, mNoHelpKey ) );
    BufferStringSet bfset( infolvltrs );
    uiListBox saveloclist( &savedlg, "Save strat list" );
    saveloclist.addItems( bfset );
    savedlg.go();
    if ( savedlg.uiResult() == 1 )
    {
	const BufferString savetxt = saveloclist.getText();
	Repos::Source src = Repos::Survey;
	if ( savetxt == infolvltrs[1] )
	    src = Repos::Data;
	else if ( savetxt == infolvltrs[2] )
	    src = Repos::User;
	else if ( savetxt == infolvltrs[3] )
	{
	    if ( !GetApplSetupDir() )
	    {
		BufferString envvarstr;
#ifdef __win__
		envvarstr = "'DTECT_WINAPPL_SETUP'";
#endif
		if ( envvarstr.isEmpty() )
		    envvarstr = "'DTECT_APPL_SETUP'";
		return uiMSG().error(
			tr("You need to set %1 to save in global level")
				.arg(envvarstr) );
	    }
	    src = Repos::ApplSetup;
	}

	repos_.writeTree( Strat::RT(), src );
	Strat::eLVLS().store( src );
    }
}


void uiStratTreeWin::switchViewCB( CallBacker* )
{
    istreedisp_ = !istreedisp_;
    uistratdisp_->display( !istreedisp_ );
    if ( uistratdisp_->control() )
	uistratdisp_->control()->setSensitive( !istreedisp_ );
    uitree_->treeView()->display( istreedisp_ );
    switchviewbut_->setIcon(
		istreedisp_ ? "stratframeworkgraph" : "strat_tree" );
    colexpbut_->setSensitive( istreedisp_ );
}


void uiStratTreeWin::unitRenamedCB( CallBacker* )
{
    needsave_ = true;
    //TODO requires Qt4 to approve/cancel renaming ( name already in use...)
}


bool uiStratTreeWin::closeOK()
{
    const bool needsave = uitree_->anyChg() || needsave_ ||  lvllist_->anyChg();
    uitree_->setNoChg();
    lvllist_->setNoChg();
    needsave_ = false;

    if ( needsave )
    {
	int res = uiMSG().askSave( tr("Stratigraphic framework has changed\n"
                                      " Do you want to save it?") );

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


void uiStratTreeWin::appCloseCB( CallBacker* )
{
    IOM().applicationClosing.remove( mCB(this,uiStratTreeWin,appCloseCB ) );
    if ( stratwin )
    {
	stratwin->close();
	delete stratwin;
	stratwin = 0;
    }
}


void uiStratTreeWin::survChgCB( CallBacker* )
{
    IOM().surveyChanged.remove( mCB(this,uiStratTreeWin,survChgCB ) );
    if ( stratwin )
	stratwin->close();

    OBJDISP()->go( uistratdisp_ );

    delete lvllist_;
    delete uitree_;
    stratwin = 0;
}


void uiStratTreeWin::moveUnitCB( CallBacker* cb )
{
    if ( cb)
	uitree_->moveUnit( cb == moveunitupbut_ );

    moveunitupbut_->setSensitive(
	    !lockbut_->isOn() && uitree_->canMoveUnit( true ) );
    moveunitdownbut_->setSensitive(
	    !lockbut_->isOn()  && uitree_->canMoveUnit( false ) );
}


void uiStratTreeWin::helpCB( CallBacker* )
{
    HelpProvider::provideHelp( HelpKey(mODHelpKey(mStratTreeWinHelpID) ) );
}


void uiStratTreeWin::manLiths( CallBacker* )
{
    uiStratLithoDlg dlg( this );
    dlg.go();
    if ( dlg.anyChg() ) needsave_ = true;
    uitree_->updateLithoCol();
}


void uiStratTreeWin::manConts( CallBacker* )
{
    uiStratContentsDlg dlg( this );
    dlg.go();
    if ( dlg.anyChg() ) needsave_ = true;
}


void uiStratTreeWin::changeLayerModelNumber( bool add )
{
    mDefineStaticLocalObject( int, nrlayermodelwin, = 0 );
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


void uiStratTreeWin::makeEditable( bool yn )
{
    if ( stratwin )
	stratwin->setEditable( yn );
}

