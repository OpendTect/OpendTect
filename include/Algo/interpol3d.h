#ifndef interpol3d_h
#define interpol3d_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2006
 RCS:		$Id$
________________________________________________________________________

*/

#include "undefval.h"


namespace Interpolate
{

/*!
\ingroup Algo
\brief Linear 3D interpolation.
*/

template <class T>
class LinearReg3D
{
public:

LinearReg3D()	{}

LinearReg3D( const T* const* const* v )
{
    set( v[0][0][0], v[1][0][0], v[0][1][0], v[1][1][0],
	 v[0][0][1], v[1][0][1], v[0][1][1], v[1][1][1] );
}

LinearReg3D( T v000, T v100, T v010, T v110, T v001, T v101, T v011, T v111 )
{
    set( v000, v100, v010, v110, v001, v101, v011, v111 );
}

inline void set( T v000, T v100, T v010, T v110, T v001, T v101, T v011, T v111)
{
    a_[0] = v000;
    a_[1] = v100 - v000;
    a_[2] = v010 - v000;
    a_[3] = v001 - v000;
    a_[4] = v110 + v000 - v100 - v010;
    a_[5] = v101 + v000 - v100 - v001;
    a_[6] = v011 + v000 - v010 - v001;
    a_[7] = v111 + v100 + v010 + v001 - (v000 + v110 + v101 + v011);
}

inline T apply( float x, float y, float z ) const
{
    return a_[0] + a_[1] * x + a_[2] * y + a_[3] * z
		 + a_[4] * x * y + a_[5] * x * z + a_[6] * y * z
		 + a_[7] * x * y * z;
}

