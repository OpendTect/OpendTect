#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		23-10-1996
 Contents:	Ranges
________________________________________________________________________

-*/

#include "gendefs.h"
#include "math2.h"
#include "ptrman.h"
#include "varlenarray.h"



/*!\brief base class for Interval<T>

Has no virtual functions and can therefore be used in places where virtual
functions are not a good idea (e.g. large memcpy operations).
Does not have sort, clone and scale functions, these are in Interval<T>.
*/

template <class T>
mClass(Basic) BasicInterval
{
public:

    typedef float		fidx_type;

    inline			BasicInterval();
    inline			BasicInterval(T,T);
    inline BasicInterval<T>&	operator=(const BasicInterval<T>&);

    inline void			set(T,T);
    inline bool			isEqual(const BasicInterval<T>&,T eps) const;
    inline bool			operator==(const BasicInterval<T>&) const;
    inline bool			operator!=(const BasicInterval<T>&) const;
    inline BasicInterval<T>	operator+(const BasicInterval<T>&) const;
    inline BasicInterval<T>&	operator+=(const BasicInterval<T>&);
    template <class X>
    const BasicInterval<T>&	setFrom(const BasicInterval<X>&);

    inline T			width(bool allowrev=true) const;
    inline T			center() const;
    inline void			shift(T);
    inline void			widen(T,bool allowrev=true);

    inline T			atIndex(int,T) const;
    template <class X>
    inline int			getIndex(X,T step) const;
    template <class X>
    inline fidx_type		getfIndex(X,T step) const;
    template <class X>
    inline int			nearestIndex(X,T step) const;
    template <class X>
    inline int			indexOnOrAfter(X,T step,
					       fidx_type eps=1e-5) const;
    template <class X>
    inline void			limitTo_( const BasicInterval<X>& i )
				{ start = i.limitValue(start);
				  stop  = i.limitValue(stop); }
    template <class X>
    inline X			limitValue(X) const;

    template <class X>
    inline bool			overlaps(const BasicInterval<X>&,
					 bool allrev=true) const;
    template <class X>
    inline bool			includes(X,bool allowrev) const;
    template <class X>
    inline bool			includes(const BasicInterval<X>&,
					 bool allowrev=true) const;
    template <class X>
    inline bool			includes(const Interval<X>&,
					 bool allowrev=true) const;

    template <class X>
    inline fidx_type		pos(X,bool limit=true,bool allowrev=true) const;
				/*!<\returns a value between 0 and 1 if val is
				 between start and stop. */
    inline void			include(T,bool allowrev=true);
    inline void			include(const BasicInterval<T>&,
					bool allowrev=true);

    T				start;
    T				stop;

    static BasicInterval<T>	udf()
				{ return BasicInterval<T>(mUdf(T),mUdf(T)); }

    inline bool			isRev() const		{ return start > stop; }
};


/*!
\brief Interval of values.

  Note that start does not need be lower than stop. Because of that, some
  parameters can be passed, and there is a sort() method.
*/

template <class T>
mClass(Basic) Interval : public BasicInterval<T>
{
public:

    inline		Interval() : BasicInterval<T>()		{}
    inline virtual	~Interval()					{}
    inline		Interval(T,T);
    inline		Interval(const BasicInterval<T>&);
    inline
    virtual Interval<T>* clone() const;

    virtual bool inline	isUdf() const;
    virtual void	setUdf()	{ *this = udf(); }

    inline virtual void limitTo(const Interval<T>&);
    inline virtual void	scale(T);

    virtual void	sort(bool asc=true);

    virtual bool	hasStep() const	{ return false; }
    static Interval<T>	udf()		{ return Interval<T>(mUdf(T),mUdf(T)); }

};


typedef Interval<int>	SampleGate;
typedef Interval<Pos::Z_Type> ZGate;


/*!\brief Interval with step. */

