/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          June 2007
 RCS:		$Id: uistratreftree.cc,v 1.14 2007-09-12 09:16:17 cvshelene Exp $
________________________________________________________________________

-*/

#include "uistratreftree.h"

#include "pixmap.h"
#include "stratlevel.h"
#include "stratunitrepos.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uirgbarray.h"
#include "uistratutildlgs.h"

#define mAddCol(nm,wdth,nr) \
    lv_->addColumn( nm ); \
    lv_->setColumnWidthMode( nr, uiListView::Manual ); \
    lv_->setColumnWidth( nr, wdth )

#define PMWIDTH		11
#define PMHEIGHT	9

static const int sUnitsCol	= 0;
static const int sDescCol	= 1;
static const int sLithoCol	= 2;

using namespace Strat;

uiStratRefTree::uiStratRefTree( uiParent* p, const RefTree* rt )
    : tree_(0)
    , itemAdded_(this)
    , itemToBeRemoved_(this)
{
    lv_ = new uiListView( p, "RefTree viewer" );
    mAddCol( "Unit", 300, 0 );
    mAddCol( "Description", 200, 1 );
    mAddCol( "Lithology", 150, 2 );
    lv_->setPrefWidth( 650 );
    lv_->setPrefHeight( 400 );
    lv_->setStretch( 2, 2 );
    lv_->setTreeStepSize(30);
    lv_->rightButtonClicked.notify( mCB( this,uiStratRefTree,rClickCB ) );

    setTree( rt );
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
    addNode( 0, *tree_, true );
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
    
    for ( int iref=nur.nrRefs()-1; iref>=0; iref-- )
    {
	const UnitRef& ref = nur.ref( iref );
	if ( ref.isLeaf() )
	{
	    uiListViewItem* item;
	    mDynamicCastGet(const LeafUnitRef&,lur,ref);
	    uiListViewItem::Setup setup = uiListViewItem::Setup()
				.label( lur.code() )
				.label( lur.description() )
				.label( UnRepo().getLithName(lur.lithology()) );
	    if ( lvit )
		item = new uiListViewItem( lvit, setup );
	    else
		item = new uiListViewItem( lv_, setup );
	    
	    ioPixmap* pm = createLevelPixmap( &lur );
	    item->setPixmap( 0, *pm );
	    delete pm;
	}
	else
	{
	    mDynamicCastGet(const NodeUnitRef&,chldnur,ref);
	    addNode( lvit, chldnur, false );
	}
    }
}


const UnitRef* uiStratRefTree::findUnit( const char* s ) const
{
    return tree_->find( s );
}


void uiStratRefTree::expand( bool yn ) const
{
    uiListViewItem* lvit = lv_->firstChild();
    while ( lvit )
    {
	lvit->setOpen( yn );
	lvit = lvit->itemBelow();
    }
}


void uiStratRefTree::makeTreeEditable( bool yn ) const
{
    uiListViewItem* lvit = lv_->firstChild();
    while ( lvit )
    {
	lvit->setRenameEnabled( sUnitsCol, yn );
	lvit->setRenameEnabled( sDescCol, yn );
	lvit->setDragEnabled( yn );
	lvit->setDropEnabled( yn );
	lvit = lvit->itemBelow();
    }
}


void uiStratRefTree::rClickCB( CallBacker* )
{
    uiListViewItem* lvit = lv_->itemNotified();
    if ( !lvit || !lvit->renameEnabled(sUnitsCol) ) return;

    int col = lv_->columnNotified();
    if ( col == sUnitsCol || col == sDescCol )
    {
	uiPopupMenu mnu( lv_->parent(), "Action" );
	mnu.insertItem( new uiMenuItem("Add unit ..."), 0 );
	mnu.insertItem( new uiMenuItem("Create sub-unit..."), 1 );
	mnu.insertItem( new uiMenuItem("Remove"), 2 );
    /*    mnu.insertSeparator();
	mnu.insertItem( new uiMenuItem("Rename"), 3 );*/

	const int mnuid = mnu.exec();
	if ( mnuid<0 ) return;
	else if ( mnuid==0 )
	insertSubUnit( 0 );
	else if ( mnuid==1 )
	    insertSubUnit( lvit );
	else if ( mnuid==2 )
	    removeUnit( lvit );
    }
    else if ( col == sLithoCol )
    {
	uiLithoDlg lithdlg( lv_->parent() );
	lithdlg.setSelectedLith( lvit->text( sLithoCol ) );
	if ( lithdlg.go() )
	    lvit->setText( lithdlg.getLithName(), sLithoCol );
    }
}


void uiStratRefTree::insertSubUnit( uiListViewItem* lvit )
{
    uiStratUnitDlg newurdlg( lv_->parent() );
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

	newitem->setRenameEnabled( sUnitsCol, true );
	newitem->setRenameEnabled( sDescCol, true );
	newitem->setDragEnabled( true );
	newitem->setDropEnabled( true );
	ioPixmap* pm = createLevelPixmap(0);
	newitem->setPixmap( 0, *pm );
	delete pm;
	
	if ( newitem->parent() )
	    newitem->parent()->setOpen( true );

	lv_->setCurrentItem( newitem );
	itemAdded_.trigger();
    }
}


void uiStratRefTree::removeUnit( uiListViewItem* lvit )
{
    itemToBeRemoved_.trigger();
    if ( lvit->parent() )
	lvit->parent()->removeItem( lvit );
    else
    {
	lv_->takeItem( lvit );
	delete lvit;
    }

    lv_->triggerUpdate();
}


ioPixmap* uiStratRefTree::createLevelPixmap( const UnitRef* ref ) const
{
    uiRGBArray rgbarr;
    rgbarr.setSize( PMWIDTH, PMHEIGHT );
    rgbarr.clear( Color::White );
    for ( int idw=0; idw<PMWIDTH; idw++ )
	rgbarr.set( idw, mNINT(PMHEIGHT/2), Color::Black );

    if ( ref )
    {
	const Level* toplvl = Strat::RT().getLevel( ref, true );
	if ( toplvl )
	{
	    Color col = toplvl->color_;
	    for ( int idw=0; idw<PMWIDTH; idw++ )
	    {
		rgbarr.set( idw, 0, col );
		rgbarr.set( idw, 1, col );
		rgbarr.set( idw, 2, col );
	    }
	}
	
	const Level* botlvl = Strat::RT().getLevel( ref, false );
	if ( botlvl )
	{
	    Color col = botlvl->color_;
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

