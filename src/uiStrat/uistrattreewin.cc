/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistrattreewin.h"

#include "compoundkey.h"
#include "file.h"
#include "filepath.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "oddirs.h"
#include "objdisposer.h"
#include "od_helpids.h"
#include "survinfo.h"
#include "stratreftree.h"
#include "strattreetransl.h"
#include "stratunitrepos.h"

#include "uicolor.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uiioobjseldlg.h"
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
#include "uitreeview.h"

using namespace Strat;

ManagedObjectSet<uiToolButtonSetup> uiStratTreeWin::tbsetups_;

static uiStratTreeWin* stratwin = nullptr;
const uiStratTreeWin& StratTWin()
{
    if ( !stratwin )
	stratwin = new uiStratTreeWin( uiMain::instance().topLevel() );

    return *stratwin;
}


uiStratTreeWin& StratTreeWin()
{
    return const_cast<uiStratTreeWin&>( StratTWin() );
}


static void saveAsDefaultTree( const MultiID& key )
{
    if ( key.isUdf() )
	return;

    SI().getPars().set( Strat::sKeyDefaultTree(), key );
    SI().savePars();
}


uiStratTreeWin::uiStratTreeWin( uiParent* p )
    : uiMainWin(p,uiStrings::phrManage(uiStrings::sStratigraphy()),3)
    , repos_(*new Strat::RepositoryAccess())
{
    createGroups();
    initMenuItems();
    createMenu();
    createToolBar();
    setIsLocked( false );
    updateButtonSensitivity();

    mAttachCB( IOM().surveyToBeChanged, uiStratTreeWin::survChgCB );
    mAttachCB( postFinalize(), uiStratTreeWin::finalizeCB );
}


uiStratTreeWin::~uiStratTreeWin()
{
    detachAllNotifiers();
    delete& repos_;
}


void uiStratTreeWin::finalizeCB( CallBacker* )
{
    initWin();
}


static int sMnuID = 0;

void uiStratTreeWin::initItem( MenuItem& itm, const uiString& txt,
			       const char* icnnm )
{
    itm.text = txt;
    itm.iconfnm = icnnm;
    itm.cb = mCB(this,uiStratTreeWin,actionCB);
    itm.id = sMnuID++;
}


void uiStratTreeWin::initMenuItems()
{
    sMnuID = 0;
    initItem( newitem_, uiStrings::sNew(), "new" );
    initItem( openitem_, uiStrings::sOpen(), "open" );
    initItem( defaultitem_, tr("Open Default"), "defset" );
    initItem( saveitem_, uiStrings::sSave(), "save" );
    initItem( saveasitem_, uiStrings::sSaveAs(), "saveas" );
    initItem( resetitem_, uiStrings::sReset(), "undo" );
    initItem( lockitem_, uiStrings::sLock(), "unlock" );
    initItem( switchviewitem_, tr("Switch to tree view"),
	      "stratframeworkgraph" );

    initItem( expanditem_, tr("Expand all"), "expand_tree" );
    initItem( collapseitem_, tr("Collapse all"), "collapse_tree" );
    initItem( moveupitem_, tr("Move unit up"), "uparrow" );
    initItem( movedownitem_, tr("Move unit down"), "downarrow" );

    initItem( lithoitem_, uiStrings::phrManage(uiStrings::sLithology(mPlural)),
	      "lithologies" );
    initItem( contentsitem_, uiStrings::phrManage(tr("Content Types")),
	      "contents" );
    initItem( helpitem_, tr("Help on this window"), "contexthelp" );
}


void uiStratTreeWin::initWin()
{
    saveLegacyTrees();

    if ( Strat::RT().isEmpty() )
    {
	MultiID key = MultiID::udf();
	SI().pars().get( Strat::sKeyDefaultTree(), key );
	if ( key.isUdf() )
	    return;

	treekey_ = key;
	readTree( treekey_ );
    }

    updateCaption();
}


