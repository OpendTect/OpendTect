#ifndef simpnumer_H
#define simpnumer_H

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Bert BRil & Kris Tingdahl
 Date:		12-4-1999
 Contents:	'Simple' numerical functions
 RCS:		$Id: simpnumer.h,v 1.7 2000-11-13 08:54:21 bert Exp $
________________________________________________________________________

*/

#include <general.h>
#include <math.h>

/*!
 linearInterpolate: regular sampling/generalised
*/

/*!>
 Interpolate linearly when the relative distance from first point is known.
 Usually occurs in sampled arrays. Also usually, 0 < pos < 1.
*/

template <class T>
inline T linearInterpolate( T y0, T y1, float pos )
{
    return pos*y1 + (1-pos)*y0;
}


/*!>
 Interpolate linearly when two points are known.
 Make sure these points are not at the same posistion (crash!).
*/

template <class T>
inline T linearInterpolate( float x0, T y0, float x1, T y1, float x )
{
    return y0 + (x-x0) * (y1-y0) / (x1-x0);
} 


/*!
 polyInterpolate: regular sampling/generalised
*/

/*!>
 Interpolate regularly sampled. Position is usually between y1 and y2.
 Then, 0 < pos < 1.
*/

template <class T>
inline T polyInterpolate( T y0, T y1, T y2, T y3, float pos )
{
    T b = (( y2 + y0 ) / 2) - y1;
    T c = y2 - (( 2*y0 + 3*y1 + y3 ) / 6);
    T a = (( y2 - y0 ) / 2) - c;

    float possq = pos * pos;
    return possq * pos * a + possq * b + pos * c + y1;
}


/*!>
 Interpolate when 4 points are known.
 Make sure none of the positions are the same. Will just crash 'silently'.
 No undefined values allowed.
*/

template <class T>
inline T polyInterpolate( float x0, T y0, float x1, T y1, float x2, T y2, 
			  float x3, T y3, float x )
{
    float xx0 = x - x0, xx1 = x-x1, xx2 = x-x2, xx3 = x-x3;
    return 	y0 * xx1 * xx2 * xx3 / ((x0 - x1) * (x0 - x2) * (x0 - x3)) +
		y1 * xx0 * xx2 * xx3 / ((x1 - x0) * (x1 - x2) * (x1 - x3)) +
		y2 * xx0 * xx1 * xx3 / ((x2 - x0) * (x2 - x1) * (x2 - x3)) +
		y3 * xx0 * xx1 * xx2 / ((x3 - x0) * (x3 - x1) * (x3 - x2));
}


/*!>
 Interpolate sampled when 4 points are known, but some may be undefined.
 If you need to do this irregularly, you can just remove those points
 and call the apropriate interpolation.
/*

template <class T>
inline T polyInterpolateWithUdf( T y0, T y1, T y2, T y3, float x )
{
    static bool udf[4];
    udf[0] = mIsUndefined(y0); udf[1] = mIsUndefined(y1);
    udf[2] = mIsUndefined(y2); udf[3] = mIsUndefined(y3);

    int nrudf = udf[0] ? 1 : 0;
    if ( udf[1] ) nrudf++; if ( udf[2] ) nrudf++; if ( udf[3] ) nrudf++;

    if ( nrudf )
    {
	if ( nrudf > 2 ) return mUndefValue;

	float pos[3]; T* v[3]; int ipos = 0;
	if ( !udf[0] ) { pos[ipos] = -1; v[ipos] = &y0; ipos++; }
	if ( !udf[1] ) { pos[ipos] = 0; v[ipos] = &y1; ipos++; }
	if ( !udf[2] ) { pos[ipos] = 1; v[ipos] = &y2; ipos++; }
	if ( !udf[3] ) { pos[ipos] = 2; v[ipos] = &y3; }

	return nrudf == 2 ?
	linearInterpolate( pos[0], *v[0], pos[1], *v[1], x )
      : parabolicInterpolate( pos[0], *v[0], pos[1], *v[1], pos[2], *v[2], x );
    }

    return polyInterpolate( y0, y1, y2, y3, x );
}


/*!>
 Interpolate when 3 points are known.
 Make sure none of the positions are the same. Will just crash 'silently'.
*/

