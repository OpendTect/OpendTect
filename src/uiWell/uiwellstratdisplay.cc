/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellstratdisplay.cc,v 1.4 2010-03-30 12:09:17 cvsbruno Exp $";

#include "uiwellstratdisplay.h"

#include "stratlevel.h"
#include "uigraphicsscene.h"
#include "welld2tmodel.h"
#include "wellmarker.h"

uiWellStratDisplay::uiWellStratDisplay( uiParent* p, bool nobg,
					const ObjectSet<Well::Marker>& mrks )
    : uiStratDisplay(p)
    , markers_(mrks)  
    , istime_(false)
    , d2tm_(0)		    
{
    if ( nobg )
    {
	setNoSytemBackGroundAttribute();
	uisetBackgroundColor( Color( 255, 255, 255, 0 )  );
	scene().setBackGroundColor( Color( 255, 255, 255, 0 )  );
    }
}

void uiWellStratDisplay::gatherInfo()
{
    data_.gatherInfo();
    for ( int idx=0; idx<nrUnits(); idx++ )
    {
	if ( getUnit( idx ) )
	    setUnitPos( *getUnit( idx ) );
    }
}


void uiWellStratDisplay::setUnitPos( StratDisp::Unit& unit )  
{
    float& toppos = unit.zpos_; 
    float& botpos = unit.zposbot_;
    const Well::Marker* topmrk = 0;
    const Well::Marker* basemrk = 0;
    for ( int idx=0; idx<markers_.size(); idx++ )
    {
	const Strat::Level* lvl = markers_[idx]->level();
	if ( lvl && !strcmp( lvl->name(), unit.toplvlnm_ ) )
	    topmrk = markers_[idx];
	if ( lvl && !strcmp( lvl->name(), unit.botlvlnm_ ) )
	    basemrk = markers_[idx];
    }
    if ( !topmrk || !basemrk ) 
    { 
	toppos = mUdf(float); 
	botpos = mUdf(float); 
    }
    else
    {
	toppos = topmrk->dah();
	botpos = basemrk->dah();
	if ( istime_ && d2tm_ ) 
	{ 
	    toppos = d2tm_->getTime( toppos ); 
	    botpos = d2tm_->getTime( botpos ); 
	}
    }
}