void uiStratTreeWin::saveLegacyTrees()
{
    bool showmsg = false;
    uiString msg = tr("Legacy Stratigraphic Frameworks found in:");

    const char* stratunitsstr = "StratUnits";
    const char* stratlevelsstr = "StratLevels";
    for ( int idx=0; idx<3; idx++ )
    {
	Repos::Source src = Repos::User;
	if ( idx==1 ) src = Repos::Survey;
	if ( idx==2 ) src = Repos::Data;

	PtrMan<RefTree> rt = repos_.readTree( src );
	if ( !rt || !rt->hasChildren() )
	    continue;

	CtxtIOObj ctio( StratTreeTranslatorGroup::ioContext() );
	ctio.ioobj_ = nullptr;
	const char* locstr = idx==0 ? "Home" : (idx==1 ? "Survey" : "DataRoot");
	ctio.setName( BufferString("Stratigraphy from ",locstr," folder") );
	ctio.fillObj();
	if ( !ctio.ioobj_ )
	    continue;

	const bool writeok = repos_.write( *rt, ctio.ioobj_->key() );
	if ( !writeok )
	    continue;

	showmsg = true;

	Repos::FileProvider unitsrfp( stratunitsstr );
	const FilePath unitsfp( unitsrfp.fileName(src) );
	Repos::FileProvider levelsrfp( stratlevelsstr );

	msg.append( tr("\n\n%1\nSaved as: %2").arg(unitsfp.pathOnly())
					      .arg(ctio.ioobj_->name()) );

	unitsrfp.removeFile( src );
	levelsrfp.removeFile( src );
    }

    if ( showmsg )
	uiMSG().message( msg );
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
}


void uiStratTreeWin::createToolBar()
{
    tb_ = new uiToolBar( this, tr("Stratigraphy Manager Tools") );
    tb_->addButton( newitem_ );
    tb_->addButton( openitem_ );
    tb_->addButton( defaultitem_ );
    tb_->addButton( saveitem_ );
    tb_->addButton( saveasitem_ );
    tb_->addButton( resetitem_ );
    tb_->addButton( lockitem_ );
    tb_->addButton( switchviewitem_ );

    stratvwtb_ = new uiToolBar( this, tr("Strat View Tools") );
    uistratdisp_->addControl( stratvwtb_ );

    treevwtb_ = new uiToolBar( this, tr("Tree View Tools") );
    treevwtb_->addButton( collapseitem_ );
    treevwtb_->addButton( expanditem_ );
    treevwtb_->addButton( moveupitem_ );
    treevwtb_->addButton( movedownitem_ );

    othertb_ = new uiToolBar( this, tr("Other Tools") );
    othertb_->addButton( lithoitem_ );
    othertb_->addButton( contentsitem_ );
    for ( int idx=0; idx<tbsetups_.size(); idx++ )
	othertb_->addButton( *tbsetups_[idx] );

    othertb_->addButton( helpitem_ );
}


void uiStratTreeWin::createGroups()
{
    uiGroup* leftgrp = new uiGroup( this, "LeftGroup" );
    leftgrp->setStretch( 1, 1 );
    uiGroup* rightgrp = new uiGroup( this, "RightGroup" );
    rightgrp->setStretch( 1, 1 );

    uitree_ = new uiStratRefTree( leftgrp );
    uitree_->treeView()->selectionChanged.notify(
			mCB(this,uiStratTreeWin,unitSelCB) );
    uitree_->treeView()->itemRenamed.notify(
			mCB(this,uiStratTreeWin,unitRenamedCB) );
    uitree_->treeView()->display( false );

    if ( !uitree_->haveTimes() )
	uitree_->setEntranceDefaultTimes();

    uistratdisp_ = new uiStratDisplay( leftgrp, *uitree_ );

    lvllist_ = new uiStratLvlList( rightgrp );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", true );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );
}


void uiStratTreeWin::setNewRT()
{
    BufferStringSet opts;
    Strat::RefTree::getStdNames( opts );

    const bool nortpresent = RT().isEmpty();
    uiString dlgmsg = tr("Stratigraphy: %1")
		    .arg(nortpresent ? tr("select initial")
				     : tr("select new"));
    uiSelectFromList::Setup su( dlgmsg, opts );
    uiSelectFromList dlg( this, su );
    if ( nortpresent )
	dlg.setButtonText( uiDialog::CANCEL, uiStrings::sEmptyString() );
    if ( !dlg.go() )
	return;

    const char* nm = opts.get( dlg.selection() );
    Strat::RefTree* rt = 0;
    Strat::LevelSet* ls = Strat::LevelSet::createStd( nm );
    if ( !ls )
	{ pErrMsg( "Cannot read LevelSet from Std!" ); return; }
    else
    {
	rt = Strat::RefTree::createStd( nm );
	if ( !rt )
	    { pErrMsg( "Cannot read RefTree from Std!" ); return; }
    }

    Strat::lvlSetMgr().setLVLS( ls );
    Strat::setRT( rt );
    needsave_ = true;
    updateDisplay();
}


void uiStratTreeWin::updateDisplay()
{
    uitree_->setTree();
    uitree_->expand( true );
    uistratdisp_->setTree();
    uitree_->setNoChg();
    lvllist_->setLevels();
    lvllist_->setNoChg();

    updateCaption();
}