template <class T>
inline T parabolicInterpolate( float x0, T y0, float x1, T y1, float x2, T y2, 
			       float x )
{
    float xx0 = x - x0, xx1 = x-x1, xx2 = x-x2;
    return 	y0 * xx1 * xx2 / ((x0 - x1) * (x0 - x2)) +
		y1 * xx0 * xx2 / ((x1 - x0) * (x1 - x2)) +
		y2 * xx0 * xx1 / ((x2 - x0) * (x2 - x1));
}

/*!>
   polyInterpolate2D - interpolates a value inside a 4*4 grid. The notation
   of the vaues are: "v"xy. I.e v00 is the value at (0,0), v13 is the value
   at (1,3). Ideally, x and y should lie within the square defined by (1,1),
   (1,2), (2,1) and (2,2).
*/

template <class T>
inline T polyInterpolate2D( T v00, T v01, T v02, T v03,
			    T v10, T v11, T v12, T v13,
			    T v20, T v21, T v22, T v23,
			    T v30, T v31, T v32, T v33, float x, float y)
{
    T v0 = polyInterpolate( v00, v01, v02, v03, y );
    T v1 = polyInterpolate( v10, v11, v12, v13, y );
    T v2 = polyInterpolate( v20, v21, v22, v23, y );
    T v3 = polyInterpolate( v30, v31, v32, v33, y );

    return polyInterpolate( v0, v1, v2, v3, x );
}


/*!>
   linearInterpolate2D - interpolates a value inside a 1*1 grid. The notation
   of the vaues are: "v"xy. I.e v00 is the value at (0,0), v13 is the value
   at (1,3). Ideally, x and y should lie within the square defined by the
   input square.
*/


template <class T>
inline T linearInterpolate2D( T v00, T v01, T v10, T v11, float x, float y )
{
    const float xm = 1 - x;
    const float ym = 1 - y;

    return xm*ym*v00 + x*ym*v10 + x*y*v11 + xm*y*v01;
}
    
/*!>
   linearInterpolate3D - interpolates a value inside a 1*1*1 cube. The notation
   of the vaues are: "v"xyz. I.e v000 is the value at (0,0,0), v132 is the value
   at (1,3,2). Ideally, x and y should lie within the cube defined by the input
   values.
*/

template <class T>
inline T linearInterpolate3D( T v000, T v001, T v010, T v011,
			      T v100, T v101, T v110, T v111,
			      float x, float y, float z )
{
    const float xm = 1 - x;
    const float ym = 1 - y;
    const float zm = 1 - z;

    return xm*ym*zm*v000 + xm*ym*z*v001 + xm*y*zm*v010 + xm*y*z*v011 +
	   x*ym*zm*v100 + x*ym*z*v101 + x*y*zm*v110 + x*y*z*v111;
}
    
/*!>
   polyInterpolate3D - interpolates a value inside a 4*4*4 cube. The notation
   of the vaues are: "v"xyz. I.e v000 is the value at (0,0,0), v132 is the value
   at (1,3,2). Ideally, x and y should lie within the cube defined by (1,1,1),
   (1,2,1), (2,1,1), (2,2,2), (1,1,2), (1,2,2), (2,1,2) and (2,2,2).
*/

