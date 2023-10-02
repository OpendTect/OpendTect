#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "arrayndimpl.h"
#include "coord.h"
#include "enums.h"
#include "arrayndslice.h"
#include "mathfunc.h"
#include "periodicvalue.h"
#include "posinfo.h"
#include "scaler.h"
#include "trckeyzsampling.h"
#include "uistrings.h"


#define mComputeTrendAandB( sz ) { \
	aval = mCast(T,( (fT)sz * crosssum - sum * sumindexes ) / \
	       ( (fT)sz * sumsqidx - sumindexes * sumindexes ) );\
	bval = mCast(T,( sum * sumsqidx - sumindexes * crosssum ) / \
	       ( (fT)sz * sumsqidx - sumindexes * sumindexes ) ); }


/*!\brief [do not use, helper function] */

template <class T, class fT>
inline bool removeLinPart( const ArrayND<T>& in_, ArrayND<T>* out, bool trend )
{
    ArrayND<T>& out_ = out ? *out : const_cast<ArrayND<T>&>(in_);
    if ( out && in_.info() != out_.info() )
	return false;

    T avg = 0;
    T sum = 0;
    fT sumindexes = 0;
    fT sumsqidx = 0;
    T crosssum = 0;
    T aval = mUdf(T);
    T bval = mUdf(T);

    const od_int64 sz = in_.info().getTotalSz();

    const T* inpptr = in_.getData();
    T* outptr = out_.getData();

    if ( inpptr && outptr )
    {
	int count = 0;
	for ( int idx=0; idx<sz; idx++ )
	{
	    const T value = inpptr[idx];
	    if ( mIsUdf(value) )
		continue;

	    sum += inpptr[idx];
	    count++;
	    if ( !trend )
		continue;

	    const fT fidx = mCast(fT,idx);
	    sumindexes += fidx;
	    sumsqidx += fidx * fidx;
	    crosssum += inpptr[idx] * fidx;
	}

	if ( count <= 1 )
	    return false;

	if ( trend )
	    mComputeTrendAandB(count)
	else
	    avg = sum / (fT)count;

	for ( int idx=0; idx<sz; idx++ )
	{
	    const T value = inpptr[idx];
	    outptr[idx] = mIsUdf(value) ? mUdf(T)
					: (trend ? value - (aval*(fT)idx+bval)
					:  value - avg);
	}
    }
    else
    {
	ArrayNDIter iter( in_.info() );
	int index = 0, count = 0;

	do
	{
	    const T value = in_.getND( iter.getPos() );
	    index++;
	    if ( mIsUdf(value) )
		continue;

	    sum += value;
	    count++;
	    if ( !trend )
		continue;

	    sumindexes += index;
	    sumsqidx += index * index;
	    crosssum += value * (fT)index;
	} while ( iter.next() );

	iter.reset();
	if ( count <= 1 )
	    return false;

	if ( trend )
	    mComputeTrendAandB(count)
	else
	    avg = sum / (fT)count;

	index = 0;
	do
	{
	    const T inpval = in_.getND( iter.getPos() );
	    const T outval = mIsUdf(inpval) ? mUdf(T)
			   : (trend ? inpval - avg-(aval*(fT)index+bval)
				    : inpval - avg);
	    out_.setND(iter.getPos(), outval );
	    index++;
	} while ( iter.next() );
    }

    return true;
}


/*!\brief Removes the bias from an ArrayND. */

template <class T, class fT>
inline bool removeBias( ArrayND<T>& inout )
{
    return removeLinPart<T,fT>( inout, 0, false );
}

/*!\brief Fills an ArrayND with an unbiased version of another. */

template <class T, class fT>
inline bool removeBias( const ArrayND<T>& in, ArrayND<T>& out )
{
    return removeLinPart<T,fT>( in, &out, false );
}

/*!\brief Removes a linear trend from an ArrayND. */

template <class T, class fT>
inline bool removeTrend( ArrayND<T>& inout )
{
    return removeLinPart<T,fT>( inout, 0, true );
}

/*!\brief Fills an ArrayND with a de-trended version of another. */

template <class T, class fT>
inline bool removeTrend( const ArrayND<T>& in, ArrayND<T>& out )
{
    return removeLinPart<T,fT>( in, &out, true );
}


/*!\brief returns the average of all defined values in the Arrray1D.
   Returns UDF if empty or only udfs encountered. */

template <class T>
inline T getAverage( const ArrayND<T>& in )
{
    const int sz = in.info().getTotalSz();
    if ( sz < 1 )
	return mUdf(T);

    T sum = 0; int count = 0;
    for ( int idx=0; idx<sz; idx++ )
    {
	const T val = in.get( idx );
	if ( !mIsUdf(val) )
	    { sum += val; count++; }
    }

    if ( count == 0 )
	return mUdf(T);

    return sum / count;
}


/*!\brief returns the Maximum of all defined values in the ArrrayND.
   Returns UDF if empty or only udfs encountered. */

template <class T>
inline T getMax( const ArrayND<T>& in )
{
    const int sz = in.info().getTotalSz();
    const T* data = in.getData();
    if ( sz < 1 || !data )
	return mUdf(T);

    T maxval = -mUdf(T);
    for ( int idx=0; idx<sz; idx++ )
    {
	const T val = data[idx];
	if ( !mIsUdf(val) && val > maxval )
	    maxval = val;
    }

    return mIsUdf(-maxval) ? mUdf(T) : maxval;
}


/*!\brief returns the Minimum of all defined values in the ArrrayND.
   Returns UDF if empty or only udfs encountered. */

template <class T>
inline T getMin( const ArrayND<T>& in )
{
    const int sz = in.info().getTotalSz();
    const T* data = in.getData();
    if ( sz < 1 || !data )
	return mUdf(T);

    T minval = mUdf(T);
    for ( int idx=0; idx<sz; idx++ )
    {
	const T val = data[idx];
	if ( !mIsUdf(val) && val < minval )
	    minval = val;
    }

    return mIsUdf(minval) ? mUdf(T) : minval;
}


/*!\brief Returns whether there are undefs in the Array1D.  */

template <class fT>
inline bool hasUndefs( const Array1D<fT>& in )
{
    const int sz = in.info().getSize(0);
    for ( int idx=0; idx<sz; idx++ )
    {
	const fT val = in.get( idx );
	if ( mIsUdf(val) )
	    return true;
    }

    return false;
}


template <class fT>
inline bool hasUndefs( const ArrayND<fT>& in )
{
    const fT* vals = in.getData();
    typedef ArrayNDInfo::total_size_type total_size_type;
    const total_size_type sz = in.totalSize();
    if ( vals )
    {
	for ( total_size_type idx=0; idx<sz; idx++ )
	{
	    if ( mIsUdf(vals[idx]) )
		return true;
	}

	return false;
    }

    const ValueSeries<fT>* stor = in.getStorage();
    if ( stor )
    {
	for ( total_size_type idx=0; idx<sz; idx++ )
	{
	    if ( mIsUdf(stor->value(idx)) )
		return true;
	}

	return false;
    }

    ArrayNDIter iter( in.info() );
    do
    {
	if ( mIsUdf(in.getND(iter.getPos())) )
	    return true;

    } while( iter.next() );

    return false;
}

/*!
   The function interpUdf fills all the undefined values in a Array1D
   by using an inter- or extrapolation from the defined values.
   It uses the BendPointBasedMathFunction for this.
   Note that even if there is only one defined value, this function will fill
   the entire array by this value.

   Returns whether any substitution was made.
*/

template <class fT>
inline bool interpUdf( Array1D<fT>& in,
	typename BendPointBasedMathFunction<fT,fT>::InterpolType ipoltyp=
			BendPointBasedMathFunction<fT,fT>::Poly )
{
    if ( !hasUndefs(in) )
	return false;

    BendPointBasedMathFunction<fT,fT> data( ipoltyp );
    const int sz = in.info().getSize(0);
    for ( int idx=0; idx<sz; idx++ )
    {
	const fT val = in.get( idx );
	if ( !mIsUdf(val) )
	    data.add( mCast(fT,idx), val );
    }

    for ( int idx=0; idx<sz; idx++ )
    {
	const fT val = in.get( idx );
	if ( mIsUdf(val) )
	    in.set( idx, data.getValue( mCast(fT,idx) ) );
    }

    return true;
}


/*!
\brief Tapers the N-dimentional ArrayND with a windowFunction.

  Usage is straightforward- construct and use. If apply()'s second argument is
  omitted, the result will be placed in the input array. apply() will return
  false if input-, output- and window-size are not equal.
  The only requirement on the windowfunction is that it should give full taper
  at x=+-1 and no taper when x=0. Feel free to implement more functions!!
*/

mExpClass(Algo) ArrayNDWindow
{
public:
    enum WindowType	{ Box, Hamming, Hanning, Blackman, Bartlett,
			  CosTaper5, CosTaper10, CosTaper20 };
			mDeclareEnumUtils(WindowType);

			ArrayNDWindow(const ArrayNDInfo&,bool rectangular,
				      WindowType=Hamming);
			ArrayNDWindow(const ArrayNDInfo&,bool rectangular,
				      const char* winnm,
				      float paramval=mUdf(float));
			ArrayNDWindow(const ArrayNDInfo&,bool rectangular,
				      const char* winnm,
				      const TypeSet<float>& paramvals);
			~ArrayNDWindow();

    bool		isOK() const	{ return window_; }

    float		getParamVal(int dim=0) const { return paramval_[dim]; }
    void		setParamVal(int dim=0, float paramval=mUdf(float));
    void		setParamVals(const TypeSet<float>&);

    float*		getValues() const { return window_; }

    void		setValue(int idx,float val) { window_[idx]=val; }
    bool		setType(WindowType);
    bool		setType(const char*,float paramval=mUdf(float));
    bool		setType(const char*,const TypeSet<float>&);

    bool		resize(const ArrayNDInfo&);

    template <class Type> bool	apply(  ArrayND<Type>* in,
					ArrayND<Type>* out_=0 ) const
    {
	ArrayND<Type>* out = out_ ? out_ : in;

	if ( out_ && in->info() != out_->info() ) return false;
	if ( in->info() != size_ ) return false;

	const od_int64 totalsz = size_.getTotalSz();

	Type* indata = in->getData();
	Type* outdata = out->getData();
	if ( indata && outdata )
	{
	    for ( unsigned long idx = 0; idx<totalsz; idx++ )
	    {
		Type inval = indata[idx];
		outdata[idx] = mIsUdf( inval ) ? inval : inval * window_[idx];
	    }
	}
	else
	{
	    const ValueSeries<Type>* instorage = in->getStorage();
	    ValueSeries<Type>* outstorage = out->getStorage();

	    if ( instorage && outstorage )
	    {
		for ( unsigned long idx = 0; idx < totalsz; idx++ )
		{
		    Type inval = instorage->value(idx);
		    outstorage->setValue(idx,
			    mIsUdf( inval ) ? inval : inval * window_[idx] );
		}
	    }
	    else
	    {
		ArrayNDIter iter( size_ );
		int idx = 0;
		do
		{
		    Type inval = in->getND(iter.getPos());
		    out->setND( iter.getPos(),
			     mIsUdf( inval ) ? inval : inval * window_[idx] );
		    idx++;

		} while ( iter.next() );
	    }
	}

	return true;
    }

protected:

    float*			window_;
    ArrayNDInfoImpl		size_;
    bool			rectangular_;

    BufferString		windowtypename_;
    TypeSet<float>		paramval_;

    bool			buildWindow(const char* winnm,float pval);
    bool			buildWindow(const char* winnm);
};