void uiStratTreeWin::updateCaption()
{
    uiString capt = uiStrings::phrManage(uiStrings::sStratigraphy());
    if ( !Strat::RT().name_.isEmpty() )
	capt.append( " - " ).append( Strat::RT().name_ );

    setCaption( capt );
}


void uiStratTreeWin::actionCB( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,action,cb)
    const int id = action ? action->getID() : -1;

    if ( id==newitem_.id )
	newTree();
    else if ( id==openitem_.id )
	openTree();
    else if ( id==defaultitem_.id )
	defaultTree();
    else if ( id==saveitem_.id )
	save( false );
    else if ( id==saveasitem_.id )
	save( true );
    else if ( id==resetitem_.id )
	reset();
    else if ( id==lockitem_.id )
	setIsLocked( !isLocked() );
    else if ( id==switchviewitem_.id )
	switchView();
    else if ( id==expanditem_.id )
	uitree_->expand( true );
    else if ( id==collapseitem_.id )
	uitree_->expand( false );
    else if ( id==moveupitem_.id )
	uitree_->moveUnit( true );
    else if ( id==movedownitem_.id )
	uitree_->moveUnit( false );
    else if ( id==lithoitem_.id )
	manLiths();
    else if ( id==contentsitem_.id )
	manConts();
    else if ( id==helpitem_.id )
	HelpProvider::provideHelp( HelpKey(mODHelpKey(mStratTreeWinHelpID) ) );
}


void uiStratTreeWin::unitSelCB( CallBacker* )
{
    updateButtonSensitivity();
}


void uiStratTreeWin::newTree()
{
    if ( !askSave() )
	return;

    Strat::lvlSetMgr().setLVLS( new Strat::LevelSet );
    Strat::setRT( new Strat::RefTree );
    needsave_ = true;
    treekey_.setUdf();

    updateDisplay();
}


void uiStratTreeWin::openTree()
{
    if ( !askSave() )
	return;

    CtxtIOObj ctio( StratTreeTranslatorGroup::ioContext() );
    ctio.ctxt_.forread_ = true;
    ctio.fillDefault();
    uiIOObjSelDlg dlg( this, ctio, tr("Open Stratigraphic Framework") );
    if ( !dlg.go() || !dlg.ioObj() )
	return;

    treekey_ = dlg.ioObj()->key();
    delete ctio.ioobj_;

    readTree( treekey_ );
    saveAsDefaultTree( treekey_ );
}


void uiStratTreeWin::readTree( const MultiID& key )
{
    Strat::RefTree* tree = repos_.read( key );
    Strat::setRT( tree );

    Strat::LevelSet* levels = LevelSet::read( key );
    Strat::lvlSetMgr().setLVLS( levels );

    updateDisplay();
}


void uiStratTreeWin::defaultTree()
{
    if ( !askSave() )
	return;

    setNewRT();
}


bool uiStratTreeWin::askSave()
{
    const bool needsave = uitree_->anyChg() || needsave_ ||  lvllist_->anyChg();
    uitree_->setNoChg();
    lvllist_->setNoChg();

    if ( needsave )
    {
	const int res = uiMSG().askSave(
				tr("Stratigraphic framework has changed."
				"\n\nDo you want to save it?") );
	if ( res == 1 )
	    return save( false );
	else if ( res == 0 )
	{
	    reset();
	    return true;
	}
	else if ( res == -1 )
	    return false;
    }

    return true;
}


bool uiStratTreeWin::save( bool saveas )
{
    MultiID key = treekey_;
    if ( key.isUdf() || saveas )
    {
	CtxtIOObj ctio( StratTreeTranslatorGroup::ioContext() );
	ctio.ctxt_.forread_ = false;
	uiIOObjSelDlg dlg( this, ctio, tr("Save Stratigraphic Framework") );
	if ( !dlg.go() || !dlg.ioObj() )
	    return false;

	key = dlg.ioObj()->key();
	delete ctio.ioobj_;
    }

    const Strat::LevelSet& levelset = Strat::LVLS();
    bool saveok = false;
    uitree_->setName( IOM().nameOf(key) );
    saveok = LevelSet::write(levelset,key) &&
	     repos_.write(*uitree_->tree(),key);
    if ( saveok )
	treekey_ = key;

    if ( saveok )
    {
	needsave_ = false;
	uitree_->setNoChg();
	lvllist_->setNoChg();
    }
    else
	uiMSG().warning( tr("Stratigraphic framework not saved") );

    updateCaption();

    return saveok;
}


