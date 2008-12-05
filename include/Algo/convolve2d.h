#ifndef convolve2d_h
#define convolve2d_h

/*@+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: convolve2d.h,v 1.9 2008-12-05 23:14:37 cvskris Exp $
________________________________________________________________________


*/

#include "arraynd.h"
#include "task.h"
#include "rowcol.h"

/*!Convolves (or correlates) two 2D signals. */


template <class T>
class Convolver2D : public ParallelTask
{
public:
    inline		Convolver2D();

    inline void		setX(const Array2D<T>&,int first0=0,int first1=0);
    inline void		setY(const Array2D<T>&,int first0=0,int first1=0);
    inline void		setZ(Array2D<T>& z )		{ z_ = &z; }
    void		setNormalize( bool n )		{ normalize_ = n; }
    			/*!<If true, the sum will be divided by
			    the sum of Y. */
    void		setCorrelate( bool yn )		{ correlate_ = yn; }
    			/*!<If true, the convolution will be replaced by a
			   correllation. */
    void		setHasUdfs(bool yn)		{ hasudfs_ = yn; }
    			//!<Default is false

protected:
    inline bool		doWork( od_int64, od_int64, int );
    od_int64		totalNr() const { return z_->info().getSize( 0 ); }
    const Array2D<T>*	x_;
    int			xshift0_;
    int			xshift1_;
    const Array2D<T>*	y_;
    int			yshift0_;
    int			yshift1_;
    Array2D<T>*		z_;
    bool		normalize_;
    bool		correlate_;
    bool		hasudfs_;
};


template <class T> inline
Convolver2D<T>::Convolver2D()
    : x_( 0 )
    , xshift0_( 0 )
    , xshift1_( 0 )
    , y_( 0 )
    , yshift0_( 0 )
    , yshift1_( 0 )
    , z_( 0 )
    , normalize_( false )
    , correlate_( false )
    , hasudfs_( false )
{}


template <class T> inline
void Convolver2D<T>::setX( const Array2D<T>& x, int first0, int first1 )
{
    x_ = &x;
    xshift0_ = first0;
    xshift1_ = first1;
}


template <class T> inline
void Convolver2D<T>::setY( const Array2D<T>& y, int first0, int first1 )
{
    y_ = &y;
    yshift0_ = first0;
    yshift1_ = first1;
}


template <class T> inline
bool Convolver2D<T>::doWork( od_int64 start, od_int64 stop, int )
{
    const int xsz0 = x_->info().getSize( 0 );
    const int xsz1 = x_->info().getSize( 1 );
    const int ysz0 = y_->info().getSize( 0 );
    const int ysz1 = y_->info().getSize( 1 );

    int startpos[2];

    if ( !z_->info().getArrayPos( start, startpos ) )
	return false;

    ArrayNDIter iterator( z_->info() );
    iterator.setPos( startpos );

    for ( int idx=start; idx<=stop; idx++ )
    {
	const int* zvar = iterator.getPos();
	T sum = 0;
	T ysum = 0;
	int nrsamples = 0;
	for ( int idx0=0; idx0<xsz0; idx0++ )
	{
	    const int xvar0 = idx0-xshift0_;
	    const int yvar0 = correlate_ ? xvar0-zvar[0] : zvar[0]-xvar0;
	    const int idy0 = yvar0+yshift0_;
	    if ( idy0<0 || idy0>=ysz0 )
		continue;

	    for ( int idx1=0; idx1<xsz1; idx1++ )
	    {
		const int xvar1 = idx1-xshift1_;
		const int yvar1 = correlate_ ? xvar1-zvar[1] : zvar[1]-xvar1;
		const int idy1 = yvar1+yshift1_;
		if ( idy1<0 || idy1>=ysz1 )
		    continue;

		const T yval = y_->get( idy0, idy1 );
		if ( mIsUdf(yval) )
		    continue;

		const T xval = x_->get( idx0, idx1 );
		if ( mIsUdf(xval) )
		    continue;

		sum += xval * yval;
		ysum += yval;
		nrsamples++;
	    }
	}

	if ( !nrsamples ) z_->setND( zvar, 0 );
	else if ( normalize_ && !mIsZero(ysum,1e-8) )
	    z_->setND( zvar, sum/ysum );
	else z_->setND( zvar, sum );

	if ( !iterator.next() && idx!=stop )
	    return false;
    }

    return true;
}


#endif
