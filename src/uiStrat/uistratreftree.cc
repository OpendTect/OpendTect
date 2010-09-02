/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratreftree.cc,v 1.44 2010-09-02 16:22:43 cvsbruno Exp $";

#include "uistratreftree.h"

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
#include "uistratmgr.h"
#include "uistratutildlgs.h"

#define mAddCol(wdth,nr) \
    lv_->setColumnWidth( nr, wdth )

#define PMWIDTH		11
#define PMHEIGHT	9

static const int cUnitsCol	= 0;
static const int cDescCol	= 1;
static const int cLithoCol	= 2;

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
				.label(nur.code()).label(nur.description() ));

    if ( parlvit || !root )
    {
	ioPixmap* pm = createUnitPixmap( nur.props().color_ );
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
	    
	    ioPixmap* pm = createUnitPixmap( lur->props().color_ );
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
    setTree( uistratmgr_->getCurTree() );
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
    uiStratUnitDlg::Setup su( uistratmgr_ );
    const uiListViewItem* parit = lv_->currentItem();
    if ( parit )
    {
	Interval<float> timerg;
	uistratmgr_->getPossibleTimeRange(getCodeFromLVIt(parit).buf(), timerg);
	if ( timerg.start == timerg.stop )
	{ uiMSG().error( "No time space left to add a new sub-unit" ); return; }
	su.timerg_ = timerg;
    }

    uiStratUnitDlg newurdlg( lv_->parent(), su );
    if ( newurdlg.go() )
    {
	UnitRef::Props props; 
	BufferString lithnm;
	newurdlg.getUnitProps( props, lithnm );
	doInsertSubUnit( lvit, props, lithnm );
    }
}


void uiStratRefTree::subdivideUnit( uiListViewItem* lvit ) 
{
    if ( !lvit ) return;

    BufferString uncode = getCodeFromLVIt( lvit );
    const UnitRef* unitref = uistratmgr_->getCurTree()->find( uncode );
    if ( !unitref ) 
    { uiMSG().error( "Can not find unit" ); return; }
    uiListViewItem* parit = lvit->parent();
    if ( parit )
	lv_->setCurrentItem( parit );

    uiStratUnitDivideDlg dlg( lv_->parent(), *uistratmgr_, unitref->props() );
    if ( dlg.go() )
    {
	ObjectSet<Strat::UnitRef::Props> pps;
	dlg.gatherProps( pps );
	if ( pps.size() <= 0 )
	{ uiMSG().error( "no valid unit found" ); return; }

	for ( int idx=0; idx<pps.size(); idx++ )
	{
	    BufferString lvlnm( pps[idx]->code_ );
	    if ( idx == 0 )
		uistratmgr_->updateUnitProps( unitref->getID(), *pps[idx] );
	    else
		doInsertSubUnit( parit, *pps[idx], 0  );
	}
	deepErase( pps );
    }
}