template <class T>
mClass(Basic) StepInterval : public Interval<T>
{
public:

    typedef typename BasicInterval<T>::fidx_type	fidx_type;
    mUseType( OD,	SnapDir );

    inline		StepInterval();
    inline		StepInterval(T strt,T sto);
    inline		StepInterval(T strt,T sto,T stp);
    inline		StepInterval(const Interval<T>&);
    inline		StepInterval(const Interval<T>&,T);
    inline		StepInterval( const StepInterval<T>& oth )
			: Interval<T>(oth), step(oth.step)	{}
    inline StepInterval<T>& operator=(const Interval<T>&);

    inline bool		isUdf() const override;
    static StepInterval<T> udf()
			{ return StepInterval<T>(mUdf(T),mUdf(T),mUdf(T)); }
    void		setUdf() override	{ *this = udf(); }

    bool		hasStep() const override { return true; }

    inline StepInterval<T>* clone() const override;
    inline void		set(T strt,T sto,T stp);

    template <class X>
    const StepInterval<T>& setFrom(const Interval<X>&);

    inline bool		isEqual(const StepInterval<T>&,T eps) const;
    inline bool		operator==(const StepInterval<T>&) const;
    inline bool		operator!=(const StepInterval<T>&) const;
    inline T		atIndex(int) const;
    template <class X>
    inline int		getIndex(X) const;
    template <class X> inline
    int			indexOnOrAfter(X,fidx_type eps) const;

    template <class X>
    bool		isPresent(X,fidx_type eps=(fidx_type)1e-5) const;
    template <class X>
    inline fidx_type	getfIndex(X) const;
    template <class X>
    inline int		nearestIndex(X) const;
    template <class X>
    inline T		snap(X,SnapDir dir=OD::SnapNearest) const;
    template <class X>
    inline T		snapAndLimit(X,SnapDir d=OD::SnapNearest) const;

    inline int		nrSteps() const;
    inline fidx_type	nrfSteps() const;
    inline void		sort(bool asc=true) override;
    inline void		scale(T) override;
    inline void		limitTo(const Interval<T>&) override;
    inline T		snappedCenter(SnapDir sd=OD::SnapNearest) const;

    inline bool		isCompatible(const StepInterval<T>&,
				     fidx_type eps=mDefEps) const;
			/*!< epsilon refers to the steps,
				i.e eps=0.1 allows b to be 0.1 steps apart. */
    inline T		snapStep(T inpstep) const;
			    /*!<Snaps inpstep to a positive multiple of step. */

     T			step;

};


typedef StepInterval<Pos::Z_Type> ZSampling;
namespace Pos
{
    typedef Interval<Index_Type>        rg_type;
    typedef StepInterval<Index_Type>    steprg_type;

    mGlobal(Basic) void normalise(steprg_type&,Index_Type defstep);
    mGlobal(Basic) bool intersect(const steprg_type&,const steprg_type&,
				  steprg_type&);
    mGlobal(Basic) void normaliseZ(ZSampling&);
    mGlobal(Basic) bool intersectF(const ZSampling&,const ZSampling&,
				   ZSampling&);
};


/*!\brief ranges in N dimensions. */

template <class T>
mClass(Basic) IntervalND
{
public:
				IntervalND( int ndim_ )
				    : ranges (new Interval<T>[ndim_] )
				    , ndim( ndim_ )
				    , isset( false ) {}

    virtual			~IntervalND() { delete [] ranges; }

    int				nDim() const { return ndim; }
    bool			isSet() const { return isset; }

    const Interval<T>&		getRange(int dim) const { return ranges[dim]; }
    template <class X> inline
    void			setRange(X val);
    template <class X> inline
    void			setRange(X start,X stop);
    template <class X> inline
    void			include(X val);

    template <class X> inline
    bool			includes(X val,bool allowrev) const;
    inline bool			intersects(const IntervalND<T>&,
					   bool allowrev) const;

protected:

    int			ndim;
    Interval<T>*	ranges;
    bool		isset;

};


#include "samplingdata.h"


template <class T> template <class X> inline
void IntervalND<T>::setRange( X val )
{
    for ( int dim=0; dim<ndim; dim++ )
	ranges[dim].start = ranges[dim].stop = (T)val[dim];

    isset = true;
}



template <class T> template <class X> inline
void IntervalND<T>::setRange( X start, X stop )
{
    for ( int dim=0; dim<ndim; dim++ )
    {
	ranges[dim].start = start[dim];
	ranges[dim].stop = stop[dim];
    }

    isset = true;
}


template <class T> template <class X> inline
void IntervalND<T>::include( X val )
{
#ifdef __debug__
    if ( !isset )
	pErrMsg("Doing include on undefined IntervalND");
#endif

    for ( int dim=0; dim<ndim; dim++ )
	ranges[dim].include((T) val[dim]);

    isset = true;
}