template <class T>
inline T polyInterpolate3D( T v000, T v001, T v002, T v003,
			    T v010, T v011, T v012, T v013,
			    T v020, T y021, T v022, T v023,
			    T v030, T v031, T v032, T v033,

			    T v100, T v101, T v102, T v103,
			    T v110, T v111, T v112, T v113,
			    T v120, T y121, T v122, T v123,
			    T v130, T v131, T v132, T v133,

			    T v200, T v201, T v202, T v203,
			    T v210, T v211, T v212, T v213,
			    T v220, T y221, T v222, T v223,
			    T v230, T v231, T v232, T v233,

			    T v300, T v301, T v302, T v303,
			    T v310, T v311, T v312, T v313,
			    T v320, T y321, T v322, T v323,
			    T v330, T v331, T v332, T v333,
			    float x, float y, float z )
{
    T val0 = polyInterpolate2D( v000, v001, v002, v003,
				v010, v011, v012, v013,
				v020, y021, v022, v023,
				v030, v031, v032, v033, y, z );

    T val1 = polyInterpolate2D(	v100, v101, v102, v103,
				v110, v111, v112, v113,
				v120, y121, v122, v123,
				v130, v131, v132, v133, y, z );

    T val2 = polyInterpolate2D(	v200, v201, v202, v203,
				v210, v211, v212, v213,
				v220, y221, v222, v223,
				v230, v231, v232, v233, y, z );

    T val3 = polyInterpolate2D(	v300, v301, v302, v303,
				v310, v311, v312, v313,
				v320, y321, v322, v323,
				v330, v331, v332, v333, y, z );


    return polyInterpolate( val0, val1, val2, val3, x );
}


/*!
 Position-sorted indexable objects.
 These are objects that return a value of type T when the [] operator is
 applied. Can range from simple arrays and TypeSets to whatever supports
 the [] operator. The goal is to interpolate between the values. Therefore,
 the position of the values must be known from either the fact that the
 values are regular samples or by specifying another indexable object that
 provides the positions (in float or double).
*/


#define mIsZero(x) ((x)<1e-30&&(x)>-1e-30)

/*!>
 Find index of nearest point below a given position.
 The 'x' must return the positions.
 The return value may be -1, which means that 'pos' is before the first
 position.
*/

template <class X>
inline int getLowIndex( const X& x, int sz, double pos )
{
    if ( pos < x[0] ) return -1;
    if ( x[sz-1] < pos ) return sz-1;

    int idx1 = 0;
    int idx2 = sz-1;
    while ( idx2 - idx1 > 1 )
    {
	int newidx = (idx2 + idx1) / 2;
	double xval = x[newidx];
	double diff = pos - xval;
	if ( mIsZero(diff) ) return newidx;
	else if ( diff > 0 ) idx1 = newidx;
	else		     idx2 = newidx;
    }
    return idx1;
}

/*!>
 Irregular interpolation.
 The 'x' must return the X-positions of the 'y' values.
*/

template <class X,class Y,class RT>
inline void interpolatePositioned( const X& x, const Y& y, int sz,
				   float desiredx, RT& ret,
				   bool extrapolate=NO,
				   RT undefval=(RT)mUndefValue )
{
    if ( sz < 1 )
	ret = undefval;
    else if ( sz == 1 )
	ret = extrapolate ? y[0] : undefval;
    else if ( sz == 2 )
	ret = linearInterpolate( x[0], y[0], x[1], y[1], desiredx );
    else if ( desiredx < x[0] || desiredx > x[sz-1] )
    { if ( !extrapolate )
	    ret = undefval;
	else
	    ret = desiredx < x[0]
		? linearInterpolate( x[0], y[0], x[1], y[1], desiredx )
		: linearInterpolate( x[sz-2], y[sz-2], x[sz-1], y[sz-1],
					 desiredx );
    }
    else
    {
	int prevpos = getLowIndex( x, sz, desiredx );
	int nextpos = prevpos + 1;

	if ( sz == 3 )
	    ret = linearInterpolate( x[prevpos], y[prevpos], 
				     x[nextpos], y[nextpos], desiredx );
	else
	{
	    if ( prevpos == 0 )
		{ prevpos++; nextpos++; } 
	    else if ( nextpos == sz-1 )
		{ prevpos--; nextpos--; } 

	    ret = polyInterpolate( x[prevpos-1], y[prevpos-1],
				   x[prevpos], y[prevpos],
				   x[nextpos], y[nextpos],
				   x[nextpos+1], y[nextpos+1],
				   desiredx );
	}
    }
}

