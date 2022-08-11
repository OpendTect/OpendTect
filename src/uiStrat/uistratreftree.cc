/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2007
________________________________________________________________________

-*/

#include "uistratreftree.h"

#include "iopar.h"
#include "randcolor.h"
#include "stratreftree.h"
#include "stratunitref.h"
#include "stratunitrefiter.h"
#include "sorting.h"

#include "uigeninput.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uipixmap.h"
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
    , anychange_(false)
{
    lv_ = new uiTreeView( p, "RefTree viewer" );
    BufferStringSet labels;
    labels.add( "Unit" );
    labels.add( "Description" );
    lv_->addColumns( labels );
    mAddCol( 300, 0 );
    mAddCol( 200, 1 );
    mAddCol( 150, 2 );
    lv_->setMinimumWidth( 650 );
    lv_->setMinimumHeight( 400 );
    lv_->setStretch( 2, 2 );
    lv_->rightButtonClicked.notify( mCB( this,uiStratRefTree,rClickCB ) );
    lv_->mouseButtonPressed.notify( mCB( this,uiStratRefTree,mousePressedCB ) );
    lv_->rightButtonPressed.notify( mCB( this,uiStratRefTree,mousePressedCB ) );

    tree_ = 0;
    setTree( eRT() );
}


uiStratRefTree::~uiStratRefTree()
{
}


void uiStratRefTree::setTree()
{
    setTree( eRT(), true );
}


void uiStratRefTree::setTree( RefTree& rt, bool force )
{
    if ( !force && &rt == tree_ ) return;

    tree_ = &rt;
    if ( !tree_ ) return;

    lv_->clear();
    addNode( 0, *((NodeUnitRef*)tree_), true );

    if  ( !haveTimes() )
	setEntranceDefaultTimes();

}


void uiStratRefTree::setName( const char* nm )
{
    if ( tree_ )
	tree_->name_ = nm;
}


const char* uiStratRefTree::name() const
{
    return tree_ ? tree_->name_.buf() : nullptr;
}


#define mCreateAndSetUnitPixmap(ur,lvit)\
    mDynamicCastGet(const LeafUnitRef*,lfur,&ur) \
    uiPixmap* pm = createUnitPixmap(lfur ? lfur->dispColor(true) : ur.color());\
    lvit->setPixmap( 0, *pm );\
    delete pm;

