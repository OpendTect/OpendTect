#ifndef idxable_h
#define idxable_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert & Kris
 Date:		Mar 2006
 RCS:		$Id$
________________________________________________________________________

*/

#include "gendefs.h"
#include "interpol1d.h"
#include "mathfunc.h"
#include "sets.h"
#include "sorting.h"

/*!\brief Position-sorted indexable objects

 These are objects that return a value of type T when the [] operator is
 applied. Can range from simple arrays and TypeSets to whatever supports
 the [] operator. The goal is to interpolate between the values. Therefore,
 the position of the values must be known from either the fact that the
 values are regular samples or by specifying another indexable object that
 provides the positions (in float or double).
*/

namespace IdxAble
{

/*!>
  Find value in indexable
*/

template <class T1,class T2,class T3>
inline T3 indexOf( const T1& arr, T3 sz, const T2& val, T3 notfoundval )
{
    for ( T3 idx=0; idx<sz; idx++ )
    {
	if ( arr[idx] == val )
	    return idx;
    }
    return notfoundval;
}

/*!>
  Find value in indexable filled with pointers.
*/

template <class T1,class T2,class T3>
inline T3 derefIndexOf( const T1& arr, T3 sz, const T2& val, T3 notfoundval )
{
    for ( T3 idx=0; idx<sz; idx++ )
    {
	if ( *arr[idx] == val )
	    return idx;
    }
    return notfoundval;
}


/*!>
  Find value in sorted array of positions.
  Equality is tested by == operator -> not for float/double!
  The 'arr' indexable object must return the positions.
  The return par 'idx' may be -1, which means that 'pos' is before the first
  position.
  Return value tells whether there is an exact match. If false, index of
  array member below pos is returned.
*/

template <class T1,class T2,class T3>
bool findPos( const T1& posarr, T3 sz, T2 pos, T3 beforefirst, T3& idx )
{
    idx = beforefirst;
    if ( sz < 1 || pos < posarr[0] )
	return false;

    if ( pos == posarr[0] )
	{ idx = 0; return true; }
    else if ( pos > posarr[sz-1] || pos == posarr[sz-1] )
    	{ idx = sz-1; return pos == posarr[sz-1]; }

    T3 idx1 = 0;
    T3 idx2 = sz-1;
    while ( idx2 - idx1 > 1 )
    {
	idx = (idx2 + idx1) / 2;
	T2 arrval = posarr[idx];
	if ( arrval == pos )		return true;
	else if ( arrval > pos )	idx2 = idx;
	else				idx1 = idx;
    }

    idx = idx1;
    return posarr[idx] == pos;
}


/*!>
  Find value in sorted array of floating point positions.
  Equality is tested by mIsZero.
  The 'arr' indexable object must return the positions.
  The return par 'idx' may be beforefirst, which means that 'pos' is before
  the first position.
  Return value tells whether there is an exact match. If false, index of
  array member below pos is returned.
*/

template <class T1,class T2,class T3>
bool findFPPos( const T1& posarr, T3 sz, T2 pos, T3 beforefirst, T3& idx,
		T2 eps=mDefEps )
{
    idx = beforefirst;
    if ( !sz ) return false;
    if ( sz < 2 || pos <= posarr[0] )
    {
	if ( mIsEqual(pos,posarr[0],eps) )
	    { idx = 0; return true; }
	else
	    return false;
    }
    else if ( pos >= posarr[sz-1] )
    	{ idx = sz-1; return mIsEqual(pos,posarr[sz-1],eps); }

    T3 idx1 = 0;
    T3 idx2 = sz-1;

    while ( idx2 - idx1 > 1 )
    {
	idx = (idx2 + idx1) / 2;
	T2 diff = posarr[idx] - pos;
	if ( mIsZero(diff,eps) )		return true;
	else if ( diff > 0  )		idx2 = idx;
	else				idx1 = idx;
    }

    idx = idx1;
    return mIsEqual(pos,posarr[idx],eps);
}

/*!>
Find index of nearest point below a given position.
The 'x' must return the positions.
The return value may be -1, which means that 'pos' is before the first
position.
*/

template <class X>
inline int getLowIdx( const X& x, int sz, double pos )
{
    int idx; findFPPos( x, sz, pos, -1, idx ); return idx;
}


/*!>
 Irregular interpolation.
 The 'x' must return the X-positions of the 'y' values.
*/

template <class X,class Y,class RT>
inline void interpolatePositioned( const X& x, const Y& y, int sz,
				   float desiredx,
				   RT& ret, bool extrapolate=false )
{
    if ( sz < 1 )
	ret = mUdf(RT);
    else if ( sz == 1 )
	ret = extrapolate ? y[0] : mUdf(RT);

    else if ( sz == 2 )
	ret = Interpolate::linear1D( x[0], y[0], x[1], y[1], desiredx );
    else if ( desiredx < x[0] || desiredx > x[sz-1] )
    {
	if ( !extrapolate )
	    ret = mUdf(RT);
	else
	    ret = desiredx < x[0]
		? Interpolate::linear1D( x[0], y[0], x[1], y[1], desiredx )
		: Interpolate::linear1D( x[sz-2], y[sz-2], x[sz-1], y[sz-1],
					 desiredx );
	return;
    }

    int prevpos = getLowIdx( x, sz, desiredx );
    int nextpos = prevpos + 1;

    if ( sz == 3 )
	ret = Interpolate::linear1D( x[prevpos], y[prevpos],
				     x[nextpos], y[nextpos], desiredx );
    else
    {
	if ( prevpos == 0 )
	    { prevpos++; nextpos++; } 
	else if ( nextpos == sz-1 )
	    { prevpos--; nextpos--; } 

	ret = Interpolate::poly1D( x[prevpos-1], y[prevpos-1],
				   x[prevpos], y[prevpos],
				   x[nextpos], y[nextpos],
				   x[nextpos+1], y[nextpos+1],
				   desiredx );
    }
}


template <class X,class Y>
inline float interpolatePositioned( const X& x, const Y& y, int sz, float pos, 
				    bool extrapolate=false )
{
    float ret = mUdf(float);
    interpolatePositioned( x, y, sz, pos, ret, extrapolate );
    return ret;
}


/*!If pos is large, it will loose it's precision. Therefore the target
position in the array is pos+offset. */

template <class T>
inline int getInterpolateIdxsWithOff( const T& idxabl, od_int64 sz,
	od_int64 offset, float pos, bool extrap, float snapdist, od_int64 p[4] )
{
    if ( sz < 1
      || (!extrap && (pos<-snapdist || (pos+offset)>sz-1+snapdist)) )
	return -1;

    od_int64 intpos = mNINT64( pos );
    const float dist = pos - intpos;
    intpos += offset;
    if ( dist>-snapdist && dist<snapdist && intpos>-1 && intpos<sz ) 
	{ p[0] = intpos; return 0; }

    p[1] = dist > 0 ? intpos : intpos - 1;
    if ( p[1] < 0 ) p[1] = 0;
    if ( p[1] >= sz ) p[1] = sz - 1;
    p[0] = p[1] < 1 ? p[1] : p[1] - 1;
    p[2] = p[1] < sz-1 ? p[1] + 1 : p[1];
    p[3] = p[2] < sz-1 ? p[2] + 1 : p[2];
    return 1;
}


template <class T>
inline int getInterpolateIdxs( const T& idxabl, int sz, float pos, bool extrap,
			       float snapdist, od_int64 p[4] )
{
    return getInterpolateIdxsWithOff<T>(idxabl,sz,0,pos,extrap,snapdist,p);
}


template <class T,class RT>
inline bool interpolateReg( const T& idxabl, int sz, float pos, RT& ret,
			    bool extrapolate=false, float snapdist=mDefEps )
{
    od_int64 p[4];
    int res = getInterpolateIdxs( idxabl, sz, pos, extrapolate, snapdist, p );
    if ( res < 0 )
	{ ret = mUdf(RT); return false; }
    else if ( res == 0 )
	{ ret = idxabl[p[0]]; return true; }

    const float relpos = pos - p[1];
    ret = Interpolate::polyReg1D( idxabl[p[0]], idxabl[p[1]], idxabl[p[2]],
				  idxabl[p[3]], relpos );
    return true;
}


/*!If pos is large, it will loose it's precision. Therefore the target
position in the array is pos+offset. */

template <class T,class RT>
inline bool interpolateRegWithUdfWithOff( const T& idxabl, od_int64 sz,
	od_int64 offset, float pos, RT& ret, bool extrapolate=false,
	float snapdist=mDefEps )
{
    od_int64 p[4];
    int res = getInterpolateIdxsWithOff<T>( idxabl, sz, offset, pos,
	    				    extrapolate, snapdist, p );
    if ( res < 0 )
	{ ret = mUdf(RT); return false; }
    else if ( res == 0 )
	{ ret = idxabl[p[0]]; return true; }

    const float relpos = pos - p[1];
    ret = Interpolate::polyReg1DWithUdf( idxabl[p[0]], idxabl[p[1]],
	    				 idxabl[p[2]], idxabl[p[3]], relpos );
    return true;
}


template <class T,class RT>
inline bool interpolateRegWithUdf( const T& idxabl, int sz, float pos, RT& ret,
			    bool extrapolate=false, float snapdist=mDefEps )
{
    return interpolateRegWithUdfWithOff<T,RT>( idxabl, sz, 0, pos, ret,
						extrapolate, snapdist );
}


template <class T>
inline float interpolateReg( const T& idxabl, int sz, float pos,
			     bool extrapolate=false, float snapdist=mDefEps )
{
    float ret = mUdf(float);
    interpolateReg( idxabl, sz, pos, ret, extrapolate, snapdist );
    return ret;
}


template <class T>
inline float interpolateRegWithUdf( const T& idxabl, int sz, float pos,
				    bool extrapolate=false,
				    float snapdist=mDefEps )
{
    float ret = mUdf(float);
    interpolateRegWithUdf( idxabl, sz, pos, ret, extrapolate, snapdist );
    return ret;
}


/*Given an array of values and a number of callibrated values at different
  positions, compute a n array of values that is callibrated everywhere.
  Depending on usefactor, the callibration will either be with absolute numbers
  or with factors. The callibration curve can optionally be output, if
  so, it should have at least sz samples allocated. */


template <class T> inline
void callibrateArray( const T* input, int sz,
	       const T* controlpts, const int* controlsamples, int nrcontrols,
	       bool usefactor, T* output,
	       float* callibrationcurve = 0 )
{
    int firstsample = mUdf(int), lastsample = -mUdf(int);
    PointBasedMathFunction func( PointBasedMathFunction::Linear );
    for ( int idx=0; idx<nrcontrols; idx++ )
    {
	const int sample = controlsamples[idx];
	if ( sample>=sz )
	    continue;

	if ( !idx || sample<firstsample )
	    firstsample = sample;
	if ( !idx || sample>=lastsample )
	    lastsample = sample;

	const float value = usefactor
	    ? controlpts[idx]/input[sample]
	    : controlpts[idx]-input[sample];

	func.add( mCast(float,sample), value );
    }

    if ( !func.size()  )
    {
	memcpy( output, input, sizeof(T)*sz );
	return;
    }

    for ( int idx=0; idx<sz; idx++ )
    {
	float callibration;
	if ( idx<=firstsample )
	   callibration = func.yVals()[0];
	else if ( idx>=lastsample )
	   callibration = func.yVals()[func.size()-1];
	else
	   callibration = func.getValue( mCast(float,idx) );

	output[idx] = usefactor
	    ? input[idx]*callibration 
	    : input[idx]+callibration;

	if ( callibrationcurve )
	    callibrationcurve[idx] = callibration;
    }
}


} // namespace IdxAble

#endif
