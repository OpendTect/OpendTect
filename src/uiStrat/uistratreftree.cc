/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratreftree.cc,v 1.37 2010-06-24 11:54:01 cvsbruno Exp $";

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
	ioPixmap* pm = createUnitPixmap( &nur );
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
	    
	    ioPixmap* pm = createUnitPixmap( lur );
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

    int col = lv_->columnNotified();
    uiPopupMenu mnu( lv_->parent(), "Action" );
    mnu.insertSeparator();
    mnu.insertItem( new uiMenuItem("&Create sub-unit..."), 0 );
    mnu.insertItem( new uiMenuItem("&Remove"), 1 );
    mnu.insertItem( new uiMenuItem("&Properties..."), 2 );

    const int mnuid = mnu.exec();
    if ( mnuid<0 ) return;
    else if ( mnuid==0 )
	insertSubUnit( lvit );
    else if ( mnuid== 1 )
	removeUnit( lvit );
    else if ( mnuid==2 )
	updateUnitProperties( lvit );
}


void uiStratRefTree::insertSubUnit( uiListViewItem* lvit )
{
    uiStratUnitDlg::Setup su( uistratmgr_ );
    const uiListViewItem* parit = lv_->currentItem();
    if ( parit )
    {
	Interval<float> partimerg;
	uistratmgr_->getPossibleTimeRange( getCodeFromLVIt(parit).buf(), 
					   partimerg );
	if (  partimerg.start == partimerg.stop )
	{ uiMSG().error( "No time space left to add a new sub-unit" ); return; }
	su.timerg_ = partimerg;
    }

    uiStratUnitDlg newurdlg( lv_->parent(), su );
    if ( newurdlg.go() )
    {
	UnitRef::Props props; 
	newurdlg.getUnitProps( props );
	doInsertSubUnit( lvit, props );
    }
}


void uiStratRefTree::doInsertSubUnit( uiListViewItem* lvit, UnitRef::Props& pp )
{
    uiListViewItem* newitem;
    uiListViewItem::Setup setup = uiListViewItem::Setup()
			    .label( pp.code_ )
			    .label( pp.desc_ )
			    .label( pp.lithnm_ );
    newitem = lvit ? new uiListViewItem( lvit, setup )
		   : new uiListViewItem( lv_, setup );
    newitem->setRenameEnabled( cUnitsCol, false );	//TODO
    newitem->setRenameEnabled( cDescCol, false );	//TODO
    newitem->setRenameEnabled( cLithoCol, false );
    newitem->setDragEnabled( true );
    newitem->setDropEnabled( true );
    ioPixmap* pm = createUnitPixmap(0);
    newitem->setPixmap( 0, *pm );
    delete pm;
    
    lv_->setCurrentItem( newitem );
    uiListViewItem* parit = newitem->parent();
    if ( parit )
    {
	parit->setOpen( true );
	uistratmgr_->prepareParentUnit( getCodeFromLVIt( parit ).buf() );
    }
    pp.code_ = getCodeFromLVIt( newitem );	
    uistratmgr_->addUnit( pp, false );
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
    uiStratUnitDlg::Setup su( uistratmgr_ );
    uiStratUnitDlg urdlg( lv_->parent(), su );
    urdlg.setTitleText("Update Unit Properties");
    BufferString uncode = getCodeFromLVIt( lvit);
    const UnitRef* unitref = uistratmgr_->getCurTree()->find( uncode );
    UnitRef::Props props;
    if ( unitref )
	props = unitref->props();
    props.code_ = lvit->text(0); 
    props.desc_ = lvit->text(2); 
    props.lithnm_ = lvit->text(cLithoCol); 
    urdlg.setUnitProps( props );

    if ( urdlg.go() )
    {
	//TODO will require an update of all children
	//lvit->setText( urdlg.getUnitName(), cUnitsCol );
        urdlg.getUnitProps( props );
	lvit->setText( props.desc_, cDescCol ); 
	lvit->setText( props.lithnm_, cLithoCol );
	props.code_ = uncode;
	uistratmgr_->updateUnitProps( props );
    }
}


ioPixmap* uiStratRefTree::createUnitPixmap( const UnitRef* ref ) const
{
    uiRGBArray rgbarr( false );
    rgbarr.setSize( PMWIDTH, PMHEIGHT );
    rgbarr.clear( Color::White() );

    if ( ref )
    {
	Color col = ref->props().color_;
	for ( int idw=0; idw<PMWIDTH; idw++ )
	{
	    for ( int idh=0; idh<PMHEIGHT; idh++ )
	    {
		rgbarr.set( idw, idh, col );
		rgbarr.set( idw, idh, col );
		rgbarr.set( idw, idh, col );
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
    ioPixmap* pm = createUnitPixmap( firstun );
    uiListViewItem* firstlvit = lv_->findItem( firstun->code().buf(),0,false );
    if ( firstlvit )
	firstlvit->setPixmap( 0, *pm );
    delete pm;
    while ( it.next() )
    {
	const UnitRef* un = it.unit();
	ioPixmap* pm = createUnitPixmap( un );
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



void uiStratRefTree::resetUnconformities( CallBacker* cb )
{
    setUnconformities( *((NodeUnitRef*)tree_), true );
}


#define mSetUpUnconf(timerg,pos)\
    Strat::UnitRef::Props props; props.timerg_ = timerg;\
    props.color_ = Color( 215, 215, 215 );\
    BufferString unconfcode( "" );\
    unconfcode += "Unconf";\
    unconfcode += toString(cpt);\
    uiListViewItem* lit = listView()->findItem(node.code(),0,false);\
    if ( lit ) listView()->setCurrentItem(lit);\
    props.code_ = unconfcode;\
    props.timerg_ = timerg;\
    doInsertSubUnit( lit, props );\
    for ( int idref=0; idref<pos; idref++ )\
	moveUnit( true );
void uiStratRefTree::setUnconformities( const Strat::NodeUnitRef& node, bool root )
{
    if ( root )
    {
	mDynamicCastGet(const Strat::NodeUnitRef*,un,&node.ref(0))
	setUnconformities( *un, false );
	return;
    }

    const Interval<float> partimerg = node.props().timerg_; int cpt = 1; 
    ObjectSet<UnitRef> refunits;
    tree_->gatherChildrenByTime( node, refunits );
    Interval<float> unconfrg;
    const float refstart = refunits[0]->props().timerg_.start;
    const float refstop = refunits[refunits.size()-1]->props().timerg_.stop;
    if ( partimerg.start < refstart )
    {
	unconfrg.set( partimerg.start, refstart );
	mSetUpUnconf( unconfrg, node.nrRefs() );
	cpt ++;
    }
    if ( partimerg.stop > refstop )
    {
	unconfrg.set( refstop, partimerg.stop );
	mSetUpUnconf( unconfrg, 0 )
	cpt ++;
    }
    for ( int iref=0; iref<refunits.size()-1; iref++ )
    {
	const float refstop = refunits[iref]->props().timerg_.stop;
	const float nextrefstart = refunits[iref+1]->props().timerg_.start;
	if ( refstop < nextrefstart )
	{
	    unconfrg.set( refstop, nextrefstart );
	    mSetUpUnconf( unconfrg, iref+1 )
	    cpt ++;
	}
    }
    for ( int iref=0; iref<node.nrRefs(); iref++ )
    {
	const Strat::UnitRef& ref = node.ref( iref );
	mDynamicCastGet(const Strat::NodeUnitRef*,chldnur,&ref);
	if ( chldnur )
	    setUnconformities( *chldnur, false );
    }
}