template <class T> template <class X> inline
bool IntervalND<T>::includes( X val, bool allowrev ) const
{
#ifdef __debug__
    if ( !isset )
	{ pErrMsg("Doing includes on undefined IntervalND"); return false; }
#endif

    for ( int dim=0; dim<ndim; dim++ )
    {
	if ( !ranges[dim].includes(val[dim], allowrev ) )
	    return false;
    }

    return true;
}


template <class T> inline
bool IntervalND<T>::intersects( const IntervalND<T>& b, bool allowrev ) const
{
    if ( !isset || !b.isset || ndim!=b.ndim)
    {
	pErrMsg("Invalid intersection");
	return false;
    }

    mAllocLargeVarLenArr( T, vector, ndim );
    mAllocLargeVarLenArr( bool, isstart, ndim );
    for ( int dim=0; dim<ndim; dim++ )
    {
	vector[dim] = ranges[dim].start;
	isstart[dim] = true;
    }

    do
    {
	if ( b.includes((T*)vector, allowrev ) )
	    return true;

	int dim = 0;
	while ( dim<ndim )
	{
	    if ( isstart[dim] )
	    {
		isstart[dim] = false;
		vector[dim] = ranges[dim].stop;
		break;
	    }
	    else
	    {
		isstart[dim] = true;
		vector[dim] = ranges[dim].start;
		dim++;
	    }
	}

	if ( dim==ndim )
	    break;

    } while ( true );


    return false;
}


template <class T1,class T2>
inline void assign( Interval<T1>& i1, const Interval<T2>& i2 )
{
    i1.start = (T1)i2.start;
    i1.stop = (T1)i2.stop;
    mDynamicCastGet(StepInterval<T1>*,si1,&i1)
    mDynamicCastGet(const StepInterval<T2>*,si2,&i2)
    if ( si1 && si2 )
	si1->step = (T1)si2->step;
}


template <class T1,class T2>
inline void assign( StepInterval<T1>& i1, const StepInterval<T2>& i2 )
{
    i1.start = (T1)i2.start;
    i1.stop = (T1)i2.stop;
    i1.step = (T1)i2.step;
}



// ---------------- BasicInterval ---------------------

template <class T>
inline BasicInterval<T>&
BasicInterval<T>::operator=( const BasicInterval<T>& oth )
{
    start = oth.start; stop = oth.stop;
    return *this;
}


template <class T> template <class X> inline
int BasicInterval<T>::nearestIndex( X x, T step ) const
{
    return SamplingData<T>( start, step ).nearestIndex( x );
}


template <class T>
template <class X> inline
int BasicInterval<T>::indexOnOrAfter( X x, T step, fidx_type eps ) const
{
    return SamplingData<T>( start, step ).indexOnOrAfter( x, eps );
}


template <class T> inline BasicInterval<T>::BasicInterval()
    : start((T)0), stop((T)0)
{}


template <class T> inline
BasicInterval<T>::BasicInterval( T t1, T t2 )
    : start(t1), stop(t2)	{}


template <class T> inline
Interval<T>* Interval<T>::clone() const
{ return new Interval<T>( *this ); }


template <class T> inline
void BasicInterval<T>::set( T t1, T t2 )
{ start = t1; stop = t2; }


template <class T> inline
bool BasicInterval<T>::isEqual( const BasicInterval<T>& i, T eps ) const
{ return mIsEqual(start,i.start,eps) && mIsEqual(stop,i.stop,eps); }


template <class T> inline
bool BasicInterval<T>::operator==( const BasicInterval<T>& i ) const
{ return start == i.start && stop == i.stop; }


template <class T> inline
bool BasicInterval<T>::operator!=( const BasicInterval<T>& i ) const
{ return ! (i == *this); }


template <class T> inline
BasicInterval<T> BasicInterval<T>::operator+( const BasicInterval<T>& i ) const
{ return Interval<T>(start+i.start, stop+i.stop); }


template <class T> inline
BasicInterval<T>& BasicInterval<T>::operator+=( const BasicInterval<T>& i )
{ start += i.start; stop += i.stop; return *this; }


template <class T> template <class X> inline
const BasicInterval<T>& BasicInterval<T>::setFrom( const BasicInterval<X>& i )
{
    start = (T)i.start;
    stop = (T)i.stop;
    return *this;
}


template <class T> inline
T BasicInterval<T>::width( bool allowrev ) const
{ return allowrev && isRev() ? start - stop : stop - start; }


