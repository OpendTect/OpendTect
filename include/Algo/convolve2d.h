#ifndef convolve2d_h
#define convolve2d_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: convolve2d.h,v 1.18 2012/02/09 15:01:27 cvskris Exp $
________________________________________________________________________


*/

#include "complex"
#include "arraynd.h"
#include "task.h"
#include "rowcol.h"

#if defined(__msvc__) && (defined(ALGO_EXPORTS) || defined(Algo_EXPORTS))
# define mAlgoClass	class dll_export
#else
# define mAlgoClass	class
#endif

namespace Fourier { class CC; }
typedef std::complex<float> float_complex;

/*!Convolves (or correlates) two 2D signals. */

template <class T>
mAlgoClass Convolver2D : public ParallelTask
{
public:
				Convolver2D();
				~Convolver2D();

    inline void			setX(const Array2D<T>&,bool hasudfs);
    inline const Array2D<T>*	getX() const		{ return x_; }
    inline void			setY(const Array2D<T>&,bool hasudfs);
    inline const Array2D<T>*	getY() const		{ return x_; }
    inline void			setZ(Array2D<T>& z )	{ z_ = &z; }
    void			setNormalize( bool n )	{ normalize_ = n; }
				/*!<If true, the sum will be divided by
				    the sum of Y. */
    void			setCorrelate( bool yn )	{ correlate_ = yn; }
				/*!<If true, the convolution will be replaced
				    by a correllation. */
    od_int64             	nrIterations() const;

protected:
    bool		doWork(od_int64,od_int64,int);
    bool		doNonFFTWork(od_int64,od_int64,int);
    bool		doPrepare(int);
    bool		shouldFFT() const;

    const Array2D<T>*	x_;
    const Array2D<T>*	y_;
    Array2D<T>*		z_;
    bool		normalize_;
    bool		correlate_;
    bool		xhasudfs_;
    bool		yhasudfs_;

    float_complex*	xf_;
    float_complex*	yf_;
    float_complex*	zf_;

    bool		updatexf_;
    bool		updateyf_;

    Fourier::CC*	fft_;
};




template <> inline
bool Convolver2D<float>::shouldFFT() const
{
    if ( xhasudfs_ || yhasudfs_ || x_->info()!=y_->info() ||
	 x_->info()!=z_->info() )
	return false;

    return true;
}


template <> inline
od_int64 Convolver2D<float>::nrIterations() const
{
    return shouldFFT()
        ? 1 
        : z_->info().getSize( 0 );
}


template <class T> inline
bool Convolver2D<T>::shouldFFT() const
{ return false; }


template <class T> inline
Convolver2D<T>::Convolver2D()
    : x_( 0 )
    , y_( 0 )
    , z_( 0 )
    , normalize_( false )
    , correlate_( false )
    , xhasudfs_( false )
    , yhasudfs_( false )
    , updatexf_( true )
    , updateyf_( true )
    , xf_( 0 )
    , yf_( 0 )
    , zf_( 0 )
    , fft_( 0 )
{}


template <>
Convolver2D<float>::~Convolver2D();


template <class T>
Convolver2D<T>::~Convolver2D()
{}

template <class T> inline
void Convolver2D<T>::setX( const Array2D<T>& x, bool hasudfs )
{
    x_ = &x;
    updatexf_ = true;
    xhasudfs_ = hasudfs;
}


template <class T> inline
void Convolver2D<T>::setY( const Array2D<T>& y, bool hasudfs )
{
    y_ = &y;
    updateyf_ = true;
    yhasudfs_ = hasudfs;
}


#define mConvolver2DSetY( dim ) \
const int firsty##dim = correlate_ \
    ? -zvar[dim] \
    : zvar[dim]; \
\
const char y##dim##inc = correlate_ ? 1 : -1


#define mConvolver2DSetIndex( dim ) \
const int idy##dim = firsty##dim+idx##dim*y##dim##inc; \
if ( idy##dim<0 ) \
{ \
    if ( correlate_ ) \
    { \
	idx##dim += (-idy##dim)-1; \
	continue; \
    } \
 \
    break; \
} \
 \
if ( idy##dim>=ysz##dim ) \
{ \
    if ( correlate_ ) \
	break; \
 \
    const int diff = idy##dim-(ysz##dim-1); \
    idx##dim += diff-1; \
    continue; \
}


template <class T> inline
od_int64 Convolver2D<T>::nrIterations() const
{
    return shouldFFT()
	? 1 
	: z_->info().getSize( 0 );
}


template <>
bool Convolver2D<float>::doWork( od_int64 start, od_int64 stop, int );


template <class T> inline
bool Convolver2D<T>::doWork( od_int64 start, od_int64 stop, int thread )
{
    return doNonFFTWork( start, stop, thread );
}


template <class T> inline
bool Convolver2D<T>::doNonFFTWork( od_int64 start, od_int64 stop, int )
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

    const ValueSeries<T>* xstor_ = x_->getStorage();
    const T* xptr_ = x_->getData();

    const ValueSeries<T>* ystor_ = y_->getStorage();
    const T* yptr_ = y_->getData();

    for ( int idx=start; idx<=stop; idx++ )
    {
	const int* zvar = iterator.getPos();
	T sum = 0;
	T ysum = 0;
	int nrsamples = 0;

	mConvolver2DSetY( 0 );
	mConvolver2DSetY( 1 );

	for ( int idx0=0; idx0<xsz0 && shouldContinue(); idx0++ )
	{
	    mConvolver2DSetIndex( 0 );

	    const od_int64 yoffset = ystor_ || yptr_ ?
		y_->info().getOffset( idy0, 0 ) : 0;

            const od_int64 xoffset = xstor_ || xptr_ ?
		x_->info().getOffset( idx0, 0 ) : 0;

	    for ( int idx1=0; idx1<xsz1; idx1++ )
	    {
		mConvolver2DSetIndex( 1 );

		const T yval = yptr_
		    ? yptr_[yoffset+idy1]
		    : ystor_
			? ystor_->value( yoffset+idy1 )
			: y_->get( idy0, idy1 );

		if ( mIsUdf(yval) )
		    continue;

		const T xval = xptr_
		    ? xptr_[xoffset+idx1]
		    : xstor_
			? xstor_->value( xoffset+idx1 )
			: x_->get( idx0, idx1 );

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

	addToNrDone( 1 );

	if ( !iterator.next() && idx!=stop )
	    return false;
    }

    return true;
}


#endif