template<class T>
inline T Array3DInterpolate( const Array3D<T>& array,
			     float p0, float p1, float p2,
			     bool posperiodic = false )
{
    const Array3DInfo& size = array.info();
    int intpos0 = mNINT32( p0 );
    float dist0 = p0 - intpos0;
    int prevpos0 = intpos0;
    if ( dist0 < 0 )
    {
	prevpos0--;
	dist0++;
    }
    if ( posperiodic ) prevpos0 = dePeriodize( prevpos0, size.getSize(0) );

    int intpos1 = mNINT32( p1 );
    float dist1 = p1 - intpos1;
    int prevpos1 = intpos1;
    if ( dist1 < 0 )
    {
	prevpos1--;
	dist1++;
    }
    if ( posperiodic ) prevpos1 = dePeriodize( prevpos1, size.getSize(1) );

    int intpos2 = mNINT32( p2 );
    float dist2 = p2 - intpos2;
    int prevpos2 = intpos2;
    if ( dist2 < 0 )
    {
	prevpos2--;
	dist2++;
    }

    if ( posperiodic ) prevpos2 = dePeriodize( prevpos2, size.getSize(2) );

    if ( !posperiodic && ( prevpos0 < 0 || prevpos0 > size.getSize(0) -2 ||
			 prevpos1 < 0 || prevpos1 > size.getSize(1) -2 ||
			 prevpos2 < 0 || prevpos2 > size.getSize(2) -2 ))
	return mUdf(T);

    if ( !posperiodic && ( !prevpos0 || prevpos0 > size.getSize(0) -3 ||
			 !prevpos1 || prevpos1 > size.getSize(1) -3 ||
			 !prevpos2 || prevpos2 > size.getSize(2) -3 ))
    {
	return linearInterpolate3D(
            array.get( prevpos0  , prevpos1  , prevpos2  ),
	    array.get( prevpos0  , prevpos1  , prevpos2+1),
	    array.get( prevpos0  , prevpos1+1, prevpos2  ),
            array.get( prevpos0  , prevpos1+1, prevpos2+1),
            array.get( prevpos0+1, prevpos1  , prevpos2  ),
            array.get( prevpos0+1, prevpos1  , prevpos2+1),
            array.get( prevpos0+1, prevpos1+1, prevpos2  ),
            array.get( prevpos0+1, prevpos1+1, prevpos2+1),
	    dist0, dist1, dist2 );
    }

    int firstpos0 = prevpos0 - 1;
    int nextpos0 = prevpos0 + 1;
    int lastpos0 = prevpos0 + 2;

    if ( posperiodic ) firstpos0 = dePeriodize( firstpos0, size.getSize(0) );
    if ( posperiodic ) nextpos0 = dePeriodize( nextpos0, size.getSize(0) );
    if ( posperiodic ) lastpos0 = dePeriodize( lastpos0, size.getSize(0) );

    int firstpos1 = prevpos1 - 1;
    int nextpos1 = prevpos1 + 1;
    int lastpos1 = prevpos1 + 2;

    if ( posperiodic ) firstpos1 = dePeriodize( firstpos1, size.getSize(1) );
    if ( posperiodic ) nextpos1 = dePeriodize( nextpos1, size.getSize(1) );
    if ( posperiodic ) lastpos1 = dePeriodize( lastpos1, size.getSize(1) );

    int firstpos2 = prevpos2 - 1;
    int nextpos2 = prevpos2 + 1;
    int lastpos2 = prevpos2 + 2;

    if ( posperiodic ) firstpos2 = dePeriodize( firstpos2, size.getSize(2) );
    if ( posperiodic ) nextpos2 = dePeriodize( nextpos2, size.getSize(2) );
    if ( posperiodic ) lastpos2 = dePeriodize( lastpos2, size.getSize(2) );

    return polyInterpolate3D (
            array.get( firstpos0  , firstpos1  , firstpos2 ),
            array.get( firstpos0  , firstpos1  , prevpos2  ),
            array.get( firstpos0  , firstpos1  , nextpos2  ),
            array.get( firstpos0  , firstpos1  , lastpos2  ),

            array.get( firstpos0  , prevpos1   , firstpos2 ),
            array.get( firstpos0  , prevpos1   , prevpos2  ),
            array.get( firstpos0  , prevpos1   , nextpos2  ),
            array.get( firstpos0  , prevpos1   , lastpos2  ),

            array.get( firstpos0  , nextpos1   , firstpos2 ),
            array.get( firstpos0  , nextpos1   , prevpos2  ),
            array.get( firstpos0  , nextpos1   , nextpos2  ),
            array.get( firstpos0  , nextpos1   , lastpos2  ),

            array.get( firstpos0  , lastpos1   , firstpos2 ),
            array.get( firstpos0  , lastpos1   , prevpos2  ),
            array.get( firstpos0  , lastpos1   , nextpos2  ),
            array.get( firstpos0  , lastpos1   , lastpos2  ),


            array.get( prevpos0  , firstpos1  , firstpos2 ),
            array.get( prevpos0  , firstpos1  , prevpos2  ),
            array.get( prevpos0  , firstpos1  , nextpos2  ),
            array.get( prevpos0  , firstpos1  , lastpos2  ),

            array.get( prevpos0  , prevpos1   , firstpos2 ),
            array.get( prevpos0  , prevpos1   , prevpos2  ),
            array.get( prevpos0  , prevpos1   , nextpos2  ),
            array.get( prevpos0  , prevpos1   , lastpos2  ),

            array.get( prevpos0  , nextpos1   , firstpos2 ),
            array.get( prevpos0  , nextpos1   , prevpos2  ),
            array.get( prevpos0  , nextpos1   , nextpos2  ),
            array.get( prevpos0  , nextpos1   , lastpos2  ),

            array.get( prevpos0  , lastpos1   , firstpos2 ),
            array.get( prevpos0  , lastpos1   , prevpos2  ),
            array.get( prevpos0  , lastpos1   , nextpos2  ),
            array.get( prevpos0  , lastpos1   , lastpos2  ),


            array.get( nextpos0  , firstpos1  , firstpos2 ),
            array.get( nextpos0  , firstpos1  , prevpos2  ),
            array.get( nextpos0  , firstpos1  , nextpos2  ),
            array.get( nextpos0  , firstpos1  , lastpos2  ),

            array.get( nextpos0  , prevpos1   , firstpos2 ),
            array.get( nextpos0  , prevpos1   , prevpos2  ),
            array.get( nextpos0  , prevpos1   , nextpos2  ),
            array.get( nextpos0  , prevpos1   , lastpos2  ),

            array.get( nextpos0  , nextpos1   , firstpos2 ),
            array.get( nextpos0  , nextpos1   , prevpos2  ),
            array.get( nextpos0  , nextpos1   , nextpos2  ),
            array.get( nextpos0  , nextpos1   , lastpos2  ),

            array.get( nextpos0  , lastpos1   , firstpos2 ),
            array.get( nextpos0  , lastpos1   , prevpos2  ),
            array.get( nextpos0  , lastpos1   , nextpos2  ),
            array.get( nextpos0  , lastpos1   , lastpos2  ),


            array.get( lastpos0  , firstpos1  , firstpos2 ),
            array.get( lastpos0  , firstpos1  , prevpos2  ),
            array.get( lastpos0  , firstpos1  , nextpos2  ),
            array.get( lastpos0  , firstpos1  , lastpos2  ),

            array.get( lastpos0  , prevpos1   , firstpos2 ),
            array.get( lastpos0  , prevpos1   , prevpos2  ),
            array.get( lastpos0  , prevpos1   , nextpos2  ),
            array.get( lastpos0  , prevpos1   , lastpos2  ),

            array.get( lastpos0  , nextpos1   , firstpos2 ),
            array.get( lastpos0  , nextpos1   , prevpos2  ),
            array.get( lastpos0  , nextpos1   , nextpos2  ),
            array.get( lastpos0  , nextpos1   , lastpos2  ),

            array.get( lastpos0  , lastpos1   , firstpos2 ),
            array.get( lastpos0  , lastpos1   , prevpos2  ),
            array.get( lastpos0  , lastpos1   , nextpos2  ),
            array.get( lastpos0  , lastpos1   , lastpos2  ),
	    dist0, dist1, dist2 );
}


template <class T>
inline bool ArrayNDCopy( ArrayND<T>& dest, const ArrayND<T>& src,
			 const TypeSet<int>& copypos, bool srcperiodic=false )
{
    const ArrayNDInfo& destsz = dest.info();
    const ArrayNDInfo& srcsz = src.info();

    const int ndim = destsz.getNDim();
    if ( ndim != srcsz.getNDim() || ndim != copypos.size() ) return false;

    for ( int idx=0; idx<ndim; idx++ )
    {
	if ( !srcperiodic &&
	     copypos[idx] + destsz.getSize(idx) > srcsz.getSize(idx) )
	    return false;
    }

    ArrayNDIter destposition( destsz );
    TypeSet<int> srcposition( ndim, 0 );

    do
    {
	for ( int idx=0; idx<ndim; idx++ )
	{
	    srcposition[idx] = copypos[idx] + destposition[idx];
	    if ( srcperiodic )
		srcposition[idx] =
		    dePeriodize( srcposition[idx], srcsz.getSize(idx) );
	}

	dest.setND( destposition.getPos(), src.get( &srcposition[0] ));


    } while ( destposition.next() );

    return true;
}