void uiStratRefTree::addNode( uiTreeViewItem* parlvit,
			      const NodeUnitRef& nur, bool root )
{
    uiTreeViewItem* lvit = parlvit
	? new uiTreeViewItem( parlvit, uiTreeViewItem::Setup()
				.label(toUiString(nur.code()))
				.label(mToUiStringTodo(nur.description())) )
	: root ? 0 : new uiTreeViewItem( lv_,uiTreeViewItem::Setup()
				.label(toUiString(nur.code()))
				.label(mToUiStringTodo(nur.description()) ));

    if ( parlvit || !root )
	{ mCreateAndSetUnitPixmap( nur, lvit ) }

    for ( int iref=0; iref<nur.nrRefs(); iref++ )
    {
	const UnitRef& ref = nur.ref( iref );
	if ( ref.isLeaf() )
	{
	    uiTreeViewItem* item;
	    mDynamicCastGet(const LeafUnitRef*,lur,&ref);
	    if ( !lur )
		continue;

	    uiTreeViewItem::Setup setup = uiTreeViewItem::Setup()
				.label( toUiString(lur->code()) )
				.label( mToUiStringTodo(lur->description()) );
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
    if ( StringView(lvit->text()) == RefTree::sKeyNoCode() )
	{ updateUnitProperties( lvit ); return; }

    uiMenu mnu( lv_->parent(), uiStrings::sAction() );

    bool caninsertsubitem = true;
    const UnitRef* un = lvit ? tree_->find( getFullCodeFromLVIt(lvit) )
				    : 0;
    if ( !(lvit && ( !un || un->isLeaf() )) )
    {
	NodeUnitRef* parun = lvit ? (NodeUnitRef*)un : tree_;
	if ( parun && parun->treeDepth()>=5 ) //avoid overwriting litho column
	    caninsertsubitem = false;
    }
    if ( caninsertsubitem )
	mnu.insertAction( new uiAction(m3Dots(tr("Create sub-unit"))), 0 );
    if ( isLeaved( lvit ) )
	mnu.insertAction( new uiAction(m3Dots(tr("Subdivide unit"))), 1 );
    mnu.insertAction( new uiAction(uiStrings::sProperties()), 2 );
    if ( lv_->currentItem() != lv_->firstItem() )
	mnu.insertAction( new uiAction(uiStrings::sRemove()), 3 );
    if ( lv_->currentItem() == lv_->firstItem() )
	 mnu.insertAction( new uiAction(tr("Add Unit below")), 4 );
    if ( isLeaved( lvit ) )
    {
	mnu.insertSeparator();
	mnu.insertAction( new uiAction(tr("Assign marker boundary")), 5 );
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
    const UnitRef* un = lvit ? tree_->find( getFullCodeFromLVIt(lvit) )
		      : nullptr;
    if ( lvit && ( !un || un->isLeaf() ) ) return;

    NodeUnitRef* parun = lvit ? (NodeUnitRef*)un : tree_;
    LeavedUnitRef tmpun( parun, 0 );

    Interval<float> trg; getAvailableTime( *parun, trg );
    tmpun.setTimeRange( trg );
    tmpun.setColor( OD::getRandStdDrawColor() );

    if ( parun->isLeaved() )
    {
	const LeavedUnitRef& lvdun = (LeavedUnitRef&)(*parun);
	tmpun.setLevelID( lvdun.levelID() );
	for ( int iref=0; iref<lvdun.nrRefs(); iref++ )
	{
	    const Strat::LithologyID id =
				((LeafUnitRef&)(lvdun.ref(iref))).lithology();
	    auto* lur = new LeafUnitRef( &tmpun, id );
	    tmpun.add( lur );
	}
    }

    uiStratUnitEditDlg newurdlg( lv_->parent(), tmpun );
    if ( !newurdlg.go() )
	return;

    if ( parun->isLeaved() )
    {
	for ( int iref=parun->nrRefs()-1; iref>=0; iref-- )
	    removeUnit( lvit->getChild( iref ) );
	parun = replaceUnit( *parun, false );
	tmpun.setUpNode( nullptr );
    }

    if ( !parun )
    {
	uiMSG().error( tr("Cannot add unit") );
	return;
    }

    auto* newun = new LeavedUnitRef( parun, tmpun.code());
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
    anychange_ = true;
}


void uiStratRefTree::addLithologies( LeavedUnitRef& un,
				     const TypeSet<Strat::LithologyID>& ids )
{
    uiTreeViewItem* lvit = getLVItFromFullCode( un.fullCode().buf() );
    if ( !lvit )
	return;

    for ( int idx=0; idx<ids.size(); idx++ )
    {
	auto* lur = new LeafUnitRef( &un, ids[idx] );
	un.add( lur );
	insertUnitInLVIT( lvit, idx, *lur );
    }
}


NodeUnitRef* uiStratRefTree::replaceUnit( NodeUnitRef& un, bool byleaved)
{
    NodeUnitRef* upnode = un.upNode();
    if ( !upnode )
	return nullptr;

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

    UnitRef* startunit = tree_->find( getFullCodeFromLVIt( lvit ) );
    if ( !startunit || !startunit->isLeaved() )
	return;
    LeavedUnitRef& ldur = (LeavedUnitRef&)(*startunit);

    NodeUnitRef* parnode = startunit->upNode();
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
	ObjectSet<LeavedUnitRef> units;
	dlg.gatherUnits( units );

	TypeSet<Strat::LithologyID> lithids;
	for ( int idx=0; idx<ldur.nrRefs(); idx++ )
	    lithids += ((LeafUnitRef&)(ldur.ref(idx))).lithology();

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
	anychange_ = true;
	updateUnitsPixmaps();
	tree_->unitAdded.trigger();
    }
}


void uiStratRefTree::insertUnitInLVIT( uiTreeViewItem* lvit, int posidx,
					const UnitRef& unit ) const
{
    uiTreeViewItem::Setup setup = uiTreeViewItem::Setup().label( toUiString(
								unit.code()) );
    uiTreeViewItem* newitem = lvit ? new uiTreeViewItem( lvit , setup )
				   : new uiTreeViewItem( lv_, setup );
    newitem->setRenameEnabled( cUnitsCol, false );	//TODO
    newitem->setRenameEnabled( cDescCol, false );	//TODO
    newitem->setRenameEnabled( cLithoCol, false );
    newitem->setDragEnabled( true );
    newitem->setDropEnabled( true );
    newitem->setText( mToUiStringTodo(unit.description()), cDescCol );

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
    UnitRef* un = tree_->find( getFullCodeFromLVIt( lvit ) );
    if ( !un ) return;
    NodeUnitRef* upnode = un->upNode();
    if ( !upnode ) return;

    TypeSet<Strat::LithologyID> lithids;
    Strat::LevelID lvlid;
    if ( un->isLeaved() )
    {
	const LeavedUnitRef& lvedun = (LeavedUnitRef&)(*un);
	for ( int idx=0; idx<lvedun.nrRefs(); idx++ )
	    lithids += ((LeafUnitRef&)(lvedun.ref(idx))).lithology();
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

    anychange_ = true;
    lv_->triggerUpdate();
}


void uiStratRefTree::updateUnitProperties( uiTreeViewItem* lvit )
{
    UnitRef* unitref = tree_->find( getFullCodeFromLVIt(lvit) );
    if ( !unitref || unitref->isLeaf() ) return;
    unitref->setCode( lvit->text(cUnitsCol) );
    unitref->setDescription( lvit->text(cDescCol) );

    NodeUnitRef& nur = (NodeUnitRef&)(*unitref);
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

	lvit->setText( toUiString(unitref->code()), cUnitsCol );
	lvit->setText( mToUiStringTodo(unitref->description()), cDescCol );
	tree_->unitChanged.trigger();
	updateUnitsPixmaps();
	anychange_ = true;
    }
}


uiPixmap* uiStratRefTree::createUnitPixmap( const OD::Color& col ) const
{
    uiRGBArray rgbarr( false );
    rgbarr.setSize( PMWIDTH, PMHEIGHT );
    rgbarr.clear( OD::Color::White() );

    for ( int idw=0; idw<PMWIDTH; idw++ )
    {
	for ( int idh=0; idh<PMHEIGHT; idh++ )
	{
	    rgbarr.set( idw, idh, col );
	    rgbarr.set( idw, idh, col );
	    rgbarr.set( idw, idh, col );
	}
    }
    return new uiPixmap( rgbarr );
}


uiTreeViewItem* uiStratRefTree::getLVItFromFullCode( const char* code ) const
{
    uiTreeViewItemIterator it( *lv_ );
    uiTreeViewItem* lvit = it.next();
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
	if ( bs == code )
	    return lvit;

	lvit = it.next();
    }
    return 0;
}


BufferString uiStratRefTree::getFullCodeFromLVIt( const uiTreeViewItem* item )
    const
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
    UnitRefIter it( *tree_ );
    const UnitRef* firstun = it.unit();
    if ( !firstun )
	return;

    uiTreeViewItem* lvit = lv_->findItem( firstun->code().buf(), 0, false);
    if ( lvit )
    {
	mCreateAndSetUnitPixmap( (*firstun), lvit )
    }

    while ( it.next() )
    {
	const UnitRef* un = it.unit();
	if ( !un )
	    continue;

	lvit = lv_->findItem( un->code().buf(), 0, false );
	if ( lvit )
	{
	    mCreateAndSetUnitPixmap( (*un), lvit )
	}
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
	if ( (up && curidx==0) || (!up && curidx==lv_->nrItems()-1) )
	    return;

	targetit = lv_->getItem( up ? curidx-1 : curidx+1 );
	lv_->takeItem( curit );
	lv_->insertItem( up ? curidx-1 : curidx+1, curit );
    }

    curit->setOpen( isexpanded );
    lv_->setCurrentItem(curit);

    UnitRef* curun = tree_->find( getFullCodeFromLVIt(curit) );
    UnitRef* targetun = tree_->find( getFullCodeFromLVIt(targetit) );
    if ( !curun || !targetun ) return;
    NodeUnitRef* upnode = curun->upNode();
    if ( !upnode ) return;

    upnode->swapChildren( upnode->indexOf(curun), upnode->indexOf(targetun) );
    anychange_ = true;
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
    const UnitRef* un = lvit ? tree_->find( getFullCodeFromLVIt(lvit) )
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
    mDynamicCastGet(LeavedUnitRef*,ldun,unitref)
    if ( !ldun )
	return;

    uiStratLinkLvlUnitDlg dlg( lv_->parent(), *ldun );
    if ( dlg.go() )
    {
	anychange_ = true;
	tree_->unitChanged.trigger();
    }
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


void uiStratRefTree::setNodesDefaultTimes( const NodeUnitRef& startnode )
{
    UnitRefIter it( startnode, UnitRefIter::AllNodes );
    NodeUnitRef* un = const_cast<NodeUnitRef*>( &startnode );
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


void uiStratRefTree::getAvailableTime( const NodeUnitRef& unit,
					Interval<float>& timerg ) const
{
    timerg = unit.timeRange();
    if ( !unit.nrRefs() || unit.isLeaved() )
	return;

    const NodeUnitRef& firstun = (NodeUnitRef&)unit.ref(0);
    if ( timerg.start < firstun.timeRange().start )
	{ timerg.stop = firstun.timeRange().start; return; }
    const NodeUnitRef& lastun = (NodeUnitRef&)unit.ref( unit.nrRefs()-1 );
    if ( timerg.stop > lastun.timeRange().stop )
	{ timerg.start = lastun.timeRange().stop; return; }

    Interval<float> trg( 0, 0 );
    for ( int idx=0; idx<unit.nrRefs()-1; idx++ )
    {
	const NodeUnitRef& cldun = (NodeUnitRef&)unit.ref(idx);
	const NodeUnitRef& nldun = (NodeUnitRef&)unit.ref(idx +1);
	timerg = nldun.timeRange();
	trg.set( cldun.timeRange().stop, nldun.timeRange().start );
	if ( trg.width() > 0 )
	   { timerg = trg; return; }
    }
    timerg.set( timerg.stop-timerg.width()/2, timerg.stop );
}


void uiStratRefTree::ensureUnitTimeOK( NodeUnitRef& unit )
{
    const NodeUnitRef* par = unit.upNode();
    if ( !par ) return;

    Interval<float> mytimerg( unit.timeRange() );
    const Interval<float> partrg( par->timeRange() );
    mytimerg.limitTo( partrg );

    const int posidx = getChildIdxFromTime( *par, mytimerg.start )-1;
    for ( int idx=0; idx<par->nrRefs(); idx++ )
    {
	NodeUnitRef& ref = (NodeUnitRef&)par->ref(idx);
	Interval<float> timerg = ref.timeRange();
	if ( &ref == &unit ) continue;
	if ( posidx-1 == idx && mytimerg.start < timerg.start )
	    mytimerg.start = timerg.start;
	else if ( posidx+1 == idx && mytimerg.stop > timerg.stop )
	    mytimerg.stop = timerg.stop;
    }

    for ( int idx=0; idx<par->nrRefs(); idx++ )
    {
	NodeUnitRef& ref = (NodeUnitRef&)par->ref(idx);
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
	NodeUnitRef& ref = (NodeUnitRef&)unit.ref(idx);
	Interval<float> timerg = ref.timeRange();
	if ( timerg.start < mytimerg.start || timerg.stop > mytimerg.stop )
	    { setNodesDefaultTimes( unit ); break; }
    }
}


int uiStratRefTree::getChildIdxFromTime( const NodeUnitRef& nur,
						float pos ) const
{
    if ( nur.isLeaved() ) return -1;
    for ( int idx=0; idx<nur.nrRefs(); idx++ )
    {
	NodeUnitRef& cnur = (NodeUnitRef& )(nur.ref(idx));
	if ( cnur.timeRange().start > pos )
	    return idx;
    }
    return nur.nrRefs();
}
