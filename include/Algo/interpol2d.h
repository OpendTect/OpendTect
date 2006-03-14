#ifndef interpol2d_h
#define interpol2d_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Mar 2006
 RCS:		$Id: interpol2d.h,v 1.2 2006-03-14 20:06:24 cvsbert Exp $
________________________________________________________________________

*/

#include "undefval.h"
#include "interpol1d.h" //TODO temp. for poly

#define mFillIfUdfFromSquare(nd,left,right,opp) \
    if ( u##nd##_ ) \
    { \
	if ( u##left##_ && u##right##_ ) \
	    v##nd = v##opp; \
	v##nd = u##left##_ || u##right##_ ? \
	       (u##right##_ ? v##left : v##right) \
			    : (v##left + v##right) / 2; \
    }

#define mRetUdfIfNearestUdf() \
    if ( haveudf_ && ( \
	 ( u00_ && x < 0.5 && y < 0.5 ) \
      || ( u10_ && x >= 0.5 && y < 0.5 ) \
      || ( u01_ && x < 0.5 && y >= 0.5 ) \
      || ( u11_ && x >= 0.5 && y >= 0.5 ) ) ) \
	return mUdf(T)


namespace Interpolate
{

/*!\brief Linear 2D interpolation. */

template <class T>
class LinearReg2D
{
public:

LinearReg2D() {}

LinearReg2D( const T* const* v )
{
    set( v[0][0], v[1][0], v[0][1], v[1][1] );
}

LinearReg2D( T v00, T v10, T v01, T v11 )
{
    set( v00, v10, v01, v11 );
}

inline void set( T v00, T v10, T v01, T v11 )
{
    a_[0] = v00;
    a_[1] = v10 - v00;
    a_[2] = v01 - v00;
    a_[3] = v11 + v00 - v10 - v01;
}

inline T apply( float x, float y ) const
{
    return a_[0] + a_[1] * x + a_[2] * y + a_[3] * x * y;
}

    T	a_[4];

};


template <class T>
inline T linearReg2D( T v00, T v10, T v01, T v11, float x, float y )
{
    return LinearReg2D<T>( v00, v10, v01, v11 ).apply ( x, y );
}


/*!\brief Linear 2D interpolation with standard undef handling.  */

template <class T>
class LinearReg2DWithUdf
{
public:

LinearReg2DWithUdf()	{}

LinearReg2DWithUdf( const T* const* v )
{
    set( v[0][0], v[1][0], v[0][1], v[1][1] );
}

LinearReg2DWithUdf( T v00, T v10, T v01, T v11 )
{
    set( v00, v10, v01, v11 );
}

inline void set( T v00, T v10, T v01, T v11 )
{
    u00_ = mIsUdf(v00);
    u10_ = mIsUdf(v10);
    u01_ = mIsUdf(v01);
    u11_ = mIsUdf(v11);
    haveudf_ = u00_ || u10_ || u01_ || u11_;

    if ( haveudf_ )
    {
	mFillIfUdfFromSquare(00,01,10,11)
	mFillIfUdfFromSquare(10,00,11,01)
	mFillIfUdfFromSquare(01,11,00,10)
	mFillIfUdfFromSquare(11,10,01,00)
    }

    intp_.set( v00, v10, v01, v11 );
}

inline T apply( float x, float y ) const
{
    mRetUdfIfNearestUdf();

    return intp_.apply( x, y );
}

    LinearReg2D<T>	intp_;
    bool		haveudf_;
    bool		u00_;
    bool		u10_;
    bool		u01_;
    bool		u11_;

};


template <class T>
inline T linearReg2DWithUdf( T v00, T v10, T v01, T v11, float x, float y )
{
    return LinearReg2DWithUdf<T>( v00, v10, v01, v11 ).apply ( x, y );
}


/*!<\brief Interpolate 2D regularly sampled, using a 3rd order surface. */

template <class T>
class PolyReg2D
{
public:

PolyReg2D() {}

PolyReg2D( const T* const* v )
{
	set( v[0][1], v[0][2],
    v[1][0], v[1][1], v[1][2], v[1][3],
    v[2][0], v[2][1], v[2][2], v[2][3],
	     v[3][1], v[3][2] );
}

PolyReg2D( T vm10, T vm11,
   T v0m1, T v00,  T v01, T v02,
   T v1m1, T v10,  T v11, T v12,
	   T v20,  T v21 )
{
    set( vm10, vm11, v0m1, v00, v01, v02, v1m1, v10, v11, v12, v20, v21 );
}

inline void set( T vm10, T vm11,
	 T v0m1, T v00,  T v01, T v02,
	 T v1m1, T v10,  T v11, T v12,
		 T v20,  T v21 )
{
    intp1d_[0].set( vm10, v00, v10, v20 );
    intp1d_[1].set( vm11, v01, v11, v21 );
    intp1d_[2].set( v0m1, v00, v01, v02 );
    intp1d_[3].set( v1m1, v10, v11, v12 );
}

inline T apply( float x, float y ) const
{
    v[0] = intp1d_[0].apply( x );
    if ( y < 0.001 )
	return v[0];
    v[1] = intp1d_[0].apply( x );
    if ( y > 0.999 )
	return v[1];
    v[2] = intp1d_[0].apply( y );
    if ( x < 0.001 )
	return v[2];
    v[3] = intp1d_[0].apply( y );
    if ( x > 0.999 )
	return v[3];

    // take weighted average
    return (x * v[3] + (1-x) * v[2] + y * v[1] + (1-y) * v[0]) / 2;
}

    // TODO: do it right
    mutable T		v[4];
    PolyReg1D<T>	intp1d_[4];

};


template <class T>
inline T polyReg2D( T vm10, T vm11, T v0m1, T v00, T v01, T v02,
	   T v1m1, T v10, T v11, T v12, T v20, T v21, float x, float y )
{
    return PolyReg2D<T>(vm10,vm11,v0m1,v00,v01,v02,v1m1,v10,v11,v12,v20,v21)
	  .apply( x, y );
}


/*!<\brief PolyReg2D which smoothly handles undefined values

  Note that this class _requires_ x and y to be between 0 and 1 for correct
  undef handling. Correct means: if the nearest sample is undefined, return
  undefined. Otherwise always return a value.

  */

template <class T>
class PolyReg2DWithUdf
{
public:

PolyReg2DWithUdf()	{}

PolyReg2DWithUdf( const T* const* v )
{
	set( v[0][1], v[0][2],
    v[1][0], v[1][1], v[1][2], v[1][3],
    v[2][0], v[2][1], v[2][2], v[2][3],
	     v[3][1], v[3][2] );
}

PolyReg2DWithUdf( T vm10, T vm11, T v0m1, T v00,  T v01, T v02,
		  T v1m1, T v10,  T v11, T v12, T v20,  T v21 )
{
    set( vm10, vm11, v0m1, v00, v01, v02, v1m1, v10, v11, v12, v20, v21 );
}

void fillOuter2Inner( T vm10, T vm11, T v0m1, T v02,
		      T v1m1, T v12, T v20, T v21,
		      T& v00, T& v10, T& v01, T& v11 )
{
#define mFillWithEither(nd,cand1,cand2) \
    if ( u##nd##_ ) \
    { \
	if ( !u##cand1##_ ) v##nd = v##cand1; \
	else if ( !u##cand2##_ ) v##nd = v##cand2; \
    }

    mFillWithEither(00,m10,0m1)
    mFillWithEither(10,1m1,20)
    mFillWithEither(11,21,12)
    mFillWithEither(01,02,m11)
}

void fillInner2Inner( T& v00, T& v10, T& v01, T& v11 )
{
    bool kpu00 = u00_, kpu10 = u10_, kpu01 = u01_, kpu11 = u11_;
    u00_ = mIsUdf(v00); u10_ = mIsUdf(v10);
    u01_ = mIsUdf(v01); u11_ = mIsUdf(v11);
    if ( u00_ || u10_ || u01_ || u11_ )
    {
	mFillIfUdfFromSquare(00,01,10,11)
	mFillIfUdfFromSquare(10,00,11,01)
	mFillIfUdfFromSquare(01,11,00,10)
	mFillIfUdfFromSquare(11,10,01,00)
    }
    u00_ = kpu00; u10_ = kpu10; u01_ = kpu01; u11_ = kpu11;
}

void fillInner2Outer( T v00, T v10, T v01, T v11,
		      T& vm10, T& vm11, T& v0m1, T& v02,
		      T& v1m1, T& v12, T& v20, T& v21 )
{
#define mFillIfUdf(nd,src) if ( mIsUdf(v##nd) ) v##nd= v##src;
    mFillIfUdf(m10,00);
    mFillIfUdf(0m1,00);
    mFillIfUdf(1m1,10);
    mFillIfUdf(20,10);
    mFillIfUdf(m11,01);
    mFillIfUdf(02,01);
    mFillIfUdf(12,11);
    mFillIfUdf(21,11);
}

inline void set( T vm10, T vm11,
	 T v0m1, T v00,  T v01, T v02,
	 T v1m1, T v10,  T v11, T v12,
		 T v20,  T v21 )
{
    u00_ = mIsUdf(v00);
    u10_ = mIsUdf(v10);
    u01_ = mIsUdf(v01);
    u11_ = mIsUdf(v11);
    um10_ = mIsUdf(vm10);
    um11_ = mIsUdf(vm11);
    u0m1_ = mIsUdf(v0m1);
    u1m1_ = mIsUdf(v1m1);
    u02_ = mIsUdf(v02);
    u12_ = mIsUdf(v12);
    u20_ = mIsUdf(v20);
    u21_ = mIsUdf(v21);
    haveudf_ = u00_ || u10_ || u01_ || u11_;

    if ( haveudf_ || u02_ || u12_ || u20_ || u21_
	          || um10_ || um11_ || u0m1_ || u1m1_ )
    {
	if ( haveudf_ )
	{
	    fillOuter2Inner( vm10, vm11, v0m1, v02, v1m1, v12, v20, v21,
			     v00, v10, v01, v11 );
	    fillInner2Inner( v00, v10, v01, v11 );
	}
	fillInner2Outer( v00, v10, v01, v11,
			 vm10, vm11, v0m1, v02, v1m1, v12, v20, v21 );
    }

    intp_.set( vm10, vm11, v0m1, v00, v01, v02, v1m1, v10, v11, v12, v20, v21 );
}

inline T apply( float x, float y ) const
{
    mRetUdfIfNearestUdf();

    return intp_.apply( x, y );
}

    // TODO: do it right
    PolyReg2D<T>	intp_;
    bool		haveudf_;
    bool		u00_;
    bool		u10_;
    bool		u01_;
    bool		u11_;
    bool		um10_;
    bool		um11_;
    bool		u0m1_;
    bool		u02_;
    bool		u1m1_;
    bool		u12_;
    bool		u20_;
    bool		u21_;

};


template <class T>
inline T polyReg2DWithUdf( T vm10, T vm11, T v0m1, T v00, T v01, T v02,
	   T v1m1, T v10, T v11, T v12, T v20, T v21, float x, float y )
{
    return PolyReg2DWithUdf<T>(vm10,vm11,v0m1,v00,v01,v02,v1m1,v10,v11,v12,v20,
	    			v21).apply( x, y );
}


} // namespace Interpolate

#endif