template <class T>
inline bool Array3DCopy( Array3D<T>& dest, const Array3D<T>& src,
			 int p0, int p1, int p2, bool srcperiodic=false )
{
    const ArrayNDInfo& destsz = dest.info();
    const ArrayNDInfo& srcsz = src.info();

    const int destsz0 = destsz.getSize(0);
    const int destsz1 = destsz.getSize(1);
    const int destsz2 = destsz.getSize(2);

    const int srcsz0 = srcsz.getSize(0);
    const int srcsz1 = srcsz.getSize(1);
    const int srcsz2 = srcsz.getSize(2);

    if ( !srcperiodic )
    {
	if ( p0 + destsz0 > srcsz0 ||
	     p1 + destsz1 > srcsz1 ||
	     p2 + destsz2 > srcsz2  )
	     return false;
    }

    int idx = 0;
    T* ptr = dest.getData();

    for ( int id0=0; id0<destsz0; id0++ )
    {
	for ( int id1=0; id1<destsz1; id1++ )
	{
	    for ( int id2=0; id2<destsz2; id2++ )
	    {
		ptr[idx++] = src.get( dePeriodize(id0 + p0, srcsz0),
					 dePeriodize(id1 + p1, srcsz1),
					 dePeriodize(id2 + p2, srcsz2));

	    }
	}
    }

    return true;
}


template <class T>
inline bool ArrayNDPaste( ArrayND<T>& dest, const ArrayND<T>& src,
			  const TypeSet<int>& pastepos,
			  bool destperiodic=false )
{
    const ArrayNDInfo& destsz = dest.info();
    const ArrayNDInfo& srcsz = src.info();

    const int ndim = destsz.getNDim();
    if ( ndim != srcsz.getNDim() || ndim != pastepos.size() ) return false;

    for ( int idx=0; idx<ndim; idx++ )
    {
	if ( !destperiodic &&
	     pastepos[idx] + srcsz.getSize(idx) > destsz.getSize(idx) )
	    return false;
    }

    ArrayNDIter srcposition( srcsz );
    TypeSet<int> destposition( ndim, 0 );

    int ptrpos = 0;
    T* ptr = src.getData();

    do
    {
	for ( int idx=0; idx<ndim; idx++ )
	{
	    destposition[idx] = pastepos[idx] + srcposition[idx];
	    if ( destperiodic )
		destposition[idx] =
		    dePeriodize( destposition[idx], destsz.getSize(idx) );
	}

	dest( destposition ) =  ptr[ptrpos++];

    } while ( srcposition.next() );

    return true;
}


template <class T>
inline bool Array2DPaste( Array2D<T>& dest, const Array2D<T>& src,
			  int p0, int p1, bool destperiodic=false )
{
    const ArrayNDInfo& destsz = dest.info();
    const ArrayNDInfo& srcsz = src.info();

    const int srcsz0 = srcsz.getSize(0);
    const int srcsz1 = srcsz.getSize(1);

    const int destsz0 = destsz.getSize(0);
    const int destsz1 = destsz.getSize(1);

    if ( !destperiodic )
    {
	if ( p0 + srcsz0 > destsz0 ||
	     p1 + srcsz1 > destsz1  )
	     return false;
    }


    int idx = 0;
    const T* ptr = src.getData();

    for ( int id0=0; id0<srcsz0; id0++ )
    {
	for ( int id1=0; id1<srcsz1; id1++ )
	{
	    dest.set( dePeriodize( id0 + p0, destsz0),
		  dePeriodize( id1 + p1, destsz1),
		  ptr[idx++]);
	}
    }

    return true;
}


template <class T>
inline bool Array3DPaste( Array3D<T>& dest, const Array3D<T>& src,
			  int p0, int p1, int p2,
			  bool destperiodic=false )
{
    const ArrayNDInfo& destsz = dest.info();
    const ArrayNDInfo& srcsz = src.info();

    const int srcsz0 = srcsz.getSize(0);
    const int srcsz1 = srcsz.getSize(1);
    const int srcsz2 = srcsz.getSize(2);

    const int destsz0 = destsz.getSize(0);
    const int destsz1 = destsz.getSize(1);
    const int destsz2 = destsz.getSize(2);

    if ( !destperiodic )
    {
	if ( p0 + srcsz0 > destsz0 ||
	     p1 + srcsz1 > destsz1 ||
	     p2 + srcsz2 > destsz2  )
	     return false;
    }


    int idx = 0;
    const T* ptr = src.getData();

    for ( int id0=0; id0<srcsz0; id0++ )
    {
	for ( int id1=0; id1<srcsz1; id1++ )
	{
	    for ( int id2=0; id2<srcsz2; id2++ )
	    {
		dest.set( dePeriodize( id0 + p0, destsz0),
		      dePeriodize( id1 + p1, destsz1),
		      dePeriodize( id2 + p2, destsz2), ptr[idx++]);
	    }
	}
    }

    return true;
}


/*!\brief Transfers the common samples from one 2D array to another */

template <class T>
mClass(Algo) Array2DCopier : public ParallelTask
{ mODTextTranslationClass(Array2DCopier);
public:
		Array2DCopier( const Array2D<T>& in,
			       const TrcKeySampling& tksin,
			       const TrcKeySampling& tksout,
			       Array2D<T>& out )
		    : in_(in)
		    , tksin_(tksin)
		    , tksout_(tksout)
		    , out_(out)
		{
		    if ( canCopyAll() )
		    {
			doPrepare(0);
			return;
		    }

		    tksin.getInterSection( tksout, commontks_ );
		}

    uiString	uiNrDoneText() const override
		{
		    return uiStrings::phrJoinStrings(
						uiStrings::sInline(mPlural),
						uiStrings::sDone().toLower() );
		}
    uiString	uiMessage() const override
		{ return tr("Transferring grid data");}

protected:

    od_int64	nrIterations() const override
		{
		    return canCopyAll() ? 0 : commontks_.nrLines();
		}

private:

    bool	canCopyAll() const
		{
		    return tksout_ == tksin_ && in_.getData() &&
			   ( out_.getData() || out_.getStorage() );
		}

    bool	doPrepare( int ) override
		{
		    if ( in_.info().getSize(0) != tksin_.nrLines() ||
			 in_.info().getSize(1) != tksin_.nrTrcs() )
		    {
			return false;
		    }

		    if ( out_.info().getSize(0) != tksout_.nrLines() ||
			 out_.info().getSize(1) != tksout_.nrTrcs() )
		    {
			mDynamicCastGet(Array2DImpl<T>*,outimpl,&out_)
			if ( !outimpl || !outimpl->setSize( tksout_.nrLines(),
							    tksout_.nrTrcs() ) )
			{
			    return false;
			}

			out_.setAll( mUdf(T) );
		    }

		    if ( canCopyAll() )
		    {
			if ( out_.getData() )
			    in_.getAll( out_.getData() );
			else if ( out_.getStorage() )
			    in_.getAll( *out_.getStorage() );
		    }

		    return true;
		}

    bool	doWork( od_int64 start, od_int64 stop, int ) override
		{
		    const TrcKeySampling tksin( tksin_ );
		    const TrcKeySampling tksout( tksout_ );
		    const TrcKeySampling tks( commontks_ );

		    const bool usearrayptrs = in_.getData() && out_.getData() &&
					      in_.getStorage() &&
					      out_.getStorage();
		    OffsetValueSeries<T>* invals = !usearrayptrs ? 0 :
			    new OffsetValueSeries<T>( *in_.getStorage(), 0,
						      in_.getSize(2) );
		    OffsetValueSeries<T>* outvals = !usearrayptrs ? 0 :
			    new OffsetValueSeries<T>( *out_.getStorage(), 0,
						      in_.getSize(2) );
		    const int nrcrl = tks.nrTrcs();
		    const od_int64 nrbytes = nrcrl * sizeof(T);

		    const int startcrl = tks.start_.crl();
		    const int startcrlidyin = tksin.trcIdx( startcrl );
		    const int startcrlidyout = tksout.trcIdx( startcrl );
		    for ( int idx=mCast(int,start); idx<=mCast(int,stop); idx++)
		    {
			const int inl = tks.lineRange().atIndex( idx );
			const int inlidxin = tksin.lineIdx( inl );
			const int inlidxout = tksout.lineIdx( inl );
			if ( usearrayptrs )
			{
			    invals->setOffset(
			      in_.info().getOffset(inlidxin,startcrlidyin) );
			    outvals->setOffset(
			      out_.info().getOffset(inlidxout,startcrlidyout) );
			    OD::sysMemCopy(outvals->arr(),invals->arr(),
					   nrbytes);
			    continue;
			}
			else
			{
			    for ( int idy=0; idy<nrcrl; idy++ )
			    {
				const T val =
					in_.get( inlidxin, startcrlidyin+idy );
				out_.set( inlidxout, startcrlidyout+idy, val );
			    }
			}
		    }

		    delete invals;
		    delete outvals;

		    return true;
		}

    const Array2D<T>&	in_;
    const TrcKeySampling&	tksin_;
    const TrcKeySampling&	tksout_;
    TrcKeySampling	commontks_;
    Array2D<T>&		out_;
};


/*!\brief Transfers the common samples from one 3D array to another */

