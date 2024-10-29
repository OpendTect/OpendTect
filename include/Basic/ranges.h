#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    inline			BasicInterval();
    inline			BasicInterval(const BasicInterval<T>&);
    inline			BasicInterval(const T& start,const T& stop);
    inline virtual		~BasicInterval();

    inline BasicInterval<T>&	operator=(const BasicInterval<T>&);

    inline void			set(const T& start,const T& stop);
    inline bool			isEqual(const BasicInterval<T>& i,
					const T& eps) const;
    inline bool			operator==(const BasicInterval<T>&) const;
    inline bool			operator!=(const BasicInterval<T>&) const;
    inline BasicInterval<T>	operator+(const BasicInterval<T>&) const;
    inline BasicInterval<T>&	operator+=(const BasicInterval<T>&);
    template <class X>
    const BasicInterval<T>&	setFrom(const BasicInterval<X>&);

    inline T			width(bool allowrev=true) const;
    inline T			center() const;
    inline float		getfCenter() const;
    inline void			shift(const T& len);
    inline void			widen(const T& len,bool allowrev=true);

    inline T			atIndex(int,const T& step) const;
    template <class X>
    inline int			getIndex(const X&,const T& step) const;
    template <class X>
    inline float		getfIndex(const X&,const T& step) const;
    template <class X>
    inline int			nearestIndex(const X& x,const T& step) const;
    template <class X>
    inline int			indexOnOrAfter(X x,const T& step,
					       float eps=1e-5) const;
    template <class X>
    inline void			limitTo_( const BasicInterval<X>& i )
				{ start_ = i.limitValue(start_);
				  stop_  = i.limitValue(stop_); }
    template <class X>
    inline X			limitValue(const X&) const;

    template <class X>
    inline bool			overlaps(const BasicInterval<X>&,
					 bool allrev=true) const;
    template <class X>
    inline bool			includes(const X&, bool allowrev ) const;
    template <class X>
    inline bool			includes(const BasicInterval<X>&,
					 bool allowrev=true) const;
    template <class X>
    inline bool			includes(const Interval<X>&,
					 bool allowrev=true) const;

    template <class X>
    inline float		pos(X val,bool limit=true,
				    bool allowrev=true) const;
				/*!<\returns a value between 0 and 1 if val is
				 between start and stop. */
    inline void			include(const T&, bool allowrev=true);
    inline void			include(const BasicInterval<T>&,
					bool allowrev=true);

    static BasicInterval<T>	udf(){return BasicInterval<T>(mUdf(T),mUdf(T));}

    inline bool			isRev() const	{ return start_ > stop_; }

    T				start_;
    T				stop_;

    mDeprecated("Use start_")
    T&				start;
    mDeprecated("Use stop_")
    T&				stop;
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

    inline		Interval() : BasicInterval<T>()	{}
    inline		Interval(const T& start,const T& stop);
    inline		Interval(const BasicInterval<T>&);
    inline		~Interval()	{}

    inline virtual Interval<T>* clone() const;

    virtual bool inline	isUdf() const;
    virtual void	setUdf();

    inline virtual void limitTo(const Interval<T>&);
    inline virtual void	scale(const T&);

    virtual void	sort(bool asc=true);

    virtual bool	hasStep() const	{ return false; }
    static Interval<T>	udf()		{ return Interval<T>(mUdf(T),mUdf(T));}
};


using SampleGate = Interval<int>;
using ZGate = Interval<float>;


/*!\brief Interval with step. */

