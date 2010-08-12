/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratdispdata.cc,v 1.10 2010-08-12 11:17:11 cvsbruno Exp $";

#include "uistratdispdata.h"
#include "uistratmgr.h"
#include "uistratreftree.h"
#include "uilistview.h"

#include "bufstringset.h"
#include "color.h"
#include "stratunitrepos.h"


#define mAskStratMgrNotif(nm)\
    uistratmgr_.nm.notify( mCB(this,uiStratTreeToDispTransl,triggerDataChange));
uiStratTreeToDispTransl::uiStratTreeToDispTransl( AnnotData& ad, 
						  const uiStratMgr& mgr )
    : CallBacker(CallBacker::CallBacker())
    , data_(ad)
    , uistratmgr_(mgr)
    , newtreeRead(this)
{
    mAskStratMgrNotif(unitCreated)
    mAskStratMgrNotif(unitChanged)
    mAskStratMgrNotif(unitRemoved)
    mAskStratMgrNotif(lithChanged)
    mAskStratMgrNotif(lithRemoved)
    mAskStratMgrNotif(lvlChanged)

    readFromTree();
}


#define mAskStratMgrRemove(nm)\
    uistratmgr_.nm.remove( mCB(this,uiStratTreeToDispTransl,triggerDataChange));
uiStratTreeToDispTransl::~uiStratTreeToDispTransl()
{
    mAskStratMgrRemove(unitCreated)
    mAskStratMgrRemove(unitChanged)
    mAskStratMgrRemove(unitRemoved)
    mAskStratMgrRemove(lithChanged)
    mAskStratMgrRemove(lithRemoved)
    mAskStratMgrRemove(lvlChanged)
}


void uiStratTreeToDispTransl::triggerDataChange( CallBacker* )
{
    readFromTree();
}


static int idlitho = 5;
static int iddesc = 6;
static int idboundaries = 7;

void uiStratTreeToDispTransl::readFromTree()
{
    data_.eraseData();
    static const char* colnms[] = { "Super Group", "Group", "Formation", 
	    "Member", "Type", "Lithology", "Description", "Boundaries", 0 };
#define mAddCol( title )\
    data_.addCol( new AnnotData::Column( title ) );
    for ( int idcol=0; colnms[idcol]; idcol++ )
	mAddCol( colnms[idcol] );
    
    data_.getCol( idlitho )->isaux_ = true;
    data_.getCol( idboundaries )->isaux_ = true;
    data_.getCol( iddesc )->isaux_ = true;

    addUnits( *((Strat::NodeUnitRef*)uistratmgr_.getCurTree()), 0 );
    addBottomBoundary();
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
    Interval<float> timerg = uref.props().timerg_;
    AnnotData::Unit* unit = new AnnotData::Unit( uref.code(), 
	    					 timerg.start, 
						 timerg.stop );
    unit->col_ = uref.props().color_;
    unit->colidx_ = order;
    unit->id_ = uref.getID();
    unit->col_ = uref.props().color_;
    if ( order >= data_.nrCols() ) return;
    data_.getCol(order)->units_ += unit;

    addBoundary( unit->id_, uref.props().lvlid_, timerg.start );
    mDynamicCastGet(const Strat::LeafUnitRef*,un,&uref)
    if ( un ) addAnnot( uistratmgr_.getLithName( *un ), timerg, true );
    addAnnot( uref.description(), timerg, false );
}


void uiStratTreeToDispTransl::addAnnot( const char* nm, Interval<float>& posrg,						bool islitho )
{
    AnnotData::Unit* unit = new AnnotData::Unit(nm,posrg.start,posrg.stop); 
    data_.getCol( islitho ? idlitho : iddesc )->units_ += unit; 
}


void uiStratTreeToDispTransl::addBoundary( int unid, int lvlid, float zpos )
{
    BufferString lvlnm; Color lvlcol;
    if ( lvlid >= 0 && uistratmgr_.getLvl( lvlid ) )
	uistratmgr_.getLvlPropsByID( lvlid, lvlnm, lvlcol );
    else 
	lvlcol = Color::Black();

    AnnotData::Marker* mrk = new AnnotData::Marker( lvlnm, zpos );
    mrk->isdotted_ = lvlnm.isEmpty();
    mrk->id_ = unid;
    mrk->col_ = lvlcol;
    data_.getCol( idboundaries )->markers_ += mrk;
}


void uiStratTreeToDispTransl::addBottomBoundary()
{
    const AnnotData::Column& col = *data_.getCol(0);
    float z = col.units_[col.units_.size()-1]->zposbot_;
    botzpos_ = z;
    int lvlid = uistratmgr_.getCurTree()->botLvlID();
    botlvlid_ = lvlid;
    addBoundary( -1, lvlid, z );
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


void uiStratDispToTreeTransl::handleUnitLvlMenu( int unid )
{
    if ( unid >=0 )
	uitree_.setUnitLvl( unid );
    else
	uitree_.setBottomLvl();
}


void uiStratDispToTreeTransl::fillUndef( CallBacker* cb )
{
    uitree_.doSetUnconformities( cb );
}