template <class T>
mClass(Algo) Array3DCopier : public ParallelTask
{ mODTextTranslationClass(Array3DCopier)
public:
		Array3DCopier( const Array3D<T>& in, Array3D<T>& out,
			       const TrcKeyZSampling& tkzsin,
			       const TrcKeyZSampling& tkzsout )
		    : ParallelTask("Array 3D Resizer")
		    , tkzsin_(tkzsin)
		    , tkzsout_(tkzsout)
		    , totalnr_(tkzsout.hsamp_.totalNr())
		    , in_(in)
		    , out_(out)
		{}

    uiString	uiMessage() const override
		{ return tr("Resizing 3D Array"); }

    uiString	uiNrDoneText() const override
		{ return ParallelTask::sTrcFinished(); }

protected:

    od_int64	nrIterations() const override	{ return totalnr_; }

private:

#define mGetInfo() \
    const Array3DInfoImpl infoin( tkzsin_.hsamp_.nrLines(), \
				  tkzsin_.hsamp_.nrTrcs(), tkzsin_.nrZ() ); \
    const Array3DInfoImpl infoout( tkzsout_.hsamp_.nrLines(), \
				   tkzsout_.hsamp_.nrTrcs(), tkzsout_.nrZ() );

    bool	doPrepare( int ) override
		{
		    mGetInfo()
		    if ( in_.info() != infoin )
			return false;

		    if ( out_.info() != infoout && !out_.setInfo(infoout) )
			return false;

		    if ( tkzsin_.hsamp_.survid_ != tkzsout_.hsamp_.survid_ )
			return false;

		    if ( !tkzsin_.zsamp_.isCompatible(tkzsout_.zsamp_) )
			return false; //Not supported

		    out_.setAll( mUdf(T) );

		    return true;
		}


    bool	doWork( od_int64 start, od_int64 stop, int ) override
		{
		    mGetInfo()
		    const TrcKeySampling tksin( tkzsin_.hsamp_ );
		    const TrcKeySampling tksout( tkzsout_.hsamp_ );
		    const int nrzout = infoout.getSize(2);
		    StepInterval<float> zrg( tkzsout_.zsamp_ );
		    zrg.limitTo( tkzsin_.zsamp_ );
		    const int nrztocopy = zrg.nrSteps() + 1;
		    const int z0in = tkzsin_.zsamp_.nearestIndex( zrg.start );
		    const int z0out = tkzsout_.zsamp_.nearestIndex( zrg.start );
		    const od_int64 nrbytes =
				   mCast(od_int64,nrztocopy) * sizeof(T);
		    const T* inptr = in_.getData();
		    T* outptr = out_.getData();
		    const ValueSeries<T>* instor = in_.getStorage();
		    ValueSeries<T>* outstor = out_.getStorage();
		    const bool hasarrayptr = inptr && outptr;
		    const bool hasstorage = instor && outstor;
		    const bool needgetset = !hasarrayptr && !hasstorage;

		    const Array2DInfoImpl info2d( infoout.getSize( 0 ),
						  infoout.getSize( 1 ) );
		    ArrayNDIter iter( info2d );
		    iter.setGlobalPos( start );

		    const od_int64 offsetout = start * nrzout + z0out;
		    outptr += offsetout;
		    od_uint64 validxout = offsetout;

		    for ( od_int64 idx=start; idx<=stop; idx++, iter.next(),
			  outptr+=nrzout, validxout+=nrzout,
			  quickAddToNrDone(idx) )
		    {
			const int inlidx = iter[0];
			const int crlidx = iter[1];
			const BinID bid( tksout.atIndex(inlidx,crlidx) );
			if ( !tksin.includes(bid) )
			    continue;

			const int inlidxin = tksin.lineIdx( bid.lineNr() );
			const int crlidxin = tksin.trcIdx( bid.trcNr() );
			const od_int64 offsetin = needgetset ? 0
				: infoin.getOffset( inlidxin, crlidxin, z0in );
			if ( hasarrayptr )
			{
			    OD::sysMemCopy( outptr, inptr+offsetin, nrbytes );
			}
			else if ( hasstorage )
			{
			    for ( int idz=0; idz<nrztocopy; idz++ )
			    {
				outstor->setValue( validxout+idz,
						   instor->value(offsetin+idz));
			    }
			}
			else
			{
			    for ( int idz=0, idzin=z0in; idz<nrztocopy; idz++,
									idzin++)
			    {
				const T val =
					in_.get( inlidxin, crlidxin, idzin );
				out_.set( inlidx, crlidx, idz, val );
			    }
			}
		    }

		    return true;
		}

    const TrcKeyZSampling&	tkzsin_;
    const TrcKeyZSampling&	tkzsout_;
    const od_int64		totalnr_;

    const Array3D<T>&		in_;
    Array3D<T>&			out_;
};


/*!\brief Scales the common samples from one 3D array and outputs to another */

template <class T>
mClass(Algo) Array3DScaler : public ParallelTask
{ mODTextTranslationClass(Array3DScaler)
public:
		Array3DScaler( const Array3D<T>& in, Array3D<T>& out,
			       const TrcKeyZSampling& tkzsin,
			       const TrcKeyZSampling& tkzsout,
			       const Scaler& scaler )
		    : ParallelTask("Array 3D Scaler")
		    , tkzsin_(tkzsin)
		    , tkzsout_(tkzsout)
		    , totalnr_(tkzsout.hsamp_.totalNr())
		    , in_(in)
		    , out_(out)
		    , scaler_(*scaler.clone())
		{}

		~Array3DScaler()
		{
		    delete &scaler_;
		}

    uiString	uiMessage() const override
		{ return tr("Scaling 3D Array"); }

    uiString	uiNrDoneText() const override
		{ return ParallelTask::sTrcFinished(); }

protected:

    od_int64	nrIterations() const override	{ return totalnr_; }

private:

#define mGetInfo() \
    const Array3DInfoImpl infoin( tkzsin_.hsamp_.nrLines(), \
				  tkzsin_.hsamp_.nrTrcs(), tkzsin_.nrZ() ); \
    const Array3DInfoImpl infoout( tkzsout_.hsamp_.nrLines(), \
				   tkzsout_.hsamp_.nrTrcs(), tkzsout_.nrZ() );

    bool	doPrepare( int ) override
		{
		    mGetInfo()
		    if ( in_.info() != infoin )
			return false;

		    if ( out_.info() != infoout && !out_.setInfo(infoout) )
			return false;

		    if ( tkzsin_.hsamp_.survid_ != tkzsout_.hsamp_.survid_ )
			return false;

		    if ( !tkzsin_.zsamp_.isCompatible(tkzsout_.zsamp_) )
			return false; //Not supported

		    out_.setAll( mUdf(T) );

		    return true;
		}


    bool	doWork( od_int64 start, od_int64 stop, int ) override
		{
		    mGetInfo()
		    const TrcKeySampling tksin( tkzsin_.hsamp_ );
		    const TrcKeySampling tksout( tkzsout_.hsamp_ );
		    const int nrzout = infoout.getSize(2);
		    StepInterval<float> zrg( tkzsout_.zsamp_ );
		    zrg.limitTo( tkzsin_.zsamp_ );
		    const int nrztocopy = zrg.nrSteps() + 1;
		    const int z0in = tkzsin_.zsamp_.nearestIndex( zrg.start );
		    const int z0out = tkzsout_.zsamp_.nearestIndex( zrg.start );
		    const T* inptr = in_.getData();
		    T* outptr = out_.getData();
		    const ValueSeries<T>* instor = in_.getStorage();
		    ValueSeries<T>* outstor = out_.getStorage();
		    const bool hasarrayptr = inptr && outptr;
		    const bool hasstorage = instor && outstor;
		    const bool needgetset = !hasarrayptr && !hasstorage;

		    const od_int64 offsetout = start * nrzout + z0out;
		    outptr += offsetout;
		    od_uint64 validxout = offsetout;

		    for ( od_int64 idx=start; idx<=stop; idx++, outptr+=nrzout,
				validxout+=nrzout, quickAddToNrDone(idx) )
		    {
			const int inlidx =
				tkzsout_.hsamp_.lineIdxFromGlobal( idx );
			const int crlidx =
				tkzsout_.hsamp_.trcIdxFromGlobal( idx );
			const BinID bid( tksout.atIndex(inlidx,crlidx) );
			if ( !tksin.includes(bid) )
			    continue;

			const int inlidxin = tksin.lineIdx( bid.lineNr() );
			const int crlidxin = tksin.trcIdx( bid.trcNr() );
			const od_int64 offsetin = needgetset ? 0
				: infoin.getOffset( inlidxin, crlidxin, z0in );
			if ( hasarrayptr )
			{
			    for ( int idz=0; idz<nrztocopy; idz++ )
				outptr[idz] = sCast( float,
					scaler_.scale( inptr[offsetin+idz]) );
			}
			else if ( hasstorage )
			{
			    for ( int idz=0; idz<nrztocopy; idz++ )
				outstor->setValue( validxout+idz,
					sCast(float,scaler_.scale(
						instor->value(offsetin+idz))) );
			}
			else
			{
			    for ( int idz=0, idzin=z0in; idz<nrztocopy; idz++,
									idzin++)
			    {
				const T val =
					in_.get( inlidxin, crlidxin, idzin );
				out_.set( inlidx, crlidx, idz,
					  sCast(float,scaler_.scale(val)) );
			    }
			}
		    }

		    return true;
		}

    const TrcKeyZSampling&	tkzsin_;
    const TrcKeyZSampling&	tkzsout_;
    Scaler&			scaler_;
    const od_int64		totalnr_;

    const Array3D<T>&		in_;
    Array3D<T>&			out_;
};


/*!
  \brief Polynomial trend with order 0 (mean), 1 (linear) or 2 (parabolic)
  The trend is derived from a set of values with positions
  and can be applied thereafter on any other position
*/

mExpClass(Algo) PolyTrend
{ mODTextTranslationClass(PolyTrend);
public:
				PolyTrend();

    bool			operator==(const PolyTrend&) const;

				enum Order	{ None, Order0, Order1, Order2};
				mDeclareEnumUtils(Order)

    static const char*		sKeyOrder()	{ return "Polynomial Order"; }
    static bool			getOrder(int nrpoints,Order&,uiString* =0);

    void			setOrder( PolyTrend::Order t )	{ order_ = t; }
    template <class IDXABLE> bool	set(const TypeSet<Coord>&,
					    const IDXABLE& valuelistj);
				/*!< Use after the order is set!
				     Sets the trend from a list of values
				     tied to a list of coordinates */

    Order			getOrder() const	{ return order_; }

    template <class T> void	apply(const Coord& pos,bool dir,T& val) const;
				/*!<Applies the trend to a value tied to
				    a position
				    \param pos
				    \param dir true for detrend,
						false for restore
				    \param val result after apply
				 */

protected:

    Order			order_;
    double			f0_;
    double			f1_;
    double			f2_;
    double			f11_;
    double			f12_;
    double			f22_;
    Coord			posc_;

    void			initOrder0(const TypeSet<double>&);
    void			initOrder1(const TypeSet<Coord>&,
					   const TypeSet<double>&);
    void			initOrder2(const TypeSet<Coord>&,
					   const TypeSet<double>&);
    void			initCenter(const TypeSet<Coord>&);

};



template <class IDXABLE> inline
bool PolyTrend::set( const TypeSet<Coord>& poslist, const IDXABLE& vals )
{
    int sz = poslist.size();
    if ( order_ == PolyTrend::None )
	return false;

    f0_ = f1_ = f2_ = f11_ = f12_ = f22_ = posc_.x = posc_.y = 0.;
    TypeSet<Coord> posnoudf;
    TypeSet<double> valsnoudf;
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( !poslist[idx].isDefined() || mIsUdf(vals[idx]) )
	    continue;

	posnoudf += poslist[idx];
	valsnoudf += (double) vals[idx];
    }

    sz = valsnoudf.size();
    getOrder( sz, order_ );
    if ( order_ == Order0 )
	initOrder0( valsnoudf );
    else if ( order_ == Order1 )
	initOrder1( posnoudf, valsnoudf );
    else if ( order_ == Order2 )
	initOrder2( posnoudf, valsnoudf );
    else
	return false;

    return true;
}


