/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uistratreftree.cc,v 1.76 2012-09-07 22:08:05 cvsnanne Exp $";

#include "uistratreftree.h"

#include "iopar.h"
#include "pixmap.h"
#include "randcolor.h"
#include "stratreftree.h"
#include "stratunitref.h"
#include "stratunitrefiter.h"
#include "sorting.h"
#include "uigeninput.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uirgbarray.h"
#include "uistratutildlgs.h"
#include "uitreeview.h"

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
    lv_ = new uiTreeView( p, "RefTree viewer" );
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
    lv_->mouseButtonPressed.notify( mCB( this,uiStratRefTree,mousePressedCB ) );
    lv_->rightButtonPressed.notify( mCB( this,uiStratRefTree,mousePressedCB ) );

    tree_ = 0;
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

    if  ( !haveTimes() )
	setEntranceDefaultTimes();

}


#define mCreateAndSetUnitPixmap(ur,lvit)\
    ioPixmap* pm = createUnitPixmap( ur.color() );\
    lvit->setPixmap( 0, *pm );\
    delete pm;

void uiStratRefTree::addNode( uiTreeViewItem* parlvit,
			      const NodeUnitRef& nur, bool root )
{
    uiTreeViewItem* lvit = parlvit
        ? new uiTreeViewItem( parlvit, uiTreeViewItem::Setup()
				.label(nur.code()).label(nur.description()) )
	: root ? 0 : new uiTreeViewItem( lv_,uiTreeViewItem::Setup()
				.label(nur.code()).label(nur.description() ));

    if ( parlvit || !root )
	{ mCreateAndSetUnitPixmap( nur, lvit ) }
    
    for ( int iref=0; iref<nur.nrRefs(); iref++ )
    {
	const UnitRef& ref = nur.ref( iref );
	if ( ref.isLeaf() )
	{
	    uiTreeViewItem* item;
	    mDynamicCastGet(const LeafUnitRef*,lur,&ref);
	    if ( !lur ) continue;

	    uiTreeViewItem::Setup setup = uiTreeViewItem::Setup()
				.label( lur->code() )
				.label( lur->description() );
	    if ( lvit )
		item = new uiTreeViewItem( lvit, setup );
	    else
		item = new uiTreeViewItem( lv_, setup );
	    
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
    uiTreeViewItem* lvit = lv_->firstItem();
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


void uiStratRefTree::mousePressedCB( CallBacker* )
{
    if ( !lv_->nrItems() )
	insertSubUnit( 0 );
}


void uiStratRefTree::rClickCB( CallBacker* )
{
    uiTreeViewItem* lvit = lv_->itemNotified();
    if ( !lvit || !lvit->dragEnabled() ) return;
    handleMenu( lvit );
}


void uiStratRefTree::handleMenu( uiTreeViewItem* lvit )
{
    if ( !strcmp( lvit->text(), Strat::RefTree::sKeyNoCode() ) )
	{ updateUnitProperties( lvit ); return; }

    uiPopupMenu mnu( lv_->parent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Create sub-unit..."), 0 );
    if ( isLeaved( lvit ) )
	mnu.insertItem( new uiMenuItem("&Subdivide unit..."), 1 );
    mnu.insertItem( new uiMenuItem("&Properties..."), 2 );
    if ( lv_->currentItem() != lv_->firstItem() )
	mnu.insertItem( new uiMenuItem("&Remove"), 3 );
    if ( lv_->currentItem() == lv_->firstItem() ) 
	 mnu.insertItem( new uiMenuItem("&Add Unit below"), 4 );
    if ( isLeaved( lvit ) )
    {
	mnu.insertSeparator();
	mnu.insertItem( new uiMenuItem("&Assign marker boundary"), 5 );
    }

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
    else if ( mnuid == 4 )
	insertSubUnit( 0 );
    else if ( mnuid == 5 ) 
	assignLevelBoundary( lvit );
}


void uiStratRefTree::insertSubUnit( uiTreeViewItem* lvit )
{
    const Strat::UnitRef* un = lvit ? tree_->find( getFullCodeFromLVIt(lvit) ) 
				    : 0;
    if ( lvit && ( !un || un->isLeaf() ) ) return;

    NodeUnitRef* parun = lvit ? (NodeUnitRef*)un : tree_;
    LeavedUnitRef tmpun( parun, 0 );

    Interval<float> trg; getAvailableTime( *parun, trg );
    tmpun.setTimeRange( trg );
    tmpun.setColor( getRandStdDrawColor() );

    if ( parun->isLeaved() )
    {
	TypeSet<int> lithids;
	const Strat::LeavedUnitRef& lvdun = (Strat::LeavedUnitRef&)(*parun);
	tmpun.setLevelID( lvdun.levelID() );	
	for ( int iref = 0; iref<lvdun.nrRefs(); iref++ )
	{
	    int id = ((Strat::LeafUnitRef&)(lvdun.ref(iref))).lithology();
	    LeafUnitRef* lur = new LeafUnitRef( &tmpun, id );
	    tmpun.add( lur );
	}
    }

    uiStratUnitEditDlg newurdlg( lv_->parent(), tmpun );
    if ( newurdlg.go() )
    {
	if ( parun->isLeaved() )
	{
	    for ( int iref=parun->nrRefs()-1; iref>=0; iref-- )
		removeUnit( lvit->getChild( iref ) );
	    parun = replaceUnit( *parun, false ); 
	}
	if ( parun )
	{
	    Strat::LeavedUnitRef* newun = 
				new Strat::LeavedUnitRef( parun, tmpun.code());
	    newun->setColor( tmpun.color() ); 
	    newun->setTimeRange( tmpun.timeRange() );
	    newun->setLevelID( tmpun.levelID() );
	    newun->setDescription( tmpun.description() );
	    int posidx = getChildIdxFromTime( *parun, newun->timeRange().start);
	    if ( posidx < parun->nrRefs() )
		parun->insert( newun, posidx );
	    else 
		parun->add( newun );
	    ensureUnitTimeOK( *newun );
	    insertUnitInLVIT( lvit, posidx, *newun );
	    addLithologies( *newun, newurdlg.getLithologies() ); 
	    tree_->unitAdded.trigger();
	}
	else
	    uiMSG().error( "Cannot add unit" );
    }
}


void uiStratRefTree::addLithologies( Strat::LeavedUnitRef& un, 
					const TypeSet<int>& ids )
{
    uiTreeViewItem* lvit = getLVItFromFullCode( un.fullCode() );
    if ( !lvit ) return;
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	LeafUnitRef* lur = new LeafUnitRef( &un, ids[idx] );
	un.add( lur );
	insertUnitInLVIT( lvit, idx, *lur );
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


void uiStratRefTree::updateLithoCol()
{
    //TODO ... was defined but not implemented - gave a linker error
}


void uiStratRefTree::subdivideUnit( uiTreeViewItem* lvit )
{
    if ( !lvit ) return;

    Strat::UnitRef* startunit = tree_->find( getFullCodeFromLVIt( lvit ) );
    if ( !startunit || !startunit->isLeaved() ) 
	return;
    LeavedUnitRef& ldur = (LeavedUnitRef&)(*startunit);
    
    Strat::NodeUnitRef* parnode = startunit->upNode();
    if ( !parnode ) return;

    int curidx = 0;
    for ( int idx=0; idx<parnode->nrRefs(); idx++ )
    {
	if ( &parnode->ref(idx) == startunit )
	{ curidx = idx; break; }
    }
    uiStratUnitDivideDlg dlg( lv_->parent(), ldur );
    if ( dlg.go() )
    {
	ObjectSet<Strat::LeavedUnitRef> units;
	dlg.gatherUnits( units );

	TypeSet<int> lithids;  
	for ( int idx=0; idx<ldur.nrRefs(); idx++ )
	    lithids += ((Strat::LeafUnitRef&)(ldur.ref(idx))).lithology();
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
		insertUnitInLVIT( lvit->parent(), curidx+idx, ur );
		addLithologies( ur, lithids );
	    }
	}
	updateUnitsPixmaps();
	tree_->unitAdded.trigger();
    }
}


void uiStratRefTree::insertUnitInLVIT( uiTreeViewItem* lvit, int posidx,
					const Strat::UnitRef& unit ) const
{
    uiTreeViewItem::Setup setup = uiTreeViewItem::Setup().label( unit.code() );
    uiTreeViewItem* newitem = lvit ? new uiTreeViewItem( lvit , setup )
                                   : new uiTreeViewItem( lv_, setup );
    newitem->setRenameEnabled( cUnitsCol, false );	//TODO
    newitem->setRenameEnabled( cDescCol, false );	//TODO
    newitem->setRenameEnabled( cLithoCol, false );
    newitem->setDragEnabled( true );
    newitem->setDropEnabled( true );
    newitem->setText( unit.description(), cDescCol );
 
    if ( lvit )
    {
	lvit->takeItem( newitem );
	lvit->insertItem( posidx, newitem );
    }
    lv_->setCurrentItem( newitem );
    uiTreeViewItem* parit = newitem->parent();
    if ( !parit ) return;
    parit->setOpen( true );
    mCreateAndSetUnitPixmap( unit, newitem )
}


void uiStratRefTree::removeUnit( uiTreeViewItem* lvit )
{
    if ( !lvit ) return;
    Strat::UnitRef* un = tree_->find( getFullCodeFromLVIt( lvit ) );
    if ( !un ) return;
    Strat::NodeUnitRef* upnode = un->upNode();
    if ( !upnode ) return;

    TypeSet<int> lithids; int lvlid = -1;
    if ( un->isLeaved() )
    {
	const Strat::LeavedUnitRef& lvedun = (Strat::LeavedUnitRef&)(*un);
	for ( int idx=0; idx<lvedun.nrRefs(); idx++ )
	    lithids += ((Strat::LeafUnitRef&)(lvedun.ref(idx))).lithology();
	lvlid = lvedun.levelID();
    }
    upnode->remove( un );
    if ( lvit->parent() )
	lvit->parent()->removeItem( lvit );
    else
    {
	lv_->takeItem( lvit );
	delete lvit;
    }
    if ( !upnode->isLeaved() && !upnode->hasChildren() )
    {
	upnode = replaceUnit( *upnode, true );
	LeavedUnitRef& lur = (LeavedUnitRef&)(*upnode);
	addLithologies( lur, lithids ); 
	lur.setLevelID( lvlid );
    }

    lv_->triggerUpdate();
}


void uiStratRefTree::updateUnitProperties( uiTreeViewItem* lvit )
{
    UnitRef* unitref = tree_->find( getFullCodeFromLVIt(lvit) );
    if ( !unitref || unitref->isLeaf() ) return;
    unitref->setCode( lvit->text(cUnitsCol) );
    unitref->setDescription( lvit->text(cDescCol) );

    Strat::NodeUnitRef& nur = (Strat::NodeUnitRef&)(*unitref);
    uiStratUnitEditDlg urdlg( lv_->parent(), nur );
    if ( urdlg.go() )
    {
	ensureUnitTimeOK( nur ); 
	if ( nur.isLeaved() )
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


uiTreeViewItem* uiStratRefTree::getLVItFromFullCode( const char* code ) const
{
    uiTreeViewItem* lvit = lv_->firstItem();
    while ( lvit )
    {
        uiTreeViewItem* item = lvit;
	BufferString bs = item->text();
	while ( item->parent() )
	{
	    item = item->parent();
	    CompoundKey kc( item->text() );
	    kc += bs.buf();
	    bs = kc.buf();
	}
	if ( !strcmp( code, bs.buf() ) )
	    return lvit;

	lvit = lvit->itemBelow();
    }
    return 0;
}


BufferString uiStratRefTree::getFullCodeFromLVIt( const uiTreeViewItem* item ) const
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
    uiTreeViewItem* lvit = lv_->findItem( firstun->code().buf(), 0, false);
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
    uiTreeViewItem* curit = lv_->currentItem();
    if ( !curit ) return;

    const bool isexpanded = curit->isOpen();
    uiTreeViewItem* targetit = up ? curit->prevSibling() : curit->nextSibling();
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
    //tree_->move( getFullCodeFromLVIt( curit ).buf(), up );
}


bool uiStratRefTree::canMoveUnit( bool up )
{
    uiTreeViewItem* curit = lv_->currentItem();
    if ( !curit ) return false;

    uiTreeViewItem* target = up ? curit->prevSibling() : curit->nextSibling();

    if ( !target && lv_->findItem( curit->text(), 0, true ) ) //may be main unit
    {
	int curidx = lv_->indexOfItem(curit);
	return !( (curidx<=0 && up) || (!up && curidx>= lv_->nrItems()-1 ) );
    }

    return target;
}


bool uiStratRefTree::isLeaved( uiTreeViewItem* lvit ) const
{
    const Strat::UnitRef* un = lvit ? tree_->find( getFullCodeFromLVIt(lvit) ) 
				    : 0;
    return ( un && un->isLeaved() ); 
}


void uiStratRefTree::assignLevelBoundary( uiTreeViewItem* lvit )
{
    if ( lvit )
	setUnitLvl( getFullCodeFromLVIt(lvit) );
}


void uiStratRefTree::setUnitLvl( const char* code ) 
{
    UnitRef* unitref = tree_->find( code );
    mDynamicCastGet(Strat::LeavedUnitRef*,ldun,unitref)
    if ( !ldun ) 
	return;

    uiStratLinkLvlUnitDlg dlg( lv_->parent(), *ldun );
    if ( dlg.go() )
	tree_->unitChanged.trigger();
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

    const int posidx = getChildIdxFromTime( *par, mytimerg.start )-1;
    for ( int idx=0; idx<par->nrRefs(); idx++ )
    {
	Strat::NodeUnitRef& ref = (Strat::NodeUnitRef&)par->ref(idx);
	Interval<float> timerg = ref.timeRange();
	if ( &ref == &unit ) continue;
	if ( posidx-1 == idx && mytimerg.start < timerg.start )
	    mytimerg.start = timerg.start;
	else if ( posidx+1 == idx && mytimerg.stop > timerg.stop )
	    mytimerg.stop = timerg.stop;
    }

    for ( int idx=0; idx<par->nrRefs(); idx++ )
    {
	Strat::NodeUnitRef& ref = (Strat::NodeUnitRef&)par->ref(idx);
	if ( &ref == &unit ) continue;
	Interval<float> timerg = ref.timeRange();
	if ( timerg.overlaps( mytimerg ) )
	{
	    if ( timerg.stop==mytimerg.start || timerg.start==mytimerg.stop )
		continue;
	    if ( timerg.start < mytimerg.start )
		timerg.stop = mytimerg.start;
	    else 
		timerg.start = mytimerg.stop;
	    ref.setTimeRange( timerg );
	    setNodesDefaultTimes( ref );
	}
    }
    unit.setTimeRange( mytimerg );
    if ( unit.isLeaved() ) return;

    for ( int idx=0; idx<unit.nrRefs(); idx++ )
    {
	Strat::NodeUnitRef& ref = (Strat::NodeUnitRef&)unit.ref(idx);
	Interval<float> timerg = ref.timeRange();
	if ( timerg.start < mytimerg.start || timerg.stop > mytimerg.stop )
	    { setNodesDefaultTimes( unit ); break; }
    }
}


int uiStratRefTree::getChildIdxFromTime( const Strat::NodeUnitRef& nur,
						float pos ) const
{
    if ( nur.isLeaved() ) return -1;
    for ( int idx=0; idx<nur.nrRefs(); idx++ )
    {
	Strat::NodeUnitRef& cnur = (Strat::NodeUnitRef& )(nur.ref(idx));
	if ( cnur.timeRange().start > pos )
	    return idx;
    }
    return nur.nrRefs();
}
