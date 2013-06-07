/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Apr 2010
-*/

static const char* rcsID = "$Id: gravhorcalc.cc,v 1.5 2011/05/04 14:39:15 cvsbert Exp $";

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
    , msg_("Loading horizons")
    , nrdone_(0)
    , totnr_(3)
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


const char* Grav::HorCalc::nrDoneText() const
{
    return ztransf_ ? "Positions done" : "Horizons loaded";
}


int Grav::HorCalc::doLoadStep()
{
    nrdone_++;
    if ( nrdone_ == 1 )
	; // load top
    else if ( nrdone_ == 2 )
	; // load bottom
    else if ( nrdone_ == 3 )
	; // load density
    else if ( nrdone_ == 4 )
    {
	// ztransf_ = ZATF().create( xxx );
	msg_ = "TODO";
	return ErrorOccurred();

	nrdone_ = 0;
    }
    return MoreToDo();
}


int Grav::HorCalc::nextStep()
{
    if ( !ztransf_ )
	return doLoadStep();

    return Finished();
}
