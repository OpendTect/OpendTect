#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
________________________________________________________________________


-*/

#include "velocitymod.h"
#include "enums.h"
#include "factory.h"
#include "dbkey.h"
#include "position.h"
#include "ranges.h"
#include "refcount.h"
#include "samplingdata.h"
#include "threadlock.h"
#include "veldesc.h"

namespace Attrib { class DataHolder; }

class BinnedValueSet;


namespace Vel
{

class FunctionSource;

/*!Velocity versus depth for one location. The source of information is
   different for each subclass, but is typically user-picks, wells
   or velocity volumes. */

mExpClass(Velocity) Function : public RefCount::Referenced
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
    mutable TypeSet<float>*	cache_;
    mutable SamplingData<double> cachesd_;
};


/*!A source of Velocity functions of a certain sort. The FunctionSource
   can create Functions at certian BinID locations. */

mExpClass(Velocity) FunctionSource : public RefCount::Referenced
				   , public CallBacker
{
public:
				mDefineFactory1ParamInClass(
					FunctionSource,const DBKey&,factory)

    virtual BufferString	userName() const;
    virtual const VelocityDesc&	getDesc() const				= 0;
    virtual void		getSurroundingPositions(const BinID&,
						BinnedValueSet&) const;
    virtual void		getAvailablePositions(BinnedValueSet&) const {}

    ConstRefMan<Function>	getFunction(const BinID&);
    virtual Function*		createFunction(const BinID&)		= 0;

    const DBKey&		dbKey() const		{ return mid_; }

    virtual NotifierAccess*	changeNotifier()	{ return 0; }
    virtual BinID		changeBinID() const	{ return BinID(-1,-1); }

    virtual void		fillPar(IOPar&) const	{}
    virtual bool		usePar(const IOPar&)	{ return true; }

    const uiString		errMsg() const;

protected:

    friend			class Function;
    void			removeFunction(const Function*);

    int				findFunction(const BinID&) const;
				//!<Caller must readlock before calling


    DBKey				mid_;
    uiString				errmsg_;

    ObjectSet<Function>			functions_;
    Threads::Lock			lock_;
};

} // namespace Vel
