/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratreftree.cc,v 1.47 2010-09-08 13:27:39 cvsbruno Exp $";

#include "uistratreftree.h"

#include "iopar.h"
#include "pixmap.h"
#include "stratreftree.h"
#include "stratunitref.h"
#include "stratunitrepos.h"
#include "sorting.h"
#include "uigeninput.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uirgbarray.h"
#include "uistratutildlgs.h"

#define mAddCol(wdth,nr) \
    lv_->setColumnWidth( nr, wdth )

#define PMWIDTH		11
#define PMHEIGHT	9

static const int cUnitsCol	= 0;
static const int cDescCol	= 1;
static const int cLithoCol	= 2;

using namespace Strat;

uiStratRefTree::uiStratRefTree( uiParent* p, UnitRepository& repos )
    : tree_(0)
    , repos_(repos)
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
    setTree( repos_.getCurTree() );
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


#define mCreateAndSetUnitPixmap(ur,lvit)\
    IOPar iop; ur.putTo( iop );\
    Color col; iop.get( sKey::Color, col );\
    ioPixmap* pm = createUnitPixmap( col );\
    lvit->setPixmap( 0, *pm );\
    delete pm;

void uiStratRefTree::addNode( uiListViewItem* parlvit,
			      const NodeUnitRef& nur, bool root )
{
    uiListViewItem* lvit = parlvit
	? new uiListViewItem( parlvit, uiListViewItem::Setup()
				.label(nur.code()).label(nur.description()) )
	: root ? 0 : new uiListViewItem( lv_,uiListViewItem::Setup()
				.label(nur.code()).label(nur.description() ));

    if ( parlvit || !root )
	{ mCreateAndSetUnitPixmap( nur, lvit ) }
    
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
				.label( repos_.getLithName(*lur) );
	    if ( lvit )
		item = new uiListViewItem( lvit, setup );
	    else
		item = new uiListViewItem( lv_, setup );
	    
	    mCreateAndSetUnitPixmap( (*lur), item )
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
	lvit->setRenameEnabled( cUnitsCol, false );  //TODO
	lvit->setRenameEnabled( cDescCol, false );   //TODO
	lvit->setRenameEnabled( cLithoCol, false );
	lvit->setDragEnabled( yn );
	lvit->setDropEnabled( yn );
	lvit = lvit->itemBelow();
    }
}


void uiStratRefTree::repoChangedCB( CallBacker* )
{
    setTree( repos_.getCurTree() );
}


void uiStratRefTree::rClickCB( CallBacker* )
{
    uiListViewItem* lvit = lv_->itemNotified();
    if ( !lvit || !lvit->dragEnabled() ) return;
    handleMenu( lvit );
}


void uiStratRefTree::handleMenu( uiListViewItem* lvit )
{
    int col = lv_->columnNotified();
    uiPopupMenu mnu( lv_->parent(), "Action" );
    mnu.insertSeparator();
    mnu.insertItem( new uiMenuItem("&Create sub-unit..."), 0 );
    if ( lvit->nrChildren() == 0 )
	mnu.insertItem( new uiMenuItem("&Subdivide unit..."), 1 );
    mnu.insertItem( new uiMenuItem("&Properties..."), 2 );
    if ( lv_->currentItem() != lv_->firstItem() )
	mnu.insertItem( new uiMenuItem("&Remove"), 3 );

    const int mnuid = mnu.exec();
    if ( mnuid<0 ) return;
    else if ( mnuid==0 )
	insertSubUnit( lvit );
    else if ( mnuid== 1 )
	subdivideUnit( lvit );
    else if ( mnuid==2 )
	updateUnitProperties( lvit );
    else if ( mnuid==3 )
	removeUnit( lvit );
}


void uiStratRefTree::insertSubUnit( uiListViewItem* lvit )
{
    const Strat::UnitRef* parun = getUnitFromLVIT( lvit );
    if ( !parun ) return;

    Strat::UnitRef* newun = createNewUnit( "<New Unit>", parun->getID() );
    if ( !newun ) return;

    uiStratUnitEditDlg newurdlg( lv_->parent(), *newun );
    if ( newurdlg.go() )
    {
	if( newun->isLeaf() )
	    repos_.updateUnitLith(newun->getID(),newurdlg.getLithology().buf());
	insertUnitInLVIT( lvit, *newun );
	repos_.unitChanged.trigger();
    }
    else
	repos_.removeUnit( newun->getID() );

}


