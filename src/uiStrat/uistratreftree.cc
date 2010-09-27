/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratreftree.cc,v 1.48 2010-09-27 11:05:19 cvsbruno Exp $";

#include "uistratreftree.h"

#include "iopar.h"
#include "pixmap.h"
#include "stratreftree.h"
#include "stratunitref.h"
#include "stratunitrefiter.h"
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

uiStratRefTree::uiStratRefTree( uiParent* p )
    : tree_(0)
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
}


uiStratRefTree::~uiStratRefTree()
{
    delete tree_;
}


void uiStratRefTree::setTree( Strat::RefTree& rt, bool force )
{
    if ( !force && &rt == tree_ ) return;

    tree_ = &rt;
    if ( !tree_ ) return;

    lv_->clear();
    addNode( 0, *((NodeUnitRef*)tree_), true );
}


#define mCreateAndSetUnitPixmap(ur,lvit)\
    ioPixmap* pm = createUnitPixmap( ur.color() );\
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
	    const Strat::Lithology* litho = 
			    &tree_->lithologies().getLith( lur->lithology() );
	    uiListViewItem::Setup setup = uiListViewItem::Setup()
				.label( lur->code() )
				.label( lur->description() ) 
				.label( litho ? litho->name() : "" );
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
    const Strat::UnitRef* parun = tree_->find( getCodeFromLVIt( lvit ) );
    if ( !parun ) return;

    Strat::LeavedUnitRef* newun = new Strat::LeavedUnitRef( 0, "<New Unit>" );
    if ( !newun ) return;

    uiStratUnitEditDlg newurdlg( lv_->parent(), *newun );
    if ( newurdlg.go() )
    {
	updateParentUnit( parun->fullCode() );
	Strat::NodeUnitRef& nur = (Strat::NodeUnitRef&)(*parun);
	nur.add( newun );
	insertUnitInLVIT( lvit, *newun );
	tree_->unitAdded.trigger();
    }
    else
	delete newun;
}


void uiStratRefTree::updateParentUnit( const char* parcode )
{
    Strat::UnitRef* parun = tree_->find( parcode );
    if ( !parun || !parun->isLeaf() ) return;

    NodeUnitRef* upnode = parun->upNode();
    if ( !upnode ) return;

    LeavedUnitRef* nodeun = new LeavedUnitRef( upnode, parun->code(),
						   parun->description() );
    nodeun->getPropsFrom( parun->pars() );
    int parunidx = upnode->indexOf( upnode->find( parun->code() ) );
    delete upnode->replace( parunidx, nodeun );
}


void uiStratRefTree::subdivideUnit( uiListViewItem* lvit ) 
{
    //TODO
    uiMSG().error( "not implemented yet" ); return;

    if ( !lvit ) return;

    Strat::UnitRef* startunit = tree_->find( getCodeFromLVIt( lvit ) );
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
	    Strat::UnitRef* newun = 0; 
	    //TODO createNewUnit( units[idx]->code() );
	    if ( newun )
	    {
		//tree_->updateUnit( newun->getID(), *units[idx] );
		insertUnitInLVIT( lvit, *newun );
	    }
	}
	deepErase( units );
    }
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
    newitem->setText( unit.description(), cDescCol );
   
    lv_->setCurrentItem( newitem );
    uiListViewItem* parit = newitem->parent();
    if ( !parit ) return;
    parit->setOpen( true );
    mCreateAndSetUnitPixmap( unit, newitem )
}


void uiStratRefTree::removeUnit( uiListViewItem* lvit )
{
    if ( !lvit ) return;
    const Strat::UnitRef* un = tree_->find( getCodeFromLVIt( lvit ) );
    if ( !un ) return;
    tree_->remove( un );
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
    UnitRef* unitref = tree_->find( getCodeFromLVIt(lvit) );
    if ( !unitref || unitref->isLeaf() ) return;
    unitref->setCode( lvit->text(cUnitsCol) );
    unitref->setDescription( lvit->text(cDescCol) );

    Strat::NodeUnitRef& nur = (Strat::NodeUnitRef&)(*unitref);
    uiStratUnitEditDlg urdlg(lv_->parent(), nur );
    urdlg.setLithology( lvit->text(cLithoCol) );
    if ( urdlg.go() )
    {
	for ( int iref=nur.nrRefs()-1; iref>=0; iref-- )
	{
	    /*
	    const UnitRef& ref = nur->ref( iref );
	    if ( ref.timeRange().start >= nur->timeRange().stop )
		removeUnit( lvit->getChild( iref ) ) ;
		*/
	}

	BufferString lithnm( urdlg.getLithology() );
	lvit->setText( unitref->code(), cUnitsCol );
	lvit->setText( unitref->description(), cDescCol );
	lvit->setText( lithnm.buf(), cLithoCol );
	tree_->unitChanged.trigger();
	updateUnitsPixmaps();
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
    Strat::UnitRefIter it( *tree_ );
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
	    { mCreateAndSetUnitPixmap( (*un), lvit ) }
    }
}


void uiStratRefTree::updateLithoCol()
{
    Strat::UnitRefIter it( *tree_ );
    UnitRef* un = it.unit();
    while ( un )
    {
	if ( un->isLeaf() )
	{
	    mDynamicCastGet( LeafUnitRef*, lur, un )
	    const Strat::Lithology* litho = 
			    &tree_->lithologies().getLith( lur->lithology() );
	    if ( !litho )
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
    //tree_->move( getCodeFromLVIt( curit ).buf(), up );
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


void uiStratRefTree::setUnitLvl( const char* code ) 
{
    UnitRef* unitref = tree_->find( code );
    mDynamicCastGet(Strat::LeavedUnitRef*,ldun,unitref)
    if ( !ldun ) 
	return;

    uiStratLinkLvlUnitDlg dlg( lv_->parent(), ldun );
    dlg.go();
}


void uiStratRefTree::setEntranceDefaultTimes()
{
    if ( tree_ )
    {
	Interval<float> timerg( 0, 2000 );
	tree_->setTimeRange( timerg );
	setNodesDefaultTimes( *tree_ );
    }
}


void uiStratRefTree::setNodesDefaultTimes( const Strat::NodeUnitRef& startnode )
{
    Strat::UnitRefIter it( startnode, UnitRefIter::AllNodes );
    NodeUnitRef* un = const_cast<Strat::NodeUnitRef*>( &startnode );
    while ( un )
    {
	if ( !un->isLeaved() )
	{
	    const int nrrefs = ((NodeUnitRef*)un)->nrRefs();
	    Interval<float> timerg = un->timeRange();
	    for ( int idx=0; idx<nrrefs; idx++ )
	    {
		mDynamicCastGet(NodeUnitRef*,nur,&un->ref(idx));
		if ( !nur ) break;
		Interval<float> rg = nur->timeRange();
		rg.start = timerg.start + (float)idx*timerg.width()/(nrrefs);
		rg.stop = timerg.start +(float)(idx+1)*timerg.width()/(nrrefs);
		nur->setTimeRange( rg );
	    }
	}
	if ( !it.next() ) 
	    break;
	un = (NodeUnitRef*)it.unit();
    }
}


bool uiStratRefTree::haveTimes() const
{
    const Interval<float> timerg = tree_->timeRange();
    return ( timerg.width() > 0 && timerg.start >= 0 
		&& timerg.stop > 0 && !mIsUdf(timerg.start) );
}