void uiStratTreeWin::setEditable( bool yn )
{
    setIsLocked( !yn );
    tb_->setSensitive( lockitem_.id, yn );
}


void uiStratTreeWin::setIsLocked( bool yn )
{
    uistratdisp_->setIsLocked( yn );
    lvllist_->setIsLocked( yn );

    uitree_->makeTreeEditable( !yn );
    tb_->setIcon( lockitem_.id, yn ? "readonly" : "unlock" );
    tb_->setToolTip( lockitem_.id,
		     yn ? uiStrings::sUnlock() : uiStrings::sLock() );

    updateButtonSensitivity();
}


bool uiStratTreeWin::isLocked() const
{ return uistratdisp_->isLocked(); }


void uiStratTreeWin::reset()
{
// TODO: Read from MultiID

//    Strat::setRT( repos_.readTree() );
//    Strat::eLVLS().read( repos_.lastSource() );
//    Strat::RefTree& bcktree = Strat::eRT();
    //for the time beeing, get back the global tree, but we may want to have
    //a snapshot copy of the actual tree we are working on...

    updateDisplay();
    needsave_ = false;
}


void uiStratTreeWin::switchView()
{
    istreedisp_ = !istreedisp_;
    uistratdisp_->display( !istreedisp_ );
    if ( uistratdisp_->control() )
	uistratdisp_->control()->setSensitive( !istreedisp_ );

    uitree_->treeView()->display( istreedisp_ );
    treevwtb_->setSensitive( istreedisp_ );

    tb_->setIcon( switchviewitem_.id,
		istreedisp_ ? "strat_tree" : "stratframeworkgraph" );
    tb_->setToolTip( switchviewitem_.id, istreedisp_ ?
		tr("Switch to graph view") : tr("Switch to tree view") );
}


void uiStratTreeWin::unitRenamedCB( CallBacker* )
{
    needsave_ = true;
    //TODO requires Qt4 to approve/cancel renaming ( name already in use...)
}


bool uiStratTreeWin::closeOK()
{
    return askSave();
}


void uiStratTreeWin::survChgCB( CallBacker* )
{
    IOM().surveyToBeChanged.remove( mCB(this,uiStratTreeWin,survChgCB ) );
    if ( stratwin )
	stratwin->close();

    OBJDISP()->go( uistratdisp_ );

    deleteAndZeroPtr( lvllist_ );
    deleteAndZeroPtr( uitree_ );
    stratwin = nullptr;
}


void uiStratTreeWin::updateButtonSensitivity()
{
    const bool islocked = isLocked();
    treevwtb_->setSensitive( moveupitem_.id,
		       !islocked && uitree_->canMoveUnit(true) );
    treevwtb_->setSensitive( movedownitem_.id,
		       !islocked && uitree_->canMoveUnit(false) );

    tb_->setSensitive( newitem_.id, !islocked );
    tb_->setSensitive( openitem_.id, !islocked );
    tb_->setSensitive( defaultitem_.id, !islocked );
    tb_->setSensitive( saveitem_.id, !islocked );

    othertb_->setSensitive( lithoitem_.id, !islocked );
    othertb_->setSensitive( contentsitem_.id, !islocked );
}


void uiStratTreeWin::manLiths()
{
    uiStratLithoDlg dlg( this );
    dlg.go();
    if ( dlg.anyChg() )
	needsave_ = true;
    uitree_->updateLithoCol();
}


void uiStratTreeWin::manConts()
{
    uiStratContentsDlg dlg( this );
    dlg.go();
    if ( dlg.anyChg() )
	needsave_ = true;
}


void uiStratTreeWin::changeLayerModelNumber( bool add )
{
    mDefineStaticLocalObject( int, nrlayermodelwin, = 0 );
    bool haschgd = false;
    if ( add )
    {
	nrlayermodelwin ++;
	if ( nrlayermodelwin == 1 )
	    haschgd = true;
    }
    else
    {
	nrlayermodelwin --;
	if ( nrlayermodelwin == 0 )
	    haschgd = true;
    }
    if ( haschgd )
    {
    /*
	const bool islocked = lockbut_->isOn();
	if ( (!islocked && nrlayermodelwin) || (islocked && !nrlayermodelwin) )
	    { lockbut_->setOn( nrlayermodelwin ); editCB(0); }
	lockbut_->setSensitive( !nrlayermodelwin );
	editmnuitem_->setEnabled( !nrlayermodelwin );
    */
    }
}


void uiStratTreeWin::makeEditable( bool yn )
{
    if ( stratwin )
	stratwin->setEditable( yn );
}
