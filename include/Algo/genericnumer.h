#ifndef genericnumer_h
#define genericnumer_h

/*@+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: genericnumer.h,v 1.8 2001-02-13 17:15:46 bert Exp $
________________________________________________________________________


*/

#include <ranges.h>
template <class T> class MathFunction;

/*!>
Compute z = x convolved with y; i.e.,

           ifx+lx-1
    z[i] =   sum    x[j]*y[i-j]  ;  i = ifz,...,ifz+lz-1
            j=ifx
*/

template<class A, class B,class C>
inline void GenericConvolve (int lx, int ifx, const A& x, 
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
inline float similarity( const A& a, const B& b, int sz,
			 int firstposa=0, int firstposb=0)
{
    float val1, val2;
    double sqdist = 0, sq1 = 0, sq2 = 0;

    int curposa = firstposa;
    int curposb = firstposb;

    for ( int idx=0; idx<sz; idx++ )
    {
	val1 = a[curposa];
	val2 = b[curposb];
	sq1 += val1 * val1;
	sq2 += val2 * val2;
	sqdist += (val1-val2) * (val1-val2);

	curposa ++;
	curposb ++;
    }

    if ( sq1 + sq2 < 1e-10 ) return 0;
    return 1 - (sqrt(sqdist) / (sqrt(sq1) + sqrt(sq2)));
}

float similarity(const MathFunction<float>&,const MathFunction<float>&, 
		 float x1, float x2, float dist, int sz);

/*!> uses parabolic search for the position where a function gets
a specific value. The target value must be in the interval f(x1) and f(x2).
There is no use to have a tolerance lower than the square root of the system's
float-precision. */


bool findValue(const MathFunction<float>&,float x1,float x2,float& res,
	       float targetval = 0,float tol=1e-5);


/*!> findValueInAperture is similar to findValue, with the difference that
findValueInAperture searches the solution that is closest to the startx. If no
solution is found, startx is returned. The parameter dx specifies how big
intervals should be used when searching for a solution. When a solution is
found in an interval, a high precision search is started in that interval.
*/

float findValueInAperture(const MathFunction<float>&,float startx, 
	 	const TimeGate& aperture,float dx,float target=0,
		float tol=1e-5);

/*!>
findExtreme - finds a functions minimal or maximum value (specified by bool 
minima) within the values x1 and x2. 
f((x1+x2)/2) should be less than f(x1) and f(x2). If no minima can be found,
mUndefValue is returned;
*/

float findExtreme( const MathFunction<float>&, bool minima, float x1, float x2,
		   float tol = 1e-5);

#endif