template <class T> inline
void PolyTrend::apply( const Coord& pos, bool dir, T& val ) const
{
    if ( order_ == None || !pos.isDefined() || mIsUdf(val) )
	return;

    const double fact = dir ? -1. : 1;
    double inp = (double) val;
    inp += fact * f0_;
    if ( order_ == Order0 )
    {
	val = (T)inp;
	return;
    }

    const double dx = pos.x - posc_.x;
    const double dy = pos.y - posc_.y;
    inp += fact * ( f1_ * dx + f2_ * dy );
    if ( order_ == Order1 )
    {
	val = (T)inp;
	return;
    }

    const double dx2 = dx * dx;
    const double dxy = dx * dy;
    const double dyy = dy * dy;
    inp += fact * ( f11_ * dx2 + f12_ * dxy + f22_ * dyy );
    val = (T)inp;
}



namespace ArrayMath
{

mClass(Algo) ArrayOperExecSetup
{
public:
		ArrayOperExecSetup(bool doadd=true,bool dosqinp=false,
				   bool dosqout=false,bool doabs=false,
				   bool donormalizesum=false,
				   bool dosqrtsum=false)
		    : doadd_(doadd)
		    , dosqinp_(dosqinp)
		    , dosqout_(dosqout)
		    , doabs_(doabs)
		    , donormalizesum_(donormalizesum)
		    , dosqrtsum_(dosqrtsum)
		{}

    bool	doadd_;
    bool	dosqinp_;
    bool	dosqout_;
    bool	doabs_;
    bool	donormalizesum_;
    bool	dosqrtsum_;
};


/*!\brief Parallel task for computing the sum of element wise operations of
	  one array and optionally a second input array.
	  Should not be used directly, instead call getSum(const ArrayND)
 */

template <class RT,class AT>
class CumArrOperExec : public ParallelTask
{ mODTextTranslationClass(CumArrOperExec);
public:
		CumArrOperExec( const ArrayND<AT>& xvals, bool noudf,
				const ArrayOperExecSetup& setup )
		    : xarr_(xvals)
		    , yarr_(0)
		    , sz_(xvals.info().getTotalSz())
		    , xfact_(mUdf(double))
		    , yfact_(mUdf(double))
		    , noudf_(noudf)
		    , cumsum_(mUdf(RT))
		    , setup_(setup)
		{}

    uiString	uiNrDoneText() const override
		{ return ParallelTask::sPosFinished(); }
    uiString	uiMessage() const override
		{ return tr("Cumulative sum executor");}

    void	setYVals( const ArrayND<AT>& yvals )	{ yarr_ = &yvals; }
    void	setScaler( double scaler, bool forx )
		{
		   if ( forx )
		      xfact_ = scaler;
		   else
		      yfact_ = scaler;
		}

    RT		getSum() const		{ return cumsum_; }

protected:

    od_int64	nrIterations() const override	{ return sz_; }

private:

    bool	doPrepare( int nrthreads ) override
		{
		    if ( yarr_ && yarr_->info().getTotalSz() != sz_ )
			return false;

		    return sumvals_.setSize( nrthreads );
		}

    bool	doWork(od_int64,od_int64,int) override;

    bool	doFinish( bool success ) override
		{
		    if ( !success )
			return false;

		    int count = 0;
		    cumsum_ = 0;
		    for ( int idx=0; idx<sumvals_.size(); idx++ )
		    {
			if ( mIsUdf(sumvals_[idx]) )
			    continue;

			cumsum_ += sumvals_[idx];
			count++;
		    }

		    if ( count == 0 )
			cumsum_ = mUdf(RT);

		    if ( setup_.donormalizesum_ )
			cumsum_ /= mCast(RT,xarr_.info().getTotalSz());

		    if ( setup_.dosqrtsum_ )
			cumsum_ = Math::Sqrt( cumsum_ );

		    return true;
		}

private:

    const ArrayOperExecSetup&	setup_;
    od_uint64		sz_;
    bool		noudf_;

    TypeSet<RT>		sumvals_;
    const ArrayND<AT>&	xarr_;
    const ArrayND<AT>*	yarr_;
    double		xfact_;
    double		yfact_;
    RT			cumsum_;
};


template <class RT,class AT> inline
bool CumArrOperExec<RT,AT>::doWork( od_int64 start, od_int64 stop,
				    int threadidx )
{
    RT sumval = 0, comp = 0;
    od_uint64 count = 0;
    const AT* xvals = xarr_.getData();
    const AT* yvals = yarr_ ? yarr_->getData() : 0;
    const ValueSeries<AT>* xstor = xarr_.getStorage();
    const ValueSeries<AT>* ystor = yarr_ ? yarr_->getStorage() : 0;
    ArrayNDIter* xiter = xvals || xstor ? 0 : new ArrayNDIter( xarr_.info() );
    ArrayNDIter* yiter = ( yarr_ && ( yvals || ystor ) ) || !yarr_
		       ? 0 : new ArrayNDIter( yarr_->info() );
    if ( xiter ) xiter->setGlobalPos( start );
    if ( yiter ) yiter->setGlobalPos( start );
    const bool doscalexvals = !mIsUdf(xfact_);
    const bool hasyvals = yarr_;
    const bool doscaleyvals = !mIsUdf(yfact_);
    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	RT xvalue = xvals ? xvals[idx]
			  : xstor ? xstor->value(idx)
				  : xarr_.getND( xiter->getPos() );
	if ( !noudf_ && mIsUdf(xvalue) )
	{
	    if ( xiter ) xiter->next();
	    if ( yiter ) yiter->next();
	    continue;
	}

	if ( setup_.dosqinp_ ) xvalue *= xvalue;
	if ( doscalexvals ) xvalue *= xfact_;
	if ( hasyvals )
	{
	    RT yvalue = yvals ? yvals[idx]
			      : ystor ? ystor->value(idx)
				      : yarr_->getND( yiter->getPos() );
	    if ( !noudf_ && mIsUdf(yvalue) )
	    {
		if ( xiter ) xiter->next();
		if ( yiter ) yiter->next();
		continue;
	    }

	    if ( setup_.dosqinp_ ) yvalue *= yvalue;
	    if ( doscaleyvals ) yvalue *= yfact_;
	    if ( setup_.doadd_ )
		xvalue += yvalue;
	    else
		xvalue *= yvalue;
	}

	if ( setup_.doabs_ )
	    xvalue = Math::Abs( xvalue );
	else if ( setup_.dosqout_ )
	    xvalue *= xvalue;

	xvalue -= comp;
	const RT t = sumval + xvalue;
	comp = ( t - sumval ) - xvalue;
	sumval = t;
	count++;
	if ( xiter ) xiter->next();
	if ( yiter ) yiter->next();
    }

    delete xiter; delete yiter;
    sumvals_[threadidx] = count==0 ? mUdf(RT) : sumval;

    return true;
}


template <> inline
bool CumArrOperExec<double,float_complex>::doWork( od_int64, od_int64, int )
{ return false; }


#define mSetResAndContinue(res) \
{ \
    if ( outvals ) \
	outvals[idx] = res; \
    else if ( outstor ) \
	outstor->setValue( idx, res );\
    else \
	outarr_.setND( outiter->getPos(), res ); \
    \
    if ( xiter ) xiter->next(); \
    if ( yiter ) yiter->next(); \
    if ( outiter ) outiter->next(); \
}

/*!\brief Parallel task for computing the element wise operations of
	  one array and optionally a second input array.
	  Should not be used directly, instead call getSum(const ArrayND)
 */

template <class OperType,class ArrType>
class ArrOperExec : public ParallelTask
{ mODTextTranslationClass(ArrOperExec);
public:
		ArrOperExec( const ArrayND<ArrType>& xvals,
			     const ArrayND<ArrType>* yvals, bool noudf,
			     const ArrayOperExecSetup& setup,
			     ArrayND<ArrType>& outvals )
		    : xarr_(xvals)
		    , yarr_(yvals)
		    , outarr_(outvals)
		    , sz_(xvals.info().getTotalSz())
		    , xfact_(mUdf(double))
		    , yfact_(mUdf(double))
		    , shift_(mUdf(double))
		    , noudf_(noudf)
		    , setup_(setup)
		{}

    uiString	uiNrDoneText() const override
		{ return ParallelTask::sPosFinished(); }
    uiString	uiMessage() const override
		{ return tr("Cumulative sum executor");}

    void	setYVals( const ArrayND<ArrType>& yvals ) { yarr_ = &yvals; }
    void	setScaler( double scaler, bool forx=true )
		{
		   if ( forx )
		      xfact_ = scaler;
		   else
		      yfact_ = scaler;
		}
    void	setShift( double shift )	{ shift_ = shift; }

protected:

    od_int64	nrIterations() const override	{ return sz_; }

private:

    bool	doPrepare( int ) override
		{
		    if ( outarr_.info().getTotalSz() != sz_ )
			return false;

		    if ( yarr_ && yarr_->info().getTotalSz() != sz_ )
			return false;

		    return true;
		}

    bool	doWork(od_int64,od_int64,int) override;

private:

    const ArrayOperExecSetup&	setup_;
    od_uint64		sz_;
    bool		noudf_;

    const ArrayND<ArrType>&	xarr_;
    const ArrayND<ArrType>*	yarr_;
    ArrayND<ArrType>&	outarr_;
    double		xfact_;
    double		yfact_;
    double		shift_;
};


template <class OperType,class ArrType>
bool ArrOperExec<OperType,ArrType>::doWork( od_int64 start, od_int64 stop, int )
{
    const ArrType* xvals = xarr_.getData();
    const ArrType* yvals = yarr_ ? yarr_->getData() : 0;
    ArrType* outvals = outarr_.getData();
    const ValueSeries<ArrType>* xstor = xarr_.getStorage();
    const ValueSeries<ArrType>* ystor = yarr_ ? yarr_->getStorage() : 0;
    ValueSeries<ArrType>* outstor = outarr_.getStorage();
    ArrayNDIter* xiter = xvals || xstor ? 0 : new ArrayNDIter( xarr_.info() );
    ArrayNDIter* yiter = ( yarr_ && ( yvals || ystor ) ) || !yarr_
		       ? 0 : new ArrayNDIter( yarr_->info() );
    ArrayNDIter* outiter = outvals || outstor
			 ? 0 : new ArrayNDIter( outarr_.info() );
    if ( xiter ) xiter->setGlobalPos( start );
    if ( yiter ) yiter->setGlobalPos( start );
    if ( outiter ) outiter->setGlobalPos( start );
    const bool doscalexvals = !mIsUdf(xfact_);
    const bool hasyvals = yarr_;
    const bool doscaleyvals = !mIsUdf(yfact_);
    const bool doshiftoutvals = !mIsUdf(shift_);
    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	OperType xvalue = xvals ? xvals[idx]
				: xstor ? xstor->value(idx)
					: xarr_.getND( xiter->getPos() );
	if ( !noudf_ && mIsUdf(xvalue) )
	    { mSetResAndContinue( mUdf(ArrType) ) continue;	}

	if ( doscalexvals ) xvalue *= xfact_;
	if ( hasyvals )
	{
	    OperType yvalue = yvals ? yvals[idx]
				    : ystor ? ystor->value(idx)
					    : yarr_->getND( yiter->getPos() );
	    if ( !noudf_ && mIsUdf(yvalue) )
		{ mSetResAndContinue( mUdf(ArrType) ) continue;	}

	    if ( doscaleyvals ) yvalue *= yfact_;
	    if ( setup_.doadd_ )
		xvalue += yvalue;
	    else
		xvalue *= yvalue;
	}

	if ( doshiftoutvals )
	    xvalue += shift_;

	mSetResAndContinue( mCast(ArrType,xvalue) )
    }

    delete xiter; delete yiter; delete outiter;

    return true;
}



