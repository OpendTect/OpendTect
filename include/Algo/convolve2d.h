#ifndef convolve2d_h
#define convolve2d_h

/*@+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: convolve2d.h,v 1.1 2007-07-09 21:05:25 cvskris Exp $
________________________________________________________________________


*/

#include "arraynd.h"
#include "basictask.h"


template <class T>
class Convolver2D : public ParalellTask
{
public:
    inline		Convolver2D();

    inline void		setX(const Array2D<T>&,int first0=0,int first1=0);
    inline void		sety(const Array2D<T>&,int first0=0,int first1=0);
    inline void		setZ(Array2D<T>& z )		{ z_ = &z; }
    void		setNormalize( bool n )		{ normalize_ = n; }
    void		setCorrelate( bool yn )		{ correlate_ = yn; }

protected:
    inline bool		doWork( int, int, int );
    int			nrTimes() const { return z_->info().getSize( 0 ); }
    const Array2D<T>*	x_;
    RowCol		xshift_;
    const Array2D<T>*	y_;
    RowCol		yshift_;
    Array2D<T>*		z_;
    RowCol		zshift_;
    bool		normalize_;
    bool		correlate_;
};


template <class T> inline
Convolver2D<T>::Convolver2D()
    : x_( 0 )
    , xshift_(0,0)
    , y_( 0 )
    , yshift(0,0)
    , z_( 0 )
    , zshift_( 0, 0 )
    , normalize_( false )
    , correlate_( false )
{}


template <class T> inline
void Convolver2D<T>::setX( const Array2D<T>& x, int first0, int first1 )
{
    x_ = &x;
    shiftx_.row = first0;
    shiftx_.col = first1;
}


template <class T> inline
void Convolver2D<T>::sety( const Array2D<T>& y, int first0, int first1 )
{
    y_ = &y;
    yshift_.row = first0;
    yshift_.col = first1;
}


template <class T> inline
bool Convolver2D<T>::doWork( int start, int stop, int )
{
    const int dim0xsz = x_->info().getSize( 0 );
    const int dim1xsz = x_->info().getSize( 1 );
    const int dim0ysz = y_->info().getSize( 0 );
    const int dim1ysz = y_->info().getSize( 1 );
    const int dim1zsz = z_->info().getSize( 1 );

    for ( int idz0=start; idz0<=stop; idz0++ )
    {
	const int zdim0 = idz0+zshift_.row;
	for ( int idz1=0; idz1<dim1zsz; idz1++ )
	{
	    const int zdim1 = idz1+zshift_.col;

	    T sum = 0;
	    int nrsamples = 0;
	    for ( int idx0=0; idx0<dim0xsz; idx0++ )
	    {
		const int xdim0 = idx0+xshift_.row;
		const int ydim0 = zdim0-xdim0;
		const int idy0 = ydim0-yshift_.row;
		if ( idy0<0 || idy0>=dim0ysz )
		    continue;

		for ( int idx1=0; idx1<dim1xsz; idx1++ )
		{
		    const int xdim1 = idx1+xshift_.col;
		    const int ydim1 = zdim1-xdim1;
		    const int idy0 = ydim1-yshift_.col;
		    if ( idy1<0 || idy1>=dim1ysz )
			continue;

		    const T yval = correlate_
			? y->get( dim0ysz-1-idy0, dim1ysz-1-idy1 )
			: y_->get( idy0, idy1 );

		    sum += x_->get( idx0, idx1 ) * yval;
		    nrsamples++;
		}
	    }

	    if ( !nrsamples )
		z_->set( idz0, idz1, 0 );
	    else if ( normalize )
		z_->set( idz0, idz1, sum/nrsamples );
	    else
		z_->set( idz0, idz1, sum );
	}
    }

    return true;
}

#endif
