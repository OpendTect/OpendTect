/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/

#include "uistratdispdata.h"
#include "uistratreftree.h"
#include "uitreeview.h"

#include "iopar.h"
#include "color.h"
#include "stratreftree.h"
#include "stratunitrefiter.h"
#include "stratlevel.h"
#include "stratlith.h"


StratDispData::StratDispData()
{}


StratDispData::~StratDispData()
{
    eraseData();
}


void StratDispData::eraseData()
{
    deepErase( cols_ );
}


void StratDispData::addCol( Column* col )
{ cols_ += col; }


int StratDispData::nrCols() const
{ return cols_.size(); }


int StratDispData::nrUnits( int colidx ) const
{
    const Column* col = getCol( colidx );
    return col ? col->units_.size() : -1;
}


void StratDispData::addUnit( int colidx, Unit* un )
{
    Column* col = getCol( colidx );
    if ( !col ) return;

    col->units_ += un;
    un->colidx_ = colidx;
}


StratDispData::Column* StratDispData::getCol( int idx )
{ return cols_.validIdx(idx) ? cols_[idx] : 0; }

const StratDispData::Column* StratDispData::getCol( int idx ) const
{ return const_cast<StratDispData*>(this)->getCol(idx); }


StratDispData::Unit* StratDispData::getUnit( int colidx, int uidx )
{
    Column* col = getCol( colidx );
    return col && col->units_.validIdx(uidx) ? col->units_[uidx] : 0;
}


const StratDispData::Unit* StratDispData::getUnit( int colidx, int uidx ) const
{ return const_cast<StratDispData*>(this)->getUnit( colidx, uidx ); }


int StratDispData::nrLevels( int colidx ) const
{
    const Column* col = getCol( colidx );
    return col ? col->levels_.size() : -1;
}


const StratDispData::Level* StratDispData::getLevel( int colidx, int lidx ) const
{
    const Column* col = getCol( colidx );
    return col && col->levels_.validIdx(lidx) ? col->levels_[lidx] : 0;
}


int StratDispData::nrDisplayedCols() const
{
    int nr = 0;
    for ( int idx=0; idx<cols_.size(); idx++)
	{ if ( cols_[idx]->isdisplayed_ ) nr++; }
    return nr;
}



// uiStratTreeToDisp

#define mAskStratNotif(obj,nm)\
    (obj)->nm.notify(mCB(this,uiStratTreeToDisp,triggerDataChange));

uiStratTreeToDisp::uiStratTreeToDisp( StratDispData& ad,
					bool witauxs, bool withlvls  )
    : data_(ad)
    , withauxs_(witauxs)
    , withlevels_(withlvls)
    , newtreeRead(this)
{
    setTree();
}


uiStratTreeToDisp::~uiStratTreeToDisp()
{
    detachAllNotifiers();
}


#define mAskStratNotif(obj,nm)\
    (obj)->nm.notify(mCB(this,uiStratTreeToDisp,triggerDataChange));


void uiStratTreeToDisp::setTree()
{
    tree_ = &Strat::eRT();
    if ( !tree_ )
	return;

    tree_->toBeDeleted.notify(mCB(this,uiStratTreeToDisp,treeDel));
    mAskStratNotif(tree_,unitAdded)
    mAskStratNotif(tree_,unitChanged)
    mAskStratNotif(tree_,unitToBeDeleted)
    mAskStratNotif(&Strat::eLVLS(),objectChanged())

    readFromTree();
}


void uiStratTreeToDisp::treeDel( CallBacker* )
{
    tree_ = 0;
}


void uiStratTreeToDisp::triggerDataChange( CallBacker* )
{
    readFromTree();
}


void uiStratTreeToDisp::readFromTree()
{
    data_.eraseData();
    const char* colnms[] = { "Super Group", "Group", "Formation",
			     "Member", "Type", 0 };

    for ( int idcol=0; colnms[idcol]; idcol++ )
    {
	StratDispData::Column* col =
			    new StratDispData::Column( colnms[idcol] );
	data_.addCol( col );
    }
    if ( withauxs_ )
    {
	lithocolidx_ = data_.nrCols();
	data_.addCol( new StratDispData::Column( "Lithologies" ) );
	desccolidx_ = data_.nrCols();
	data_.addCol( new StratDispData::Column( "Description" ) );
    }
    if ( withlevels_ )
    {
	levelcolidx_ = data_.nrCols();
	data_.addCol( new StratDispData::Column( "Boundaries" ) );
    }

    readUnits();
}


