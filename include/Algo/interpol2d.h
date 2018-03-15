#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2006
________________________________________________________________________

*/

#include "interpol1d.h"

namespace Interpolate
{

/*!
\brief specification for a 2D interpolator

  The 'set' method accepts values arranged like this:
  <pre>
    7 9
  5 1 3 11
  4 0 2 10
    6 8
  </pre>
  The interpolation is supposed to take place in the 0-1-3-2 'base square'.
  This looks crazy but the idea is that 0-3 are always needed, and the rest is
  provided bottom-left to top-right.

  In some cases, you don't have or don't want to provide data outside the base
  square.  If you want to be 100% sure that any applier is able to use the data,
  make sure that the size is at least 5, that 0-3 are filled (possibly with
  undefineds) and set the v[4] to -mUdf(T) (that is a minus there).

  The 'apply' method needs the relative distance in x and y direction from
  the origin (where v[0] is located), and should therefore generally be between
  0 and 1, although you can also use the classes for near extrapolation.
*/

template <class PosT,class ValT=PosT>
mClass(Algo) Applier2D
{
public:

    virtual		~Applier2D()				{}
    virtual void	set(const ValT*)			= 0;

    virtual ValT	apply(PosT x,PosT y) const	= 0;

};


/*!
\brief Linear 2D interpolation.
*/

template <class PosT,class ValT=PosT>
mClass(Algo) LinearReg2D : public Applier2D<PosT,ValT>
{
public:

    inline		LinearReg2D();
    inline		LinearReg2D(const ValT*);
    inline		LinearReg2D(ValT v00,ValT v10,ValT v01,ValT v11);

    inline void		set(const ValT*);
    inline void		set(ValT v00,ValT v01,ValT v10,ValT v11);
    inline ValT		apply(PosT x,PosT y) const;

protected:

    ValT		a_[4];
};


template <class ValT,class PosT>
inline ValT linearReg2D( ValT v00, ValT v01, ValT v10, ValT v11, PosT x, PosT y)
{ return LinearReg2D<PosT,ValT>( v00, v01, v10, v11 ).apply( x, y ); }


/*!
\brief Linear 2D interpolation with standard undef handling.
*/

template <class PosT,class ValT=PosT>
mClass(Algo) LinearReg2DWithUdf : public Applier2D<PosT,ValT>
{
public:

    inline		LinearReg2DWithUdf();
    inline		LinearReg2DWithUdf(const ValT*);
    inline		LinearReg2DWithUdf(ValT v00,ValT v10,ValT v01,ValT v11);

    inline void		set(const ValT*);
    inline void		set(ValT v00,ValT v01,ValT v10,ValT v11);
    inline ValT		apply(PosT x,PosT y) const;

protected:

    LinearReg2D<PosT,ValT> intp_;
    bool		haveudf_;
    bool		u00_;
    bool		u10_;
    bool		u01_;
    bool		u11_;
};


template <class PosT,class ValT>
inline ValT linearReg2DWithUdf( ValT v00, ValT v01, ValT v10, ValT v11,
				PosT x, PosT y )
{
    return LinearReg2DWithUdf<PosT,ValT>( v00, v01, v10, v11 ).apply( x, y );
}


/*!
\brief Interpolate 2D regularly sampled, using a 2nd order surface.

  Contrary to teh linear approach it does matter whether deltaX is different
  from deltaY. That is why you can supply an xstretch. If xstretch > 1 then
  the deltaX < deltaY, moreover: xstretch = deltaY / deltaX;
*/

template <class PosT,class ValT=PosT>
mClass(Algo) PolyReg2D : public Applier2D<PosT,ValT>
{
public:

    inline		PolyReg2D(float xstretch=1);
    inline		PolyReg2D(const ValT*,float xstretch=1);
    inline		PolyReg2D(    ValT vm10,ValT vm11,
			   ValT v0m1, ValT v00, ValT v01,  ValT v02,
			   ValT v1m1, ValT v10, ValT v11,  ValT v12,
				      ValT v20, ValT v21,
				  float xstretch=1.f);

    inline void		set(const ValT*);
    inline void		set(          ValT vm10,ValT vm11,
			   ValT v0m1, ValT v00, ValT v01,  ValT v02,
			   ValT v1m1, ValT v10, ValT v11,  ValT v12,
				      ValT v20, ValT v21);

    inline ValT		apply(PosT x,PosT y) const;

protected:

    PolyReg1D<ValT>	ix0_, ix1_, iy0_, iy1_;
    ValT		vm10_, v0m1_, v20_, v02_;
    ValT		delxm1_, delym1_, delx2_, dely2_;
    float		xs_;

};


template <class PosT,class ValT>
inline ValT polyReg2D( ValT vm10, ValT vm11, ValT v0m1, ValT v00, ValT v01,
	   ValT v02, ValT v1m1, ValT v10, ValT v11, ValT v12, ValT v20,
	   ValT v21, PosT x, PosT y, float xs=1 )
{
    return PolyReg2D<PosT,ValT>(vm10,vm11,v0m1,v00,v01,v02,v1m1,v10,v11,v12,
				v20,v21,xs).apply( x, y );
}


/*!
\brief PolyReg2D which smoothly handles undefined values.

  Note that this class _requires_ x and y to be between 0 and 1 for correct
  undef handling. Correct means: if the nearest sample is undefined, return
  undefined. Otherwise always return a value.
*/

template <class PosT,class ValT=PosT>
mClass(Algo) PolyReg2DWithUdf : public Applier2D<PosT,ValT>
{
public:

    inline		PolyReg2DWithUdf(float xstretch=1);
    inline		PolyReg2DWithUdf(const ValT*,float xstretch=1);
    inline		PolyReg2DWithUdf(ValT vm10,ValT vm11,ValT v0m1,ValT v00,
					 ValT v01,ValT v02,ValT v1m1,ValT v10,
					 ValT v11,ValT v12,ValT v20,ValT v21,
					 float xstretch=1);

    inline void		set(const ValT*);
    inline void		set(        ValT vm10, ValT vm11,
			   ValT v0m1,ValT v00, ValT v01,  ValT v02,
			   ValT v1m1,ValT v10, ValT v11,  ValT v12,
				     ValT v20, ValT v21);

    inline ValT		apply(PosT x,PosT y) const;

protected:

    inline void		fillOuter2Inner(ValT,ValT,ValT,ValT,ValT,ValT,ValT,
					ValT,ValT&,ValT&,ValT&,ValT&);
    inline void		fillInner2Inner(ValT&,ValT&,ValT&,ValT&);
    inline void		fillInner2Outer(ValT,ValT,ValT,ValT,ValT&,ValT&,ValT&,
				        ValT&,ValT&,ValT&,ValT&,ValT&);

    PolyReg2D<PosT,ValT> intp_;
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


template <class PosT,class ValT>
inline ValT polyReg2DWithUdf( ValT vm10, ValT vm11, ValT v0m1, ValT v00,
	    ValT v01, ValT v02, ValT v1m1, ValT v10, ValT v11, ValT v12,
	    ValT v20, ValT v21, PosT x, PosT y )
{
    return PolyReg2DWithUdf<PosT,ValT>(vm10,vm11,v0m1,v00,v01,v02,
				  v1m1,v10,v11,v12,v20,v21).apply( x, y );
}

//--- LinearReg2D Implementation

template <class PosT,class ValT> inline
LinearReg2D<PosT,ValT>::LinearReg2D() {}


template <class PosT,class ValT> inline
LinearReg2D<PosT,ValT>::LinearReg2D( const ValT* v )
{
    set( v[0], v[1], v[2], v[3] );
}


template <class PosT,class ValT> inline
LinearReg2D<PosT,ValT>::LinearReg2D( ValT v00, ValT v01, ValT v10, ValT v11 )
{
    set( v00, v01, v10, v11 );
}


template <class PosT,class ValT> inline
void LinearReg2D<PosT,ValT>::set( const ValT* v )
{
    set( v[0], v[1], v[2], v[3] );
}


template <class PosT,class ValT> inline
void LinearReg2D<PosT,ValT>::set( ValT v00, ValT v01, ValT v10, ValT v11 )
{
    a_[0] = v00;
    a_[1] = v10 - v00;
    a_[2] = v01 - v00;
    a_[3] = v11 + v00 - v10 - v01;
}


template <class PosT,class ValT> inline
ValT LinearReg2D<PosT,ValT>::apply( PosT x, PosT y ) const
{
    return a_[0] + a_[1] * x + a_[2] * y + a_[3] * x * y;
}


//---  LinearReg2DWithUdf Implementation

template <class PosT,class ValT> inline
LinearReg2DWithUdf<PosT,ValT>::LinearReg2DWithUdf() {}


template <class PosT,class ValT> inline
LinearReg2DWithUdf<PosT,ValT>::LinearReg2DWithUdf( const ValT* v )
{
    set( v[0], v[1], v[2], v[3] );
}


template <class PosT,class ValT> inline
LinearReg2DWithUdf<PosT,ValT>::LinearReg2DWithUdf( ValT v00, ValT v01, ValT v10,
					      ValT v11 )
{
    set( v00, v01, v10, v11 );
}


template <class PosT,class ValT> inline
void LinearReg2DWithUdf<PosT,ValT>::set( const ValT* v )
{
    set( v[0], v[1], v[2], v[3] );
}


#define mFillIfUdfFromSquare(nd,left,right,opp) \
    if ( u##nd##_ ) \
    { \
	if ( u##left##_ && u##right##_ ) \
	    v##nd = v##opp; \
	else \
	    v##nd = u##left##_ || u##right##_ ? \
		   (u##right##_ ? v##left : v##right) \
				: (v##left + v##right) / 2; \
    }

template <class PosT,class ValT> inline
void LinearReg2DWithUdf<PosT,ValT>::set( ValT v00, ValT v01, ValT v10,
					 ValT v11 )
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

    intp_.set( v00, v01, v10, v11 );
}


#define mRetUdfIfNearestUdf() \
    if ( haveudf_ && ( \
	 ( u00_ && x < 0.5 && y < 0.5 ) \
      || ( u10_ && x >= 0.5 && y < 0.5 ) \
      || ( u01_ && x < 0.5 && y >= 0.5 ) \
      || ( u11_ && x >= 0.5 && y >= 0.5 ) ) ) \
	return mUdf(ValT)



template <class PosT,class ValT> inline
ValT LinearReg2DWithUdf<PosT,ValT>::apply( PosT x, PosT y ) const
{
    mRetUdfIfNearestUdf();

    return intp_.apply( x, y );
}


//--- PolyReg2D Implementation


template <class PosT,class ValT> inline
PolyReg2D<PosT,ValT>::PolyReg2D( float xs )
    : xs_(xs)
{}


template <class PosT,class ValT> inline
PolyReg2D<PosT,ValT>::PolyReg2D( const ValT* v, float xs )
    : xs_(xs)
{
    set( v );
}


template <class PosT,class ValT> inline
PolyReg2D<PosT,ValT>::PolyReg2D( ValT vm10, ValT vm11,
	         ValT v0m1, ValT v00,  ValT v01, ValT v02,
	         ValT v1m1, ValT v10,  ValT v11, ValT v12,
		         ValT v20,  ValT v21, float xs )
    : xs_(xs)
{
    set( vm10, vm11, v0m1, v00, v01, v02, v1m1, v10, v11, v12, v20, v21 );
}


template <class PosT,class ValT> inline
void PolyReg2D<PosT,ValT>::set( const ValT* v )
{
    if ( !mIsUdf(-v[4]) )
	set( v[4], v[5], v[6], v[0], v[1], v[7], v[8], v[2], v[3],
	     v[9], v[10], v[11] );
    else
	set( v[0], v[1], v[0], v[0], v[1], v[1], v[2], v[2], v[3],
	     v[3], v[2], v[3] );
}


template <class PosT,class ValT> inline
void PolyReg2D<PosT,ValT>::set(  ValT vm10, ValT vm11,
		 ValT v0m1, ValT v00,  ValT v01, ValT v02,
		 ValT v1m1, ValT v10,  ValT v11, ValT v12,
			 ValT v20,  ValT v21 )
{
    vm10_ = vm10; v0m1_ = v0m1; v20_ = v20; v02_ = v02;
    delxm1_ = vm11 - vm10; delym1_ = v1m1 - v0m1;
    delx2_ = v21 - v20; dely2_ = v12 - v02;
    ix0_.set( v0m1, v00, v01, v02 ); ix1_.set( v1m1, v10, v11, v12 );
    iy0_.set( vm10, v00, v10, v20 ); iy1_.set( vm11, v01, v11, v21 );
}


template <class PosT,class ValT> inline
ValT PolyReg2D<PosT,ValT>::apply( PosT x, PosT y ) const
{
    // Exactly on border or outside: handle now
    if ( x <= 0 ) return ix0_.apply( y );
    else if ( y <= 0 ) return iy0_.apply( x );
    else if ( x >= 1 ) return ix1_.apply( y );
    else if ( y >= 1 ) return iy1_.apply( x );

    // Values on X-line through point
    const ValT vxm1 = vm10_ + delxm1_ * y;
    const ValT vx0 = ix0_.apply( y );
    const ValT vx1 = ix1_.apply( y );
    const ValT vx2 = v20_ + delx2_ * y;

    // Values on Y-line through point
    const ValT vym1 = v0m1_ + delym1_ * x;
    const ValT vy0 = iy0_.apply( x );
    const ValT vy1 = iy1_.apply( x );
    const ValT vy2 = v02_ + dely2_ * x;

    // Result is weighted average, weight dep on distance from border
    const ValT estx = polyReg1D( vxm1, vx0, vx1, vx2, x );
    const ValT esty = polyReg1D( vym1, vy0, vy1, vy2, y );
    const PosT distfromedgex = x > (PosT)(0.5) ? 1 - x : x;
    const PosT distfromedgey = y > (PosT)(0.5) ? 1 - y : y;
    const float wtx = (float)(distfromedgey);
    const float wty = (float)(distfromedgex * xs_);
    return (wtx * estx + wty * esty) / (wtx + wty);
}



//--- PolyReg2DWithUdf Implementation

template <class PosT,class ValT> inline
PolyReg2DWithUdf<PosT,ValT>::PolyReg2DWithUdf( float xs )
    : intp_(xs)
{
}


template <class PosT,class ValT> inline
PolyReg2DWithUdf<PosT,ValT>::PolyReg2DWithUdf( const ValT* v, float xs )
    : intp_(xs)
{
    set( v );
}


template <class PosT,class ValT> inline
PolyReg2DWithUdf<PosT,ValT>::PolyReg2DWithUdf( ValT vm10, ValT vm11, ValT v0m1,
	ValT v00, ValT v01, ValT v02, ValT v1m1, ValT v10, ValT v11, ValT v12,
	ValT v20, ValT v21, float xs )
    : intp_(xs)
{
    set( vm10, vm11, v0m1, v00, v01, v02, v1m1, v10, v11, v12, v20, v21 );
}


template <class PosT,class ValT> inline
void PolyReg2DWithUdf<PosT,ValT>::set( const ValT* v )
{
    if ( !mIsUdf(-v[4]) )
	set( v[4], v[5], v[6], v[0], v[1], v[7], v[8], v[2], v[3],
	     v[9], v[10], v[11] );
    else
	set( v[0], v[1], v[0], v[0], v[1], v[1], v[2], v[2], v[3],
	     v[3], v[2], v[3] );
}


template <class PosT,class ValT> inline
void PolyReg2DWithUdf<PosT,ValT>::fillOuter2Inner( ValT vm10, ValT vm11,
	ValT v0m1, ValT v02, ValT v1m1, ValT v12, ValT v20, ValT v21, ValT& v00,
	ValT& v01, ValT& v10, ValT& v11 )
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

#undef mFillWithEither
}


template <class PosT,class ValT> inline
void PolyReg2DWithUdf<PosT,ValT>::fillInner2Inner( ValT& v00, ValT& v01,
			    ValT& v10, ValT& v11 )
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


template <class PosT,class ValT> inline
void PolyReg2DWithUdf<PosT,ValT>::fillInner2Outer( ValT v00, ValT v01, ValT v10,
	ValT v11, ValT& vm10, ValT& vm11, ValT& v0m1, ValT& v02,
	ValT& v1m1, ValT& v12, ValT& v20, ValT& v21 )
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
#   undef mFillIfUdf
}


template <class PosT,class ValT> inline
void PolyReg2DWithUdf<PosT,ValT>::set( ValT vm10, ValT vm11,
		       ValT v0m1, ValT v00,  ValT v01, ValT v02,
		       ValT v1m1, ValT v10,  ValT v11, ValT v12,
				  ValT v20,  ValT v21 )
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
			     v00, v01, v10, v11 );
	    fillInner2Inner( v00, v01, v10, v11 );
	}
	fillInner2Outer( v00, v01, v10, v11,
			 vm10, vm11, v0m1, v02, v1m1, v12, v20, v21 );
    }

    intp_.set( vm10, vm11, v0m1, v00, v01, v02, v1m1, v10, v11, v12, v20, v21 );
}

template <class PosT,class ValT> inline
ValT PolyReg2DWithUdf<PosT,ValT>::apply( PosT x, PosT y ) const
{
    mRetUdfIfNearestUdf();

    return intp_.apply( x, y );
}


#undef mFillIfUdfFromSquare
#undef mRetUdfIfNearestUdf


}// namespace Interpolate