template <class T>
mClass(Basic) StepInterval : public Interval<T>
{
public:
    inline		StepInterval();
    inline		StepInterval(const T& start,const T& stop,
				     const T& step);
    inline		StepInterval(const Interval<T>&);
    inline		StepInterval(const Interval<T>&,const T& step);
    inline		StepInterval(const StepInterval<T>&);

    inline StepInterval<T>& operator=(const Interval<T>&) = delete;
    inline StepInterval<T>& operator=(const StepInterval<T>&);

    inline bool		isUdf() const override;
    void		setUdf() override;
    static StepInterval<T> udf()
			{ return StepInterval<T>(mUdf(T),mUdf(T),mUdf(T)); }

    bool		hasStep() const override		{ return true; }

    inline StepInterval<T>*	clone() const override;
    inline void		set(const T& start,const T& stop,const T& step);
    inline void		set(const Interval<T>& rg,const T& step);
    inline void		setInterval(const Interval<T>&);

    template <class X>
    const StepInterval<T>& setFrom(const Interval<X>&);

    inline bool		isEqual(const StepInterval<T>& i,const T& eps) const;
    inline bool		operator==(const StepInterval<T>&) const;
    inline bool		operator!=(const StepInterval<T>&) const;
    inline T		atIndex(int) const;
    template <class X>
    inline int		getIndex(const X&) const;
    template <class X> inline
    int			indexOnOrAfter( X x, float eps ) const;
    template <class X>
    bool		isPresent(const X&,float eps=1e-5f) const;

    template <class X>
    inline float	getfIndex(const X&) const;
    template <class X>
    inline int		nearestIndex(const X&) const;
    template <class X>
    inline T		snap(const X&,OD::SnapDir dir=OD::SnapNearest) const;
    template <class X>
    inline T		snapAndLimit(X,OD::SnapDir d=OD::SnapNearest) const;

    inline int		nrSteps() const;
    inline float	nrfSteps() const;
    inline void		sort(bool asc=true) override;
    inline void		scale(const T&) override;
    inline void		limitTo(const Interval<T>&) override;
    inline T		snappedCenter(OD::SnapDir sd=OD::SnapNearest) const;

    inline bool		isCompatible(const StepInterval<T>&,
				     float eps=mDefEps) const;
			/*!< epsilon refers to the steps,
				i.e eps=0.1 allows b to be 0.1 steps apart.
			*/
    inline bool		isMultipleOfStep(const T&) const;
    inline T		snapStep(const T& inpstep) const;
			/*!<Snaps inpstep to a positive multiple of step. */

    StepInterval<T>	niceInterval(int,bool allowrev=true) const;

    T			step_;
    mDeprecated("Use with underscore")
    T&			step;

};


using ZSampling = StepInterval<float>;


namespace Pos
{
    mGlobal(Basic)	void normalize(StepInterval<int>&,int defstep);
    mGlobal(Basic)	bool intersect(const StepInterval<int>&,
					const StepInterval<int>&,
					StepInterval<int>&);
    mGlobal(Basic)	void normalizeZ(ZSampling&);
    mGlobal(Basic)	bool intersectF(const ZSampling&,const ZSampling&,
					ZSampling&);
    mDeprecated("Use normalize")
    mGlobal(Basic)	void normalise(StepInterval<int>&,int defstep);
    mDeprecated("Use normalizeZ")
    mGlobal(Basic)	void normaliseZ(ZSampling&);
};


/*!\brief ranges in N dimensions. */

template <class T>
mClass(Basic) IntervalND
{
public:
				IntervalND( int ndim )
				    : ranges_(new Interval<T>[ndim] )
				    , ndim_( ndim )
				    , isset_( false ) {}

    virtual			~IntervalND() { delete [] ranges_; }

    int				nDim() const { return ndim_; }
    bool			isSet() const { return isset_; }

    const Interval<T>&		getRange(int dim) const { return ranges_[dim]; }
    template <class X> inline
    void			setRange(const X& val);
    template <class X> inline
    void			setRange(const X& start,const X& stop);
    template <class X> inline
    void			include(const X& val);

    template <class X> inline
    bool			includes(const X& val,bool allowrev) const;
    inline bool			intersects(const IntervalND<T>&,
					   bool allowrev) const;

protected:

    int			ndim_;
    Interval<T>*	ranges_;
    bool		isset_;

};


#include "samplingdata.h"


template <class T> template <class X> inline
void IntervalND<T>::setRange( const X& val )
{
    for ( int dim=0; dim<ndim_; dim++ )
	ranges_[dim].start_ = ranges_[dim].stop_ = (T) val[dim];

    isset_ = true;
}



template <class T> template <class X> inline
void IntervalND<T>::setRange( const X& start, const X& stop)
{
    for ( int dim=0; dim<ndim_; dim++ )
    {
	ranges_[dim].start_ = start[dim];
	ranges_[dim].stop_ = stop[dim];
    }

    isset_ = true;
}


