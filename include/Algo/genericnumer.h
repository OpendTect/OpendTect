#ifndef genericnumer_h
#define genericnumer_h

/*@+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: genericnumer.h,v 1.12 2004-07-21 11:37:43 nanne Exp $
________________________________________________________________________


*/

#include "ranges.h"
template <class T> class MathFunction;

/*!>
Compute z = x convolved with y; i.e.,

           ifx+lx-1
    z[i] =   sum    x[j]*y[i-j]  ;  i = ifz,...,ifz+lz-1
            j=ifx
*/

template<class A, class B,class C>
void GenericConvolve(int lx, int ifx, const A& x, 
		     int ly, int ify, const B& y,
               	     int lz, int ifz, C& z);


/*!> similarity is the hyperspace distance between two vectors divided by
the sum of the lengths */

template <class A, class B>
float similarity(const A& a, const B& b, int sz, bool normalize=false,
		 int firstposa=0, int firstposb=0);


float similarity(const MathFunction<float>&,const MathFunction<float>&, 
		 float x1, float x2, float dist, int sz, bool normalize );

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
