/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratdispdata.cc,v 1.16 2010-09-27 11:05:19 cvsbruno Exp $";

#include "uistratdispdata.h"
#include "uistratreftree.h"
#include "uilistview.h"

#include "keystrs.h"
#include "iopar.h"
#include "bufstringset.h"
#include "color.h"
#include "stratreftree.h"
#include "stratlevel.h"


#define mAskStratNotif(obj,nm,act)\
    obj.nm.act(mCB(this,uiStratTreeToDispTransl,triggerDataChange));

uiStratTreeToDispTransl::uiStratTreeToDispTransl( StratDispData& ad, 
						    Strat::RefTree& tree )
    : data_(ad)
    , tree_(tree) 
    , withauxs_(false)		  
    , withlevels_(false)  
    , newtreeRead(this)
{
    mAskStratNotif(tree_,unitAdded,notify)
    mAskStratNotif(tree_,unitChanged,notify)
    mAskStratNotif(tree_,unitToBeDeleted,notify)
    mAskStratNotif(Strat::eLVLS(),levelChanged,notify)

    readFromTree();
}


uiStratTreeToDispTransl::~uiStratTreeToDispTransl()
{
    mAskStratNotif(tree_,unitAdded,remove)
    mAskStratNotif(tree_,unitChanged,remove)
    mAskStratNotif(tree_,unitToBeDeleted,remove)
    mAskStratNotif(Strat::eLVLS(),levelChanged,remove)

    readFromTree();
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
	StratDispData::Column* col = new StratDispData::Column( colnms[idcol] );
	data_.addCol( col );
    }
    if ( withauxs_ ) 
    {
	data_.addCol( new StratDispData::Column( "Lithologies" ) );
	data_.addCol( new StratDispData::Column( "Description" ) );
    }
    if ( withlevels_ )
    {
	data_.addCol( new StratDispData::Column( "Boundaries" ) );
    }

    addUnits( (Strat::NodeUnitRef&)(tree_), 0 );
    newtreeRead.trigger();
}


void uiStratTreeToDispTransl::addUnits( const Strat::NodeUnitRef& nur, int order ) 
{
    if ( order == 0 )
	order++;

    for ( int iref=0; iref<nur.nrRefs(); iref++ )
    {
	const Strat::UnitRef& ref = nur.ref( iref );
	if ( ref.isLeaf() )
	{
	    mDynamicCastGet(const Strat::LeafUnitRef*,lur,&ref);
	    if ( !lur ) continue;
	    if ( order )
		addUnit( *lur, order-1 );
	}
	else
	{
	    mDynamicCastGet(const Strat::NodeUnitRef*,chldnur,&ref);
	    if ( chldnur )
	    { 
		if ( order )
		    addUnit( *chldnur, order-1 );
		addUnits( *chldnur, order+1 ); 
	    }
	}
    }
}


void uiStratTreeToDispTransl::addUnit( const Strat::UnitRef& uref, int order )
{
    mDynamicCastGet(const Strat::NodeUnitRef*,nur,&uref)
    if ( nur )
    {
	StratDispData::Unit* sdun = new StratDispData::Unit( nur->code(), 
							     nur->color() );
	sdun->zrg_ = nur->timeRange();
	sdun->colidx_ = order;
	if ( order < data_.nrCols() )
	    data_.addUnit( order, sdun );
    }
}


void uiStratTreeToDispTransl::addBoundary( int unid, int lvlid, float zpos )
{
    /*
    BufferString lvlnm; Color lvlcol;
    if ( lvlid >= 0 && unitrepos_.getLvl( lvlid ) )
	unitrepos_.getLvlPars( lvlid, lvlnm, lvlcol );
    else 
	lvlcol = Color::Black();

    StratDispData::Marker* mrk = new StratDispData::Marker( lvlnm, zpos );
    mrk->isdotted_ = lvlnm.isEmpty();
    mrk->id_ = unid;
    mrk->col_ = lvlcol;
    data_.getCol( idboundaries )->markers_ += mrk;
    */
}




uiStratDispToTreeTransl::uiStratDispToTreeTransl( uiStratRefTree& uitree ) 
    : uitree_(uitree)
{}

#define mGetLItem(t) uiListViewItem* lit = uitree_.listView()->findItem(t,0,false); if ( lit ) { uitree_.listView()->setCurrentItem(lit); } else return;

void uiStratDispToTreeTransl::handleUnitMenu( const char* txt )
{
    if ( txt )
    {
	mGetLItem( txt )
	uitree_.handleMenu( lit );
    }
}


void uiStratDispToTreeTransl::handleUnitLvlMenu( const char* code )
{
    uitree_.setUnitLvl( code );
}

