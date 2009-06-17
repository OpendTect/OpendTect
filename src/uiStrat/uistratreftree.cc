/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          June 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratreftree.cc,v 1.33 2009-06-17 13:00:44 cvssatyaki Exp $";

#include "uistratreftree.h"

#include "pixmap.h"
#include "stratreftree.h"
#include "stratunitref.h"
#include "stratunitrepos.h"
#include "uigeninput.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uirgbarray.h"
#include "uistratmgr.h"
#include "uistratutildlgs.h"

#define mAddCol(wdth,nr) \
    lv_->setColumnWidth( nr, wdth )

#define PMWIDTH		11
#define PMHEIGHT	9

static const int sUnitsCol	= 0;
static const int sDescCol	= 1;
static const int sLithoCol	= 2;

using namespace Strat;

uiStratRefTree::uiStratRefTree( uiParent* p, uiStratMgr* uistratmgr )
    : tree_(0)
    , uistratmgr_(uistratmgr)
{
    lv_ = new uiListView( p, "RefTree viewer" );
    BufferStringSet labels;
    labels.add( "Unit" );
    labels.add( "Description" );
    labels.add( "Lithology" );
    lv_->addColumns( labels );
    mAddCol( 300, 0 );
    mAddCol( 200, 1 );
    mAddCol( 150, 2 );
    lv_->setPrefWidth( 650 );
    lv_->setPrefHeight( 400 );
    lv_->setStretch( 2, 2 );
    lv_->rightButtonClicked.notify( mCB( this,uiStratRefTree,rClickCB ) );

    Strat::UnRepo().changed.notify( mCB(this,uiStratRefTree,repoChangedCB) );
    setTree( uistratmgr_->getCurTree() );
}


uiStratRefTree::~uiStratRefTree()
{
    delete lv_;
}


void uiStratRefTree::setTree( const RefTree* rt, bool force )
{
    if ( !force && rt == tree_ ) return;

    tree_ = rt;
    if ( !tree_ ) return;

    lv_->clear();
    addNode( 0, *((NodeUnitRef*)tree_), true );
}


void uiStratRefTree::addNode( uiListViewItem* parlvit,
			      const NodeUnitRef& nur, bool root )
{
    uiListViewItem* lvit = parlvit
	? new uiListViewItem( parlvit, uiListViewItem::Setup()
				.label(nur.code()).label(nur.description()) )
	: root ? 0 : new uiListViewItem( lv_,uiListViewItem::Setup()
				.label(nur.code()).label(nur.description()) );

    if ( parlvit || !root )
    {
	ioPixmap* pm = createLevelPixmap( &nur );
	lvit->setPixmap( 0, *pm );
	delete pm;
    }
    
    for ( int iref=0; iref<nur.nrRefs(); iref++ )
    {
	const UnitRef& ref = nur.ref( iref );
	if ( ref.isLeaf() )
	{
	    uiListViewItem* item;
	    mDynamicCastGet(const LeafUnitRef*,lur,&ref);
	    if ( !lur ) continue;
	    uiListViewItem::Setup setup = uiListViewItem::Setup()
				.label( lur->code() )
				.label( lur->description() )
				.label( uistratmgr_->getLithName(*lur) );
	    if ( lvit )
		item = new uiListViewItem( lvit, setup );
	    else
		item = new uiListViewItem( lv_, setup );
	    
	    ioPixmap* pm = createLevelPixmap( lur );
	    item->setPixmap( 0, *pm );
	    delete pm;
	}
	else
	{
	    mDynamicCastGet(const NodeUnitRef*,chldnur,&ref);
	    if ( chldnur )
		addNode( lvit, *chldnur, false );
	}
    }
}


void uiStratRefTree::expand( bool yn ) const
{
    if ( yn )
	lv_->expandAll();
    else
	lv_->collapseAll();
}


void uiStratRefTree::makeTreeEditable( bool yn ) const
{
    uiListViewItem* lvit = lv_->firstItem();
    while ( lvit )
    {
	lvit->setRenameEnabled( sUnitsCol, false );  //TODO
	lvit->setRenameEnabled( sDescCol, false );   //TODO
	lvit->setRenameEnabled( sLithoCol, false );
	lvit->setDragEnabled( yn );
	lvit->setDropEnabled( yn );
	lvit = lvit->itemBelow();
    }
}


void uiStratRefTree::repoChangedCB( CallBacker* )
{
    setTree( uistratmgr_->getCurTree() );
}


