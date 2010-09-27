/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellstratdisplay.cc,v 1.19 2010-09-27 11:05:19 cvsbruno Exp $";

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
    , transparency_(0)							
{
    drawer_.setNewAxis( new uiAxisHandler(scene_,
				uiAxisHandler::Setup(uiRect::Left)
			    	.noborderspace(true)
			    	.border(uiBorder(0))
			    	.nogridline(true)), false );
    drawer_.setNewAxis( new uiAxisHandler(scene_,
				uiAxisHandler::Setup(uiRect::Top)
				.noborderspace(true)
				.border(uiBorder(0))
				.nogridline(true) ), true );
    drawer_.xAxis()->setBounds( StepInterval<float>( 0, 100, 10 ) );

    uidatagather_ = new uiStratTreeToDispTransl( data_, Strat::eRT() );
    uidatagather_->newtreeRead.notify(
	    			mCB(this,uiWellStratDisplay,dataChangedCB));
}


uiWellStratDisplay::~uiWellStratDisplay()
{
    delete uidatagather_;
}


void uiWellStratDisplay::gatherOrgUnits()
{
    deepErase( orgunits_ );
    for ( int colidx=0; colidx<data_.nrCols(); colidx++ )
    {
	StratDispData::Column& col = *data_.getCol( colidx );
	for ( int idx=0; idx<col.units_.size(); idx++ )
	{
	    const StratDispData::Unit* un = col.units_[idx];
	    StratDispData::Unit* newun = 
		    new StratDispData::Unit( un->name_, un->color_ );
	    newun->zrg_ = un->zrg_;
	    orgunits_ += newun;
	}
    }
}


void uiWellStratDisplay::gatherInfo()
{
    gatherOrgUnits();
    for ( int colidx=0; colidx<data_.nrCols(); colidx++ )
    {
	StratDispData::Column& col = *data_.getCol( colidx );
	col.isdisplayed_ = false;
	for ( int idx=0; idx<col.units_.size(); idx++ )
	{
	    StratDispData::Unit* unit = col.units_[idx];
	    if ( unit )
	    {
		unit->isdisplayed_ = false;
		setUnitTopPos( *unit );
		setUnitBotPos( *unit );
		if ( unit->isdisplayed_ )
		{
		    //unit->col_.setTransparency( transparency_ );
		    //unit->nmcol_.setTransparency( transparency_ );
		    col.isdisplayed_ = true;
		}
	    }
	}
    }
    deepErase( orgunits_ );
}


void uiWellStratDisplay::draw()
{
    Interval<float> zrg( zdata_.zrg_.start*0.001, zdata_.zrg_.stop*0.001 );
    drawer_.yAxis()->setRange( zrg );
    drawer_.draw();
}


void uiWellStratDisplay::setUnitTopPos( StratDispData::Unit& unit )
{
    float& toppos = unit.zrg_.start; 
    const Well::Marker* mrk = 0;
    toppos = getPosFromMarkers( unit );
    unit.isdisplayed_ = !mIsUdf( toppos );
}


void uiWellStratDisplay::setUnitBotPos( StratDispData::Unit& unit )
{
    if ( !unit.isdisplayed_ ) return;
    float& botpos = unit.zrg_.stop;
    const StratDispData::Unit* nextunit = getNextTimeUnit( botpos );
    if ( nextunit ) 
    {
	botpos = getPosFromMarkers( *nextunit );
    }
    unit.isdisplayed_ = !mIsUdf( botpos );
}


const StratDispData::Unit* uiWellStratDisplay::getNextTimeUnit( float pos ) const
{
    for ( int idx=0; idx<orgunits_.size(); idx++ )
    {
	const StratDispData::Unit* unit = orgunits_[idx];
	if ( unit && unit->zrg_.start == pos )
	    return unit;
    }
    return 0;
}


float uiWellStratDisplay::getPosFromMarkers( const StratDispData::Unit& unit ) const
{
    float pos = mUdf(float);
    if ( !zdata_.markers_ ) return pos;
    const Strat::UnitRef* uref = Strat::RT().find( unit.name_ );
    if ( uref && uref->isLeaved() )
	pos = getPosMarkerLvlMatch( ((Strat::LeavedUnitRef*)uref)->levelID() );
    return pos;
}


float uiWellStratDisplay::getPosMarkerLvlMatch( int lvlid ) const
{
    float pos = mUdf( float );
    if ( !Strat::LVLS().isPresent(lvlid) )
	return pos;

    const Well::Marker* mrk = 0;
    for ( int idx=0; idx<zdata_.markers_->size(); idx++ )
    {
	const Well::Marker* curmrk = (*zdata_.markers_)[idx];
	if ( curmrk && curmrk->levelID() >=0 )
	{
	    if ( lvlid == curmrk->levelID() )
	    {
		pos = curmrk->dah();
		if ( zdata_.zistime_ && zdata_.d2tm_ ) 
		    pos = zdata_.d2tm_->getTime( pos ); 
		return pos;
	    }
	}
    }
    return pos;
}