template <class T> template <class X> inline
void IntervalND<T>::include( const X& val )
{
#ifdef __debug__
    if ( !isset_ )
	pErrMsg("Doing include on undefined IntervalND");
#endif

    for ( int dim=0; dim<ndim_; dim++ )
	ranges_[dim].include((T) val[dim]);

    isset_ = true;
}


template <class T> template <class X> inline
bool IntervalND<T>::includes( const X& val, bool allowrev ) const
{
#ifdef __debug__
    if ( !isset_ )
    {
	pErrMsg("Doing includes on undefined IntervalND");
	return false;
    }
#endif

    for ( int dim=0; dim<ndim_; dim++ )
    {
	if ( !ranges_[dim].includes(val[dim], allowrev ) )
	    return false;
    }

    return true;
}


template <class T> inline
bool IntervalND<T>::intersects( const IntervalND<T>& b, bool allowrev ) const
{
    if ( !isset_ || !b.isset_ || ndim_!=b.ndim_)
    {
	pErrMsg("Invalid intersection");
	return false;
    }

    mAllocLargeVarLenArr( T, vector, ndim_ );
    mAllocLargeVarLenArr( bool, isstart, ndim_ );
    for ( int dim=0; dim<ndim_; dim++ )
    {
	vector[dim] = ranges_[dim].start_;
	isstart[dim] = true;
    }

    do
    {
	if ( b.includes( mVarLenArr(vector), allowrev ) )
	    return true;

	int dim = 0;
	while ( dim<ndim_ )
	{
	    if ( isstart[dim] )
	    {
		isstart[dim] = false;
		vector[dim] = ranges_[dim].stop_;
		break;
	    }
	    else
	    {
		isstart[dim] = true;
		vector[dim] = ranges_[dim].start_;
		dim++;
	    }
	}

	if ( dim==ndim_ )
	    break;

    } while ( true );


    return false;
}


template <class T1,class T2>
inline void assign( Interval<T1>& i1, const Interval<T2>& i2 )
{
    i1.start_ = (T1)i2.start_;
    i1.stop_ = (T1)i2.stop_;
    mDynamicCastGet(StepInterval<T1>*,si1,&i1)
    mDynamicCastGet(const StepInterval<T2>*,si2,&i2)
    if ( si1 && si2 )
	si1->step_ = (T1)si2->step_;
}


template <class T1,class T2>
inline void assign( StepInterval<T1>& i1, const StepInterval<T2>& i2 )
{ i1.start_ = (T1)i2.start_; i1.stop_ = (T1)i2.stop_; i1.step_ = (T1)i2.step_; }



// ---------------- BasicInterval ---------------------

mStartAllowDeprecatedSection

template <class T> inline
BasicInterval<T>::BasicInterval()
    : start_(0)
    , stop_(0)
    , start(start_)
    , stop(stop_)
{}


template <class T> inline
BasicInterval<T>::BasicInterval( const T& t1, const T& t2 )
    : start_(t1)
    , stop_(t2)
    , start(start_)
    , stop(stop_)
{}


template <class T> inline
BasicInterval<T>::BasicInterval( const BasicInterval<T>& oth )
    : start_(oth.start_)
    , stop_(oth.stop_)
    , start(start_)
    , stop(stop_)
{}

mStopAllowDeprecatedSection


template <class T>
BasicInterval<T>::~BasicInterval()
{}

template <class T> inline
BasicInterval<T>& BasicInterval<T>::operator=( const BasicInterval<T>& intv )
{ start_ = intv.start_; stop_ = intv.stop_; return *this; }


template <class T> template <class X> inline
int BasicInterval<T>::nearestIndex( const X& x, const T& step ) const
{
    return SamplingData<T>( start_, step ).nearestIndex( x );
}


template <class T>
template <class X> inline
int BasicInterval<T>::indexOnOrAfter( X x, const T& step, float eps ) const
{
    return SamplingData<T>( start_, step ).indexOnOrAfter( x, eps );
}


template <class T> inline
Interval<T>* Interval<T>::clone() const
{ return new Interval<T>( *this ); }


