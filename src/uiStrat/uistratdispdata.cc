/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratdispdata.cc,v 1.27 2012/07/03 12:05:37 cvsbruno Exp $";

#include "uistratdispdata.h"
#include "uistratreftree.h"
#include "uilistview.h"

#include "keystrs.h"
#include "iopar.h"
#include "bufstringset.h"
#include "color.h"
#include "stratreftree.h"
#include "stratunitrefiter.h"
#include "stratlevel.h"
#include "stratlith.h"


#define mAskStratNotif(obj,nm,act)\
    (obj)->nm.act(mCB(this,uiStratTreeToDispTransl,triggerDataChange));

uiStratTreeToDispTransl::uiStratTreeToDispTransl( StratDispData& ad, 
					bool witauxs, bool withlvls  ) 
    : data_(ad)
    , withauxs_(witauxs)		  
    , withlevels_(withlvls)  
    , newtreeRead(this)
{
    setTree();
}


uiStratTreeToDispTransl::~uiStratTreeToDispTransl()
{
    mAskStratNotif(&Strat::eLVLS(),levelChanged,remove)
    if ( tree_ )
    {
	tree_->deleteNotif.remove(mCB(this,uiStratTreeToDispTransl,treeDel));
	mAskStratNotif(tree_,unitAdded,remove)
	mAskStratNotif(tree_,unitChanged,remove)
	mAskStratNotif(tree_,unitToBeDeleted,remove)
    }
}


void uiStratTreeToDispTransl::setTree()
{
    tree_ = &Strat::eRT();
    if ( !tree_ ) 
	return;

    tree_->deleteNotif.notify(mCB(this,uiStratTreeToDispTransl,treeDel));
    mAskStratNotif(tree_,unitAdded,notify)
    mAskStratNotif(tree_,unitChanged,notify)
    mAskStratNotif(tree_,unitToBeDeleted,notify)
    mAskStratNotif(&Strat::eLVLS(),levelChanged,notify)

    readFromTree();
}


void uiStratTreeToDispTransl::treeDel( CallBacker* )
{
    tree_ = 0;
}


void uiStratTreeToDispTransl::triggerDataChange( CallBacker* )
{
    readFromTree();
}


void uiStratTreeToDispTransl::readFromTree()
{
    data_.eraseData();
    static const char* colnms[] = { "Super Group", "Group", "Formation", 
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


void uiStratTreeToDispTransl::readUnits()
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


void uiStratTreeToDispTransl::addUnit( const Strat::NodeUnitRef& ur ) 
{
    StratDispData::Unit* un = new StratDispData::Unit( ur.code(), ur.fullCode(), 							ur.color() );
    un->color_.setTransparency( 0 );
    un->zrg_ = ur.timeRange();
    data_.addUnit( ur.treeDepth()-1, un );
}


void uiStratTreeToDispTransl::addDescs( const Strat::LeavedUnitRef& ur ) 
{
    StratDispData::Unit* un = new StratDispData::Unit( ur.description() );
    un->zrg_ = ur.timeRange();
    data_.addUnit( desccolidx_, un );
}


void uiStratTreeToDispTransl::addLithologies( const Strat::LeavedUnitRef& ur )
{
    if ( !tree_ ) return;

    const Strat::LithologySet& lithos = tree_->lithologies();
    BufferString lithnm; 
    for ( int idx=0; idx<ur.nrRefs(); idx++ )
    {
	Strat::LeafUnitRef& lref = (Strat::LeafUnitRef&)ur.ref( idx );
	const int lithidx = lref.lithology();
	const Strat::Lithology* lith = lithidx >= 0 ? lithos.get( lithidx ) : 0;
	if ( lith ) { if ( idx ) lithnm += ", "; lithnm += lith->name(); }
    }
    StratDispData::Unit* un = new StratDispData::Unit( lithnm.buf() );
    un->zrg_ = ur.timeRange();
    data_.addUnit( lithocolidx_, un );
}


void uiStratTreeToDispTransl::addLevel( const Strat::LeavedUnitRef& ur )
{
    BufferString lvlnm; Color lvlcol;
    const int id = ur.levelID();
    const Strat::LevelSet& lvls = Strat::LVLS();
    lvlcol = lvls.isPresent( id ) ? lvls.get( id )->color() : Color::Black();
    lvlnm = lvls.isPresent( id ) ? lvls.get( id )->name() : "";

    StratDispData::Level* lvl = new StratDispData::Level( lvlnm.buf(),
	    						  ur.fullCode() );
    lvl->zpos_ = ur.timeRange().start;
    lvl->color_ = lvlcol; 
    data_.getCol( levelcolidx_ )->levels_ += lvl;
}




uiStratDispToTreeTransl::uiStratDispToTreeTransl( uiStratRefTree& uitree ) 
    : uitree_(uitree)
{}

#define mGetLItem(t) uiListViewItem* lit = uitree_.getLVItFromFullCode( txt);\
if ( lit ) { uitree_.listView()->setCurrentItem(lit); } else return;

void uiStratDispToTreeTransl::handleUnitMenu( const char* txt )
{
    if ( txt )
    {
	mGetLItem( txt )
	uitree_.handleMenu( lit );
    }
}


void uiStratDispToTreeTransl::addUnit( const char* txt )
{
    if ( txt )
    {
	mGetLItem( txt )
	uitree_.insertSubUnit( lit );
    }
    else
	uitree_.insertSubUnit( 0 );
}



void uiStratDispToTreeTransl::setUnitLvl( const char* code )
{
    uitree_.setUnitLvl( code );
}

