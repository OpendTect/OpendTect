/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellstratdisplay.cc,v 1.18 2010-09-17 12:26:07 cvsbruno Exp $";

#include "uiwellstratdisplay.h"

#include "stratunitrepos.h"
#include "uigraphicsscene.h"
#include "welldisp.h"
#include "welld2tmodel.h"
#include "wellmarker.h"

uiWellStratDisplay::uiWellStratDisplay( uiParent* p )
    : uiWellDahDisplay(p,"Well Strat Display")
    , annots_(AnnotData())  
    , drawer_(uiAnnotDrawer(scene(),annots_))  
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

    uidatagather_ = new uiStratTreeToDispTransl( annots_ );
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
    for ( int colidx=0; colidx<annots_.nrCols(); colidx++ )
    {
	AnnotData::Column& col = *annots_.getCol( colidx );
	for ( int idx=0; idx<col.units_.size(); idx++ )
	{
	    if ( col.isaux_ ) continue;
	    const AnnotData::Unit* un = col.units_[idx];
	    AnnotData::Unit* newun = 
		    new AnnotData::Unit( un->name_, un->zpos_, un->zposbot_ );
	    newun->id_ = un->id_;
	    orgunits_ += newun;
	}
    }
}


void uiWellStratDisplay::gatherInfo()
{
    gatherOrgUnits();
    for ( int colidx=0; colidx<annots_.nrCols(); colidx++ )
    {
	AnnotData::Column& col = *annots_.getCol( colidx );
	col.isdisplayed_ = false;
	if ( col.isaux_ ) continue;
	for ( int idx=0; idx<col.units_.size(); idx++ )
	{
	    AnnotData::Unit* unit = col.units_[idx];
	    if ( unit )
	    {
		unit->draw_ = false;
		setUnitTopPos( *unit );
		setUnitBotPos( *unit );
		if ( unit->draw_ )
		{
		    unit->col_.setTransparency( transparency_ );
		    unit->nmcol_.setTransparency( transparency_ );
		    col.isdisplayed_ = true;
		}
	    }
	}
    }
    deepErase( orgunits_ );
}


void uiWellStratDisplay::draw()
{
    drawer_.draw();
}


void uiWellStratDisplay::setUnitTopPos( AnnotData::Unit& unit )
{
    float& toppos = unit.zpos_; 
    const Well::Marker* mrk = 0;
    toppos = getPosFromMarkers( unit );
    unit.draw_ = !mIsUdf( toppos );
}


void uiWellStratDisplay::setUnitBotPos( AnnotData::Unit& unit )
{
    if ( !unit.draw_ ) return;
    float& botpos = unit.zposbot_;
    const AnnotData::Unit* nextunit = getNextTimeUnit( botpos );
    if ( nextunit ) 
    {
	botpos = getPosFromMarkers( *nextunit );
    }
    else if ( unit.zposbot_ == uidatagather_->botzpos_ )
    {
	botpos = getPosMarkerLvlMatch( uidatagather_->botlvlid_ );
    }
    unit.draw_ = !mIsUdf( botpos );
}


const AnnotData::Unit* uiWellStratDisplay::getNextTimeUnit( float pos ) const
{
    for ( int idx=0; idx<orgunits_.size(); idx++ )
    {
	const AnnotData::Unit* unit = orgunits_[idx];
	if ( unit && unit->zpos_ == pos )
	    return unit;
    }
    return 0;
}


float uiWellStratDisplay::getPosFromMarkers( const AnnotData::Unit& unit ) const
{
    float pos = mUdf(float);
    if ( !zdata_.markers_ ) return pos;
    const Strat::UnitRef* uref = Strat::UnRepo().find( unit.id_ );
    if ( uref )
	pos = getPosMarkerLvlMatch( uref->getLvlID() );
    return pos;
}


float uiWellStratDisplay::getPosMarkerLvlMatch( int lvlid ) const
{
    float pos = mUdf( float );
    if ( lvlid<0 || !Strat::UnRepo().getLvl( lvlid ) ) return pos;
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


