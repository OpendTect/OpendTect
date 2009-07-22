#ifndef array2dresample_h
#define array2dresample_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          26/07/2000
 RCS:           $Id: array2dresample.h,v 1.6 2009-07-22 16:01:15 cvsbert Exp $
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
3. (optional) call set with new arrays, and call execute() again.
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

    inline void		set(const Array2D<T>& from, Array2D<TT>& to,
	    		    bool fromhasudfs,
			    const Geom::PosRectangle<float>* rectinfrom=0 );
    			/*!<\param rectinfrom specifies a part of from
			     that should serve as source. If ommitted,
			     the entire from array is used. */

    inline od_int64	nrIterations() const;

private:
    inline bool		doWork( od_int64 start, od_int64 stop, int );

    const Array2D<T>*		from_;
    Array2D<TT>*		to_;
    Array2DFunc<TT,float,T>	func_;
    SamplingData<float>		xsampling_;
    SamplingData<float>		ysampling_;
};

#define mXDim	0
#define mYDim	1


template <class T, class TT> inline
Array2DReSampler<T,TT>::Array2DReSampler( const Array2D<T>& from,
			Array2D<TT>& to, bool fromhasudfs,
			const Geom::PosRectangle<float>* rectinfrom )
{ set( from, to, fromhasudfs, rectinfrom ); }


template <class T, class TT> inline
void Array2DReSampler<T,TT>::set( const Array2D<T>& from, Array2D<TT>& to,
				 bool fromhasudfs,
				 const Geom::PosRectangle<float>* rectinfromptr)
{
    from_ = &from; to_ = &to;
    func_.set( from, fromhasudfs );

    const int xsize = to.info().getSize( mXDim );
    const int ysize = to.info().getSize( mYDim );

    Geom::PosRectangle<float> rectinfrom( 0, 0, from.info().getSize(mXDim)-1,
	    			       from.info().getSize(mYDim)-1);

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
    return to_->info().getSize( mXDim );
}



template <class T, class TT> inline
bool Array2DReSampler<T,TT>::doWork( od_int64 start, od_int64 stop, int )
{
    const int ysize = to_->info().getSize( mYDim );

    for ( int idx=start; idx<=stop; idx++, addToNrDone(1) )
    {
	const float sourcex = xsampling_.atIndex( idx );

	for ( int idy=0; idy<ysize; idy++ )
	{
	    const float sourcey = ysampling_.atIndex( idy );
	    to_->set( idx, idy, func_.getValue( sourcex, sourcey ) );
	}
    }

    return true;
}


#undef mXDim
#undef mYDim



#endif
