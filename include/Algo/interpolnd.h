#ifndef interpolnd_h
#define interpolnd_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2010
 RCS:		$Id$
________________________________________________________________________

*/

#include "math2.h"


namespace Interpolate
{

/*!>
 Linear ND interpolation.

Input:
 * Array sz=2^N values as following 4D example
   Arr[0] = val[0][0][0][0]
   Arr[1] = val[1][0][0][0]
   Arr[2] = val[0][1][0][0]
   Arr[3] = val[1][1][0][0]
   Arr[4] = val[0][0][1][0]
   Arr[5] = val[1][0][1][0]
   Arr[6] = val[0][1][1][0]
   Arr[7] = val[1][1][1][0]
   Arr[8] = val[0][0][0][1]
   Arr[9] = val[1][0][0][1]
   Arr[10] = val[0][1][0][1]
   Arr[11] = val[1][1][0][1]
   Arr[12] = val[0][0][1][1]
   Arr[13] = val[1][0][1][1]
   Arr[14] = val[0][1][1][1]
   Arr[15] = val[1][1][1][1]

   Fill the vals with something like:

    od_int64 sz = IntPower( 2, N );
    for ( od_int64 ipt=0; ipt<sz; ipt++ )
    {
    	TypeSet<int> idxs( N, 0 );
	od_int64 bits = ipt;
	for ( int idim=0; idim<nrdims; idim++ )
	{
	    if ( bits & 1 ) idxs[idim]++;
	    bits >>= 1;
	}
	pts += getVals( idxs );
    }

  You therefore provide all the points in the (hyper)cube around the point of
  evaluation. The [0][0]...[0] point can be determined using 'floor', as in:

    for ( int idim=0; idim<nrdims; idim++ )
    {
	const float fidx = samplings[idim].getIndex( vals[idim] );
	const int idx0 = (int)floor(fidx);
	pos[idim] = fidx - idx0; idx0s += idx0;
    }

*/

template <class T>
inline T linearRegND( int N, const T* v, const T* pos )
{
    if ( N == 0 )
	return v[0];
    else if ( N == 1 )
	return v[0] * (1-pos[0]) + v[1] * pos[0];
    else if ( N == 2 )
    {
	T a[4];
	a[0] = v[0];
	a[1] = v[1] - v[0];
	a[2] = v[2] - v[0];
	a[3] = v[3] + v[0] - v[1] - v[2];
	return a[0] + a[1]*pos[0] + a[2]*pos[1] + a[3]*pos[0]*pos[1];
    }
    else
    {
	const int lowerN = N-1;
	const od_int64 nlowerpts = Math::IntPowerOf( ((od_int64)2), lowerN );
	float* lowerv = new float [nlowerpts];
	const float lastpos = pos[lowerN];
	for ( od_int64 idx=0; idx<nlowerpts; idx++ )
	{
	    const float v0 = v[idx];
	    const float v1 = v[idx+nlowerpts];
	    lowerv[idx] = (1-lastpos) * v0 + lastpos * v1;
	}
	const float res = linearRegND( lowerN, lowerv, pos );
	delete [] lowerv;
	return res;
    }
}


} // namespace Interpolate

#endif
