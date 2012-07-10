#ifndef ranges_h
#define ranges_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		23-10-1996
 Contents:	Ranges
 RCS:		$Id: ranges.h,v 1.71 2012-07-10 08:05:25 cvskris Exp $
________________________________________________________________________

-*/

#include "general.h"
#include "errh.h"
#include "ptrman.h"


/*!\brief interval of values.

Note that start does not need to be lower than stop. That's why there is a
sort() method.

*/

template <class T>
class Interval
{
public:
    inline		Interval();
    inline virtual	~Interval()		{}
    inline		Interval(const T& start,const T& stop);
    inline Interval<T>&	operator=(const Interval<T>&);
    virtual bool inline	isUdf() const;
    inline
    virtual Interval<T>* clone() const;

    inline void		set(const T& start,const T& stop);
    inline bool		isEqual(const Interval<T>& i,const T& eps) const;
    inline bool		operator==(const Interval<T>&) const;
    inline bool		operator!=(const Interval<T>&) const;
    inline Interval<T>	operator+(const Interval<T>&) const;
    inline Interval<T>&	operator+=(const Interval<T>&);
    template <class X>
    const Interval<T>&	setFrom(const Interval<X>&);

    inline T		width(bool allowrev=true) const;
    inline virtual T	center() const;
    inline float	getfCenter() const;
    inline void		shift(const T& len);
    inline void		widen(const T& len,bool allowrev=true);
    inline virtual void	scale(const T&);

    inline T		atIndex(int,const T& step) const;
    template <class X>
    inline int		getIndex(const X&,const T& step) const;
    template <class X>
    inline float	getfIndex(const X&,const T& step) const;
    template <class X>
    inline int		nearestIndex(const X& x,const T& step) const;

    template <class X>
    inline void		limitTo( const Interval<X>& i )
    			{ start = i.limitValue(start);
			  stop  = i.limitValue(stop); }
    template <class X>
    inline X		limitValue(const X&) const;

    template <class X>
    inline bool		overlaps(const Interval<X>&,bool allrev=true) const;
    template <class X>
    inline bool		includes(const X&, bool allowrev ) const;
    template <class X>
    inline bool		includes(const Interval<X>&, bool allowrev=true) const;
    template <class X>
    inline float        pos(X val,bool limit=true,bool allowrev=true) const;
			/*!<\returns a value between 0 and 1 if val is 
			     between start and stop. */
    inline void		include(const T&, bool allowrev=true);
    inline void		include(const Interval<T>&,bool allowrev=true);

    virtual void	sort( bool asc=true );

    T			start;
    T			stop;

    inline bool		isRev() const		{ return start > stop; }
    virtual bool	hasStep() const		{ return false; }
};


typedef Interval<int> SampleGate;
typedef Interval<float> ZGate;


#define cloneTp	mPolyRet( Interval<T>, StepInterval<T> )

/*!\brief Interval with step. */

template <class T>
class StepInterval : public Interval<T>
{
public:

    inline		StepInterval();
    inline		StepInterval(const T& start,const T& stop,
				     const T& step);
    inline		StepInterval(const Interval<T>&);
    inline		StepInterval( const StepInterval<T>& si )
			: Interval<T>(si), step(si.step)	{}
    inline StepInterval<T>& operator=(const Interval<T>&);

    virtual bool inline	isUdf() const;
    virtual bool	hasStep() const		{ return true; }

    inline
    virtual cloneTp*	clone() const;
    inline void		set(const T& start,const T& stop,const T& step);

    template <class X>
    const StepInterval<T>& setFrom(const Interval<X>&);

    inline bool		isEqual(const StepInterval<T>& i,const T& eps) const;
    inline bool		operator==(const StepInterval<T>&) const;
    inline bool		operator!=(const StepInterval<T>&) const;
    inline T		atIndex(int) const;
    template <class X>
    inline int		getIndex(const X&) const;
    template <class X>
    inline float	getfIndex(const X&) const;
    template <class X>
    inline int		nearestIndex(const X&) const;
    template <class X>
    inline T		snap(const X&) const;

