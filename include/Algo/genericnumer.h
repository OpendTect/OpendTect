#ifndef genericnumer_h
#define genericnumer_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id$
________________________________________________________________________


*/

#include "algomod.h"
#include "mathfunc.h"
#include <math.h>

/*!>
Compute z = x convolved with y; i.e.,

           ifx+lx-1
    z[i] =   sum    x[j]*y[i-j]  ;  i = ifz,...,ifz+lz-1
            j=ifx
*/

template <class A, class B, class C>
inline void GenericConvolve( int lx, int ifx, const A& x, 
			     int ly, int ify, const B& y,
			     int lz, int ifz, C& z)
{
    int ilx=ifx+lx-1,ily=ify+ly-1,ilz=ifz+lz-1,i,j,jlow,jhigh;
    float sum;

    for ( i=ifz; i<=ilz; ++i )
    {
	jlow = i-ily;
	if ( jlow < ifx ) jlow = ifx;

	jhigh = i-ify;
	if ( jhigh > ilx ) jhigh = ilx;

	for ( j=jlow,sum=0.0; j<=jhigh; ++j )
	    sum += x[j-ifx]*y[i-j-ify];

	z[i-ifz] = sum;
    }
}



/*!> similarity is the hyperspace distance between two vectors divided by
the sum of the lengths */

template <class A, class B>
inline float similarity( const A& a, const B& b, int sz, bool normalize=false,
			 int firstposa=0, int firstposb=0 )
{
    float val1, val2;
    double sqdist = 0, sq1 = 0, sq2 = 0;

    double meana = mUdf(double), stddeva = mUdf(double);
    double meanb = mUdf(double), stddevb = mUdf(double);

    if ( normalize )
    {
	if ( sz==1 ) normalize = false;
	else
	{
	    double asum=0,bsum=0;
	    for ( int idx=0; idx<sz; idx++ )
	    {
		asum += a[firstposa+idx];
		bsum += b[firstposb+idx];
	    }

	    meana = asum/sz;
	    meanb = bsum/sz;

	    asum = 0;
	    bsum = 0;
	    for ( int idx=0; idx<sz; idx++ )
	    {
		const double adiff = a[firstposa+idx]-meana;
		const double bdiff = b[firstposb+idx]-meanb;
		asum += adiff*adiff;
		bsum += bdiff*bdiff;
	    }

	    stddeva = Math::Sqrt(asum/(sz-1));
	    stddevb = Math::Sqrt(bsum/(sz-1));

	    if ( mIsZero(stddeva,mDefEps) || mIsZero(stddevb,mDefEps) )
		normalize=false;
	}
    }

    int curposa = firstposa;
    int curposb = firstposb;

    for ( int idx=0; idx<sz; idx++ )
    {
	val1 = normalize ? (float) ( (a[curposa]-meana)/stddeva ) : a[curposa];
	val2 = normalize ? (float) ( (b[curposb]-meanb)/stddevb ) : b[curposb];
	if ( mIsUdf(val1) || mIsUdf(val2) )
	    return mUdf(float);

	sq1 += val1 * val1;
	sq2 += val2 * val2;
	sqdist += (val1-val2) * (val1-val2);

	curposa ++;
	curposb ++;
    }

    if ( mIsZero(sq1,mDefEps) && mIsZero(sq2,mDefEps) )
	return 1;

    if ( mIsZero(sq1,mDefEps) || mIsZero(sq2,mDefEps) )
	return 0;

    const float rt = 
	    (float) ( Math::Sqrt(sqdist) / (Math::Sqrt(sq1) + Math::Sqrt(sq2)) );
    return 1 - rt;
}


mGlobal(Algo) float similarity(const FloatMathFunction&,const FloatMathFunction&, 
		 float x1, float x2, float dist, int sz, bool normalize );


mGlobal(Algo) float semblance( const ObjectSet<float>& signals,const Interval<int>& );

mGlobal(Algo) float semblance( const ObjectSet<float>& signals,int signalsize,
			 const TypeSet<float>& signalstarts,
			 const Interval<int>& gate );

mGlobal(Algo) double LanczosKernel( int size, double x );

/*!> uses parabolic search for the position where a function gets
a specific value. The target value must be in the interval f(x1) and f(x2).
There is no use to have a tolerance lower than the square root of the system's
float-precision. */


mGlobal(Algo) bool findValue(const FloatMathFunction&,float x1,float x2,float& res,
	       float targetval = 0,float tol=1e-5);


/*!> findValueInAperture is similar to findValue, with the difference that
findValueInAperture searches the solution that is closest to the startx. If no
solution is found, startx is returned. The parameter dx specifies how big
intervals should be used when searching for a solution. When a solution is
found in an interval, a high precision search is started in that interval.
*/

mGlobal(Algo) float findValueInAperture(const FloatMathFunction&,float startx, 
	 	const Interval<float>& aperture,float dx,float target=0,
		float tol=1e-5);

/*!>
findExtreme - finds a functions minimal or maximum value (specified by bool 
minima) within the values x1 and x2. 
f((x1+x2)/2) should be less than f(x1) and f(x2). If no minima can be found,
mUdf(float) is returned;
*/

mGlobal(Algo) float findExtreme(const FloatMathFunction&,bool minima,float x1,float x2,
		  float tol = 1e-5);


template <class A, class B> inline
void reSample( const FloatMathFunction& input, const A& samplevals,
	       B& output, int nrsamples )
{
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const float sampleval = samplevals[idx];
	output[idx] = Values::isUdf(sampleval) ? mUdf(float) :
	    	      input.getValue(sampleval);
    }
}


/*!Computes the greatest common divisor from two intigers. Uses the algorithm
   published by Josef Stein. */

mGlobal(Algo) unsigned int greatestCommonDivisor( unsigned int u, unsigned int v );


/*!>
Compute z = x cross-correlated with y; i.e.,

	   ifx+lx-1
   z[i] =   sum    x[j]*y[i+j]  ;  i = ifz,...,ifz+lz-1
	   j=ifx

Cross correlation will be performed using GenericConvolve function, here is the
method used:
1) reverse the samples in the x array ->copy them to a temporary array,
2) use the temporary array to call function GenericConvolve() 
   with ifx set to 1-ifx-lx.
*/

template <class A, class B, class C>
inline void genericCrossCorrelation( int lx, int ifx, const A& x, 
			     	     int ly, int ify, const B& y,
			     	     int lz, int ifz, C& z)
{
    ArrPtrMan<float> xreversed = new float[lx];

    for ( int i=0,j=lx-1; i<lx; ++i,--j)
	xreversed[i] = x[j];
    GenericConvolve( lx, 1-ifx-lx, xreversed, ly, ify, y, lz, ifz, z );
}


template <class A>
inline void reverseArray( A* in, int sz, A* out=0 )
{
    if ( out )
	for ( int idx=0; idx<sz; idx++ )
	    out[idx] = in[sz-1-idx];
    else
    {
	mAllocVarLenArr( A, tmparr, sz/2 );
	for ( int idx=0; idx<sz/2; idx++ )
	{
	    tmparr[idx] = in[idx];
	    int opsamp = sz-1-idx;
	    in[idx] = in[opsamp];
	    in[opsamp] = tmparr[idx];
	}
    }
}


#endif