Strat::UnitRef* uiStratRefTree::createNewUnit(const char* code, int parid) const
{
    BufferString fullcode; 
    repos_.getFullCode( parid, fullcode );
    Interval<float> timerg;
    repos_.getNewUnitTimeRange( parid, timerg );
    if ( !fullcode.isEmpty() && timerg.width() == 0 )
	{ uiMSG().error("No time space left to add a new unit"); return 0; }

    repos_.prepareParentUnit( parid );
    CompoundKey ck; ck += fullcode; ck += code;
    if ( repos_.addUnit( ck.buf() ) )
    {
	UnitRef* newunit = repos_.find( ck.buf() );
	if ( newunit ) 
	    newunit->setTimeRange( timerg );
	return newunit;
    }
    return 0;
}


void uiStratRefTree::subdivideUnit( uiListViewItem* lvit ) 
{
    if ( !lvit ) return;

    Strat::UnitRef* startunit = getUnitFromLVIT( lvit );
    if ( !startunit ) return;

    uiListViewItem* parit = lvit->parent();
    if ( parit )
	lv_->setCurrentItem( parit );

    const Strat::UnitRef* parnode = startunit->upNode();
    if ( !parnode ) return;

    uiStratUnitDivideDlg dlg( lv_->parent(), *startunit );
    if ( dlg.go() )
    {
	ObjectSet<Strat::UnitRef> units;
	dlg.gatherUnits( units );
	if ( units.size() <= 0 )
	    { uiMSG().error( "can not modify unit" ); return; }

	removeUnit( lvit );
	lvit = parit;
	for ( int idx=0; idx<units.size(); idx++ )
	{
	    Strat::UnitRef* newun = createNewUnit( units[idx]->code(), 
						    parnode->getID() );
	    if ( newun )
	    {
		repos_.updateUnit( newun->getID(), *units[idx] );
		insertUnitInLVIT( lvit, *newun );
	    }
	}
	deepErase( units );
    }
}


Strat::UnitRef* uiStratRefTree::getUnitFromLVIT( uiListViewItem* lvit ) const
{
    BufferString uncode = getCodeFromLVIt( lvit );
    Strat::UnitRef* unit = repos_.find( uncode.buf() );
    if ( !unit ) 
	{ uiMSG().error( "Can not find unit" ); return 0; }
    return unit;
}


void uiStratRefTree::insertUnitInLVIT( uiListViewItem* lvit, 
					const Strat::UnitRef& unit ) const
{
    uiListViewItem* newitem;
    uiListViewItem::Setup setup = uiListViewItem::Setup().label( unit.code() );
    newitem = lvit ? new uiListViewItem( lvit, setup )
		   : new uiListViewItem( lv_, setup );
    newitem->setRenameEnabled( cUnitsCol, false );	//TODO
    newitem->setRenameEnabled( cDescCol, false );	//TODO
    newitem->setRenameEnabled( cLithoCol, false );
    newitem->setDragEnabled( true );
    newitem->setDropEnabled( true );
   
    lv_->setCurrentItem( newitem );
    uiListViewItem* parit = newitem->parent();
    if ( !parit ) return;
    parit->setOpen( true );
    mCreateAndSetUnitPixmap( unit, lvit )
}


void uiStratRefTree::removeUnit( uiListViewItem* lvit )
{
    if ( !lvit ) return;
    const Strat::UnitRef* un = getUnitFromLVIT( lvit );
    if ( !un ) return;
    repos_.removeUnit( un->getID() );
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
    UnitRef* unitref = repos_.find( getCodeFromLVIt(lvit) );
    if ( !unitref ) return;
    unitref->setCode( lvit->text(cUnitsCol) );
    unitref->setDescription( lvit->text(cDescCol) );

    uiStratUnitEditDlg urdlg( lv_->parent(), *unitref );
    urdlg.setLithology( lvit->text(cLithoCol) );
    if ( urdlg.go() )
    {
	repos_.unitChanged.trigger();

	BufferString lithnm = urdlg.getLithology();
	lvit->setText( unitref->code(), cUnitsCol );
	lvit->setText( unitref->description(), cDescCol ); 
	lvit->setText( lithnm.buf(), cLithoCol );

	mDynamicCastGet(const Strat::NodeUnitRef*,nur,unitref)
	if ( nur )
	{
	    for ( int iref=nur->nrRefs()-1; iref>=0; iref-- )
	    {
		const UnitRef& ref = nur->ref( iref );
		if ( ref.timeRange().start >= nur->timeRange().stop )
		    removeUnit( lvit->getChild( iref ) ) ;
	    }
	}
	updateUnitsPixmaps();
	if( unitref->isLeaf() )
	    repos_.updateUnitLith( unitref->getID(), lithnm.buf() );
    }
}