template <class T>
class CumSumExec : public ParallelTask
{ mODTextTranslationClass(CumSumExec);
public:
		CumSumExec( const T* vals, od_int64 sz, bool noudf )
		    : vals_(vals)
		    , sz_(sz)
		    , noudf_(noudf)
		    , cumsum_(mUdf(T))
		{}

    uiString	uiNrDoneText() const override
		{ return ParallelTask::sPosFinished(); }
    uiString	uiMessage() const override
		{ return tr("Cumulative sum executor");}

    T		getSum() const		{ return cumsum_; }

protected:

    od_int64	nrIterations() const override	{ return sz_; }

private:

    bool	doPrepare( int nrthreads ) override
		{
		    return sumvals_.setSize( nrthreads );
		}

    bool	doWork( od_int64 start, od_int64 stop, int threadidx ) override
		{
		    T sumval = 0;
		    od_int64 count = 0;
		    for ( int idx=mCast(int,start); idx<=stop; idx++ )
		    {
			const T value = vals_[idx];
			if ( !noudf_ && mIsUdf(value) )
			    continue;

			sumval += value;
			count++;
		    }

		    sumvals_[threadidx] = count==0 ? mUdf(T) : sumval;

		    return true;
		}

    bool	doFinish( bool success ) override
		{
		    if ( !success )
			return false;

		    int count = 0;
		    cumsum_ = 0;
		    for ( int idx=0; idx<sumvals_.size(); idx++ )
		    {
			if ( mIsUdf(sumvals_[idx]) )
			    continue;

			cumsum_ += sumvals_[idx];
			count++;
		    }

		    if ( count == 0 )
			cumsum_ = mUdf(T);

		    return true;
		}

    od_int64	sz_;
    bool	noudf_;

    TypeSet<T>	sumvals_;
    const T*	vals_;
    T		cumsum_;
};


/*!\brief returns the sum of all defined values in the Array.
   Returns UDF if empty or only udfs encountered. */

template <class T>
inline T getSum( const ArrayND<T>& in, bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    CumArrOperExec<double,T> sumexec( in, noudf, setup );
    if ( !sumexec.executeParallel(parallel) )
	return mUdf(T);

    return mCast(T,sumexec.getSum());
}


template <>
inline float_complex getSum( const ArrayND<float_complex>& in, bool noudf,
			     bool parallel )
{ return mUdf(float_complex); }



/*!\brief returns the average amplitude of the array */

template <class T>
inline T getAverage( const ArrayND<T>& in, bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    setup.donormalizesum_ = true;
    CumArrOperExec<double,T> avgexec( in, noudf, setup );
    if ( !avgexec.executeParallel(parallel) )
	return mUdf(T);

    return mCast(T,avgexec.getSum());
}


//!Specialization for complex numbers.
template <>
inline float_complex getAverage<float_complex>(const ArrayND<float_complex>& in,
					       bool noudf, bool parallel )
{
    const od_uint64 sz = in.info().getTotalSz();
    const float_complex sumvals = getSum( in, noudf, parallel );
    return mIsUdf(sumvals) ? mUdf(float_complex) : sumvals / mCast(float,sz);
}


template <class T>
mDefParallelCalc5Pars(ScalingExec,
		  od_static_tr("ScalingExec","Array scaler executor"),
		  const T*,arrin,T*,arrout,T,fact,T,shift,bool,noudf)
mDefParallelCalcBody(,const T inpval = arrin_[idx]; \
		      if ( !noudf_ && ( mIsUdf(inpval) ) ) \
			  { arrout_[idx] = mUdf(T); continue; } \
		      arrout_[idx] = fact_ * inpval + shift_; , )

/*!\brief returns a scaled array */

template <class T>
inline void getScaledArray( const ArrayND<T>& in, ArrayND<T>* out_, double fact,
			    double shift, bool noudf, bool parallel )
{
    ArrayND<T>& out = out_ ? *out_ : const_cast<ArrayND<T>&>( in );
    ArrayOperExecSetup setup;
    ArrOperExec<double,T> scalinngexec( in, 0, noudf, setup, out );
    scalinngexec.setScaler( fact );
    scalinngexec.setShift( shift );
    scalinngexec.executeParallel( parallel );
}



template <class T>
mDefParallelCalc6Pars(SumExec,
		      od_static_tr("SumExec","Array addition executor"),
		  const T*,arr1,const T*,arr2,T*,out,T,fact1,T,fact2,bool,noudf)
mDefParallelCalcBody(,const T val1 = arr1_[idx]; const T val2 = arr2_[idx]; \
		      if ( !noudf_ && ( mIsUdf(val1) || mIsUdf(val2) ) ) \
			  { out_[idx] = mUdf(T); continue; } \
		      out_[idx] = fact1_ * val1 + fact2_ * val2; , )

/*!\brief computes the sum array between two arrays with scaling */

template <class T>
inline void getSumArrays( const ArrayND<T>& in1, const ArrayND<T>& in2,
			  ArrayND<T>& out, double fact1, double fact2,
			  bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    ArrOperExec<double,T> sumexec( in1, &in2, noudf, setup, out );
    sumexec.setScaler( fact1 );
    sumexec.setScaler( fact2, false );
    sumexec.executeParallel( parallel );
}



template <class T>
mDefParallelCalc4Pars(ProdExec,
		      od_static_tr("ProdExec","Array product executor"),
		      const T*,arr1,const T*,arr2,T*,out,bool,noudf)
mDefParallelCalcBody(,const T val1 = arr1_[idx]; const T val2 = arr2_[idx]; \
		      if ( !noudf_ && ( mIsUdf(val1) || mIsUdf(val2) ) ) \
			  { out_[idx] = mUdf(T); continue; } \
		      out_[idx] = val1*val2; , )


/*!\brief computes the product array between two arrays */

template <class T>
inline void getProduct( const ArrayND<T>& in1, const ArrayND<T>& in2,
			ArrayND<T>& out, bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    setup.doadd_ = false;
    ArrOperExec<double,T> prodexec( in1, &in2, noudf, setup, out );
    prodexec.executeParallel( parallel );
}



/*!\brief computes the sum array between two arrays */

template <class T>
inline void getSum( const ArrayND<T>& in1, const ArrayND<T>& in2,
		    ArrayND<T>& out, bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    ArrOperExec<double,T> sumexec( in1, &in2, noudf, setup, out );
    sumexec.executeParallel( parallel );
}


/*!\brief returns the sum of product amplitudes between two vectors */

template <class T>
inline T getSumProduct( const ArrayND<T>& in1, const ArrayND<T>& in2,
			bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    setup.doadd_ = false;
    CumArrOperExec<double,T> sumprodexec( in1, noudf, setup );
    sumprodexec.setYVals( in2 );
    if ( !sumprodexec.executeParallel(parallel) )
	return mUdf(T);

    return mCast(T,sumprodexec.getSum());
}

template <class T>
inline double getSumProductD( const ArrayND<T>& in1, const ArrayND<T>& in2,
			      bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    setup.doadd_ = false;
    CumArrOperExec<double,T> sumprodexec( in1, noudf, setup );
    sumprodexec.setYVals( in2 );
    if ( !sumprodexec.executeParallel(parallel) )
	return mUdf(double);

    return sumprodexec.getSum();
}


/*!\brief returns the sum of squarred amplitudes of the array */

template <class T>
inline T getSumSq( const ArrayND<T>& in, bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    setup.dosqinp_ = true;
    CumArrOperExec<double,T> sumsqexec( in, noudf, setup );
    if ( !sumsqexec.executeParallel(parallel) )
	return mUdf(T);

    return mCast(T,sumsqexec.getSum());
}


/*!\brief return the Norm-2 of the array */

template <class T>
inline T getNorm2( const ArrayND<T>& in, bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    setup.dosqinp_ = true;
    setup.dosqrtsum_ = true;
    CumArrOperExec<double,T> norm2exec( in, noudf, setup );
    if ( !norm2exec.executeParallel(parallel) )
	return mUdf(T);

    return mCast(T,norm2exec.getSum());
}

template <class T>
inline double getNorm2D( const ArrayND<T>& in, bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    setup.dosqinp_ = true;
    setup.dosqrtsum_ = true;
    CumArrOperExec<double,T> norm2exec( in, noudf, setup );
    if ( !norm2exec.executeParallel(parallel) )
	return mUdf(double);

    return norm2exec.getSum();
}


/*!\brief return the RMS of the array */

template <class T>
inline T getRMS( const ArrayND<T>& in, bool noudf, bool parallel )
{
    ArrayOperExecSetup setup;
    setup.dosqinp_ = true;
    setup.donormalizesum_ = true;
    setup.dosqrtsum_ = true;
    CumArrOperExec<double,T> rmsexec( in, noudf, setup );
    if ( !rmsexec.executeParallel(parallel) )
	return mUdf(T);

    return mCast(T,rmsexec.getSum());
}


/*!\brief return the Variance of the array */

template <class RT,class AT>
inline RT getVariance( const ArrayND<AT>& in, bool noudf, bool parallel )
{
    const od_int64 sz = in.totalSize();
    if ( sz < 2 )
	return 0;

    ArrayOperExecSetup sumsetup, stdsetup;
    stdsetup.dosqinp_ = true;
    CumArrOperExec<RT,AT> sumexec( in, noudf, sumsetup );
    CumArrOperExec<RT,AT> stdexec( in, noudf, stdsetup );
    if ( !sumexec.executeParallel(parallel) ||
	 !stdexec.executeParallel(parallel) )
	return mUdf(AT);

    const long double sumx = sumexec.getSum();
    const long double sumxx = stdexec.getSum();
    const RT retval = (sumxx - (sumx*sumx/sz) ) / ( (long double)(sz-1) );
    return retval;
}


