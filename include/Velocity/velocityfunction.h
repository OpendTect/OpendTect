#ifndef velocityfunction_h
#define velocityfunction_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: velocityfunction.h,v 1.5 2009-07-22 16:01:19 cvsbert Exp $
________________________________________________________________________


-*/

#include "enums.h"
#include "factory.h"
#include "multiid.h"
#include "position.h"
#include "ranges.h"
#include "refcount.h"
#include "samplingdata.h"
#include "thread.h"
#include "veldesc.h"

namespace Attrib { class DataHolder; };

class BinIDValueSet;
class HorSampling;


namespace Vel
{

class FunctionSource;

/*!Velocity versus depth for one location. The source of information is 
   different for each subclass, but is typically user-picks, wells
   or velocity volumes. */

mClass Function
{ mRefCountImpl(Function)
public:
				Function(FunctionSource&);
    const FunctionSource&	getSource() const 	{ return source_; }

    virtual const VelocityDesc&	getDesc() const;

    float			getVelocity(float z) const;
    const BinID&		getBinID() const;
    virtual bool		moveTo(const BinID&);

    virtual void		removeCache();

    virtual StepInterval<float>	getAvailableZ() const			= 0;
    void			setDesiredZRange(const StepInterval<float>&);
    const StepInterval<float>&	getDesiredZ() const;
    
protected:

    virtual bool		computeVelocity(float z0, float dz, int nr,
						float* res ) const	= 0;

    FunctionSource&		source_;
    BinID			bid_;
    StepInterval<float>		desiredrg_;

private:

    mutable Threads::Mutex	cachelock_;
    mutable TypeSet<float>*	cache_;
    mutable SamplingData<double> cachesd_;
};


/*!A source of Velocity functions of a certain sort. The FunctionSource
   can create Functions at certian BinID locations. */

mClass FunctionSource : public CallBacker
{ mRefCountImplNoDestructor(FunctionSource);
public:
    mDefineFactory1ParamInClass( FunctionSource, const MultiID&, factory );

    virtual const char*		type() const 				= 0;
    virtual BufferString	userName() const;
    virtual const VelocityDesc&	getDesc() const				= 0;
    virtual void		getSurroundingPositions(const BinID&,
				    BinIDValueSet&) const;
    virtual void		getAvailablePositions(BinIDValueSet&) const {}
    virtual void		getAvailablePositions(HorSampling&) const {}

    RefMan<const Function>	getFunction(const BinID&);
    virtual Function*		createFunction(const BinID&)		= 0;

    const MultiID&		multiID() const		{ return mid_; }

    virtual NotifierAccess*	changeNotifier()	{ return 0; }
    virtual BinID		changeBinID() const	{ return BinID(-1,-1); }

    virtual void		fillPar(IOPar&) const	{}
    virtual bool		usePar(const IOPar&)	{ return true; }

    const char*			errMsg() const;

protected:

    friend			class Function;
    void			removeFunction(Function* v);
    				/*!<Called by function when they are deleted. */

    int				findFunction(const BinID&) const;
    				//!<Caller must readlock before calling


    mutable Threads::ReadWriteLock	functionslock_;
    MultiID				mid_;
    BufferString			errmsg_;
    ObjectSet<Function>			functions_;
};

}; //namespace


#endif