template <class T> inline
void BasicInterval<T>::set( const T& t1, const T& t2 )
{ start_ = t1; stop_ = t2; }


template <class T> inline
bool BasicInterval<T>::isEqual( const BasicInterval<T>& i, const T& eps ) const
{ return mIsEqual(start_,i.start_,eps) && mIsEqual(stop_,i.stop_,eps); }


template <class T> inline
bool BasicInterval<T>::operator==( const BasicInterval<T>& i ) const
{ return start_ == i.start_ && stop_ == i.stop_; }


template <class T> inline
bool BasicInterval<T>::operator!=( const BasicInterval<T>& i ) const
{ return ! (i == *this); }


template <class T> inline
BasicInterval<T> BasicInterval<T>::operator+( const BasicInterval<T>& i ) const
{ return Interval<T>(start_+i.start_, stop_+i.stop_); }


template <class T> inline
BasicInterval<T>& BasicInterval<T>::operator+=( const BasicInterval<T>& i )
{ start_ += i.start_; stop_ += i.stop_; return *this; }


template <class T> template <class X> inline
const BasicInterval<T>& BasicInterval<T>::setFrom( const BasicInterval<X>& i )
{
    start_ = (T)i.start_;
    stop_ = (T)i.stop_;
    return *this;
}


template <class T> inline
T BasicInterval<T>::width( bool allowrev ) const
{ return allowrev && isRev() ? start_ - stop_ : stop_ - start_; }


#define mCenterImpl(func,typ) \
template <class T> inline  \
typ BasicInterval<T>::func() const \
{ return ((typ)(start_+stop_))/2; }

mCenterImpl(center, T )
mCenterImpl(getfCenter, float )

#undef mCenterImpl


template <class T> inline
void BasicInterval<T>::shift( const T& len )
{ start_ += len; stop_ += len; }


template <class T> inline
void BasicInterval<T>::widen( const T& len, bool allowrev )
{
    if ( allowrev && isRev() )
	{ start_ += len; stop_ -= len; }
    else
	{ start_ -= len; stop_ += len; }
}


template <class T> template <class X> inline
bool BasicInterval<T>::includes( const X& t, bool allowrev ) const
{
    return allowrev && isRev()
	? t>=stop_ && start_>=t
	: t>=start_ && stop_>=t;
}


template <class T> template <class X> inline
bool BasicInterval<T>::includes( const BasicInterval<X>& t, bool allowrev )const
{
    return includes( t.start_, allowrev ) && includes( t.stop_, allowrev );
}


template <class T> template <class X> inline
bool BasicInterval<T>::includes( const Interval<X>& t, bool allowrev ) const
{
    return BasicInterval<T>::includes(
			static_cast<BasicInterval>(t), allowrev );
}


template <class T> template <class X> inline
float BasicInterval<T>::pos( X val, bool limit, bool allowrev ) const
{
    float res = allowrev && isRev()
	? (val-stop_)/(start_-stop_)
	: (val-start_)/(stop_-start_);

    if ( limit )
    {
	if ( res<0 ) res = 0;
	else if ( res>1 ) res = 1;
    }

    return res;
}


template <class T> template <class X> inline
bool BasicInterval<T>::overlaps( const BasicInterval<X>& t,
				 bool allowrev ) const
{
    return includes( t.start_, allowrev ) || includes( t.stop_, allowrev ) ||
	   t.includes( start_, allowrev ) || t.includes( stop_, allowrev );
}


template <class T> inline
void BasicInterval<T>::include( const T& i, bool allowrev )
{
    if ( mIsUdf(i) )
	return;

    if ( mIsUdf(start_) || mIsUdf(stop_) )
	start_ = stop_ = i;
    else if ( allowrev && isRev() )
    {
	if ( stop_>i )
	    stop_=i;
	if ( start_<i )
	    start_=i;
    }
    else
    {
	if ( start_>i )
	    start_=i;
	if ( stop_<i )
	    stop_=i;
    }
}


template <class T> inline
void BasicInterval<T>::include( const BasicInterval<T>& i, bool allowrev )
{
    include( i.start_, allowrev );
    include( i.stop_, allowrev );
}


