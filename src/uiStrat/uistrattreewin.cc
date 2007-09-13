/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene
 Date:          July 2007
 RCS:		$Id: uistrattreewin.cc,v 1.16 2007-09-13 14:36:12 cvshelene Exp $
________________________________________________________________________

-*/

#include "uistrattreewin.h"

#include "compoundkey.h"
#include "stratlevel.h"
#include "stratunitrepos.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilistbox.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uisplitter.h"
#include "uistratreftree.h"
#include "uistratutildlgs.h"

#define	mExpandTxt	"&Expand all"
#define	mCollapseTxt	"&Collapse all"
#define	mEditTxt	"&Edit"
#define	mLockTxt	"&Lock"


static const char* sNoLevelTxt      = "--- Empty ---";

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
    : uiMainWin(p,"Manage Stratigraphy", 0, true, false)
    , tmptree_( 0 )
{
    createMenus();
    createGroups();
    setExpCB(0);
    editCB(0);
}


uiStratTreeWin::~uiStratTreeWin()
{
    delete uitree_;
    if ( tmptree_ ) delete tmptree_;
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
    uitree_->itemAdded_.notify( mCB( this,uiStratTreeWin,unitAddedCB ) );
    uitree_->itemToBeRemoved_.notify( mCB( this,uiStratTreeWin,unitToBeDelCB ));
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
    if ( doedit && !tmptree_ )
	createTmpTree();
}


void uiStratTreeWin::createTmpTree()
{
    const Strat::RefTree* rt = &Strat::RT();
    tmptree_ = new Strat::RefTree( rt->treeName(), rt->source() );
    for ( int idx=0; idx<rt->nrLevels(); idx++ )
    {
	Level* lvl = new Level( *rt->level(idx) );
	tmptree_->addLevel( lvl );
    }

    UnitRef::Iter it( *rt );
    const UnitRef& firstun = *it.unit();
    tmptree_->addCopyOfUnit( firstun );
    while ( it.next() )
	tmptree_->addCopyOfUnit( *it.unit() );
}


#define mSaveAtLoc( loc )\
{\
    if ( tmptree_ )\
    {\
	eUnRepo().replaceTree( tmptree_ );\
	createTmpTree();\
    }\
    const_cast<UnitRepository*>(&UnRepo())->copyCurTreeAtLoc( loc );\
    UnRepo().write( loc );\
}\
    

void uiStratTreeWin::saveCB( CallBacker* )
{
    mSaveAtLoc( Repos::Survey );
}


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
	if ( !strcmp( savetxt, infolvltrs[1] ) )
	    mSaveAtLoc( Repos::Data )
	else if ( !strcmp( savetxt, infolvltrs[2] ) )
	    mSaveAtLoc( Repos::User )
	else if ( !strcmp( savetxt, infolvltrs[3] ) )
	    mSaveAtLoc( Repos::ApplSetup )
	else
	    mSaveAtLoc( Repos::Survey )
    }
}


void uiStratTreeWin::resetCB( CallBacker* )
{
    uitree_->setTree( &Strat::RT(), true );
    uitree_->expand( true );
    if ( tmptree_ )
	{ delete tmptree_; tmptree_ = 0; }
    
    if ( !strcmp( editmnuitem_->text(), mEditTxt ) )
	createTmpTree();
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
    mnu.insertItem( new uiMenuItem("Create New ..."), 0 );
    if ( curit>-1 && !lvllistfld_->box()->isPresent( sNoLevelTxt ) )
    {
	mnu.insertItem( new uiMenuItem("Edit ..."), 1 );
	mnu.insertItem( new uiMenuItem("Remove"), 2 );
    }
    const int mnuid = mnu.exec();
    if ( mnuid<0 || mnuid>2 ) return;
    if ( mnuid == 2 )
    {
	const Level* curlvl = RT().getLevel( lvllistfld_->box()->getText() );
	tmptree_->remove( curlvl );
	lvllistfld_->box()->removeItem( lvllistfld_->box()->currentItem() );
	return;
    }

    editLevel( mnuid ? false : true );
}


void uiStratTreeWin::fillLvlList()
{
    lvllistfld_->box()->empty();
    int nrlevels = RT().nrLevels();
    for ( int idx=0; idx<nrlevels; idx++ )
    {
	const Level* lvl = RT().level( idx );
	if ( !lvl ) return;
	lvllistfld_->box()->addItem( lvl->name(), lvl->color_ );
    }
    if ( !nrlevels )
	lvllistfld_->box()->addItem( sNoLevelTxt );
}


BufferString uiStratTreeWin::getCodeFromLVIt( const uiListViewItem* item ) const
{
    BufferString bs = item->text();
    int itemdepth = item->depth();
    for ( int idx=itemdepth-1; idx>=0; idx-- )
    {
	item = item->parent();
	CompoundKey kc( item->text() );
	kc += bs.buf();
	bs = kc.buf();
    }

    return bs;
}