/*!\brief return the Standard deviation of the array */

template <class RT,class AT>
inline RT getStdDev( const ArrayND<AT>& in, bool noudf, bool parallel )
{
    return Math::Sqrt( getVariance<RT,AT>(in,noudf,parallel) );
}


/*!\brief returns the residual differences of two arrays */

template <class T>
inline T getResidual( const ArrayND<T>& in1, const ArrayND<T>& in2, bool noudf,
		      bool parallel )
{
    ArrayOperExecSetup setup;
    setup.doabs_ = true;
    setup.donormalizesum_ = true;
    CumArrOperExec<double,T> residualexec( in1, noudf, setup );
    residualexec.setYVals( in2 );
    residualexec.setScaler( -1., false );
    if ( !residualexec.executeParallel(parallel) )
	return mUdf(T);

    return mCast(T,residualexec.getSum());
}


/*!\brief returns the sum of squarred differences of two arrays */

template <class T>
inline T getSumXMY2( const ArrayND<T>& in1, const ArrayND<T>& in2, bool noudf,
		     bool parallel )
{
    ArrayOperExecSetup setup;
    setup.dosqout_ = true;
    CumArrOperExec<double,T> sumxmy2exec( in1, noudf, setup );
    sumxmy2exec.setYVals( in2 );
    sumxmy2exec.setScaler( -1., false );
    if ( !sumxmy2exec.executeParallel(parallel) )
	return mUdf(T);

    return mCast(T,sumxmy2exec.getSum());
}


/*!\brief returns the sum of summed squarred amplitudes of two arrays */

template <class T>
inline T getSumX2PY2( const ArrayND<T>& in1, const ArrayND<T>& in2, bool noudf,
		      bool parallel )
{
    ArrayOperExecSetup setup;
    setup.dosqinp_ = true;
    CumArrOperExec<double,T> sumx2py2exec( in1, noudf, setup );
    sumx2py2exec.setYVals( in2 );
    if ( !sumx2py2exec.executeParallel(parallel) )
	return mUdf(T);

    return mCast(T,sumx2py2exec.getSum());
}


/*!\brief returns the sum of subtracted squarred amplitudes of two arrays */

template <class T>
inline T getSumX2MY2( const ArrayND<T>& in1, const ArrayND<T>& in2, bool noudf,
		      bool parallel )
{
    ArrayOperExecSetup setup;
    setup.dosqinp_ = true;
    CumArrOperExec<double,T> sumx2my2exec( in1, noudf, setup );
    sumx2my2exec.setYVals( in2 );
    sumx2my2exec.setScaler( -1., false );
    if ( !sumx2my2exec.executeParallel(parallel) )
	return mUdf(T);

    return mCast(T,sumx2my2exec.getSum());
}


/*!\brief returns the intercept and gradient of two arrays */

template <class T, class fT>
inline bool getInterceptGradient( const ArrayND<T>& iny, const ArrayND<T>* inx_,
				  T& intercept, T& gradient )
{
    const od_uint64 sz = iny.info().getTotalSz();
    T avgyvals = getAverage( iny, false, true );
    if ( mIsUdf(avgyvals) )
	return false;

    const bool hasxvals = inx_;
    const ArrayND<T>* inx = hasxvals ? inx_ : 0;
    if ( !hasxvals )
    {
	Array1DImpl<T>* inxtmp = new Array1DImpl<T>( mCast(int,sz) );
	if ( !inxtmp->isOK() )
	    { delete inxtmp; return false; }

	T* inxvals = inxtmp->getData();
	if ( inxvals )
	{
	    for ( od_uint64 idx=0; idx<sz; idx++ )
		inxvals[idx] = mCast(fT,idx);
	}
	else
	{
	    ArrayNDIter iter( inxtmp->info() );
	    od_uint64 idx = 0;
	    do
	    {
		inxtmp->setND( iter.getPos(), mCast(fT,idx) );
		idx++;
	    } while ( iter.next() );
	}

	inx = inxtmp;
    }

    T avgxvals = getAverage( *inx, false, true );
    if ( mIsUdf(avgxvals) )
	{ if ( !hasxvals) delete inx; return false; }

    ArrayND<T>& inyed = const_cast<ArrayND<T>&>( iny );
    ArrayND<T>& inxed = const_cast<ArrayND<T>&>( *inx );
    removeBias<T,fT>( inyed );
    removeBias<T,fT>( inxed );

    Array1DImpl<T> crossprodxy( mCast(int,sz) );
    if ( !crossprodxy.isOK() )
	{ if ( !hasxvals ) delete inx; return false; }

    getProduct( *inx, iny, crossprodxy, false, true );

    gradient = getSumProduct( *inx, iny, false, true ) /
	       getSumSq( *inx, false, true );
    intercept = avgyvals - gradient * avgxvals;
    getScaledArray( iny, &inyed, 1., avgyvals, false, true );

    if ( !hasxvals )
	delete inx;
    else
	getScaledArray( *inx, &inxed, 1., avgxvals, false, true );

    return true;
}

} // namespace ArrayMath



/*!<Replaces the undefined samples in a 2D/3D array. Optionally provides
    the list of replaced samples.
    If a PosInfo::CubeData is provided the samples where traces are not present
    will not be substituted
 */

template <class T>
mClass(Algo) ArrayUdfValReplacer : public ParallelTask
{ mODTextTranslationClass(ArrayUdfValReplacer)
public:
		ArrayUdfValReplacer( Array2D<T>& inp,
				     LargeValVec<od_uint64>* undefidxs )
		    : ParallelTask("Array Udf Replacer")
		    , inp_(inp)
		    , replval_(0.f)
		    , undefidxs_(undefidxs)
		    , tks_(0)
		    , trcssampling_(0)
		    , totalnr_(inp.info().getTotalSz()/inp.info().getSize(1))
		{}

		ArrayUdfValReplacer( Array3D<T>& inp,
				     LargeValVec<od_uint64>* undefidxs )
		    : ParallelTask("Array Udf Replacer")
		    , inp_(inp)
		    , replval_(0.f)
		    , undefidxs_(undefidxs)
		    , tks_(0)
		    , trcssampling_(0)
		    , totalnr_(inp.info().getTotalSz()/inp.info().getSize(2))
		{}

    uiString	uiMessage() const override
		{
		    return tr("Replacing undefined values");
		}

    uiString	uiNrDoneText() const override
		{ return ParallelTask::sTrcFinished(); }

    void	setReplacementValue( T val )	{ replval_ = val; }

    void	setSampling( const TrcKeySampling& tks,
			     const PosInfo::CubeData* trcssampling )
		{
		    tks_ = &tks;
		    trcssampling_ = trcssampling;
		}

protected:

    od_int64	nrIterations() const override	{ return totalnr_; }

private:

    bool	doPrepare( int ) override
		{
		    if ( undefidxs_ )
			undefidxs_->setEmpty();

		    return true;
		}

    bool	doWork( od_int64 start, od_int64 stop, int ) override
		{
		    const bool isrect = tks_ && trcssampling_
				       ? trcssampling_->isFullyRectAndReg()
				       : true;
		    const ArrayNDInfo& info = inp_.info();
		    const int nrtrcsp = info.getSize( inp_.get1DDim() );
		    T* dataptr = inp_.getData();
		    ValueSeries<T>* datastor = inp_.getStorage();
		    const bool hasarrayptr = dataptr;
		    const bool hasstorage = datastor;
		    const bool neediterator = !hasarrayptr && !hasstorage;
		    const od_int64 offset = start * nrtrcsp;
		    dataptr += offset;
		    od_uint64 validx = offset;
		    ArrayNDIter* iter = neediterator
				       ? new ArrayNDIter( info ) : 0;
		    if ( iter )
			iter->setGlobalPos( offset );

		    const T replval = replval_;
		    for ( od_int64 idx=start; idx<=stop; idx++,
							 quickAddToNrDone(idx))
		    {
			const bool hastrcdata = isrect ? true
					: trcssampling_->isValid(idx,*tks_);
			if ( hastrcdata )
			{
			    for ( int idz=0; idz<nrtrcsp; idz++ )
			    {
				const int* pos = iter ? iter->getPos() : 0;
				const T val = hasarrayptr ? *dataptr
					    : hasstorage
						? datastor->value( validx )
						: inp_.getND( pos );
				if ( !mIsUdf(val) )
				{
				    if ( hasarrayptr ) dataptr++;
				    else if ( hasstorage ) validx++;
				    else iter->next();

				    continue;
				}

				if ( undefidxs_ )
				{
				    lck_.lock();
				    *undefidxs_ += idx*nrtrcsp + idz;
				    lck_.unLock();
				}

				if ( hasarrayptr )
				    *dataptr++ = replval;
				else if ( hasstorage )
				    datastor->setValue( validx++, replval );
				else
				{
				    inp_.setND( pos, replval );
				    iter->next();
				}
			    }
			}
			else
			{
			    if ( hasarrayptr )
			    {
				dataptr =
				OD::sysMemValueSet( dataptr, replval, nrtrcsp );
			    }
			    else if ( hasstorage )
			    {
				for ( int idz=0; idz<nrtrcsp; idz++ )
				    datastor->setValue( validx++, replval );
			    }
			    else
			    {
				for ( int idz=0; idz<nrtrcsp; idz++ )
				{
				    inp_.setND( iter->getPos(), replval );
				    iter->next();
				}
			    }
			}
		    }

		    delete iter;

		    return true;
		}

    ArrayND<T>&			inp_;
    T				replval_;
    LargeValVec<od_uint64>*		undefidxs_;
    const TrcKeySampling*	tks_;
    const PosInfo::CubeData*	trcssampling_;
    const od_int64		totalnr_;
    Threads::Mutex		lck_;
};


/*!< Filters a list of global indexes with two different sampling */

mGlobal(Algo) void convertUndefinedIndexList(const TrcKeyZSampling& tkzsin,
					     const TrcKeyZSampling& tkzsout,
					     LargeValVec<od_uint64>&);


/*!<Replaces undefined values back to an ND array */

