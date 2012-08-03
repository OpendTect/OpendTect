#ifndef velocityfunction_h
#define velocityfunction_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2005
 RCS:		$Id: velocityfunction.h,v 1.12 2012-08-03 13:00:44 cvskris Exp $
________________________________________________________________________


-*/

#include "velocitymod.h"
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


namespace Vel
{

class FunctionSource;

/*!Velocity versus depth for one location. The source of information is 
   different for each subclass, but is typically user-picks, wells
   or velocity volumes. */

mClass(Velocity) Function
{
public:
    void			ref() const;
    void			unRef() const;
    void			unRefNoDelete() const;
    
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
    virtual 			~Function();

    virtual bool		computeVelocity(float z0, float dz, int nr,
						float* res ) const	= 0;

    FunctionSource&		source_;
    BinID			bid_;
    StepInterval<float>		desiredrg_;

private:
    friend			class FunctionSource;

    mutable Threads::Mutex	cachelock_;
    mutable TypeSet<float>*	cache_;
    mutable SamplingData<double> cachesd_;
};


/*!A source of Velocity functions of a certain sort. The FunctionSource
   can create Functions at certian BinID locations. */

mClass(Velocity) FunctionSource : public CallBacker
{ mRefCountImplNoDestructor(FunctionSource);
public:
    mDefineFactory1ParamInClass( FunctionSource, const MultiID&, factory );

    virtual BufferString	userName() const;
    virtual const VelocityDesc&	getDesc() const				= 0;
    virtual void		getSurroundingPositions(const BinID&,
				    BinIDValueSet&) const;
    virtual void		getAvailablePositions(BinIDValueSet&) const {}

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
    void			refFunction(const Function* v);
    bool			unRefFunction(const Function* v);

    int				findFunction(const BinID&) const;
    				//!<Caller must readlock before calling


    MultiID				mid_;
    BufferString			errmsg_;

    ObjectSet<Function>			functions_;
    TypeSet<int>			refcounts_;
    Threads::Mutex			lock_;
};

}; //namespace


#endif