void uiStratTreeWin::editLevel( bool create )
{
    uiStratLevelDlg newlvldlg( this );
    Level* curlvl = 0;
    if ( !create && lvllistfld_->box()->currentItem()>-1 )
    {
	curlvl = const_cast<Level*> ( tmptree_->getLevel( lvllistfld_->box()
		    						->getText() ) );
	if ( curlvl )
	{
	    newlvldlg.lvlnmfld_->setText( curlvl->name() );
	    bool isiso = curlvl->isTimeLevel();
	    newlvldlg.lvltvstrgfld_->setValue(isiso);
	    if ( isiso )
		newlvldlg.lvltimefld_->setValue( curlvl->timerg_.start );
	    else
		newlvldlg.lvltimergfld_->setValue( curlvl->timerg_ );
	    
	    newlvldlg.lvlcolfld_->setColor( curlvl->color_ );
	}
    }
    if ( newlvldlg.go() )
	fillInLvlPars( curlvl, newlvldlg, create );
}


void uiStratTreeWin::fillInLvlPars( Level* curlvl, 
				    const uiStratLevelDlg& newlvldlg,
				    bool create )
{
    if ( !curlvl )
    {
	curlvl = new Level( newlvldlg.lvlnmfld_->text(), 0, false );
	tmptree_->addLevel( curlvl );
    }
    else
	curlvl->setName( newlvldlg.lvlnmfld_->text() );

    curlvl->color_ = newlvldlg.lvlcolfld_->color();
    if ( newlvldlg.lvltvstrgfld_->getBoolValue() )
    {
	curlvl->timerg_.start = newlvldlg.lvltimefld_->getfValue();
	curlvl->timerg_.stop = newlvldlg.lvltimefld_->getfValue();
    }
    else
	curlvl->timerg_ = newlvldlg.lvltimergfld_->getFInterval();
    
    if ( create )
    {
	lvllistfld_->box()->addItem( curlvl->name(), curlvl->color_ );
	if ( lvllistfld_->box()->isPresent( sNoLevelTxt ) )
	    lvllistfld_->box()->removeItem( 0 );
    }
    else
    {
	int curit = lvllistfld_->box()->currentItem();
	lvllistfld_->box()->setItemText( curit, curlvl->name() );
	lvllistfld_->box()->setPixmap( curit, curlvl->color_ );
    }
}


void uiStratTreeWin::unitRenamedCB( CallBacker* )
{
    //TODO requires Qt4 to approve/cancel renaming ( name already in use...)
}


void uiStratTreeWin::unitAddedCB( CallBacker* )
{
    //TODO attention pb subunit if unit has litho
    prepareParentUnit();
    addUnit();
}


void uiStratTreeWin::prepareParentUnit()
{
    uiListViewItem* curit = uitree_->listView()->currentItem();
    if ( !curit ) return;
    
    uiListViewItem* parit = curit->parent();
    if ( !parit ) return;

    UnitRef* parun = tmptree_->find( getCodeFromLVIt( parit ) );
    if ( !parun || !parun->isLeaf() ) return;

    NodeUnitRef* upnode = parun->upNode();
    if ( !upnode ) return;
    
    NodeUnitRef* nodeun = new NodeUnitRef( upnode, parun->code(),
	    				   parun->description() );
    for ( int idx=0; idx<parun->nrProperties(); idx++ )
	((UnitRef*)nodeun)->add( parun->property(idx) );

    Level* toplvl = const_cast<Level*>(tmptree_->getLevel( parun, true ));
    if ( toplvl )
	toplvl->unit_ = nodeun;
    
    Level* baselvl = const_cast<Level*>(tmptree_->getLevel( parun, false ));
    if ( baselvl )
	baselvl->unit_ = nodeun;
    
    int parunidx = upnode->indexOf( upnode->find( parun->code() ) );
    delete upnode->replace( parunidx, nodeun );
}


void uiStratTreeWin::addUnit()
{
    uiListViewItem* curit = uitree_->listView()->currentItem();
    if ( !curit ) return;

    BufferString unitdesc;
    const char* lithotxt = curit->text(2);
    if ( lithotxt )
    {
	int lithid = UnRepo().getLithID( lithotxt );
	if ( lithid >-1 )
	{
	    unitdesc = lithid;
	    unitdesc += "`";
	}
    }
    unitdesc += curit->text(1);
    tmptree_->addUnit( getCodeFromLVIt( curit ), unitdesc, true );
}


void uiStratTreeWin::unitToBeDelCB( CallBacker* )
{
    uiListViewItem* curit = uitree_->listView()->currentItem();
    if ( !curit ) return;

    UnitRef* ur = tmptree_->find( getCodeFromLVIt( curit ) );
    tmptree_->untieLvlsFromUnit( ur, true );

    NodeUnitRef* upnode = ur->upNode();
    if ( upnode )
	upnode->remove( upnode->indexOf(ur) );
    else
	((NodeUnitRef*)tmptree_)->remove( tmptree_->indexOf( ur ) );
}