template <class T> inline
T BasicInterval<T>::center() const
{
    return (start+stop) / ((T)2);
}


template <class T> inline
void BasicInterval<T>::shift( T len )
{ start += len; stop += len; }


template <class T> inline
void BasicInterval<T>::widen( T len, bool allowrev )
{
    if ( allowrev && isRev() )
	{ start += len; stop -= len; }
    else
	{ start -= len; stop += len; }
}


template <class T> template <class X> inline
bool BasicInterval<T>::includes( X t, bool allowrev ) const
{
    return allowrev && isRev()
	? t>=stop && start>=t
	: t>=start && stop>=t;
}


template <class T> template <class X> inline
bool BasicInterval<T>::includes( const BasicInterval<X>& t, bool allowrev )const
{
    return includes( t.start, allowrev ) && includes( t.stop, allowrev );
}


template <class T> template <class X> inline
bool BasicInterval<T>::includes( const Interval<X>& t, bool allowrev ) const
{
    return BasicInterval<T>::includes( static_cast<BasicInterval>( t ));
}


template <class T> template <class X> inline
typename BasicInterval<T>::fidx_type
BasicInterval<T>::pos( X val, bool limit, bool allowrev ) const
{
    fidx_type res = allowrev && isRev()
		  ? (val-stop) / (start-stop)
		  : (val-start) / (stop-start);

    if ( limit )
    {
	if ( res<0 )
	    res = (fidx_type)0;
	else if ( res>1 )
	    res = (fidx_type)1;
    }

    return res;
}


template <class T> template <class X> inline
bool BasicInterval<T>::overlaps( const BasicInterval<X>& t,
				 bool allowrev ) const
{
    return includes( t.start, allowrev ) || includes( t.stop, allowrev ) ||
	   t.includes( start, allowrev ) || t.includes( stop, allowrev );
}


template <class T> inline
void BasicInterval<T>::include( T i, bool allowrev )
{
    if ( mIsUdf(i) )
	return;
    if ( mIsUdf(start) || mIsUdf(stop) )
	start = stop =i;
    else if ( allowrev && isRev() )
	{ if ( stop>i ) stop=i; if ( start<i ) start=i; }
    else
	{ if ( start>i ) start=i; if ( stop<i ) stop=i; }
}


template <class T> inline
void BasicInterval<T>::include( const BasicInterval<T>& i, bool allowrev )
{ include( i.start, allowrev ); include( i.stop, allowrev ); }


template <class T> inline
T BasicInterval<T>::atIndex( int idx, T step ) const
{ return start + step * idx; }

template <> inline
od_int16 BasicInterval<od_int16>::atIndex( int idx, od_int16 step ) const
{ return start + step * mCast(od_int16,idx); }

template <class T> template <class X> inline
int BasicInterval<T>::getIndex( X t, T step ) const
{ return (int)(( t  - start ) / step); }


template <class T> template <class X> inline
typename BasicInterval<T>::fidx_type
BasicInterval<T>::getfIndex( X t, T step ) const
{
    return SamplingData<T>( start, step ).getfIndex( t );
}


template <class T> template <class X> inline
X BasicInterval<T>::limitValue( X x ) const
{
    const bool isrev = isRev();
    if ( (!isrev&&x>stop) || (isrev&&x<stop) ) return (X)stop;
    if ( (!isrev&&x<start) || (isrev&&x>start) ) return (X)start;
    return x;
}


// ---------------- Interval ------------------------

template <class T> inline
Interval<T>::Interval( T startval, T stopval )
    : BasicInterval<T>( startval, stopval )
{}


template <class T> inline
Interval<T>::Interval( const BasicInterval<T>& b )
    : BasicInterval<T>( b )
{}


template <class T> inline
void Interval<T>::sort( bool asc )
{
    if ( (asc && BasicInterval<T>::stop<BasicInterval<T>::start) ||
	 (!asc && BasicInterval<T>::start<BasicInterval<T>::stop) )
	std::swap( BasicInterval<T>::start, BasicInterval<T>::stop );
}


template <class T> inline
void Interval<T>::limitTo( const Interval<T>& i )
{ return BasicInterval<T>::limitTo_( i ); }


template <class T> inline
void Interval<T>::scale( T factor )
{ BasicInterval<T>::start *= factor; BasicInterval<T>::stop *= factor; }