template <class T> inline
T BasicInterval<T>::atIndex( int idx, const T& step ) const
{ return start_ + step * idx; }


template <class T> template <class X> inline
int BasicInterval<T>::getIndex( const X& t, const T& step ) const
{ return (int)(( t  - start_ ) / step); }


template <class T> template <class X> inline
float BasicInterval<T>::getfIndex( const X& t, const T& step ) const
{ return SamplingData<T>( start_, step ).getfIndex( t ); }


template <class T> template <class X> inline
X BasicInterval<T>::limitValue( const X& t ) const
{
    const bool isrev = isRev();
    if ( (!isrev&&t>stop_) || (isrev&&t<stop_) )
	return stop_;
    if ( (!isrev&&t<start_) || (isrev&&t>start_) )
	return start_;

    return t;
}


// ---------------- Interval ------------------------

template <class T> inline
Interval<T>::Interval( const T& startval, const T& stopval )
    : BasicInterval<T>( startval, stopval )
{}


template <class T> inline
Interval<T>::Interval( const BasicInterval<T>& b )
    : BasicInterval<T>( b )
{}


template <class T> inline
void Interval<T>::sort( bool asc )
{
    if ( (asc && BasicInterval<T>::stop_<BasicInterval<T>::start_) ||
	 (!asc && BasicInterval<T>::start_<BasicInterval<T>::stop_) )
	Swap(BasicInterval<T>::start_,BasicInterval<T>::stop_);
}


template <class T> inline
void Interval<T>::limitTo( const Interval<T>& i )
{
    if ( BasicInterval<T>::overlaps(i) )
	BasicInterval<T>::limitTo_( i );
    else
	setUdf();
}


template <class T> inline
void Interval<T>::scale( const T& factor )
{ BasicInterval<T>::start_ *= factor; BasicInterval<T>::stop_ *= factor; }


template <class T> inline
bool Interval<T>::isUdf() const
{
    return mIsUdf(BasicInterval<T>::start_) || mIsUdf(BasicInterval<T>::stop_);
}


template <class T> inline
void Interval<T>::setUdf()
{ BasicInterval<T>::set( mUdf(T), mUdf(T) ); }



// ---------------- StepInterval --------------------

mStartAllowDeprecatedSection

template <class T>
StepInterval<T>::StepInterval()
    : step_(1)
    , step(step_)
{}


template <class T>
StepInterval<T>::StepInterval( const T& t1, const T& t2, const T& t3 )
    : Interval<T>(t1,t2)
    , step_(t3)
    , step(step_)
{}


template <class T>
StepInterval<T>::StepInterval( const Interval<T>& intv )
    : Interval<T>(intv)
    , step(step_)
{
    step_ = 1;
    if ( intv.hasStep() )
    {
	mDynamicCastGet(const StepInterval<T>*,sintv,&intv);
	step_ = sintv ? sintv->step_ : 1;
    }
}

template <class T>
StepInterval<T>::StepInterval( const Interval<T>& intv, const T& stp )
    : Interval<T>(intv)
    , step_(stp)
    , step(step_)
{}


template <class T>
StepInterval<T>::StepInterval( const StepInterval<T>& si )
    : Interval<T>(si)
    , step_(si.step_)
    , step(step_)
{}

mStopAllowDeprecatedSection


template <class T>
inline StepInterval<T>& StepInterval<T>::operator=( const StepInterval<T>& intv)
{
    assign( *this, intv );
    return *this;
}


template <class T> inline
bool StepInterval<T>::isUdf() const
{
    return Interval<T>::isUdf() || mIsUdf(step_);
}


template <class T> inline
void StepInterval<T>::setUdf()
{
    Interval<T>::set( mUdf(T), mUdf(T) );
    step_ = mUdf(T);
}


template <class T> inline
StepInterval<T>* StepInterval<T>::clone() const
{ return new StepInterval<T>( *this ); }


template <class T> inline
void StepInterval<T>::set( const T& t1, const T& t2, const T& t3 )
{ Interval<T>::set( t1, t2 ); step_ = t3; }


template <class T> inline
void StepInterval<T>::set( const Interval<T>& rg, const T& stp )
{ set( rg.start_, rg.stop_, stp ); }