    inline int		nrSteps() const;
    inline float	nrfSteps() const;
    virtual inline void	sort(bool asc=true);
    inline void		scale(const T&);
    inline T		snappedCenter() const;

    inline bool		isCompatible(const StepInterval<T>&,
	    			     float eps=mDefEps) const;
			/*!< epsilon refers to the steps,
			  	i.e eps=0.1 allows b to be 0.1 steps apart.
			*/
    inline T		snapStep(const T& inpstep) const;
    			/*!<Snaps inpstep to a positive multiple of step. */

     T			step;

};


template <class T> class IntervalND
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
    void 			setRange(const X& val);
    template <class X> inline
    void			setRange(const X& start,const X& stop);
    template <class X> inline
    void			include(const X& val);

    template <class X> inline
    bool			includes(const X& val,bool allowrev) const;
    inline bool			intersects(const IntervalND<T>&,
	    				   bool allowrev) const;

protected:

    int			ndim;
    Interval<T>*	ranges;
    bool		isset;

};


template <class T> template <class X> inline
void IntervalND<T>::setRange( const X& val )
{
    for ( int dim=0; dim<ndim; dim++ )
	ranges[dim].start = ranges[dim].stop = val[dim];

    isset = true;
}



template <class T> template <class X> inline
void IntervalND<T>::setRange( const X& start, const X& stop)
{
    for ( int dim=0; dim<ndim; dim++ )
    {
	ranges[dim].start = start[dim];
	ranges[dim].stop = stop[dim];
    }

    isset = true;
}


template <class T> template <class X> inline
void IntervalND<T>::include( const X& val )
{
#ifdef __debug__
    if ( !isset )
	pErrMsg("Doing include on undefined IntervalND");
#endif

    for ( int dim=0; dim<ndim; dim++ )
	ranges[dim].include(val[dim]);

    isset = true;
}


