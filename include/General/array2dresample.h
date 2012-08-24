#ifndef array2dresample_h
#define array2dresample_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          26/07/2000
 RCS:           $Id: array2dresample.h,v 1.10 2012-08-24 22:19:51 cvsnanne Exp $
________________________________________________________________________

-*/

#include "arraynd.h"
#include "array2dfunc.h"
#include "task.h"
#include "geometry.h"
#include "interpol2d.h"

/*!Fills an Array2D from another Array2D of another size.
Usage:
1. Create
2. call execute();
3. (optional) call set with rew arrays, and call execute() again.
*/


template <class T, class TT>
class Array2DReSampler : public ParallelTask
{
public:

    inline		Array2DReSampler(const Array2D<T>& from,
			    Array2D<TT>& to, bool fromhasudfs,
			    const Geom::PosRectangle<float>* rectinfrom=0 );
    			/*!<\param rectinfrom specifies a part of from
			     that should serve as source. If ommitted,
			     the entire from array is used. */
    inline		Array2DReSampler(const Array2D<T>& from,
			    TT* to, int sz0, int sz1, bool fromhasudfs,
			    const Geom::PosRectangle<float>* rectinfrom=0 );
    			/*!<\param rectinfrom specifies a part of from
			     that should serve as source. If ommitted,
			     the entire from array is used. */
    inline		Array2DReSampler(const Array2D<T>& from,
			    ValueSeries<TT>& to,int sz0,int sz1,
			    bool fromhasudfs,
			    const Geom::PosRectangle<float>* rectinfrom=0 );
    			/*!<\param rectinfrom specifies a part of from
			     that should serve as source. If ommitted,
			     the entire from array is used. */

    inline void		set(const Array2D<T>& from, Array2D<TT>& to,
	    		    bool fromhasudfs,
			    const Geom::PosRectangle<float>* rectinfrom=0 );
    			/*!<\param rectinfrom specifies a part of from
			     that should serve as source. If ommitted,
			     the entire from array is used. */
    inline void		set(const Array2D<T>& from, TT* to, int sz0, int sz1,
	    		    bool fromhasudfs,
			    const Geom::PosRectangle<float>* rectinfrom=0 );
    			/*!<\param rectinfrom specifies a part of from
			     that should serve as source. If ommitted,
			     the entire from array is used. */
    inline void		set(const Array2D<T>& from,ValueSeries<TT>& to,
			    int sz0, int sz1,bool fromhasudfs,
			    const Geom::PosRectangle<float>* rectinfrom=0 );
    			/*!<\param rectinfrom specifies a part of from
			     that should serve as source. If ommitted,
			     the entire from array is used. */

    inline od_int64	nrIterations() const;
    void		setInterpolate(bool yn) { interpolate_ = yn; }

private:
    inline void		updateScale(const Geom::PosRectangle<float>*);
    inline bool		doWork(od_int64,od_int64, int );

    const Array2D<T>*		from_;
    Array2D<TT>*		to_;
    Array2DInfoImpl		toinfo_;
    TT*				toptr_;
    ValueSeries<TT>*		tovs_;
    Array2DFunc<TT,float,T>	func_;
    SamplingData<float>		xsampling_;
    SamplingData<float>		ysampling_;
    bool			interpolate_;
};

#define mXDim	0
#define mYDim	1


template <class T, class TT> inline
Array2DReSampler<T,TT>::Array2DReSampler( const Array2D<T>& from,
			Array2D<TT>& to, bool fromhasudfs,
			const Geom::PosRectangle<float>* rectinfrom )
    : ParallelTask("Re-sampling data")
{ set( from, to, fromhasudfs, rectinfrom ); }


template <class T, class TT> inline
Array2DReSampler<T,TT>::Array2DReSampler( const Array2D<T>& from,
			TT* to, int sz0, int sz1, bool fromhasudfs,
			const Geom::PosRectangle<float>* rectinfrom )
    : ParallelTask("Re-sampling data")
{ set( from, to, sz0, sz1, fromhasudfs, rectinfrom ); }


template <class T, class TT> inline
Array2DReSampler<T,TT>::Array2DReSampler( const Array2D<T>& from,
			ValueSeries<TT>& to, int sz0, int sz1, bool fromhasudfs,
			const Geom::PosRectangle<float>* rectinfrom )
    : ParallelTask("Re-sampling data")
{ set( from, to, sz0, sz1, fromhasudfs, rectinfrom ); }