void uiStratTreeToDisp::readUnits()
{
    if ( !tree_ ) return;

    Strat::UnitRefIter it( *tree_, Strat::UnitRefIter::AllNodes );
    while ( it.next() )
    {
	const Strat::NodeUnitRef* un = (Strat::NodeUnitRef*)it.unit();
	if ( un && un->treeDepth() <= data_.nrCols() )
	{
	    addUnit( (Strat::NodeUnitRef&)(*un) );
	    if ( un->isLeaved() )
	    {
		const Strat::LeavedUnitRef& lur = (Strat::LeavedUnitRef&)(*un);
		if ( withauxs_ )
		{
		    addLithologies( lur );
		    addDescs( lur  );
		}
		if ( withlevels_ )
		    addLevel( lur );
	    }
	}
    }
    newtreeRead.trigger();
}


void uiStratTreeToDisp::addUnit( const Strat::NodeUnitRef& ur )
{
    StratDispData::Unit* un = new StratDispData::Unit( ur.code(), ur.fullCode(),
							ur.color() );
    un->color_.setTransparency( 0 );
    un->zrg_ = ur.timeRange();
    data_.addUnit( ur.treeDepth()-1, un );
}


void uiStratTreeToDisp::addDescs( const Strat::LeavedUnitRef& ur )
{
    StratDispData::Unit* un = new StratDispData::Unit( ur.description() );
    un->zrg_ = ur.timeRange();
    data_.addUnit( desccolidx_, un );
}


void uiStratTreeToDisp::addLithologies( const Strat::LeavedUnitRef& ur )
{
    if ( !tree_ ) return;

    const Strat::LithologySet& lithos = tree_->lithologies();
    BufferString lithnm;
    for ( int idx=0; idx<ur.nrRefs(); idx++ )
    {
	mDynamicCastGet(const Strat::LeafUnitRef*,lref,&ur.ref(idx));
	if ( !lref )
	    continue;

	const int lithidx = lref->lithology();
	const Strat::Lithology* lith = lithidx >= 0 ? lithos.get( lithidx ) : 0;
	if ( lith ) { if ( idx ) lithnm += ", "; lithnm += lith->name(); }
    }
    StratDispData::Unit* un = new StratDispData::Unit( lithnm.buf() );
    un->zrg_ = ur.timeRange();
    data_.addUnit( lithocolidx_, un );
}


void uiStratTreeToDisp::addLevel( const Strat::LeavedUnitRef& ur )
{
    BufferString lvlnm; Color lvlcol;
    const Strat::Level::ID id = ur.levelID();
    const Strat::LevelSet& lvls = Strat::LVLS();
    lvlcol = lvls.isPresent( id ) ? lvls.colorOf( id ) : Color::Black();
    lvlnm = lvls.isPresent( id ) ? lvls.nameOf( id ) : "";

    auto* lvl = new StratDispData::Level( lvlnm, ur.fullCode() );
    lvl->zpos_ = ur.timeRange().start;
    lvl->color_ = lvlcol;
    data_.getCol( levelcolidx_ )->levels_ += lvl;
}



// uiStratDispToTree
uiStratDispToTree::uiStratDispToTree( uiStratRefTree& uitree )
    : uitree_(uitree)
{}


uiStratDispToTree::~uiStratDispToTree()
{}


uiTreeViewItem* uiStratDispToTree::setCurrentTreeItem( const char* txt )
{
    uiTreeViewItem* lit = uitree_.getLVItFromFullCode( txt);
    if ( lit )
	uitree_.treeView()->setCurrentItem(lit);

    return lit;
}


void uiStratDispToTree::handleUnitMenu( const char* txt )
{
    if ( !txt )
	return;

    uiTreeViewItem* lit = setCurrentTreeItem( txt );
    if ( lit )
	uitree_.handleMenu( lit );
}


void uiStratDispToTree::handleUnitProperties( const char* txt )
{
    if ( !txt )
	return;

    uiTreeViewItem* lit = setCurrentTreeItem( txt );
    if ( lit )
	uitree_.updateUnitProperties( lit );
}


void uiStratDispToTree::addUnit( const char* txt )
{
    if ( txt )
    {
	uiTreeViewItem* lit = setCurrentTreeItem( txt );
	if ( lit )
	    uitree_.insertSubUnit( lit );
    }
    else
	uitree_.insertSubUnit( 0 );
}



void uiStratDispToTree::setUnitLvl( const char* code )
{
    uitree_.setUnitLvl( code );
}