template <class T> template <class X> inline
bool IntervalND<T>::includes( const X& val, bool allowrev ) const
{
#ifdef __debug__
    if ( !isset )
    {
	pErrMsg("Doing includes on undefined IntervalND");
	return false;
    }
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

    ArrPtrMan<T> vector = new T [ndim];
    ArrPtrMan<bool> isstart = new bool [ndim];
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
{ i1.start = (T1)i2.start; i1.stop = (T1)i2.stop; i1.step = (T1)i2.step; }



// ---------------- Interval ---------------------

template <class T>
inline Interval<T>& Interval<T>::operator=( const Interval<T>& intv )
{ start = intv.start; stop = intv.stop; return *this; }

template <class T> template <class X> inline
int Interval<T>::nearestIndex( const X& x, const T& step ) const
{
    int nr = getIndex(x,step);
    const T atindex = atIndex(nr,step);
    const float reldiff = (float)(x-atindex)/step;

    if ( reldiff>=0.5 ) return nr+1;
    else if ( reldiff<=-0.5 ) return nr-1;
    return nr;
}


template <class T> inline Interval<T>::Interval()
{ start = 0; stop = 0; }


template <class T> inline Interval<T>::Interval( const T& t1, const T& t2 )
{ start = t1; stop = t2; }

template <class T> inline
bool Interval<T>::isUdf() const
{
    return mIsUdf(start) || mIsUdf(stop);
}

template <class T> inline
Interval<T>* Interval<T>::clone() const		
{ return new Interval<T>( *this ); }


template <class T> inline
void Interval<T>::set( const T& t1, const T& t2 )
{ start = t1; stop = t2; }


template <class T> inline
bool Interval<T>::isEqual( const Interval<T>& i, const T& eps ) const
{ return mIsEqual(start,i.start,eps) && mIsEqual(stop,i.stop,eps); }


template <class T> inline
bool Interval<T>::operator==( const Interval<T>& i ) const
{ return start == i.start && stop == i.stop; }


template <class T> inline
bool Interval<T>::operator!=( const Interval<T>& i ) const
{ return ! (i == *this); }


template <class T> inline
Interval<T> Interval<T>::operator+( const Interval<T>& i ) const
{ return Interval<T>(start+i.start, stop+i.stop); }

template <class T> inline
Interval<T>& Interval<T>::operator+=( const Interval<T>& i )
{ start += i.start; stop += i.stop; return *this; }


template <class T> template <class X> inline
const Interval<T>& Interval<T>::setFrom( const Interval<X>& i )
{
    start = (T) i.start;
    stop = (T) i.stop;
    return *this;
}


template <class T> inline
T Interval<T>::width( bool allowrev ) const
{ return allowrev && isRev() ? start - stop : stop - start; }

#define mCenterImpl(func,typ) \
template <class T> inline  \
typ Interval<T>::func() const \
{ return ((typ)(start+stop))/2; }

mCenterImpl(center, T )
mCenterImpl(getfCenter, float )

#undef mCenterImpl


template <class T> inline
void Interval<T>::shift( const T& len )
{ start += len; stop += len; }


template <class T> inline
void Interval<T>::widen( const T& len, bool allowrev )
{
    if ( allowrev && isRev() )
	{ start += len; stop -= len; }
    else
	{ start -= len; stop += len; }
}


template <class T> inline
void Interval<T>::scale( const T& factor )
{ start *= factor; stop *= factor; }


template <class T> template <class X> inline
bool Interval<T>::includes( const X& t, bool allowrev ) const
{
    return allowrev && isRev()
	? t>=stop && start>=t
	: t>=start && stop>=t;
}


template <class T> template <class X> inline
bool Interval<T>::includes( const Interval<X>& t, bool allowrev ) const
{
    return includes( t.start, allowrev ) && includes( t.stop, allowrev );
}


template <class T> template <class X> inline
float Interval<T>::pos( X val, bool limit, bool allowrev ) const
{
    float res = allowrev && isRev()
	? (val-stop)/(start-stop)
	: (val-start)/(stop-start);

    if ( limit )
    {
	if ( res<0 ) res = 0;
	else if ( res>1 ) res = 1;
    }

    return res;
}


template <class T> template <class X> inline
bool Interval<T>::overlaps( const Interval<X>& t, bool allowrev ) const
{
    return includes( t.start, allowrev ) || includes( t.stop, allowrev ) ||
	   t.includes( start, allowrev ) || t.includes( stop, allowrev );
}


template <class T> inline
void Interval<T>::include( const T& i, bool allowrev )
{
    if ( allowrev && isRev() )
	{ if ( stop>i ) stop=i; if ( start<i ) start=i; }
    else
	{ if ( start>i ) start=i; if ( stop<i ) stop=i; }
}


template <class T> inline
void Interval<T>::include( const Interval<T>& i, bool allowrev )
{ include( i.start, allowrev ); include( i.stop, allowrev ); }


template <class T> inline
T Interval<T>::atIndex( int idx, const T& step ) const
{ return start + step * idx; }


template <class T> template <class X> inline
int Interval<T>::getIndex( const X& t, const T& step ) const
{ return (int)(( t  - start ) / step); }


template <class T> template <class X> inline
float Interval<T>::getfIndex( const X& t, const T& step ) const
{ return (( (float)t  - start ) / step); }


template <class T> template <class X> inline
X Interval<T>::limitValue( const X& t ) const
{
    const bool isrev = isRev();
    if ( (!isrev&&t>stop) || (isrev&&t<stop) ) return stop;
    if ( (!isrev&&t<start) || (isrev&&t>start) ) return start;
    return t;
}
		

template <class T> 
void Interval<T>::sort( bool asc )
{
    if ( (asc && stop<start) || (!asc && start<stop) )
	 Swap(start,stop);
}



// ---------------- StepInterval --------------------

template <class T>
StepInterval<T>::StepInterval()
{ step = 1; }


template <class T>
StepInterval<T>::StepInterval( const T& t1, const T& t2, const T& t3 )
    : Interval<T>(t1,t2)
{ step = t3; }


template <class T>
StepInterval<T>::StepInterval( const Interval<T>& intv )
    : Interval<T>(intv)
{ step = intv.hasStep() ? ((StepInterval<T>&)intv).step : 1; }

template <class T>
inline StepInterval<T>& StepInterval<T>::operator=( const Interval<T>& intv )
{ assign( *this, intv ); return *this; }


template <class T> inline
bool StepInterval<T>::isUdf() const
{
    return Interval<T>::isUdf() || mIsUdf(step);
}


template <class T> inline
cloneTp* StepInterval<T>::clone() const	
{ return new StepInterval<T>( *this ); }


template <class T> inline
void StepInterval<T>::set( const T& t1, const T& t2, const T& t3 )
{ Interval<T>::set( t1, t2 ); step = t3; }

template <class T> inline
bool StepInterval<T>::isEqual( const StepInterval<T>& i, const T& eps ) const
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
int StepInterval<T>::getIndex( const X& t ) const
{ return Interval<T>::getIndex( t, step ); }


template <class T> template <class X> inline
float StepInterval<T>::getfIndex( const X& t ) const
{ return Interval<T>::getfIndex( t, step ); }


template <class T> template <class X> inline
int StepInterval<T>::nearestIndex( const X& x ) const
{ return Interval<T>::nearestIndex( x, step ); }


template <class T> template <class X> inline
T StepInterval<T>::snap( const X& t ) const
{ return atIndex( nearestIndex( t ) ); }


template <class T> inline
void StepInterval<T>::sort( bool asc )
{
    Interval<T>::sort(asc);
    if ( (asc && step < 0) || (!asc && step > 0) )
	step = -step;
}

template <class T> inline
void StepInterval<T>::scale( const T& factor )
{
    Interval<T>::scale( factor );
    step *= factor;
}

template <class T> inline
T StepInterval<T>::snappedCenter() const
{ return snap( Interval<T>::center() ); }

template <class T> inline
T StepInterval<T>::snapStep( const T& inputstep ) const
{
    const float relstep = (float) inputstep/step;
    int nrsteps = mNINT32(relstep);
    if ( nrsteps<1 ) nrsteps = 1;
    return step*nrsteps;
}


template <class T> inline
float StepInterval<T>::nrfSteps() const
{
    const float w = Interval<T>::width( true );
    return w/step;
}


template <class T>
inline int StepInterval<T>::nrSteps() const
{
    if ( !step ) return 0;
    int ret = (((int)this->start) - this->stop) / step;
    return ret < 0 ? -ret : ret;
}

#define mDefFNrSteps(typ) \
template <> \
inline int StepInterval<typ>::nrSteps() const \
{ \
    if ( !step ) return 0; \
    typ ns = ( (start > stop ? start : stop) \
	    - (start > stop ? stop : start) ) \
	      / (step > 0 ? step : -step); \
    return mNINT32(ns); \
}

mDefFNrSteps(float)
mDefFNrSteps(double)

template <class T>
inline bool StepInterval<T>::isCompatible( const StepInterval<T>& b,
					   float ) const
{
    if ( step>b.step || b.step%step )
	return false;

    // const T diff = static_cast<const Interval<T>*>(this)->start - b.start;
    const T diff = this->start - b.start;
    return !(diff%step);
}


#define mDefFltisCompat(typ) \
template <> \
inline bool StepInterval<typ>::isCompatible( const StepInterval<typ>& b, \
			float eps ) const \
{ \
    if ( !mIsEqual(step,b.step,eps) ) return false; \
 \
    typ nrsteps = (start - b.start) / step; \
    int nrstepsi = mNINT32( nrsteps ); \
    typ diff = nrsteps - nrstepsi; \
    return ( (diff) < (eps) && (diff) > (-eps) ); \
}

mDefFltisCompat(float)
mDefFltisCompat(double)


#undef cloneTp

#endif
