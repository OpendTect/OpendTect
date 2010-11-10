/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellstratdisplay.cc,v 1.29 2010-11-10 14:34:07 cvsbruno Exp $";

#include "uiwellstratdisplay.h"

#include "stratreftree.h"
#include "uigraphicsscene.h"
#include "welldisp.h"
#include "welld2tmodel.h"
#include "wellmarker.h"

uiWellStratDisplay::uiWellStratDisplay( uiParent* p )
    : uiWellDahDisplay(p,"Well Strat Display")
    , data_(StratDispData())  
    , drawer_(uiStratDrawer(scene(),data_))  
    , transparency_(254)							
{
    drawer_.setNewAxis( new uiAxisHandler(scene_,
				uiAxisHandler::Setup(uiRect::Left)
			    	.noannot(true)
			    	.border(uiBorder(0))), false );
    drawer_.setNewAxis( new uiAxisHandler(scene_,
				uiAxisHandler::Setup(uiRect::Top)
				.noannot(true)
				.border(uiBorder(0))), true );
    drawer_.xAxis()->setBounds( StepInterval<float>( 0, 100, 10 ) );

    uidatagather_ = new uiStratTreeToDispTransl( data_, false, false );
    uidatagather_->newtreeRead.notify(
	    			mCB(this,uiWellStratDisplay,dataChangedCB));
}


uiWellStratDisplay::~uiWellStratDisplay()
{
    delete uidatagather_;
}


void uiWellStratDisplay::gatherInfo()
{
    gatherLeavedUnits();
    assignTimesToLeavedUnits();
    assignTimesToAllUnits();
}


void uiWellStratDisplay::draw()
{
    zdata_.zrg_.sort( false );
    drawer_.setZRange( zdata_.zrg_ );
}


void uiWellStratDisplay::gatherLeavedUnits()
{
    if ( !zdata_.markers_ ) return;
    posset_.erase(); leaveddispunits_.erase(); leavedunits_.erase();
    units_.erase(); dispunits_.erase();
    for ( int idcol=0; idcol<data_.nrCols(); idcol++ )
    {
	data_.getCol( idcol )->isdisplayed_ = false;
	for ( int idun=0; idun<data_.nrUnits( idcol ); idun++ )
	{
	    StratDispData::Unit& unit = *data_.getUnit( idcol, idun );
	    unit.isdisplayed_ = false;
	    unit.color_.setTransparency( transparency_ );
	    const Strat::UnitRef* ur = Strat::RT().find( unit.fullCode() );
	    mDynamicCastGet( const Strat::NodeOnlyUnitRef*, nur,ur );
	    if ( nur )
	    {
		units_ += nur;
		unit.zrg_.set( mUdf(float), mUdf(float) );
		dispunits_ += &unit;
	    }
	    mDynamicCastGet( const Strat::LeavedUnitRef*, lur,ur );
	    if ( !lur || lur->levelID() < 0)
		continue;
	    const Well::Marker* mrk = getMarkerFromLvlID( lur->levelID() );
	    if ( mrk )
	    {
		leavedunits_ += lur; 
		leaveddispunits_ += &unit;
		float pos = mrk->dah();
		if ( zdata_.zistime_ && zdata_.d2tm_ ) 
		    pos = zdata_.d2tm_->getTime( pos )*1000; 
		posset_ += pos;
	    }
	}
    }
}


const Well::Marker* uiWellStratDisplay::getMarkerFromLvlID( int lvlid ) const
{
    for ( int idx=0; idx<zdata_.markers_->size(); idx++ )
    {
	const Well::Marker* curmrk = (*zdata_.markers_)[idx];
	if ( curmrk && curmrk->levelID() >=0 )
	{
	    if ( lvlid == curmrk->levelID() )
	    {
		return curmrk;
	    }
	}
    }
    return 0;
}


void uiWellStratDisplay::assignTimesToLeavedUnits()
{
    for ( int idx=0; idx<leavedunits_.size()-1; idx++ )
    {
	const Strat::LeavedUnitRef& lur1 = *leavedunits_[idx];
	const Strat::LeavedUnitRef& lur2 = *leavedunits_[idx+1];
	StratDispData::Unit& unit = *leaveddispunits_[idx];
	unit.zrg_.set( posset_[idx], posset_[idx+1] );
	unit.zrg_.sort();
	unit.isdisplayed_ = true; 
	data_.getCol( unit.colidx_ )->isdisplayed_ = true;
    }
}




void uiWellStratDisplay::assignTimesToAllUnits()
{
    for ( int idx=0; idx<leaveddispunits_.size(); idx++ )
    {
	const StratDispData::Unit& dunit = *leaveddispunits_[idx];
	if ( dunit.isdisplayed_ )
	{
	    const Strat::LeavedUnitRef& ref = *leavedunits_[idx];
	    const Strat::NodeOnlyUnitRef* un = 
				(Strat::NodeOnlyUnitRef*)ref.upNode();
	    while ( un )
	    {
		const int idnode = units_.indexOf( un );
		if ( idnode >= 0 && idnode < dispunits_.size() )
		{
		    StratDispData::Unit& dispnode = *dispunits_[idnode];
		    dispnode.isdisplayed_ = true;
		    if ( mIsUdf ( dispnode.zrg_.start ) )
			dispnode.zrg_ = dunit.zrg_;
		    else
			dispnode.zrg_.include( dunit.zrg_ );
		    data_.getCol( dispnode.colidx_ )->isdisplayed_ = true;
		}
		un = (Strat::NodeOnlyUnitRef*)un->upNode();
	    }
	}
    }
}

