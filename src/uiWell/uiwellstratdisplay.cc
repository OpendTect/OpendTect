/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellstratdisplay.h"

#include "uiaxishandler.h"
#include "uigraphicsscene.h"

#include "stratreftree.h"
#include "survinfo.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "welltrack.h"


uiWellStratDisplay::uiWellStratDisplay( uiParent* p )
    : uiWellDahDisplay(p,uiWellDahDisplay::Setup())
    , stratgen_(0)
    , data_(StratDispData())
    , drawer_(uiStratDrawer(scene(),data_))
    , transparency_(0)
{
    drawer_.setNewAxis( new uiAxisHandler(scene_,
				uiAxisHandler::Setup(uiRect::Left)
			    	.noannot(true)
				.annotinside(true)
			    	.border(uiBorder(0))), false );
    drawer_.setNewAxis( new uiAxisHandler(scene_,
				uiAxisHandler::Setup(uiRect::Top)
				.noannot(true)
				.annotinside(true)
				.border(uiBorder(0))), true );
    drawer_.xAxis()->setBounds( StepInterval<float>( 0, 100, 10 ) );
}


uiWellStratDisplay::~uiWellStratDisplay()
{
    delete stratgen_;
}


void uiWellStratDisplay::gatherInfo()
{
    if ( zdata_.mrks() )
    {
	delete stratgen_;
	stratgen_ = zdata_.wd_ ? new WellStratUnitGen(data_,*zdata_.wd_) : 0;
    }
}


void uiWellStratDisplay::draw()
{
    for ( int idcol=0; idcol<data_.nrCols(); idcol++ )
    {
	for ( int idun=0; idun<data_.nrUnits( idcol ); idun++ )
	{
	    StratDispData::Unit& unit = *data_.getUnit( idcol, idun );
	    unit.color_.setTransparency( mCast(char,transparency_) );
	}
    }

    drawer_.xAxis()->setNewDevSize( viewWidth(), viewHeight() );
    drawer_.yAxis()->setNewDevSize( viewHeight(), viewWidth() );
    drawer_.yAxis()->updateScene();
    zdata_.zrg_.sort( false );
    drawer_.setZRange( zdata_.zrg_ );
    drawer_.drawColumns();
}


// WellStratUnitGen
WellStratUnitGen::WellStratUnitGen( StratDispData& data,
				    const Well::Data& wd )
    : data_(data)
    , markers_(wd.markers())
    , d2tmodel_(wd.d2TModel())
    , track_(wd.track())
{
    uidatagather_ = new uiStratTreeToDisp( data_, false, false );
    uidatagather_->newtreeRead.notify(mCB(this,WellStratUnitGen,dataChangedCB));
    gatherInfo();
}


WellStratUnitGen::~WellStratUnitGen()
{
    delete uidatagather_;
}


void WellStratUnitGen::gatherInfo()
{
    for ( int idcol=0; idcol<data_.nrCols(); idcol++ )
	data_.getCol( idcol )->isdisplayed_ = false;

    gatherLeavedUnits();
    assignTimesToLeavedUnits();
    assignTimesToAllUnits();
}


void WellStratUnitGen::gatherLeavedUnits()
{
    if ( markers_.isEmpty() ) return;
    posset_.erase(); leaveddispunits_.erase(); leavedunits_.erase();
    units_.erase(); dispunits_.erase();
    TypeSet<float> absunitpos;
    for ( int idcol=0; idcol<data_.nrCols(); idcol++ )
    {
	for ( int idun=0; idun<data_.nrUnits( idcol ); idun++ )
	{
	    StratDispData::Unit& unit = *data_.getUnit( idcol, idun );
	    unit.isdisplayed_ = false;
	    const Strat::UnitRef* ur = Strat::RT().find( unit.fullCode() );
	    mDynamicCastGet( const Strat::NodeOnlyUnitRef*, nur,ur );
	    if ( nur )
	    {
		units_ += nur;
		unit.zrg_.set( mUdf(float), mUdf(float) );
		dispunits_ += &unit;
	    }
	    mDynamicCastGet( const Strat::LeavedUnitRef*, lur,ur );
	    if ( !lur || !lur->levelID().isValid() )
		continue;

	    const Well::Marker* mrk = getMarkerFromLvlID( lur->levelID() );
	    if ( mrk )
	    {
		float pos = mrk->dah();
		if ( SI().zIsTime() && d2tmodel_ )
		    pos = d2tmodel_->getTime(pos, track_)*
			  SI().zDomain().userFactor();
		else
		    pos = (float) track_.getPos( pos ).z;

		int idunit = 0;
		for ( ; idunit<posset_.size(); idunit ++ )
		{
		    if ( unit.zrg_.stop < absunitpos[idunit] )
			break;
		}
		leaveddispunits_.insertAt( &unit, idunit );
		leavedunits_.insertAt( lur, idunit );
		absunitpos.insert( idunit, unit.zrg_.stop );
		posset_.insert( idunit, pos );
	    }
	}
    }
}



const Well::Marker*
	WellStratUnitGen::getMarkerFromLvlID( Strat::LevelID lvlid ) const
{
    for ( int idx=0; idx<markers_.size(); idx++ )
    {
	const Well::Marker* curmrk = markers_[idx];
	if ( curmrk && curmrk->levelID().isValid() )
	{
	    if ( lvlid == curmrk->levelID() )
	    {
		return curmrk;
	    }
	}
    }

    return nullptr;
}


void WellStratUnitGen::assignTimesToLeavedUnits()
{
    for ( int idx=0; idx<leavedunits_.size()-1; idx++ )
    {
	StratDispData::Unit& unit = *leaveddispunits_[idx];
	unit.zrg_.set( posset_[idx], posset_[idx+1] );
	unit.zrg_.sort();
	unit.isdisplayed_ = true;
	data_.getCol( unit.colidx_ )->isdisplayed_ = true;
    }
}


void WellStratUnitGen::assignTimesToAllUnits()
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
