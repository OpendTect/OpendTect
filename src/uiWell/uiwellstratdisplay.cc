/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellstratdisplay.cc,v 1.1 2010-03-24 10:05:51 cvsbruno Exp $";

#include "uiwellstratdisplay.h"

#include "stratlevel.h"
#include "stratunitrepos.h"
#include "welld2tmodel.h"
#include "wellmarker.h"

uiWellStratDisplay::uiWellStratDisplay( uiParent* p, 
					const ObjectSet<Well::Marker>& mrks )
    : uiStratDisplay(p)
    , markers_(mrks)  
    , istime_(false)
    , d2tm_(0)		    
{
}


Interval<float> uiWellStratDisplay::getUnitPos( const Strat::Level& toplvl,
						const Strat::Level& baselvl )

{
    Interval<float> posrg( mUdf(float), mUdf(float) );
    const Well::Marker* topmrk = 0;
    const Well::Marker* basemrk = 0;
    for ( int idx=0; idx<markers_.size(); idx++ )
    {
	if ( markers_[idx]->level() == &toplvl )
	    topmrk = markers_[idx];
	if ( markers_[idx]->level() == &baselvl )
	    basemrk = markers_[idx];
    }
    if ( !topmrk || !basemrk ) 
	return posrg;
    posrg.start = topmrk->dah();
    posrg.stop = basemrk->dah();
    if ( istime_ && d2tm_ ) 
    { 
	posrg.start = d2tm_->getTime( posrg.start ); 
	posrg.stop = d2tm_->getTime( posrg.stop ); 
    }
    return posrg;    
}

