#ifndef simpnumer_H
#define simpnumer_H

/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		12-4-1999
 Contents:	Simple numerical functions
 RCS:		$Id: simpnumer.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
________________________________________________________________________

*/

#include <general.h>


//
// sort quickly (algorithm taken from xv).
//

template <class T>
inline void sort_array( T* arr, int sz )
{
    for ( int d=sz/2; d>0; d=d/2 )
	for ( int i=d; i<sz; i++ )
	    for ( int j=i-d; j>=0 && arr[j]>arr[j+d]; j-=d )
		Swap( arr[j], arr[j+d] );
}


//
// sort_coupled: sort and remember where it was before sorting.
//

template <class T>
inline void sort_coupled( T* arr, int* idxs, int sz )
{
    for ( int d=sz/2; d>0; d=d/2 )
	for ( int i=d; i<sz; i++ )
	    for ( int j=i-d; j>=0 && arr[j]>arr[j+d]; j-=d )
	    {
		Swap(arr[j],arr[j+d]);
		Swap(idxs[j],idxs[j+d]);
	    }
}


//
// linearInterpolate: regular sampling/generalised
//

// Interpolate linearly when the relative distance from first point is known.
// Usually occurs in sampled arrays. Also usually, 0 < pos < 1.
//

template <class T>
inline T linearInterpolate( T y0, T y1, float pos )
{
    return pos*y1 + (1-pos)*y0;
}


// Interpolate linearly when two points are known.
// Make sure these points are not at the same posistion (crash!).
//

template <class T>
inline T linearInterpolate( float x0, T y0, float x1, T y1, float x )
{
    return y0 + (x-x0) * (y1-y0) / (x1-x0);
} 


//
// polyInterpolate: regular sampling/generalised
//

// Interpolate regularly sampled. Position is usually between y1 and y2.
// Then, 0 < pos < 1.
//

template <class T>
inline T polyInterpolate( T y0, T y1, T y2, T y3, float pos )
{
    T b = (( y2 + y0 ) / 2) - y1;
    T c = y2 - (( 2*y0 + 3*y1 + y3 ) / 6);
    T a = (( y2 - y0 ) / 2) - c;

    float possq = pos * pos;
    return possq * pos * a + possq * b + pos * c + y1;
}


// Interpolate when 4 points are known.
// Make sure none of the positions are the same. Will just crash 'silently'.
//

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


//
// Position-sorted indexable objects.
// These are objects that return a value of type T when the [] operator is
// applied. Can range from simple arrays and TypeSets to whatever supports
// the [] operator. The goal is to interpolate between the values. Therefore,
// the position of the values must be know from either the fact that the
// values are regular samples or by specifying another indexable object that
// provides the positions (in float or double).
//


#define mIsZero(x) ((x)<1e-30&&(x)>-1e-30)

//
// Find index of nearest point below a given position.
// The 'x' must return the positions.
// The return value may be -1, which means that 'pos' is before the first
// position.
//

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

//
// Irregular interpolation.
// The 'x' must return the X-positions of the 'y' values.
//

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


//
// Regular interpolation.
//

template <class T,class RT>
inline void interpolateSampled( const T& idxabl, int sz, float pos, RT& ret,
				bool extrapolate=NO,
				RT undefval=(RT)mUndefValue )
{
    int intpos = mNINT( pos );
    float dist = pos - intpos;
    if( mIsZero(dist) && intpos >= 0 && intpos < sz ) 
	{ ret = idxabl[intpos]; return; }

    int prevpos = dist > 0 ? intpos : intpos - 1;
    if ( !extrapolate && (prevpos > sz-2 || prevpos < 0) )
	ret = undefval;
    else if ( prevpos < 1 )
	ret = linearInterpolate( idxabl[0], idxabl[1], pos );
    else if ( prevpos > sz-3 )
	ret = linearInterpolate( idxabl[sz-2], idxabl[sz-1], pos-(sz-2) );
    else
	ret = polyInterpolate( (RT)idxabl[prevpos-1], 
			       (RT)idxabl[prevpos], 
			       (RT)idxabl[prevpos+1],
			       (RT)idxabl[prevpos+2], pos - prevpos );
}


template <class T>
inline float interpolateSampled( const T& idxabl, int sz, float pos,
				 bool extrapolate=NO,
				 float undefval=mUndefValue )
{
    float ret = undefval;
    interpolateSampled( idxabl, sz, pos, ret, extrapolate, undefval );
    return ret;
}


//
// Taper an indexable array from 1 to taperfactor. If lowpos is less 
// than highpos, the samples array[0] to array[lowpos] will be set to zero. 
// If lowpos is more than highpos, the samples array[lowpos]  to array[sz-1]
// will be set to zero. The taper can be either cosine or linear.
//

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


//
// Gradient from a series of 4 points, sampled like:
// x:  -2  -1  0  1  2
// y: ym2 ym1 y0 y1 y2
// The gradient estimation is done at x=0. y0 is generally not needed, but it
// will be used if there are one or more undefineds.
// The function will return mUndefValue if there are too many missing values.
//
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


#endif