template <class T>
mClass(Algo) ArrayUdfValRestorer : public ParallelTask
{ mODTextTranslationClass(ArrayUdfValRestorer)
public:
		ArrayUdfValRestorer( const LargeValVec<od_uint64>& undefidxs,
				      ArrayND<T>& outp )
		    : ParallelTask("Udf retriever")
		    , undefidxs_(undefidxs)
		    , outp_(outp)
		    , totalnr_(undefidxs.size())
		{}

    uiString	uiMessage() const override
		{ return tr("Replacing undefined values"); }

    uiString	uiNrDoneText() const override
		{ return ParallelTask::sPosFinished(); }

protected:

    od_int64	nrIterations() const override { return totalnr_; }

private:

    bool	doWork( od_int64 start, od_int64 stop, int ) override
		{
		    T* outpptr = outp_.getData();
		    ValueSeries<T>* outpstor = outp_.getStorage();
		    mDeclareAndTryAlloc(int*,pos,int[outp_.info().getNDim()])
		    if ( !pos )
			return false;

		    const T udfval = mUdf(T);
		    const ArrayNDInfo& info = outp_.info();
		    for ( od_int64 idx=start; idx<=stop; idx++,
							 quickAddToNrDone(idx) )
		    {
			const od_uint64 sidx = undefidxs_[idx];
			if ( outpptr )
			    outpptr[sidx] = udfval;
			else if ( outpstor )
			    outpstor->setValue( sidx, udfval );
			else
			{
			    info.getArrayPos( sidx, pos );
			    outp_.setND( pos, udfval );
			}
		    }

		    delete [] pos;

		    return true;
		}

    const LargeValVec<od_uint64>&	undefidxs_;
    ArrayND<T>&			outp_;
    const od_int64		totalnr_;
};


/*!<Replaces undefined values back from missing traces to a 3D array */

template <class T>
mClass(Algo) Array3DUdfTrcRestorer : public ParallelTask
{ mODTextTranslationClass(Array3DUdfTrcRestorer)
public:
		Array3DUdfTrcRestorer( const PosInfo::CubeData& trcssampling,
				       const TrcKeySampling& tks,
				       Array3D<T>& outp )
		    : ParallelTask("Udf traces retriever")
		    , trcssampling_(trcssampling)
		    , tks_(tks)
		    , outp_(outp)
		    , totalnr_(trcssampling.totalSizeInside(tks) ==
			       mCast(int,tks.totalNr()) ? 0 :
			       outp.info().getTotalSz()/outp.info().getSize(2))
		{}

    uiString	uiMessage() const override
		{ return tr("Restoring undefined values"); }

    uiString	uiNrDoneText() const override
		{ return ParallelTask::sTrcFinished(); }

protected:

    od_int64	nrIterations() const override	{ return totalnr_; }

private:

    bool	doWork( od_int64 start, od_int64 stop, int ) override
		{
		    const Array3DInfo& info = outp_.info();
		    const int nrtrcsp = info.getSize( outp_.get1DDim() );
		    T* outpptr = outp_.getData();
		    ValueSeries<T>* outstor = outp_.getStorage();
		    const bool hasarrayptr = outpptr;
		    const bool hasstorage = outstor;
		    const od_int64 offset = start * nrtrcsp;
		    outpptr += offset;
		    od_uint64 validx = offset;
		    const Array2DInfoImpl hinfo( info.getSize(0),
						 info.getSize(1) );
		    ArrayNDIter* hiter = !hasarrayptr && !hasstorage
				       ? new ArrayNDIter( hinfo ) : 0;
		    if ( hiter )
			hiter->setGlobalPos( start );

		    for ( od_int64 idx=start; idx<=stop; idx++ )
		    {
			if ( trcssampling_.isValid(idx,tks_) )
			{
			    if ( hasarrayptr ) outpptr+=nrtrcsp;
			    else if ( hasstorage ) validx+=nrtrcsp;
			    else hiter->next();

			    continue;
			}

			if ( hasarrayptr )
			{
			    outpptr =
				OD::sysMemValueSet( outpptr, mUdf(T), nrtrcsp );
			}
			else if ( hasstorage )
			{
			    for ( int idz=0; idz<nrtrcsp; idz++ )
				outstor->setValue( validx++, mUdf(T) );
			}
			else
			{
			    const int inlidx = (*hiter)[0];
			    const int crlidx = (*hiter)[1];
			    for ( int idz=0; idz<nrtrcsp; idz++ )
				outp_.set( inlidx, crlidx, idz, mUdf(T) );
			}
		    }

		    delete hiter;

		    return true;
		}

    const PosInfo::CubeData&	trcssampling_;
    const TrcKeySampling&	tks_;
    Array3D<T>&			outp_;

    const od_int64		totalnr_;
};


/*!<Determines the start/end of live data in a 2D/3D array. The returned index
    is the index of the first live sample
    The output arrays must have one dimension less than the data array
 */

template <class T>
mClass(Algo) MuteArrayExtracter : public ParallelTask
{ mODTextTranslationClass(MuteArrayExtracter)
public:
		MuteArrayExtracter( const ArrayND<T>& data,
				    ArrayND<int>& topmute,
				    ArrayND<int>& tailmute )
		    : ParallelTask("Mute Array Extracter")
		    , data_(data)
		    , topmute_(topmute)
		    , tailmute_(tailmute)
		    , tks_(0)
		    , trcssampling_(0)
		    , totalnr_(data.info().getTotalSz()/
			       data.info().getSize(data.get1DDim()))
		{}

    uiString	uiMessage() const override
		{
		    return tr("Extracting mute positions");
		}

    uiString	uiNrDoneText() const override
		{ return ParallelTask::sTrcFinished(); }

    void	setSampling( const TrcKeySampling& tks,
			     const PosInfo::CubeData* trcssampling )
		{
		    tks_ = &tks;
		    trcssampling_ = trcssampling;
		}

protected:

    od_int64	nrIterations() const override	{ return totalnr_; }

private:

    bool	doPrepare( int ) override
		{
		    const int data1ddim = data_.get1DDim();
		    if ( ( data1ddim != 1 && data1ddim != 2 ) ||
			 topmute_.get1DDim() != data1ddim-1 ||
			 tailmute_.get1DDim() != data1ddim-1 )
			return false;

		    topmute_.setAll( 0 );
		    const int nrz =
				mCast(int,data_.info().getTotalSz()/totalnr_);
		    tailmute_.setAll( nrz-1 );

		    return true;
		}

    bool	doWork( od_int64 start, od_int64 stop, int ) override
		{
		    const bool isrect = tks_ && trcssampling_
				       ? trcssampling_->isFullyRectAndReg()
				       : true;
		    const T* dataptr = data_.getData();
		    int* topmuteptr = topmute_.getData();
		    int* tailmuteptr = tailmute_.getData();
		    const ValueSeries<T>* datastor = data_.getStorage();
		    ValueSeries<int>* topmutestor = topmute_.getStorage();
		    ValueSeries<int>* tailmutestor = tailmute_.getStorage();
		    const bool hasarrayptr = dataptr && topmuteptr &&
					     tailmuteptr;
		    const bool hasstorage = datastor && topmutestor &&
					    tailmutestor;
		    const bool neediterator = !hasarrayptr && !hasstorage;
		    const ArrayNDInfo& info = data_.info();
		    const int zidx = data_.get1DDim();
		    const int nrtrcsp = info.getSize( zidx );
		    const od_int64 offset = start * nrtrcsp;
		    if ( hasarrayptr )
		    {
			dataptr += offset;
			topmuteptr += start;
			tailmuteptr += start;
		    }

		    od_uint64 validx = offset;
		    const int ndim = info.getNDim();
		    const bool is2d = ndim == 2;
		    const int nrlines = is2d ? 1 : info.getSize(0);
		    const int nrtrcs = info.getSize( is2d ? 0 : 1 );
		    const Array2DInfoImpl hinfo( nrlines, nrtrcs );
		    ArrayNDIter* hiter = neediterator
				       ? new ArrayNDIter( hinfo ) : 0;
		    if ( hiter )
			hiter->setGlobalPos( start );

		    const T zeroval = mCast(T,0);
		    mDeclareAndTryAlloc(int*,pos,int[ndim])
		    if ( !pos )
			return false;

		    for ( od_int64 idx=start; idx<=stop; idx++,
							 quickAddToNrDone(idx) )
		    {
			const bool hastrcdata = isrect ? true
					: trcssampling_->isValid(idx,*tks_);
			if ( !hastrcdata )
			{
			    if ( hasarrayptr )
			    {
				dataptr+=nrtrcsp;
				topmuteptr++;
				tailmuteptr++;
			    }
			    if ( hasstorage ) validx+=nrtrcsp;
			    else hiter->next();

			    continue;
			}

			const int* hpos = hiter ? hiter->getPos() : 0;
			if ( hiter )
			{
			    for ( int ipos=0; ipos<ndim; ipos++ )
				pos[ipos] = hpos[ipos];
			    hiter->next();
			}

			bool allnull = true;
			for ( int idz=0; idz<nrtrcsp; idz++ )
			{
			    if ( hiter ) pos[zidx] = idz;
			    const T val = hasarrayptr
					    ? *dataptr++
					    : hasstorage
						? datastor->value( validx++ )
						: data_.getND( pos );
			    if ( val == zeroval )
				continue;

			    if ( hasarrayptr )
			    {
				*topmuteptr++ = idz;
				dataptr += nrtrcsp-idz-2;
			    }
			    else if ( hasstorage )
			    {
				topmutestor->setValue( idx, idz );
				validx += nrtrcsp-idz-2;
			    }
			    else
				topmute_.setND( hpos, idz );

			    allnull = false;
			    break;
			}

			if ( allnull )
			{
			    if ( hasarrayptr )
			    {
				*topmuteptr++ = nrtrcsp;
				*tailmuteptr++ = -1;
			    }
			    else if ( hasstorage )
			    {
				topmutestor->setValue( idx, nrtrcsp );
				tailmutestor->setValue( idx, -1 );
			    }
			    else
			    {
				topmute_.setND( hpos, nrtrcsp );
				tailmute_.setND( hpos, -1 );
			    }

			    continue;
			}

			for ( int idz=nrtrcsp-1; idz>=0; idz-- )
			{
			    if ( hiter ) pos[zidx] = idz;
			    const T val = hasarrayptr
					    ? *dataptr--
					    : hasstorage
						? datastor->value( validx-- )
						: data_.getND( pos );
			    if ( val == zeroval )
				continue;

			    if ( hasarrayptr )
			    {
				*tailmuteptr++ = idz;
				dataptr += nrtrcsp-idz+1;
			    }
			    else if ( hasstorage )
			    {
				tailmutestor->setValue( idx, idz );
				validx += nrtrcsp-idz+1;
			    }
			    else
				tailmute_.setND( hpos, idz );

			    break;
			}

		    }

		    delete [] pos;
		    delete hiter;

		    return true;
		}

    const ArrayND<T>&		data_;
    const TrcKeySampling*	tks_;
    const PosInfo::CubeData*	trcssampling_;
    ArrayND<int>&		topmute_;
    ArrayND<int>&		tailmute_;

    const od_int64		totalnr_;
};