#define mUpdateResampler \
    from_ = &from;  \
    func_.set( from, fromhasudfs ); \
    updateScale( rectinfromptr )

template <class T, class TT> inline
void Array2DReSampler<T,TT>::set( const Array2D<T>& from, Array2D<TT>& to,
				 bool fromhasudfs,
				 const Geom::PosRectangle<float>* rectinfromptr)
{
    if ( to.getData() )
    {
	toptr_ = to.getData();
	to_ = 0;
	tovs_ = 0;
    }
    else if ( to.getStorage() )
    {
	to_ = 0;
	toptr_ = 0;
	tovs_ = to.getStorage();
    }
    else
    {
	to_ = &to;
	toptr_ = 0;
	tovs_ = 0;
    }

    toinfo_ = to.info();

    mUpdateResampler;
}


template <class T, class TT> inline
void Array2DReSampler<T,TT>::set( const Array2D<T>& from, TT* to,
	int sz0, int sz1, bool fromhasudfs,
	const Geom::PosRectangle<float>* rectinfromptr)
{
    toptr_ = to; 
    to_ = 0;
    tovs_ = 0;

    toinfo_.setSize( mXDim, sz0 );
    toinfo_.setSize( mYDim, sz1 );

    mUpdateResampler;
}


template <class T, class TT> inline
void Array2DReSampler<T,TT>::set( const Array2D<T>& from, ValueSeries<TT>& to,
	int sz0, int sz1, bool fromhasudfs,
	const Geom::PosRectangle<float>* rectinfromptr)
{
    if ( to.arr() )
    {
	toptr_ = to.arr();
	tovs_ = 0;
	to_ = 0;
    }
    else
    {
	toptr_ = 0; 
	tovs_ = &to;
	to_ = 0;
    }

    toinfo_.setSize( mXDim, sz0 );
    toinfo_.setSize( mYDim, sz1 );
    mUpdateResampler;
}


template <class T, class TT> inline
void Array2DReSampler<T,TT>::updateScale(
	const Geom::PosRectangle<float>* rectinfromptr )
{
    const int xsize = toinfo_.getSize( mXDim );
    const int ysize = toinfo_.getSize( mYDim );

    Geom::PosRectangle<float> rectinfrom( 0, 0, from_->info().getSize(mXDim)-1,
					        from_->info().getSize(mYDim)-1);

    if ( rectinfromptr )
    {
	Geom::PosRectangle<float> nrect( *rectinfromptr );
	nrect.checkCorners( true, true );
	if ( rectinfrom.contains( nrect, 1e-3 ) )
	    rectinfrom = nrect;
    }

    xsampling_.start = rectinfrom.left();
    xsampling_.step = rectinfrom.width()/(xsize-1);

    ysampling_.start = rectinfrom.left();
    ysampling_.step = rectinfrom.height()/(ysize-1);
}


template <class T, class TT> inline
od_int64 Array2DReSampler<T,TT>::nrIterations() const
{
    return toinfo_.getSize( mXDim );
}



template <class T, class TT> inline
bool Array2DReSampler<T,TT>::doWork( od_int64 start, od_int64 stop, int )
{
    const int ysize = toinfo_.getSize( mYDim );
    int localnrdone = 0;
    od_int64 offset = start*ysize;
    TT* toptr = toptr_ ? toptr_+offset : 0;
    for ( int idx=start; idx<=stop; idx++ )
    {
	const float sourcex = xsampling_.atIndex( idx );
	for ( int idy=0; idy<ysize; idy++ )
	{
	    const float sourcey = ysampling_.atIndex( idy );

	    const TT val = interpolate_ 
		? func_.getValue( sourcex, sourcey )
		: from_->get( mNINT32(sourcex), mNINT32( sourcey ) );
	    if ( toptr )
	    {
		*toptr = val;
		toptr++;
	    }
	    else if ( tovs_ )
		tovs_->setValue( offset++, val );
	    else
		to_->set( idx, idy , val );
	}

	localnrdone++;

	if ( localnrdone>100 )
	{
	    addToNrDone( localnrdone );
	    localnrdone = 0;
	}
    }

    addToNrDone( localnrdone );

    return true;
}

#undef mXDim
#undef mYDim
#undef mUpdateResampler



#endif