void uiStratRefTree::rClickCB( CallBacker* )
{
    uiListViewItem* lvit = lv_->itemNotified();
    if ( !lvit || !lvit->dragEnabled() ) return;

    int col = lv_->columnNotified();
    uiPopupMenu mnu( lv_->parent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Specify level boundary ..."), 0 );
    mnu.insertSeparator();
    mnu.insertItem( new uiMenuItem("&Create sub-unit..."), 1 );
    mnu.insertItem( new uiMenuItem("&Add unit ..."), 2 );
    mnu.insertItem( new uiMenuItem("&Remove"), 3 );
    mnu.insertItem( new uiMenuItem("&Properties..."), 4 );

    const int mnuid = mnu.exec();
    if ( mnuid<0 ) return;
    else if ( mnuid==0 )
	selBoundary();
    else if ( mnuid==1 )
	insertSubUnit( lvit );
    else if ( mnuid==2 )
	insertSubUnit( 0 );
    else if ( mnuid==3 )
	removeUnit( lvit );
    else if ( mnuid==4 )
	updateUnitProperties( lvit );
}


void uiStratRefTree::insertSubUnit( uiListViewItem* lvit )
{
    uiStratUnitDlg newurdlg( lv_->parent(), uistratmgr_ );
    if ( newurdlg.go() )
    {
	uiListViewItem* newitem;
	uiListViewItem::Setup setup = uiListViewItem::Setup()
				    .label( newurdlg.getUnitName() )
				    .label( newurdlg.getUnitDesc() )
				    .label( newurdlg.getUnitLith() );
	if ( lvit )
	    newitem = new uiListViewItem( lvit, setup );
	else
	    newitem = new uiListViewItem( lv_, setup );

	newitem->setRenameEnabled( sUnitsCol, false );	//TODO
	newitem->setRenameEnabled( sDescCol, false );	//TODO
	newitem->setRenameEnabled( sLithoCol, false );
	newitem->setDragEnabled( true );
	newitem->setDropEnabled( true );
	ioPixmap* pm = createLevelPixmap(0);
	newitem->setPixmap( 0, *pm );
	delete pm;
	
	if ( newitem->parent() )
	    newitem->parent()->setOpen( true );

	lv_->setCurrentItem( newitem );
	uiListViewItem* parit = newitem->parent();
	if ( parit )
	    uistratmgr_->prepareParentUnit( getCodeFromLVIt( parit ).buf() );
	
	BufferString codestr = getCodeFromLVIt( newitem );
	BufferString description = newitem->text(1);
	BufferString lithonm = newitem->text(2);
	uistratmgr_->addUnit( codestr.buf(), lithonm, description, false );
    }
}


void uiStratRefTree::removeUnit( uiListViewItem* lvit )
{
    if ( !lvit ) return;
    uistratmgr_->removeUnit( getCodeFromLVIt( lvit ).buf() );
    if ( lvit->parent() )
	lvit->parent()->removeItem( lvit );
    else
    {
	lv_->takeItem( lvit );
	delete lvit;
    }

    lv_->triggerUpdate();
}


void uiStratRefTree::updateUnitProperties( uiListViewItem* lvit )
{
    uiStratUnitDlg urdlg( lv_->parent(), uistratmgr_ );
    urdlg.setTitleText("Update Unit Properties");
    BufferString uncode = getCodeFromLVIt( lvit);
    const UnitRef* unitref = uistratmgr_->getCurTree()->find( uncode );
    if ( unitref )
	urdlg.setUnitIsLeaf( unitref->isLeaf() );

    urdlg.setUnitName( lvit->text(0) ); 
    urdlg.setUnitDesc( lvit->text(1) ); 
    urdlg.setUnitLith( lvit->text(sLithoCol) ); 
    if ( urdlg.go() )
    {
	//TODO will require an update of all children
	//lvit->setText( urdlg.getUnitName(), sUnitsCol ); 
	lvit->setText( urdlg.getUnitDesc(), sDescCol ); 
	lvit->setText( urdlg.getUnitLith(), sLithoCol );
	uistratmgr_->updateUnitProps( uncode.buf(), urdlg.getUnitDesc(),
				      urdlg.getUnitLith() );
    }
}


ioPixmap* uiStratRefTree::createLevelPixmap( const UnitRef* ref ) const
{
    uiRGBArray rgbarr( false );
    rgbarr.setSize( PMWIDTH, PMHEIGHT );
    rgbarr.clear( Color::White() );
    for ( int idw=0; idw<PMWIDTH; idw++ )
	rgbarr.set( idw, mNINT(PMHEIGHT/2), Color::Black() );

    if ( ref )
    {
	Color col;
	if ( uistratmgr_->getLvlCol( ref, true, col ) )
	{
	    for ( int idw=0; idw<PMWIDTH; idw++ )
	    {
		rgbarr.set( idw, 0, col );
		rgbarr.set( idw, 1, col );
		rgbarr.set( idw, 2, col );
	    }
	}
	
	if ( uistratmgr_->getLvlCol( ref, false, col ) )
	{
	    for ( int idw=0; idw<PMWIDTH; idw++ )
	    {
		rgbarr.set( idw, PMHEIGHT-3, col );
		rgbarr.set( idw, PMHEIGHT-2, col );
		rgbarr.set( idw, PMHEIGHT-1, col );
	    }
	}
    }
    return new ioPixmap( rgbarr );
}


BufferString uiStratRefTree::getCodeFromLVIt( const uiListViewItem* item ) const
{
    if ( !item )
	return BufferString();

    BufferString bs = item->text();

    while ( item->parent() )
    {
	item = item->parent();
	CompoundKey kc( item->text() );
	kc += bs.buf();
	bs = kc.buf();
    }

    return bs;
}


void uiStratRefTree::updateLvlsPixmaps()
{
    UnitRef::Iter it( *uistratmgr_->getCurTree() );
    const UnitRef* firstun = it.unit();
    ioPixmap* pm = createLevelPixmap( firstun );
    uiListViewItem* firstlvit = lv_->findItem( firstun->code().buf(),0,false );
    if ( firstlvit )
	firstlvit->setPixmap( 0, *pm );
    delete pm;
    while ( it.next() )
    {
	const UnitRef* un = it.unit();
	ioPixmap* pm = createLevelPixmap( un );
	uiListViewItem* lvit = lv_->findItem( un->code().buf(), 0, false );
	if ( lvit )
	    lvit->setPixmap( 0, *pm );
	delete pm;
    }
}


void uiStratRefTree::updateLithoCol()
{
    UnitRef::Iter it( *uistratmgr_->getCurTree() );
    UnitRef* un = it.unit();
    while ( un )
    {
	if ( un->isLeaf() )
	{
	    mDynamicCastGet( LeafUnitRef*, lur, un )
	    int lithidx = Strat::UnRepo().findLith( lur->lithology() );
	    if ( lithidx<0 )
	    {
		uiListViewItem* lvit = lv_->findItem(un->code().buf(),0,false);
		if ( lvit )
		    lvit->setText( "", sLithoCol );
	    }
	}
	if ( !it.next() ) break;
	un = it.unit();
    }
}


#define mUpdateLvlInfo(loc,istop)\
{\
    bool has##loc##lvl = lvllinkdlg.lvl##loc##listfld_->isChecked();\
    if ( !has##loc##lvl )\
	uistratmgr_->freeLevel( getCodeFromLVIt( curit ).buf(), istop);\
    if ( has##loc##lvl )\
	uistratmgr_->linkLevel( getCodeFromLVIt( curit ).buf(), \
				lvllinkdlg.lvl##loc##listfld_->text(), istop );\
}


void uiStratRefTree::selBoundary()
{
    uiListViewItem* curit = lv_->currentItem();
    if ( !curit ) return;

    uiStratLinkLvlUnitDlg lvllinkdlg( lv_->parent(), 
	    			      getCodeFromLVIt( curit ).buf(),
	   			      uistratmgr_ );
    lvllinkdlg.go();
    if ( lvllinkdlg.uiResult() == 1 )
    {
	mUpdateLvlInfo( top, true )
	mUpdateLvlInfo( base, false )
	updateLvlsPixmaps();
	lv_->triggerUpdate();
    }
}


void uiStratRefTree::moveUnit( bool up )
{
    uiListViewItem* curit = lv_->currentItem();
    if ( !curit ) return;

    const bool isexpanded = curit->isOpen();
    uiListViewItem* targetit = up ? curit->prevSibling() : curit->nextSibling();
    if ( targetit )
	curit->moveItem( targetit );
    else if ( lv_->findItem( curit->text(), 0, true ) ) //may be main unit 
    {
	int curidx = lv_->indexOfItem(curit);
	if ( curidx<0 ) return;
	lv_->takeItem( curit );
	lv_->insertItem( up ? curidx-1 : curidx+1, curit );
    }

    curit->setOpen( isexpanded );
    lv_->setCurrentItem(curit);
    uistratmgr_->moveUnit( getCodeFromLVIt( curit ).buf(), up );
}


bool uiStratRefTree::canMoveUnit( bool up )
{
    uiListViewItem* curit = lv_->currentItem();
    if ( !curit ) return false;

    uiListViewItem* target = up ? curit->prevSibling() : curit->nextSibling();

    if ( !target && lv_->findItem( curit->text(), 0, true ) ) //may be main unit
    {
	int curidx = lv_->indexOfItem(curit);
	return !( (curidx<=0 && up) || (!up && curidx>= lv_->nrItems()-1 ) );
    }

    return target;
}