template <class T> inline
void StepInterval<T>::setInterval( const Interval<T>& rg )
{ Interval<T>::set( rg.start_, rg.stop_ ); }

template <class T> inline
bool StepInterval<T>::isEqual( const StepInterval<T>& i, const T& eps ) const
{ return Interval<T>::isEqual(i,eps) && mIsEqual(step_,i.step_,eps); }


template <class T> inline
bool StepInterval<T>::operator==( const StepInterval<T>& i ) const
{ return Interval<T>::operator==(i) && i.step_==step_; }


template <class T> inline
bool StepInterval<T>::operator!=( const StepInterval<T>& i ) const
{ return ! (i == *this); }


template <class T> template <class X> inline
const StepInterval<T>& StepInterval<T>::setFrom( const Interval<X>& i )
{
    Interval<T>::setFrom( i );
    if ( i.hasStep() )
	step_ = (T)(((const StepInterval<X>&)i).step_);

    return *this;
}


template <class T> inline
T StepInterval<T>::atIndex( int idx ) const
{ return Interval<T>::atIndex(idx,step_); }


template <class T> template <class X> inline
int StepInterval<T>::getIndex( const X& t ) const
{ return Interval<T>::getIndex( t, step_ ); }


template <class T> template <class X> inline
float StepInterval<T>::getfIndex( const X& t ) const
{ return Interval<T>::getfIndex( t, step_ ); }


template <class T> template <class X> inline
int StepInterval<T>::indexOnOrAfter( X x, float eps ) const
{
    return Interval<T>::indexOnOrAfter( x, step_, eps );
}


template <class T> template <class X> inline
bool StepInterval<T>::isPresent( const X& t, float eps ) const
{
    const float fidx = getfIndex( t );
    const float snapdiff = std::abs( fidx - int(fidx) );
    if ( snapdiff > eps )
	return false;

    return fidx > -eps && fidx <= nrSteps() + eps;
}


template <class T> template <class X> inline
int StepInterval<T>::nearestIndex( const X& x ) const
{ return Interval<T>::nearestIndex( x, step_ ); }


template <class T> template <class X> inline
T StepInterval<T>::snap( const X& t, OD::SnapDir dir ) const
{
    if ( dir==OD::SnapNearest )
	return atIndex( nearestIndex(t) );

    const float fidx = getfIndex( t );
    const int snappedidx = mNINT32(fidx);
    const float releps = mIsZero(fidx,1e-4f) ? 1e-4f
					     : fidx < 0.f ? -fidx*1e-4f
							  :  fidx*1e-4f;
    const int idx = mIsEqual(fidx,snappedidx,releps) ? snappedidx
			: mNINT32( dir==OD::SnapDownward ? Math::Floor(fidx)
							 : Math::Ceil(fidx) );
    return atIndex( idx );
}


template <class T> template <class X> inline
T StepInterval<T>::snapAndLimit( X t, OD::SnapDir dir ) const
{
    T ret = snap( t, dir );
    return this->limitValue( ret );
}


template <class T> inline
void StepInterval<T>::sort( bool asc )
{
    Interval<T>::sort(asc);
    if ( (asc && step_ < 0) || (!asc && step_ > 0) )
	step_ = -step_;
}


template <class T> inline
void StepInterval<T>::scale( const T& factor )
{
    Interval<T>::scale( factor );
    step_ *= factor;
}


template <class T> inline
T StepInterval<T>::snappedCenter( OD::SnapDir dir ) const
{ return snap( Interval<T>::center(), dir ); }


template <class T> inline
T StepInterval<T>::snapStep( const T& inputstep ) const
{
    const double relstep = ((double)inputstep) / step_;
    int nrsteps = mNINT32( relstep );
    if ( nrsteps < 1 )
	nrsteps = 1;
    return step_ * nrsteps;
}


template <class T> inline
float StepInterval<T>::nrfSteps() const
{
    const float w = Interval<T>::width( true );
    return w/step_;
}


