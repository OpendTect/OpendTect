#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocitymod.h"

#include "thread.h"
#include "velocityfunction.h"

class BinIDValueSet;
class Gridder2D;

namespace Vel
{

class IntervalSource;

/*!A velocity funcion that computes interval velocity from
   another velocity function with RMS velocity */

mExpClass(Velocity) IntervalFunction : public Function
{
public:
			IntervalFunction(IntervalSource&);

    const ZDomain::Info& zDomain() const override;
    ZSampling		getAvailableZ() const override;

    bool		moveTo(const BinID&) override;
    Function&		setInput(Function*);
    Function&		setZDomain(const ZDomain::Info&) override;

protected:
			~IntervalFunction();

    bool		computeVelocity(float z0,float dz,int sz,
					float* res ) const override;

    Function*		inputfunc_ = nullptr;
};


mExpClass(Velocity) IntervalSource : public FunctionSource
{
public:
			IntervalSource();

    const VelocityDesc& getDesc() const override	{ return desc_; }
    const ZDomain::Info& zDomain() const override;
    const UnitOfMeasure* getVelUnit() const override;

    const char*		factoryKeyword() const override { return sType(); }
    static const char*	sType() { return "Interval"; }

    void		setInput(FunctionSource*);
    void		getSources(TypeSet<MultiID>&) const;
    void		getAvailablePositions(BinIDValueSet&) const override;

    NotifierAccess*	changeNotifier() override;
    BinID		changeBinID() const override;

protected:
			~IntervalSource();

    void		sourceChangeCB(CallBacker*);
    IntervalFunction*	createFunction(const BinID&) override;

    FunctionSource*	inputsource_ = nullptr;
    VelocityDesc&	desc_;
};

} // namespace Vel
