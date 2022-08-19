/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
    , msg_(tr("Loading horizons"))
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


uiString Grav::HorCalc::uiNrDoneText() const
{
    return ztransf_ ? tr("Positions done") : tr("Horizons loaded");
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
	msg_ = tr("TODO");
	nrdone_ = 0;
	return ErrorOccurred();
	
    }
    return MoreToDo();
}


int Grav::HorCalc::nextStep()
{
    if ( !ztransf_ )
	return doLoadStep();

    return Finished();
}
