#ifndef simpnumer_H
#define simpnumer_H

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril & Kris Tingdahl
 Date:		12-4-1999
 Contents:	'Simple' numerical functions
 RCS:		$Id$
________________________________________________________________________

*/

#include "algomod.h"
#include "undefval.h"
#include "math2.h"
#include <math.h>
#include <limits.h>


/*!>Handles roundoff errors when looking for the previous sample in an array.*/


inline int getPrevSample( float target, int size, float& relpos )
{
    int sampl = (int) target;
    relpos = target-sampl;
    if ( sampl==-1 && mIsEqual(relpos,1,1e-3) )
    { sampl = 0; relpos = 0; }
    else if ( sampl==size-1 && mIsZero(relpos,1e-3) )
    { sampl = size-2; relpos = 1; }

    return sampl;
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
 isPower determines whether a value is a power of a base, i.e. if
val=base^pow.
 If that is the case, isPower returns pow, if not, it returns zero.
*/


inline
int isPower( int val, int base )
{
    if ( val==base ) return 1;

    if ( val%base )
	return 0;

    int res = isPower( val/base, base );
    if ( res ) return 1 + res;

    return 0;
}


inline
int nextPower( int val, int base )
{
    int res = 1;
    while ( res<val ) res *= base;
    return res;
}

inline int getPow2Sz(int actsz, bool above=true, int minsz=1, int maxsz=INT_MAX)
{
    char npow = 0; char npowextra = actsz == 1 ? 1 : 0;
    int sz = actsz;
    while ( sz>1 )
    {
	if ( above && !npowextra && sz % 2 )
	    npowextra = 1;
	sz /= 2; npow++;
    }
    
    sz = intpow( 2, npow + npowextra );
    if ( sz<minsz ) sz = minsz;
    if ( sz>maxsz ) sz = maxsz;
    return sz;
}


inline int nextPower2( int nr, int minnr, int maxnr )
{
    if ( nr>maxnr )
	return maxnr;
    
    int newnr = minnr;
    while ( nr > newnr )
	newnr *= 2;
    
    return newnr;
}


/*!>
 Find number of blocks when given total number of samples, the base size for  
 each block and the number of samples overlaped between two blocks.
 */


inline
int nrBlocks( int totalsamples, int basesize, int overlapsize )
{
    int res = 0;
    while ( totalsamples>basesize )
    {
	res++;
	totalsamples = totalsamples - basesize + overlapsize;
    }
    
    return res+1;
}


/*!
\brief Taper an indexable array from 1 to taperfactor. If lowpos is less 
than highpos, the samples array[0] to array[lowpos] will be set to zero. 
If lowpos is more than highpos, the samples array[lowpos]  to array[sz-1]
will be set to zero. The taper can be either cosine or linear.
*/

mExpClass(Algo) Taper
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
	return false;

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

    return true;
}


/*!>
 Gradient from a series of 4 points, sampled like:
 x:  -2  -1  0  1  2
 y: ym2 ym1 y0 y1 y2
 The gradient estimation is done at x=0. y0 is generally not needed, but it
 will be used if there are one or more undefineds.
 The function will return mUdf(T) if there are too many missing values.
*/
template <class T>
inline T sampledGradient( T ym2, T ym1, T y0_, T y1_, T y2 )
{
    bool um1 = Values::isUdf(ym1), u0 = Values::isUdf(y0_),
    		u1 = Values::isUdf(y1_);

    if ( Values::isUdf(ym2) || Values::isUdf(y2) )
    {
        if ( um1 || u1 )
	    { if ( !u0 && !(um1 && u1) ) return um1 ? y1_ - y0_ : y0_ - ym1; }
        else
            return (y1_-ym1) * .5;
    }
    else if ( (um1 && u1) || (u0 && (um1 || u1)) )
        return (y2-ym2) * .25;
    else if ( um1 || u1 )
    {
        return um1 ? ((16*y1_)   - 3*y2  - ym2) / 12 - y0_
                   : ((-16*ym1) + 3*ym2 + y2)  / 12 + y0_;
    }
    else
        return (8 * ( y1_ - ym1 ) - y2 + ym2) / 12;

    return mUdf(T);
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
    if ( sz < 2 ) return mUdf(float);

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
	const double minus_twosqrt_q = -2*Math::Sqrt(q);
	const double theta = Math::ACos(r/Math::Sqrt(q3));
	static const double twopi = 2*M_PI;


	root0 = minus_twosqrt_q*cos(theta/3)+minus_a_through_3; 
	root1=minus_twosqrt_q*cos((theta-twopi)/3)+minus_a_through_3;
	root2=minus_twosqrt_q*cos((theta+twopi)/3)+minus_a_through_3;
	return 3;
    }

    const double A=(r>0?-1:1)*pow(fabs(r)+Math::Sqrt(r2-q3),1/3);
    const double B=mIsZero(A,mDefEps)?0:q/A;

    root0 = A+B+minus_a_through_3;

    /*!
    The complex roots can be calculated as follows:
    static const double sqrt3_through_2 = Math::Sqrt(3)/2;

    root1 = complex_double( 0.5*(A+B)+minus_a_through_3,
			   sqrt3_through_2*(A-B));

    root2 = complex_double( 0.5*(A+B)+minus_a_through_3,
			   -sqrt3_through_2*(A-B));
    */
			     

    return 1;
}


template <class T>
inline bool holdsClassValue( const T val, const unsigned int maxclss=50 )
{
    if ( mIsUdf(val) ) return true;
    if ( val < -mDefEps ) return false;
    const int ival = (int)(val + .5);
    return ival <= maxclss && mIsEqual(val,ival,mDefEps);
}


template <class T>
inline bool holdsClassValues( const T* vals, od_int64 sz,
			      const unsigned int maxclss=50,
			      const unsigned int samplesz=100 )
{
    if ( sz < 1 ) return true;
    if ( sz <= samplesz )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    if ( !holdsClassValue(vals[idx],maxclss) )
		return false;
	}
	return true;
    }

    static od_int64 seed = mUdf(od_int64);
    seed *= seed + 1; // Clumsy but cheap sort-of random generation

    for ( int idx=0; idx<samplesz; idx++ )
    {
	od_int64 arridx = ((1+idx) * seed) % samplesz;
	if ( arridx<0 ) 
	    arridx = -arridx;

	if ( !holdsClassValue(vals[arridx],maxclss) )
	    return false;
    }
    return true;
}


template <class T>
inline bool isSigned8BitesValue( const T val )
{
    if ( val>127 || val<-128 )
	return false;

    const int ival = (int)(val>0 ? val+.5 : val-.5);
    return mIsEqual(val,ival,mDefEps);
}


template <class T>
inline bool is8BitesData( const T* vals, od_int64 sz,
	const unsigned int samplesz=100 )
{
    if ( holdsClassValues(vals,sz,255,samplesz) )
	return true;

    if ( sz <= samplesz )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    if ( !isSigned8BitesValue(vals[idx]) )
		return false;
	}
	return true;
    }

    static od_int64 seed = mUdf(od_int64);
    seed *= seed + 1; // Clumsy but cheap sort-of random generation

    for ( int idx=0; idx<samplesz; idx++ )
    {
	od_int64 arridx = ((1+idx) * seed) % samplesz;
	if ( arridx<0 ) 
	    arridx = -arridx;
	
	if ( !isSigned8BitesValue(vals[arridx]) )
	    return false;
    }
    return true;
}

#endif

