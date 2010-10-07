/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratreftree.cc,v 1.55 2010-10-07 15:56:58 cvsbruno Exp $";

#include "uistratreftree.h"

#include "iopar.h"
#include "pixmap.h"
#include "randcolor.h"
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
{
    lv_ = new uiListView( p, "RefTree viewer" );
    BufferStringSet labels;
    labels.add( "Unit" );
    labels.add( "Description" );
    lv_->addColumns( labels );
    mAddCol( 300, 0 );
    mAddCol( 200, 1 );
    mAddCol( 150, 2 );
    lv_->setPrefWidth( 650 );
    lv_->setPrefHeight( 400 );
    lv_->setStretch( 2, 2 );
    lv_->rightButtonClicked.notify( mCB( this,uiStratRefTree,rClickCB ) );

    setTree( Strat::eRT() );
}


uiStratRefTree::~uiStratRefTree()
{
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
	    const int lidx = lur->lithology();
	    const Strat::LithologySet& liths = tree_->lithologies();
	    const Strat::Lithology* litho = ( lidx >= 0 && lidx<liths.size() ) ?
				    &liths.getLith( lur->lithology() ) : 0;
	    uiListViewItem::Setup setup = uiListViewItem::Setup()
				.label( lur->code() )
				.label( lur->description() );
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
    uiPopupMenu mnu( lv_->parent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Create sub-unit..."), 0 );
    mnu.insertItem( new uiMenuItem("&Subdivide unit..."), 1 );
    mnu.insertItem( new uiMenuItem("&Properties..."), 2 );
    if ( lv_->currentItem() != lv_->firstItem() )
	mnu.insertItem( new uiMenuItem("&Remove"), 3 );

    const int mnuid = mnu.exec();
    if ( mnuid<0 ) return;
    else if ( mnuid == 0 )
	insertSubUnit( lvit );
    else if ( mnuid == 1 )
	subdivideUnit( lvit );
    else if ( mnuid == 2 )
	updateUnitProperties( lvit );
    else if ( mnuid == 3 )
	removeUnit( lvit );
}


void uiStratRefTree::insertSubUnit( uiListViewItem* lvit )
{
    const Strat::UnitRef* un = lvit ? tree_->find( getCodeFromLVIt(lvit) ) : 0;
    if ( lvit && ( !un || un->isLeaf() ) ) return;

    NodeUnitRef* parun = lvit ? (NodeUnitRef*)un : tree_;
    LeavedUnitRef* tmpun = new LeavedUnitRef( parun, 0 );

    Interval<float> trg; getAvailableTime( *parun, trg );
    tmpun->setTimeRange( trg );
    tmpun->setColor( getRandStdDrawColor() );

    uiStratUnitEditDlg newurdlg( lv_->parent(), *tmpun );
    if ( newurdlg.go() )
    {
	if ( parun->isLeaved() )
	     parun = replaceUnit( *parun, false); 
	if ( parun )
	{
	    Strat::LeavedUnitRef* newun = 
				new Strat::LeavedUnitRef( parun, tmpun->code());
	    newun->setColor( tmpun->color() ); 
	    newun->setTimeRange( tmpun->timeRange() );
	    parun->add( newun );
	    ensureUnitTimeOK( *newun );
	    insertUnitInLVIT( lvit, *newun );
	    addLithologies( *newun, newurdlg.getLithologies() ); 
	    tree_->unitAdded.trigger();
	}
	else
	    uiMSG().error( "Cannot add unit" );
    }
    delete tmpun;
}


void uiStratRefTree::addLithologies( Strat::LeavedUnitRef& un, 
					const TypeSet<int>& ids )
{
    uiListViewItem* lvit = lv_->currentItem();
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	LeafUnitRef* lur = new LeafUnitRef( &un, ids[idx] );
	un.add( lur );
	insertUnitInLVIT( lvit, *lur );
    }
}


Strat::NodeUnitRef* uiStratRefTree::replaceUnit( NodeUnitRef& un, bool byleaved)
{
    NodeUnitRef* upnode = un.upNode();
    if ( !upnode ) return 0;

    NodeUnitRef* newpar = byleaved ? 
	(NodeUnitRef*)new LeavedUnitRef( upnode, un.code(), un.description() ) :
	(NodeUnitRef*)new NodeOnlyUnitRef( upnode, un.code(), un.description());

    IOPar iop; un.putPropsTo( iop ); newpar->getPropsFrom( iop );
    delete upnode->replace( upnode->indexOf(&un), newpar );
    return newpar;
}


