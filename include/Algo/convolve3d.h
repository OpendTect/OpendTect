#ifndef convolve3d_h
#define convolve3d_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          Feb 2008
 RCS:           $Id$
________________________________________________________________________


*/

#include "arraynd.h"
#include "task.h"
#include "math2.h"

/*!
\ingroup Algo
\brief Convolves (or correlates) two 3D signals.
*/

template <class T>
class Convolver3D : public ParallelTask
{
public:
    inline		Convolver3D();

    inline void		setX(const Array3D<T>&,
	    		     int first0=0,int first1=0, int first2=0);
    inline void		setY(const Array3D<T>&,
	    		     int first0=0,int first1=0, int first2=0);
    inline void		setZ(Array3D<T>& z )		{ z_ = &z; }
    void		setNormalize( bool n )		{ normalize_ = n; }
    			/*!<If true, the sum will be divided by
			    the sum of Y.*/
    void		setCorrelate( bool yn )		{ correlate_ = yn; }
    			/*!<If true, the convolution will be replaced by a
			   correllation. */
    void		setHasUdfs(bool yn)		{ hasudfs_ = yn; }
    			//!<Default is false

    virtual bool	execute()	{ return execute( true ); }
    virtual bool	execute(bool);

protected:
    inline bool		shouldFFT() const;

    bool		doFFT();
    inline bool		doWork( od_int64, od_int64, int );
    od_int64		nrIterations() const { return z_->info().getTotalSz(); }
    const Array3D<T>*	x_;
    int			xshift0_;
    int			xshift1_;
    int			xshift2_;
    const Array3D<T>*	y_;
    int			yshift0_;
    int			yshift1_;
    int			yshift2_;

    Array3D<T>*		z_;
    bool		normalize_;
    bool		correlate_;

    bool		hasudfs_;
};


template <class T> inline
Convolver3D<T>::Convolver3D()
    : x_( 0 )
    , xshift0_( 0 )
    , xshift1_( 0 )
    , xshift2_( 0 )
    , y_( 0 )
    , yshift0_( 0 )
    , yshift1_( 0 )
    , yshift2_( 0 )
    , z_( 0 )
    , normalize_( false )
    , correlate_( false )
    , hasudfs_( false )
{}


template <class T> inline
void Convolver3D<T>::setX( const Array3D<T>& x,
			   int first0, int first1, int first2 )
{
    x_ = &x;
    xshift0_ = first0;
    xshift1_ = first1;
    xshift2_ = first2;
}


template <class T> inline
void Convolver3D<T>::setY( const Array3D<T>& y, int first0, int first1,
       			   int first2 )
{
    y_ = &y;
    yshift0_ = first0;
    yshift1_ = first1;
    yshift2_ = first2;
}


#define mConvolver3DSetY( dim ) \
const int firsty##dim = correlate_ \
    ? -xshift##dim##_-zvar[dim]+yshift##dim##_ \
    : zvar[dim]+xshift##dim##_+yshift##dim##_; \
\
    const char y##dim##inc = correlate_ ? 1 : -1


#define mConvolver3DSetIndex( dim ) \
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
bool Convolver3D<T>::doWork( od_int64 start, od_int64 stop, int )
{
    const int xsz0 = x_->info().getSize( 0 );
    const int xsz1 = x_->info().getSize( 1 );
    const int xsz2 = x_->info().getSize( 2 );
    const int ysz0 = y_->info().getSize( 0 );
    const int ysz1 = y_->info().getSize( 1 );
    const int ysz2 = y_->info().getSize( 2 );

    int startpos[3];

    if ( !z_->info().getArrayPos( start, startpos ) )
	return false;

    ArrayNDIter iterator( z_->info() );
    iterator.setPos( startpos );

    const ValueSeries<T>* xstor_ = x_->getStorage();
    const T* xptr_ = x_->getData();

    const ValueSeries<T>* ystor_ = y_->getStorage();
    const T* yptr_ = y_->getData();

    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	const int* zvar = iterator.getPos();
	T sum = 0;
	T ysum = 0;
	int nrsamples = 0;

	mConvolver3DSetY( 0 );
	mConvolver3DSetY( 1 );
	mConvolver3DSetY( 2 );

	for ( int idx0=0; idx0<xsz0 && shouldContinue(); idx0++ )
	{
	    mConvolver3DSetIndex( 0 );

	    for ( int idx1=0; idx1<xsz1; idx1++ )
	    {
		mConvolver3DSetIndex( 1 );

		const od_int64 yoffset = ystor_ || yptr_ ?
		    y_->info().getOffset( idy0, idy1, 0 ) : 0;

		const od_int64 xoffset = xstor_ || xptr_ ?
		    x_->info().getOffset( idx0, idx1, 0 ) : 0;

		for ( int idx2=0; idx2<xsz2; idx2++ )
		{
		    mConvolver3DSetIndex( 2 );

		    const T yval = yptr_
			? yptr_[yoffset+idy2]
			: ystor_
			    ? ystor_->value( yoffset+idy2 )
			    : y_->get( idy0, idy1, idy2 );

		    if ( mIsUdf(yval) )
			continue;

		    const T xval = xptr_
			? xptr_[xoffset+idx2]
			: xstor_
			    ? xstor_->value( xoffset+idx2 )
			    : x_->get( idx0, idx1, idx2 );

		    if ( mIsUdf(xval) )
			continue;

		    sum += xval * yval;
		    ysum += yval;
		    nrsamples++;
		}
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


template <class T> inline
bool Convolver3D<T>::execute( bool yn )
{
    if ( shouldFFT() )
	return doFFT();

    return ParallelTask::execute( yn );
}


template <class T> inline
bool Convolver3D<T>::doFFT()
{
    //TODO
    return false;
}
	

template <class T> inline
bool Convolver3D<T>::shouldFFT() const
{
    return false;
}

template <> inline
bool Convolver3D<float>::shouldFFT() const
{
    return false; //Remove when doFFT is implemented
/*
    if ( correlate_ || normalize_ )
	return false;

    if ( !x_ || !y_ || !z_ )
	return false;

    const int xsz = x_->info().getTotalSz();
    const int ysz = y_->info().getTotalSz();
    const int zsz = z_->info().getTotalSz();

    int maxsz = x_->info().getTotalSz();
    maxsz = mMAX( maxsz, y_->info().getTotalSz() );
    maxsz = mMAX( maxsz, z_->info().getTotalSz() );

    const int tradsz = zsz * mMIN(ysz,xsz);
    const float fftszf = maxsz * Math::Log( (float) maxsz );
    const int fftsz = mNINT32( fftszf );


    return fftsz<tradsz;
    */
}


#endif
