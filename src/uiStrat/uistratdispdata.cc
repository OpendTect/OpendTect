/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratdispdata.cc,v 1.15 2010-09-17 12:26:07 cvsbruno Exp $";

#include "uistratdispdata.h"
#include "uistratreftree.h"
#include "uilistview.h"

#include "keystrs.h"
#include "iopar.h"
#include "bufstringset.h"
#include "color.h"
#include "stratunitrepos.h"


#define mAskStratRepoNotif(nm)\
    unitrepos_.nm.notify( mCB(this,uiStratTreeToDispTransl,triggerDataChange));

uiStratTreeToDispTransl::uiStratTreeToDispTransl( AnnotData& ad ) 
    : data_(ad)
    , unitrepos_(Strat::UnRepo())
    , newtreeRead(this)
{
    mAskStratRepoNotif(unitCreated)
    mAskStratRepoNotif(unitChanged)
    mAskStratRepoNotif(unitRemoved)
    mAskStratRepoNotif(lithoChanged)
    mAskStratRepoNotif(lithoRemoved)
    mAskStratRepoNotif(levelChanged)

    readFromTree();
}


#define mAskStratRepoRemove(nm)\
    unitrepos_.nm.remove( mCB(this,uiStratTreeToDispTransl,triggerDataChange));

uiStratTreeToDispTransl::~uiStratTreeToDispTransl()
{
    mAskStratRepoRemove(unitCreated)
    mAskStratRepoRemove(unitChanged)
    mAskStratRepoRemove(unitRemoved)
    mAskStratRepoRemove(lithoChanged)
    mAskStratRepoRemove(lithoRemoved)
    mAskStratRepoRemove(levelChanged)
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
    
    for ( int idcol=0; colnms[idcol]; idcol++ )
	data_.addCol( new AnnotData::Column( colnms[idcol] ) );
    
    data_.getCol( idlitho )->isaux_ = true;
    data_.getCol( idboundaries )->isaux_ = true;
    data_.getCol( iddesc )->isaux_ = true;

    addUnits( *((Strat::NodeUnitRef*)unitrepos_.getCurTree()), 0 );
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
    Interval<float> timerg; Color col; int lvlid;
    IOPar iop; uref.putTo( iop ); 
    iop.get( sKey::Time, timerg );
    iop.get( sKey::Color, col );
    iop.get( Strat::UnitRepository::sKeyLevel(), lvlid );

    AnnotData::Unit* unit = new AnnotData::Unit( uref.code(), 
	    					 timerg.start, 
						 timerg.stop );
    unit->col_ = col;
    unit->colidx_ = order;
    unit->id_ = uref.getID();
    if ( order >= data_.nrCols() ) return;
    data_.getCol(order)->units_ += unit;

    addBoundary( unit->id_, lvlid, timerg.start );
    mDynamicCastGet(const Strat::LeafUnitRef*,un,&uref)
    if ( un ) addAnnot( unitrepos_.getLithName( *un ), timerg, true );
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
    if ( lvlid >= 0 && unitrepos_.getLvl( lvlid ) )
	unitrepos_.getLvlPars( lvlid, lvlnm, lvlcol );
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
    int lvlid = unitrepos_.getCurTree()->botLvlID();
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