    T	a_[8];

};


template <class T>
inline T linearReg3D( T v000, T v100, T v010, T v110,
		      T v001, T v101, T v011, T v111,
		      float x, float y, float z )
{
    return LinearReg3D<T>( v000, v100, v010, v110, v001, v101, v011, v111 )
		.apply( x, y, z );
}


/*!
\ingroup Algo
\brief Linear 3D interpolation with standard undef handling.
*/

template <class T>
class LinearReg3DWithUdf
{
public:

LinearReg3DWithUdf()	{}

LinearReg3DWithUdf( const T* const* const* v )
{
    set( v[0][0][0], v[1][0][0], v[0][1][0], v[1][1][0],
	 v[0][0][1], v[1][0][1], v[0][1][1], v[1][1][1] );
}

LinearReg3DWithUdf( T v000, T v100, T v010, T v110,
		    T v001, T v101, T v011, T v111 )
{
    set( v000, v100, v010, v110, v001, v101, v011, v111 );
}

inline static T getReplVal( T v1, T v2, T v3, bool u1, bool u2, bool u3 )
{

    if ( u1 )
	return u2 ? v3 : (u3 ? v2 : (v2 + v3) / 2);
    else if ( u2 )
	return u3 ? v1 : (v1 + v3) / 2;
    else if ( u3 )
	return (v1 + v2) / 2;

    return (v1 + v2 + v3) / 3;
}

inline void set( T v000, T v100, T v010, T v110, T v001, T v101, T v011, T v111)
{
    u000_ = mIsUdf(v000);
    u100_ = mIsUdf(v100);
    u010_ = mIsUdf(v010);
    u110_ = mIsUdf(v110);
    u001_ = mIsUdf(v001);
    u101_ = mIsUdf(v101);
    u011_ = mIsUdf(v011);
    u111_ = mIsUdf(v111);
    haveudf_ = u000_ || u100_ || u010_ || u110_
	    || u001_ || u101_ || u011_ || u111_;

    if ( haveudf_ )
    {
#	define mFillIfUdf(nd,nearx,neary,nearz,diagxy,diagxz,diagyz,opp) \
	if ( u##nd##_ ) \
	{ \
	    if ( !u##nearx##_ || !u##neary##_ || !u##nearz##_ ) \
		v##nd = getReplVal( v##nearx, v##neary, v##nearz, \
				    u##nearx##_, u##neary##_, u##nearz##_ ); \
	    else \
	    { \
		if ( u##diagxy##_ && u##diagxz##_ && u##diagyz##_ ) \
		    v##nd = v##opp; \
		else \
		    v##nd = getReplVal( v##diagxy, v##diagxz, v##diagyz, \
				u##diagxy##_, u##diagxz##_, u##diagyz##_ ); \
	    } \
	}

	mFillIfUdf(000,100,010,001,110,101,011,111)
	mFillIfUdf(100,000,110,101,010,001,111,011)
	mFillIfUdf(010,110,000,011,100,111,001,101)
	mFillIfUdf(110,010,100,111,000,011,101,001)
	mFillIfUdf(001,101,011,000,111,100,010,110)
	mFillIfUdf(101,001,111,100,011,000,110,010)
	mFillIfUdf(011,111,001,010,101,110,000,100)
	mFillIfUdf(111,011,101,110,001,010,100,000)

#	undef mFillIfUdf
    }

    intp_.set( v000, v100, v010, v110, v001, v101, v011, v111 );
}

inline T apply( float x, float y, float z ) const
{
    // return undef if the nearest node is undef
    if ( haveudf_ && (
         ( u000_ && x < 0.5 && y < 0.5 && z < 0.5 )
      || ( u100_ && x >= 0.5 && y < 0.5 && z < 0.5 )
      || ( u010_ && x < 0.5 && y >= 0.5 && z < 0.5 )
      || ( u110_ && x >= 0.5 && y >= 0.5 && z < 0.5 )
      || ( u001_ && x < 0.5 && y < 0.5 && z >= 0.5 )
      || ( u101_ && x >= 0.5 && y < 0.5 && z >= 0.5 )
      || ( u011_ && x < 0.5 && y >= 0.5 && z >= 0.5 )
      || ( u111_ && x >= 0.5 && y >= 0.5 && z >= 0.5 ) ) )
	return mUdf(T);

    return intp_.apply( x, y, z );
}

    LinearReg3D<T>	intp_;
    bool		haveudf_;
    bool		u000_;
    bool		u100_;
    bool		u010_;
    bool		u110_;
    bool		u001_;
    bool		u101_;
    bool		u011_;
    bool		u111_;

};

template <class T>
inline T linearReg3DWithUdf( T v000, T v100, T v010, T v110,
			     T v001, T v101, T v011, T v111,
			     float x, float y, float z )
{
    return LinearReg3DWithUdf<T>(v000,v100,v010,v110,v001,v101,v011,v111)
		.apply( x, y, z );
}



/*!
\ingroup Algo
\brief Interpolate 3D regularly sampled, using a 3rd order surface.
  
  Current implementation takes the average of the outer squares.
  In the parameter passing, the z is the fastest dimension.
  <pre>

       ..    ..       Z  Y-dir
  ..  ....  ....  ..  ^ /
  ..  ....  ....  ..  | --> X-dir
       ..    ..

  ^- From here to -^
  x=-1   0      1     2
  
  </pre>
*/

template <class T>
class PolyReg3D
{
public:

PolyReg3D() {}

PolyReg3D( const T* const* const * v )
{
    set( v[0][1][1], v[0][1][2], v[0][2][1], v[0][2][2],
	 v[1][0][1], v[1][0][2], v[1][1][0], v[1][1][1],
	 v[1][1][2], v[1][1][3], v[1][2][0], v[1][2][1],
	 v[1][2][2], v[1][2][3], v[1][3][1], v[1][3][2],
	 v[2][0][1], v[2][0][2], v[2][1][0], v[2][1][1],
	 v[2][1][2], v[2][1][3], v[2][2][0], v[2][2][1],
	 v[2][2][2], v[2][2][3], v[2][3][1], v[2][3][2],
	 v[3][1][1], v[3][1][2], v[3][2][1], v[3][2][2] );
}

PolyReg3D(
	T vm100, T vm101, T vm110, T vm111,
	T v0m10, T v0m11, T v00m1, T v000,
	T v001,  T v002,  T v01m1, T v010,
	T v011,  T v012,  T v020,  T v021,
	T v1m10, T v1m11, T v10m1, T v100,
	T v101,  T v102,  T v11m1, T v110,
	T v111,  T v112,  T v120,  T v121,
	T v200,  T v201,  T v210,  T v211 )
{
    set( vm100, vm101, vm110, vm111,
	 v0m10, v0m11, v00m1, v000,
	 v001,  v002,  v01m1, v010,
	 v011,  v012,  v020,  v021,
	 v1m10, v1m11, v10m1, v100,
	 v101,  v102,  v11m1, v110,
	 v111,  v112,  v120,  v121,
	 v200,  v201,  v210,  v211 );
}

inline void set(
	T vm100, T vm101, T vm110, T vm111,
	T v0m10, T v0m11, T v00m1, T v000,
	T v001,  T v002,  T v01m1, T v010,
	T v011,  T v012,  T v020,  T v021,
	T v1m10, T v1m11, T v10m1, T v100,
	T v101,  T v102,  T v11m1, T v110,
	T v111,  T v112,  T v120,  T v121,
	T v200,  T v201,  T v210,  T v211 )
{
    set( v000, v100, v010, v110, v001, v101, v011, v111,
	(v00m1 + v01m1 + v10m1 + v11m1) / 4,
	(v0m10 + v0m11 + v1m10 + v1m11) / 4,
	(vm100 + vm101 + vm110 + vm111) / 4,
	(v002 + v012 + v102 + v112) / 4,
	(v020 + v021 + v120 + v121) / 4,
	(v200 + v201 + v210 + v211) / 4 );
}

inline void set( T v000, T v100, T v010, T v110, T v001, T v101, T v011, T v111,
		 T vxym1, T vxzm1, T vyzm1, T vxy1, T vxz1, T vyz1 )
{
    // TODO
}

inline T apply( float x, float y, float z ) const
{
    return a_[0] + a_[1] * x + a_[2] * y + a_[3] * z
		 + a_[4] * x * y + a_[5] * x * z + a_[6] * y * z
		 + a_[7] * x * y * z
		 + a_[8]  * x * x * y + a_[9]  * x * x * z
		 + a_[10] * y * y * x + a_[11] * y * y * z
		 + a_[12] * z * z * x + a_[13] * z * z * y;
}

    T	a_[14];

};


template <class T>
inline T polyReg3D( const T* const* const * v, float x, float y, float z )
{
    return PolyReg3D<T>( v ).apply( x, y, z );
}


/*!<\brief PolyReg3D which smoothly handles undefined values

  Note that this class _requires_ x, y and z to be between 0 and 1 for correct
  undef handling. Correct means: if the nearest sample is undefined, return
  undefined. Otherwise always return a value.

*/


} // namespace Interpolate

#endif