void uiStratRefTree::doInsertSubUnit( uiListViewItem* lvit, UnitRef::Props& pp, 					const char* lithnm ) const
{
    uiListViewItem* newitem;
    uiListViewItem::Setup setup = uiListViewItem::Setup()
			    .label( pp.code_ )
			    .label( pp.desc_ )
			    .label( lithnm );
    newitem = lvit ? new uiListViewItem( lvit, setup )
		   : new uiListViewItem( lv_, setup );
    newitem->setRenameEnabled( cUnitsCol, false );	//TODO
    newitem->setRenameEnabled( cDescCol, false );	//TODO
    newitem->setRenameEnabled( cLithoCol, false );
    newitem->setDragEnabled( true );
    newitem->setDropEnabled( true );
    ioPixmap* pm = createUnitPixmap( pp.color_ );
    newitem->setPixmap( 0, *pm );
    delete pm;
   
    if ( !uistratmgr_->isNewUnitName( pp.code_ ) ) 
    { 
	uiMSG().error( "Can not insert sub-unit, unit name already in use" ); 
	return; 
    }

    lv_->setCurrentItem( newitem );
    uiListViewItem* parit = newitem->parent();
    if ( parit )
    {
	parit->setOpen( true );
	uistratmgr_->prepareParentUnit( getCodeFromLVIt( parit ).buf() );
    }
    uistratmgr_->addUnit( getCodeFromLVIt( newitem ), lithnm, pp, false );	
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
    BufferString uncode = getCodeFromLVIt( lvit );
    const UnitRef* unitref = uistratmgr_->getCurTree()->find( uncode );
    if ( !unitref ) 
	return;

    uiStratUnitDlg::Setup su( uistratmgr_ );
    su.entrancename_ = unitref->props().code_; 
    uiStratUnitDlg urdlg( lv_->parent(), su );
    urdlg.setTitleText("Update Unit Properties");
    UnitRef::Props props;
    props = unitref->props();
    props.code_ = lvit->text(cUnitsCol); 
    props.desc_ = lvit->text(cDescCol); 
    BufferString lithnm = lvit->text(cLithoCol); 
    urdlg.setUnitProps( props, lithnm, unitref->isLeaf() );

    if ( urdlg.go() )
    {
        urdlg.getUnitProps( props, lithnm );
	lvit->setText( props.code_, cUnitsCol );
	lvit->setText( props.desc_, cDescCol ); 
	lvit->setText( lithnm.buf(), cLithoCol );

	mDynamicCastGet(const Strat::NodeUnitRef*,nur,unitref)
	if ( nur )
	{
	    for ( int iref=nur->nrRefs()-1; iref>=0; iref-- )
	    {
		const UnitRef& ref = nur->ref( iref );
		if ( ref.props().timerg_.start >= props.timerg_.stop )
		    removeUnit( lvit->getChild( iref ) ) ;
	    }
	}
	updateUnitsPixmaps();
	uistratmgr_->updateUnitProps( unitref->getID(), props );
	if( unitref->isLeaf() )
	    uistratmgr_->updateUnitLith( unitref->getID(), lithnm.buf() );
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
    UnitRef::Iter it( *uistratmgr_->getCurTree() );
    const UnitRef* firstun = it.unit();
    if ( !firstun ) return;
    ioPixmap* pm = createUnitPixmap( firstun->props().color_ ); 
    uiListViewItem* firstlvit = lv_->findItem( firstun->code().buf(), 0, false);
    if ( firstlvit )
	firstlvit->setPixmap( 0, *pm );
    delete pm;
    while ( it.next() )
    {
	const UnitRef* un = it.unit();
	if ( !un ) continue;
	ioPixmap* pm = createUnitPixmap( un->props().color_ );
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


void uiStratRefTree::setUnitLvl( int unid ) 
{
    const UnitRef* unitref = uistratmgr_->getCurTree()->getByID( unid );
    if ( !unitref ) 
	return;

    uiStratLinkLvlUnitDlg dlg( lv_->parent(), unid, *uistratmgr_ );
    if ( dlg.go() )
    {
	UnitRef::Props pp;
	pp = unitref->props();
	pp.lvlid_ = dlg.lvlid_;
	uistratmgr_->updateUnitProps( unid, pp );
    }
}


void uiStratRefTree::setBottomLvl() 
{
    uiStratLinkLvlUnitDlg dlg( lv_->parent(), -1, *uistratmgr_ );
    if ( dlg.go() )
	uistratmgr_->setBotLvlID( dlg.lvlid_ );
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
	    mDynamicCastGet(const Strat::NodeUnitRef*,un,&node.ref(0))
	    if ( un )
		setUnconformities( *un, false, 0 );
	}
	return;
    }

#define mSetUpUnconf(timerg,pos)\
    Strat::UnitRef::Props props; props.timerg_ = timerg;\
    props.color_ = Color( 215, 215, 215 );\
    BufferString unconfcode("");\
    unconfcode += "Unconf";\
    unconfcode += toString(nrunconf++);\
    uiListViewItem* lit = listView()->findItem(node.code(),0,false);\
    if ( lit ) listView()->setCurrentItem(lit);\
    props.isunconf_ = true;\
    props.code_ = unconfcode;\
    props.timerg_ = timerg;\
    doInsertSubUnit( lit, props, 0 );\
    for ( int idref=0; idref<pos; idref++ )\
	moveUnit( true );\

    TypeSet< Interval<float> > timergs;
    tree_->getLeavesTimeGaps( node, timergs );
    for ( int idx=0; idx<timergs.size(); idx++ )
    {
	mSetUpUnconf( timergs[idx], node.nrRefs() - idx );
    }
    for ( int iref=0; iref<node.nrRefs(); iref++ )
    {
	const Strat::UnitRef& ref = node.ref( iref );
	mDynamicCastGet(const Strat::NodeUnitRef*,chldnur,&ref);
	if ( chldnur )
	    setUnconformities( *chldnur, false, nrunconf );
    }
}