ioPixmap* uiStratRefTree::createUnitPixmap( const Color& col ) const
{
    uiRGBArray rgbarr( false );
    rgbarr.setSize( PMWIDTH, PMHEIGHT );
    rgbarr.clear( Color::White() );

    for ( int idw=0; idw<PMWIDTH; idw++ )
    {
	for ( int idh=0; idh<PMHEIGHT; idh++ )
	{
	    rgbarr.set( idw, idh, col );
	    rgbarr.set( idw, idh, col );
	    rgbarr.set( idw, idh, col );
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


void uiStratRefTree::updateUnitsPixmaps()
{
    UnitRef::Iter it( *repos_.getCurTree() );
    const UnitRef* firstun = it.unit();
    if ( !firstun ) return;
    uiListViewItem* lvit = lv_->findItem( firstun->code().buf(), 0, false);
    if ( lvit )
	{ mCreateAndSetUnitPixmap( (*firstun), lvit ) }
    while ( it.next() )
    {
	const UnitRef* un = it.unit();
	if ( !un ) continue;
	lvit = lv_->findItem( un->code().buf(), 0, false );
	if ( lvit )
	    { mCreateAndSetUnitPixmap( (*firstun),lvit ) }
    }
}


void uiStratRefTree::updateLithoCol()
{
    UnitRef::Iter it( *repos_.getCurTree() );
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
		    lvit->setText( "", cLithoCol );
	    }
	}
	if ( !it.next() ) break;
	un = it.unit();
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
    repos_.moveUnit( getCodeFromLVIt( curit ).buf(), up );
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


void uiStratRefTree::setUnitLvl( int unid ) 
{
    UnitRef* unitref = repos_.find( unid );
    if ( !unitref ) 
	return;

    uiStratLinkLvlUnitDlg dlg( lv_->parent(), unitref );
    dlg.go();
}


void uiStratRefTree::setBottomLvl() 
{
    uiStratLinkLvlUnitDlg dlg( lv_->parent(), 0 );
    dlg.go();
}


void uiStratRefTree::doSetUnconformities( CallBacker* ) 
{
    setUnconformities( *((NodeUnitRef*)tree_), true, 0 );
}


void uiStratRefTree::setUnconformities( const Strat::NodeUnitRef& node, 
						bool root, int nrunconf )
{
    if ( root )
    {
	for ( int idx=0; idx<node.nrRefs(); idx++ )
	{
	    mDynamicCastGet(const Strat::NodeUnitRef*,un,&node.ref(idx))
	    if ( un )
		setUnconformities( *un, false, 0 );
	}
	return;
    }

    TypeSet< Interval<float> > timergs;
    tree_->getLeavesTimeGaps( node, timergs );
    for ( int idx=0; idx<timergs.size(); idx++ )
    {
	Interval<float> timerg = timergs[idx];
	BufferString unconfcode("Unconf");
	unconfcode += toString( nrunconf++ );
	uiListViewItem* lit = listView()->findItem(node.code(),0,false);
	if ( lit ) listView()->setCurrentItem(lit);
	Strat::UnitRef* un = createNewUnit( unconfcode, node.getID() );
	if ( !un ) continue;;
	IOPar iop; iop.set( sKey::Color, Color( 215, 215, 215 ) );
	iop.set( sKey::Time, timerg );
	un->getFrom( iop );
	insertUnitInLVIT( lit, *un );
	repos_.unitChanged.trigger();
	for ( int idref=0; idref<(node.nrRefs()-idx); idref++ )
	    moveUnit( true );
    }
    for ( int iref=0; iref<node.nrRefs(); iref++ )
    {
	const Strat::UnitRef& ref = node.ref( iref );
	mDynamicCastGet(const Strat::NodeUnitRef*,chldnur,&ref);
	if ( chldnur )
	    setUnconformities( *chldnur, false, nrunconf );
    }
}