template <class X,class Y>
inline float interpolatePositioned( const X& x, const Y& y, int sz, float pos, 
				    bool extrapolate=NO,
				    float undefval=mUndefValue )
{
    float ret = undefval;
    interpolatePositioned( x, y, sz, pos, ret, extrapolate, undefval );
    return ret;
}


/*!>
 Regular interpolation.
*/

template <class T,class RT>
inline void interpolateSampled( const T& idxabl, int sz, float pos, RT& ret,
				bool extrapolate=NO,
				RT undefval=(RT)mUndefValue,
				float snapdist=mEPSILON )
{
    if ( !extrapolate && pos < -snapdist || pos > sz - 1 + snapdist )
	{ ret = undefval; return; }

    const int intpos = mNINT( pos );
    const float dist = pos - intpos;
    if( dist > -snapdist && dist < snapdist && intpos > -1 && intpos < sz ) 
	{ ret = idxabl[intpos]; return; }

    const int prevpos = dist > 0 ? intpos : intpos - 1;
    if ( prevpos < 1 )
	ret = linearInterpolate( (RT)idxabl[0], (RT)idxabl[1], pos );
    else if ( prevpos > sz-3 )
	ret = linearInterpolate( (RT)idxabl[sz-2], (RT)idxabl[sz-1],pos-(sz-2));
    else
	ret = polyInterpolate( (RT)idxabl[prevpos-1], 
			       (RT)idxabl[prevpos], 
			       (RT)idxabl[prevpos+1],
			       (RT)idxabl[prevpos+2], pos - prevpos );
}


template <class T>
inline float interpolateSampled( const T& idxabl, int sz, float pos,
				 bool extrapolate=NO,
				 float undefval=mUndefValue,
				 float snapdist=mEPSILON )
{
    float ret = undefval;
    interpolateSampled( idxabl, sz, pos, ret, extrapolate, undefval, snapdist );
    return ret;
}


/*!>
 dePeriodize returns a periodic (defined by y(x) = y(x) + N * P) value's value
 in the functions first period (between 0 and P).
*/

template <class T>
inline T dePeriodize( T val, T period )
{
    int n = (int) (val / period);
    if ( val < 0 ) n--;

    return n ? val - n * period : val; 
}

/*!>
 intpow returns the integer power of an arbitary value. Faster than
 pow( double, double ), more general than IntPowerOf(double,int).
*/

template <class T> inline
T intpow( T x, char y)
{
    T res = 1; while ( y ) { res *= x; y--; }
    return res;
}


/*!>
 interpolateYPeriodicSampled interpolates in an indexable storage with
 periodic entities ( defined by y(x) = y(x) + N*P )
*/

template <class T, class RT>
inline void interpolateYPeriodicSampled( const T& idxabl, int sz, float pos,
				RT& ret, RT period,
				bool extrapolate=NO,
				RT undefval=(RT)mUndefValue )
{
    const float halfperiod = period / 2;
    int intpos = mNINT( pos );
    float dist = pos - intpos;
    if( mIsZero(dist) && intpos >= 0 && intpos < sz ) 
	{ ret = idxabl[intpos]; return; }

    int prevpos = dist > 0 ? intpos : intpos - 1;
    if ( !extrapolate && (prevpos > sz-2 || prevpos < 0) )
	ret = undefval;
    else if ( prevpos < 1 )
    {
	const float val0 = idxabl[0];
	RT val1 = idxabl[1];
	while ( val1 - val0 > halfperiod ) val1 -= period; 
	while ( val1 - val0 < -halfperiod ) val1 += period; 

	ret = dePeriodize(linearInterpolate( val0, val1, pos ), period );
    }
    else if ( prevpos > sz-3 )
    {
	const RT val0 = idxabl[sz-2];
	RT val1 = idxabl[sz-1];
	while ( val1 - val0 > halfperiod ) val1 -= period; 
	while ( val1 - val0 < -halfperiod ) val1 += period; 
	ret = dePeriodize(linearInterpolate( val0, val1, pos-(sz-2) ), period );
    }
    else
    {
	const RT val0 = idxabl[prevpos-1];

	RT val1 = idxabl[prevpos];
	while ( val1 - val0 > halfperiod ) val1 -= period; 
	while ( val1 - val0 < -halfperiod ) val1 += period; 

	RT val2 = idxabl[prevpos+1];
	while ( val2 - val1 > halfperiod ) val2 -= period; 
	while ( val2 - val1 < -halfperiod ) val2 += period; 

	RT val3 = idxabl[prevpos+2];
	while ( val3 - val2 > halfperiod ) val3 -= period; 
	while ( val3 - val2 < -halfperiod ) val3 += period; 

	ret = dePeriodize(polyInterpolate( val0, val1, val2, val3,
			  pos - prevpos ), period );
    }
}