template <class T>
inline int StepInterval<T>::nrSteps() const
{
    if ( !step_ ) return 0;
    int ret = (((int)this->start_) - ((int) this->stop_)) / ((int) step_);
    return ret < 0 ? -ret : ret;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define mDefFNrSteps(typ) \
template <> \
inline int StepInterval<typ>::nrSteps() const \
{ \
    if ( !step_ ) return 0; \
    typ ns = ( (start_ > stop_ ? start_ : stop_) \
	    - (start_ > stop_ ? stop_ : start_) ) \
	      / (step_ > 0 ? step_ : -step_); \
    return mNINT32(ns); \
}

mDefFNrSteps(float)
mDefFNrSteps(double)

#endif // DOXYGEN_SHOULD_SKIP_THIS


template <class T>
inline bool StepInterval<T>::isMultipleOfStep( const T& val ) const
{
    return val % step_ == 0;
}


#define mDefFltIsMultipleOfStep(typ,releps) \
template <> \
inline bool StepInterval<typ>::isMultipleOfStep( const typ& val ) const \
{ \
    return mIsZero( remainder(val,step_), step_*releps ); \
}

mDefFltIsMultipleOfStep(float,1e-4f)
mDefFltIsMultipleOfStep(double,1e-8)
// Do not change the above releps values as they originate from the types.

template <class T>
inline bool StepInterval<T>::isCompatible( const StepInterval<T>& b,
					   float ) const
{
    if ( !step_ || step_>b.step_ || !isMultipleOfStep(b.step_) )
	return false;

    const T diff = this->start_ - b.start_;
    return isMultipleOfStep( diff );
}


template <class T> inline
void StepInterval<T>::limitTo( const Interval<T>& oth )
{
    if ( !BasicInterval<T>::overlaps(oth) )
    {
	setUdf();
	return;
    }

    if ( !oth.hasStep() )
    {
	const StepInterval<T> org( *this );
	Interval<T>::limitTo_( oth );
	Interval<T>::start_ = org.snap( Interval<T>::start_, OD::SnapUpward );
	Interval<T>::stop_ = org.snap( Interval<T>::stop_, OD::SnapDownward );
	return;
    }

    mDynamicCastGet(const StepInterval<T>*,othsi,&oth)
    if ( isCompatible(*othsi) )
    {
	Interval<T>::limitTo( Interval<T>(oth) );
	Interval<T>::start_ = othsi->snap( Interval<T>::start_,
					   OD::SnapUpward );
	Interval<T>::stop_ = othsi->snap( Interval<T>::stop_,
					  OD::SnapDownward );
	step_ = othsi->step_;
	return;
    }

    const StepInterval<T> org( *this );
    Interval<T>::limitTo_( oth );
    Interval<T>::start_ = org.snap( Interval<T>::start_, OD::SnapUpward );
    Interval<T>::stop_ = org.snap( Interval<T>::stop_, OD::SnapDownward );
}


template <class T> inline
StepInterval<T> StepInterval<T>::niceInterval( int maxsteps, bool canrev ) const
{
    if ( isUdf() )
	return udf();

    const bool isrev = Interval<T>::isRev();
    T min = isrev ? this->stop_ : this->start_;
    T max = isrev ? this->start_ : this->stop_;
    if ( mIsZero(min, mDefEps) && mIsZero(max, mDefEps) )
	return StepInterval<T>();

    T range = Math::Abs( max - min );
    if ( mIsZero(range, mDefEps) )
    {
	range = 0.2 * Math::Abs( min );
	min -= range;
	max += range;
	range += range;
    }

    const T nice_step = Math::NiceNumber( range/(maxsteps), true );
    const T del = 0.001*Math::Abs(nice_step);
    const T nice_min = Math::Floor( (min+del)/nice_step ) * nice_step;
    const T nice_max = Math::Ceil( (max-del)/nice_step ) * nice_step;
    return isrev && canrev ? StepInterval<T>( nice_max, nice_min, nice_step ) :
			     StepInterval<T>( nice_min, nice_max, nice_step );
}


mGlobal(Basic) BufferString toUserString(const Interval<int>&);
mGlobal(Basic) BufferString toUserString(const Interval<float>&,int precision);
mGlobal(Basic) BufferString toUserStringF(const Interval<float>&,int nrdec);
mGlobal(Basic) BufferString toUserString(const Interval<double>&,int precision);
