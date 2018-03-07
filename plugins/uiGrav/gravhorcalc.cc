/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Apr 2010
-*/

#include "gravhorcalc.h"
#include "timedepthconv.h"
#include "emhorizon3d.h"


Grav::HorCalc::HorCalc( const DBKey& calc, const DBKey* top,
			const DBKey* bot, float ang )
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


uiString Grav::HorCalc::nrDoneText() const
{
    return ztransf_ ? uiStrings::sPositionsDone() : tr("Horizons loaded");
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
	msg_ = uiStrings::phrTODONotImpl("Grav::HorCalc");
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