template <class T>
inline float interpolateYPeriodicSampled( const T& idxabl, int sz, float pos,
                                 float period, bool extrapolate=NO,
                                 float undefval=mUndefValue )
{
    float ret = undefval;
    interpolateYPeriodicSampled( idxabl, sz, pos, ret,
				 period, extrapolate, undefval );
    return ret;
}


/*!>
 interpolateXPeriodicSampled interpolates in an periodic indexable ( where
 the position is periodic ), defined by y(x) = x(x+n*P). The period is equal
 to the size of the given idxabl.
*/

template <class T, class RT>
inline void interpolateXPeriodicSampled( const T& idxabl, int sz, float pos,
					RT& ret)
{
    int intpos = mNINT( pos );
    float dist = pos - intpos;
    if( mIsZero(dist) && intpos >= 0 && intpos < sz ) 
	{ ret = idxabl[intpos]; return; }

    int prevpos = dist > 0 ? intpos : intpos - 1;
    const float relpos = pos - prevpos;
    prevpos = dePeriodize( prevpos, sz );

    int prevpos2 = prevpos - 1; 
    prevpos2 = dePeriodize( prevpos2, sz );

    const int nextpos = prevpos + 1;
    nextpos = dePeriodize( nextpos, sz );

    const int nextpos2 = prevpos + 2;
    nextpos2 = dePeriodize( nextpos2, sz );

    const RT prevval2 = idxabl[prevpos2];
    const RT prevval = idsabl[prevpos];
    const RT nextval = idxabl[nextpos];
    const RT nextval2 = idxabl[nextpos2];

    ret = polyInterpolate( prevval2, 
			   prevval, 
			   nextval,
			   nextval2, relpos );
}


/*!>
 Taper an indexable array from 1 to taperfactor. If lowpos is less 
 than highpos, the samples array[0] to array[lowpos] will be set to zero. 
 If lowpos is more than highpos, the samples array[lowpos]  to array[sz-1]
 will be set to zero. The taper can be either cosine or linear.
*/

class Taper
{
public:
    enum Type { Cosine, Linear };
};


template <class T>
bool taperArray( T* array, int sz, int lowpos, int highpos, Taper::Type type )
{
    if ( lowpos  >= sz || lowpos  < 0 ||
         highpos >= sz || highpos < 0 ||
	 highpos == lowpos )
	return NO;

    int pos = lowpos < highpos ? 0 : sz - 1 ;
    int inc = lowpos < highpos ? 1 : -1 ;

    while ( pos != lowpos )
    {
	array[pos] = 0;
	pos += inc;
    }

    int tapersz = abs( highpos - lowpos ); 
    float taperfactor = type == Taper::Cosine ? M_PI : 0;
    float taperfactorinc = ( type == Taper::Cosine ? M_PI : 1 ) / tapersz;
				

    while ( pos != highpos )
    {
	array[pos] *= type == Taper::Cosine ? .5 + .5 * cos( taperfactor ) 
					    : taperfactor;
	pos += inc;
	taperfactor += taperfactorinc;
    }

    return YES;
}