void uiStratRefTree::subdivideUnit( uiListViewItem* lvit ) 
{
    if ( !lvit ) return;

    Strat::UnitRef* startunit = tree_->find( getCodeFromLVIt( lvit ) );
    if ( !startunit || !startunit->isLeaved() ) 
	{ uiMSG().error( "Only tail units can be subdivided" ); return; }

    Strat::NodeUnitRef* parnode = startunit->upNode();
    if ( !parnode ) return;

    uiStratUnitDivideDlg dlg( lv_->parent(), (LeavedUnitRef&)(*startunit) );
    if ( dlg.go() )
    {
	ObjectSet<Strat::LeavedUnitRef> units;
	dlg.gatherUnits( units );

	TypeSet<int> liths; liths += -1;
	for ( int idx=0; idx<units.size(); idx++ )
	{
	    LeavedUnitRef& ur = *units[idx];
	    if ( idx == 0)
	    {
		IOPar iop; ur.putPropsTo( iop ); 
		startunit->getPropsFrom( iop );
		delete &ur;
	    }
	    else
	    {
		parnode->add( &ur );
		insertUnitInLVIT( lvit->parent(), ur );
		addLithologies( ur, liths );
	    }
	}
	updateUnitsPixmaps();
	tree_->unitAdded.trigger();
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
    Strat::UnitRef* un = tree_->find( getCodeFromLVIt( lvit ) );
    if ( !un ) return;
    Strat::NodeUnitRef* upnode = un->upNode();
    if ( !upnode ) return;
    upnode->remove( un );
    if ( !upnode->isLeaved() && !upnode->hasChildren() )
	replaceUnit( *upnode, true );
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
    uiStratUnitEditDlg urdlg( lv_->parent(), nur );
    if ( urdlg.go() )
    {
	ensureUnitTimeOK( nur ); 
	if ( !nur.isLeaved() )
	{
	    for ( int iref=nur.nrRefs()-1; iref>=0; iref-- )
	    {
		const NodeUnitRef& ref = (NodeUnitRef&)nur.ref( iref );
		Interval<float> trg = ref.timeRange();
		const Interval<float> partrg = nur.timeRange();
		if ( !partrg.includes(trg.start) && partrg.includes(trg.stop) )
		    removeUnit( lvit->getChild( iref ) ) ;
		else
		    trg.limitTo( partrg );
	    }
	}
	else
	{
	    for ( int iref=nur.nrRefs()-1; iref>=0; iref-- )
		removeUnit( lvit->getChild( iref ) );

	    addLithologies( (LeavedUnitRef&)nur, urdlg.getLithologies() ); 
	}

	lvit->setText( unitref->code(), cUnitsCol );
	lvit->setText( unitref->description(), cDescCol );
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

    uiStratLinkLvlUnitDlg dlg( lv_->parent(), *ldun );
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


void uiStratRefTree::getAvailableTime( const Strat::NodeUnitRef& unit, 
					Interval<float>& timerg ) const 
{
    timerg = unit.timeRange();
    if ( !unit.nrRefs() || unit.isLeaved() ) 
	return;

    const Strat::NodeUnitRef& firstun = (Strat::NodeUnitRef&)unit.ref(0);
    if ( timerg.start < firstun.timeRange().start ) 
	{ timerg.stop = firstun.timeRange().start; return; }
    const Strat::NodeUnitRef& lastun = 
			    (Strat::NodeUnitRef&)unit.ref( unit.nrRefs()-1 );
    if ( timerg.stop > lastun.timeRange().stop )
	{ timerg.start = lastun.timeRange().stop; return; }

    Interval<float> trg( 0, 0 );
    for ( int idx=0; idx<unit.nrRefs()-1; idx++ )
    {
	const Strat::NodeUnitRef& cldun = (Strat::NodeUnitRef&)unit.ref(idx);
	const Strat::NodeUnitRef& nldun = (Strat::NodeUnitRef&)unit.ref(idx +1);
	timerg = nldun.timeRange();
	trg.set( cldun.timeRange().stop, nldun.timeRange().start );
	if ( trg.width() > 0 ) 
	   { timerg = trg; return; }
    }
    timerg.set( timerg.stop-timerg.width()/2, timerg.stop );
}


void uiStratRefTree::ensureUnitTimeOK( Strat::NodeUnitRef& unit )
{
    const Strat::NodeUnitRef* par = unit.upNode();
    if ( !par ) return;

    Interval<float> mytimerg( unit.timeRange() );
    const Interval<float> partrg( par->timeRange() );
    mytimerg.limitTo( partrg );

    for ( int idx=0; idx<par->nrRefs(); idx++ )
    {
	Strat::NodeUnitRef& ref = (Strat::NodeUnitRef&)par->ref(idx);
	if ( &ref == &unit ) 
	    continue;
	Interval<float> timerg = ref.timeRange();
	if ( mytimerg.includes( timerg.start ) 
		&& mytimerg.includes( timerg.stop ) )
	    getAvailableTime( unit, mytimerg );
	else if ( timerg.overlaps( mytimerg ) )
	{
	    if ( timerg.start < mytimerg.start )
		timerg.stop = mytimerg.start;
	    else 
		timerg.start= mytimerg.stop;
	    ref.setTimeRange( timerg );
	    setNodesDefaultTimes( ref );
	}
    }
    unit.setTimeRange( mytimerg );
}

