/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Apr 2010
-*/

static const char* rcsID = "$Id: gravhorcalc.cc,v 1.2 2010-04-20 12:53:18 cvsbert Exp $";

#include "gravhorcalc.h"
#include "timedepthconv.h"
#include "emhorizon3d.h"


Grav::HorCalc::HorCalc( const MultiID& calc, const MultiID* top,
			const MultiID* bot, float ang )
    : Executor("Calculate gravity on horizon")
    , calchor_(0)
    , tophor_(0)
    , bothor_(0)
    , ztransf_(0)
    , cutoffangle_(ang)
{
}


Grav::HorCalc::~HorCalc()
{
    if ( calchor_ && calchor_ != tophor_ )
	calchor_->unRef();
    if ( tophor_ )
	tophor_->unRef();
    if ( bothor_ )
	bothor_->unRef();
    if ( ztransf_ )
	ztransf_->unRef();
}
