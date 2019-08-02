#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		December 2008
________________________________________________________________________


-*/

#include "velocitymod.h"
#include "samplingdata.h"
#include "thread.h"
#include "velocityfunction.h"

class BinnedValueSet;
class Gridder2D;

namespace Vel
{

class IntervalSource;

/*!A velocity funcion that computes interval velocity from where from
   another velocity function with RMS velocity */

mExpClass(Velocity) IntervalFunction : public Function
{
public:
			IntervalFunction(IntervalSource&);

    StepInterval<float>	getAvailableZ() const;
    bool		moveTo(const BinID&);
    void		setInput(Function*);

protected:
			~IntervalFunction();

    bool		computeVelocity(float z0, float dz, int nr,
					float* res ) const;

    Function*		inputfunc_;
};


mExpClass(Velocity) IntervalSource : public FunctionSource
{
public:
			IntervalSource();
    const VelocityDesc&	getDesc() const;
    const char*		factoryKeyword() const { return sType(); }
    static const char*	sType() { return "Interval"; }

    void		setInput(FunctionSource*);
    void		getSources(DBKeySet&) const;
    void		getAvailablePositions(BinnedValueSet&) const;

    NotifierAccess*	changeNotifier();
    BinID		changeBinID() const;

protected:
    void		sourceChangeCB(CallBacker*);
    IntervalFunction*	createFunction(const BinID&);
			~IntervalSource();

    FunctionSource*	inputsource_;
    VelocityDesc	veldesc_;
};

} // namespace Vel
