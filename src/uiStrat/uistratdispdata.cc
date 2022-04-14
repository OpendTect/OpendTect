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

#include "keystrs.h"
#include "iopar.h"
#include "bufstringset.h"
#include "color.h"
#include "stratreftree.h"
#include "stratunitrefiter.h"
#include "stratlevel.h"
#include "stratlith.h"


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


void uiStratTreeToDisp::setTree()
{
    tree_ = &Strat::eRT();
    if ( !tree_ )
	return;

    mAttachCB( tree_->objectToBeDeleted, uiStratTreeToDisp::treeDel );
    mAttachCB( tree_->unitAdded, uiStratTreeToDisp::triggerDataChange );
    mAttachCB( tree_->unitChanged, uiStratTreeToDisp::triggerDataChange );
    mAttachCB( tree_->unitToBeDeleted, uiStratTreeToDisp::triggerDataChange );
    mAttachCB( Strat::eLVLS().changed, uiStratTreeToDisp::triggerDataChange );
    mAttachCB( Strat::eLVLS().levelAdded,
	       uiStratTreeToDisp::triggerDataChange );
    mAttachCB( Strat::eLVLS().levelToBeRemoved,
	       uiStratTreeToDisp::triggerDataChange );

    readFromTree();
}


void uiStratTreeToDisp::treeDel( CallBacker* )
{
    tree_ = nullptr;
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
	auto* col = new StratDispData::Column( colnms[idcol] );
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
    auto* un =
	new StratDispData::Unit( ur.code(), ur.fullCode().buf(), ur.color() );
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
    if ( !tree_ )
	return;

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

    auto* un = new StratDispData::Unit( lithnm.buf() );
    un->zrg_ = ur.timeRange();
    data_.addUnit( lithocolidx_, un );
}


void uiStratTreeToDisp::addLevel( const Strat::LeavedUnitRef& ur )
{
    BufferString lvlnm;
    OD::Color lvlcol;
    const Strat::Level::ID id = ur.levelID();
    const Strat::LevelSet& lvls = Strat::LVLS();
    lvlcol = lvls.isPresent( id ) ? lvls.colorOf( id ) : OD::Color::Black();
    lvlnm = lvls.isPresent( id ) ? lvls.nameOf( id ) : BufferString::empty();

    auto* lvl = new StratDispData::Level( lvlnm.buf(), ur.fullCode().buf() );
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