template <class T> inline
bool Interval<T>::isUdf() const
{
    return mIsUdf(BasicInterval<T>::start) || mIsUdf(BasicInterval<T>::stop);
}


// ---------------- StepInterval --------------------

template <class T>
StepInterval<T>::StepInterval()
    : step(1)		{}


template <class T>
StepInterval<T>::StepInterval( T t1, T t2 )
    : Interval<T>(t1,t2), step(T(1))		{}

template <class T>
StepInterval<T>::StepInterval( T t1, T t2, T t3 )
    : Interval<T>(t1,t2), step(t3)		{}


template <class T>
StepInterval<T>::StepInterval( const Interval<T>& intv )
    : Interval<T>(intv)
{ step = intv.hasStep() ? ((const StepInterval<T>&)intv).step : T(1); }

template <class T>
StepInterval<T>::StepInterval( const Interval<T>& intv, T stp )
    : Interval<T>(intv), step(stp)		{}


template <class T>
inline StepInterval<T>& StepInterval<T>::operator=( const Interval<T>& intv )
{ assign( *this, intv ); return *this; }


template <class T> inline
bool StepInterval<T>::isUdf() const
{ return Interval<T>::isUdf() || mIsUdf(step); }


template <class T> inline
StepInterval<T>* StepInterval<T>::clone() const
{ return new StepInterval<T>( *this ); }


template <class T> inline
void StepInterval<T>::set( T t1, T t2, T t3 )
{ Interval<T>::set( t1, t2 ); step = t3; }


template <class T> inline
bool StepInterval<T>::isEqual( const StepInterval<T>& i, T eps ) const
{ return Interval<T>::isEqual(i,eps) && mIsEqual(step,i.step,eps); }


template <class T> inline
bool StepInterval<T>::operator==( const StepInterval<T>& i ) const
{ return Interval<T>::operator==(i) && i.step==step; }


template <class T> inline
bool StepInterval<T>::operator!=( const StepInterval<T>& i ) const
{ return ! (i == *this); }


template <class T> template <class X> inline
const StepInterval<T>& StepInterval<T>::setFrom( const Interval<X>& i )
{
    Interval<T>::setFrom( i );
    if ( i.hasStep() )
	step = (T)(((const StepInterval<X>&)i).step);

    return *this;
}


template <class T> inline
T StepInterval<T>::atIndex( int idx ) const
{ return Interval<T>::atIndex(idx,step); }


template <class T> template <class X> inline
int StepInterval<T>::getIndex( X t ) const
{ return Interval<T>::getIndex( t, step ); }


template <class T> template <class X> inline
bool StepInterval<T>::isPresent( X t, fidx_type eps ) const
{
    const auto fidx = getfIndex( t );
    const fidx_type snapdiff = std::abs( fidx - ((int)fidx) );
    if ( snapdiff > eps )
	return false;
    return fidx > -eps && fidx <= nrSteps() + eps;
}


template <class T> template <class X> inline
typename BasicInterval<T>::fidx_type
StepInterval<T>::getfIndex( X t ) const
{ return Interval<T>::getfIndex( t, step ); }


template <class T> template <class X> inline
int StepInterval<T>::indexOnOrAfter( X x, fidx_type eps ) const
{
    return Interval<T>::indexOnOrAfter( x, step, eps );
}


template <class T> template <class X> inline
int StepInterval<T>::nearestIndex( X x ) const
{
    return Interval<T>::nearestIndex( x, step );
}


template <class T> template <class X> inline
T StepInterval<T>::snap( X t, SnapDir dir ) const
{
    if ( dir==OD::SnapNearest )
	return atIndex( nearestIndex(t) );

    const fidx_type fidx = getfIndex( t );
    const int snappedidx = mNINT32(fidx);
    const fidx_type releps = mIsZero(fidx,1e-5f) ? 1e-5f
					     : fidx < 0.f ? -fidx*1e-5f
							  :  fidx*1e-5f;
    const int idx = mIsEqual(fidx,snappedidx,releps) ? snappedidx
			  : mNINT32( dir==OD::SnapDownward ? Math::Floor(fidx)
							   : Math::Ceil(fidx) );
    return atIndex( idx );
}


template <class T> template <class X> inline
T StepInterval<T>::snapAndLimit( X t, SnapDir dir ) const
{
    T ret = snap( t, dir );
    return this->limitValue( ret );
}