/*!>
 Gradient from a series of 4 points, sampled like:
 x:  -2  -1  0  1  2
 y: ym2 ym1 y0 y1 y2
 The gradient estimation is done at x=0. y0 is generally not needed, but it
 will be used if there are one or more undefineds.
 The function will return mUndefValue if there are too many missing values.
*/
template <class T>
inline T sampledGradient( T ym2, T ym1, T y0, T y1, T y2 )
{
    bool um1 = mIsUndefined(ym1), u0 = mIsUndefined(y0), u1 = mIsUndefined(y1);

    if ( mIsUndefined(ym2) || mIsUndefined(y2) )
    {
        if ( um1 || u1 )
	    { if ( !u0 && !(um1 && u1) ) return um1 ? y1 - y0 : y0 - ym1; }
        else
            return (y1-ym1) * .5;
    }
    else if ( (um1 && u1) || (u0 && (um1 || u1)) )
        return (y2-ym2) * .25;
    else if ( um1 || u1 )
    {
        return um1 ? ((16*y1)   - 3*y2  - ym2) / 12 - y0
                   : ((-16*ym1) + 3*ym2 + y2)  / 12 + y0;
    }
    else
        return (8 * ( y1 - ym1 ) - y2 + ym2) / 12;

    return mUndefValue;
}


template <class X, class Y, class RT>
inline void getGradient( const X& x, const Y& y, int sz, int firstx, int firsty,
		  RT* gradptr=0, RT* interceptptr=0 )
{
    RT xy_sum = 0, x_sum=0, y_sum=0, xx_sum=0;

    for ( int idx=0; idx<sz; idx++ )
    {
	RT xval = x[idx+firstx];
	RT yval = y[idx+firsty];

	x_sum += xval;
	y_sum += yval;
	xx_sum += xval*xval;
	xy_sum += xval*yval;
    }

    RT grad = ( sz*xy_sum - x_sum*y_sum ) / ( sz*xx_sum - x_sum*x_sum );
    if ( gradptr ) *gradptr = grad;

    if ( interceptptr ) *interceptptr = (y_sum - grad*x_sum)/sz;
}


template <class X>
inline float variance( const X& x, int sz )
{
    if ( sz < 2 ) return mUndefValue;

    float sum=0;
    float sqsum=0;

    for ( int idx=0; idx<sz; idx++ )
    {
	float val = x[idx];

	sum += val;
	sqsum += val*val;
    }

    return (sqsum - sum * sum / sz)/ (sz -1);
}


/*!>
solve3DPoly - finds the roots of the equation 

    z^3+a*z^2+b*z+c = 0

solve3DPoly returns the number of real roots found.

Algorithms taken from NR, page 185.

*/

inline int solve3DPoly( double a, double b, double c, 
			 double& root0,
			 double& root1,
			 double& root2 )
{
    const double a2 = a*a;
    const double q = (a2-3*b)/9;
    const double r = (2*a2*a-9*a*b+27*c)/54;
    const double q3 = q*q*q;
    const double r2 = r*r;
    

    const double minus_a_through_3 = -a/3;

    if ( r2<q3 )
    {
	const double minus_twosqrt_q = -2*sqrt(q);
	const double theta = acos(r/sqrt(q3));
	static const double twopi = 2*M_PI;


	root0 = minus_twosqrt_q*cos(theta/3)+minus_a_through_3; 
	root1=minus_twosqrt_q*cos((theta-twopi)/3)+minus_a_through_3;
	root2=minus_twosqrt_q*cos((theta+twopi)/3)+minus_a_through_3;
	return 3;
    }

    const double A=(r>0?-1:1)*pow(fabs(r)+sqrt(r2-q3),1/3);
    const double B=mIS_ZERO(A)?0:q/A;

    root0 = A+B+minus_a_through_3;

    /*!
    The complex roots can be calculated as follows:
    static const double sqrt3_through_2 = sqrt(3)/2;

    root1 = complex_double( 0.5*(A+B)+minus_a_through_3,
			   sqrt3_through_2*(A-B));

    root2 = complex_double( 0.5*(A+B)+minus_a_through_3,
			   -sqrt3_through_2*(A-B));
    */
			     

    return 1;
}


#endif
