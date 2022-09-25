#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "velocitymod.h"
#include "sharedobject.h"

#include "enums.h"
#include "factory.h"
#include "multiid.h"
#include "position.h"
#include "ranges.h"
#include "samplingdata.h"
#include "threadlock.h"
#include "veldesc.h"


namespace Attrib { class DataHolder; }

class BinIDValueSet;

namespace Vel
{

class FunctionSource;

/*!Velocity versus depth for one location. The source of information is
   different for each subclass, but is typically user-picks, wells
   or velocity volumes. */

mExpClass(Velocity) Function : public ReferencedObject
{
public:
				Function(FunctionSource&);
    const FunctionSource&	getSource() const	{ return source_; }

    virtual const VelocityDesc&	getDesc() const;

    float			getVelocity(float z) const;
    const BinID&		getBinID() const;
    virtual bool		moveTo(const BinID&);

    virtual void		removeCache();

    virtual StepInterval<float>	getAvailableZ() const			= 0;
    void			setDesiredZRange(const StepInterval<float>&);
    const StepInterval<float>&	getDesiredZ() const;

protected:
    virtual			~Function();

    virtual bool		computeVelocity(float z0, float dz, int nr,
						float* res ) const	= 0;

    FunctionSource&		source_;
    BinID			bid_;
    StepInterval<float>		desiredrg_;

private:
    friend			class FunctionSource;

    mutable Threads::Lock	cachelock_;
    mutable TypeSet<float>*	cache_ = nullptr;
    mutable SamplingData<double> cachesd_;
};


/*!A source of Velocity functions of a certain sort. The FunctionSource
   can create Functions at certian BinID locations. */

mExpClass(Velocity) FunctionSource : public SharedObject
{
public:
				mDefineFactory1ParamInClass(
					FunctionSource,const MultiID&,factory)

    virtual BufferString	userName() const;
    virtual const VelocityDesc&	getDesc() const				= 0;
    virtual void		getSurroundingPositions(const BinID&,
				    BinIDValueSet&) const;
    virtual void		getAvailablePositions(BinIDValueSet&) const {}

    ConstRefMan<Function>	getFunction(const BinID&);
    virtual Function*		createFunction(const BinID&)		= 0;

    const MultiID&		multiID() const		{ return mid_; }

    virtual NotifierAccess*	changeNotifier()	{ return 0; }
    virtual BinID		changeBinID() const	{ return BinID(-1,-1); }

    virtual void		fillPar(IOPar&) const	{}
    virtual bool		usePar(const IOPar&)	{ return true; }

    const char*			errMsg() const;

protected:
				FunctionSource();
				~FunctionSource();

    friend			class Function;
    void			removeFunction(const Function*);

    int				findFunction(const BinID&) const;
				//!<Caller must readlock before calling


    MultiID				mid_;
    BufferString			errmsg_;

    ObjectSet<Function>			functions_;
    Threads::Lock			lock_;
};

} // namespace Vel