template <class T> inline
void StepInterval<T>::sort( bool asc )
{
    Interval<T>::sort(asc);
    if ( (asc && step < 0) || (!asc && step > 0) )
	step = -step;
}


template <class T> inline
void StepInterval<T>::scale( T factor )
{
    Interval<T>::scale( factor );
    step *= factor;
}


template <class T> inline
T StepInterval<T>::snappedCenter( SnapDir dir ) const
{
    return snap( Interval<T>::center(), dir );
}


template <class T> inline
T StepInterval<T>::snapStep( T inputstep ) const
{
    const double relstep = ((double)inputstep) / step;
    int nrsteps = mNINT32( relstep );
    if ( nrsteps < 1 )
	nrsteps = 1;
    return step * nrsteps;
}


template <class T> inline
typename BasicInterval<T>::fidx_type StepInterval<T>::nrfSteps() const
{
    return Interval<T>::width(true) / step;
}


template <class T>
inline int StepInterval<T>::nrSteps() const
{
    if ( !step ) return 0;
    int ret = (((int)this->start) - ((int) this->stop)) / ((int) step);
    return ret < 0 ? -ret : ret;
}


#define mDefFPNrSteps(typ) \
template <> \
inline int StepInterval<typ>::nrSteps() const \
{ \
    if ( !step ) return 0; \
    typ ns = ( (start > stop ? start : stop) \
	    - (start > stop ? stop : start) ) \
	      / (step > 0 ? step : -step); \
    return mNINT32(ns); \
}

mDefFPNrSteps(float)
mDefFPNrSteps(double)

template <class T>
inline bool StepInterval<T>::isCompatible( const StepInterval<T>& b,
					   fidx_type ) const
{
    if ( step>b.step || b.step%step )
	return false;

    const T diff = this->start - b.start;
    return !(diff%step);
}


#define mDefFPisCompat(typ,releps) \
template <> \
inline bool StepInterval<typ>::isCompatible( const StepInterval<typ>& b, \
			fidx_type eps ) const \
{ \
    const typ typeps = (typ)eps; \
    if ( !mIsEqual(step,b.step,typeps) ) return false; \
 \
    typ nrsteps = (start - b.start) / step; \
    const int nrstepsi = mNINT32( nrsteps ); \
    if ( !nrstepsi ) \
	return mIsEqual( start, b.start, \
		     mIsZero(start,releps) ? releps \
					   : start < 0. ? -start * releps \
							:  start * releps );\
\
    typ reldiff = (nrsteps - nrstepsi) / nrsteps; \
    return ( (reldiff) < (releps) && (reldiff) > (-releps) ); \
}

mDefFPisCompat(float,1e-5f)
mDefFPisCompat(double,1e-10f)


template <class T> inline
void StepInterval<T>::limitTo( const Interval<T>& oth )
{
    if ( !BasicInterval<T>::overlaps(oth) )
	{ Interval<T>::start = Interval<T>::stop = oth.start; return; }
    else if ( !oth.hasStep() )
    {
	const StepInterval<T> org( *this );
	Interval<T>::limitTo_( oth );
	Interval<T>::start = org.snap( Interval<T>::start, OD::SnapUpward );
	Interval<T>::stop = org.snap( Interval<T>::stop, OD::SnapDownward );
	return;
    }

    mDynamicCastGet(const StepInterval<T>*,othsi,&oth)
    if ( !othsi || isCompatible(*othsi) )
	{ limitTo( Interval<T>(oth) ); return; }

    if ( BasicInterval<T>::includes(oth.start,true) )
	Interval<T>::start = snap( oth.start, OD::SnapUpward );
    if ( BasicInterval<T>::includes(oth.stop,true) )
	Interval<T>::stop = snap( oth.stop, OD::SnapDownward );

    if ( othsi->step <= step )
	return;

    double fidxstart = mCast(double,Interval<T>::start) / step;
    fidxstart -= Math::Floor( fidxstart );
    double othfidxstart = mCast(double,oth.start) / step;
    othfidxstart -= Math::Floor( othfidxstart );
    const double releps = mIsZero(fidxstart,1e-6) ? 1e-6
					    : fidxstart < 0. ? -fidxstart*1e-6
							      : fidxstart*1e-6;
    if ( mIsEqual(fidxstart,othfidxstart,releps) )
	step = othsi->step;
}
