#ifndef genericnumer_h
#define genericnumer_h

/*@+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: genericnumer.h,v 1.3 1999-11-18 16:41:14 bert Exp $
________________________________________________________________________


*/

#include <gendefs.h>
class MathFunction;

/*
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

template <class A, class B>
inline float similarity( const A& a, const B& b, int sz, int firstpos=0)
{
    float val1, val2;
    double sqdist = 0, sq1 = 0, sq2 = 0;

    int curpos = firstpos;

    for ( int idx=0; idx<sz; idx++ )
    {
	val1 = a[curpos];
	val2 = b[curpos];
	sq1 += val1 * val1;
	sq2 += val2 * val2;
	sqdist += (val1-val2) * (val1-val2);

	curpos ++;
    }

    if ( sq1 + sq2 < 1e-10 ) return 0;
    return 1 - (sqrt(sqdist) / (sqrt(sq1) + sqrt(sq2)));
}

float similarity(const MathFunction&,const MathFunction&, 
		 float x1, float x2, float dist, int sz);

/* 
findValue - uses parabolic search for the position where a function gets
a specific value. The target value must be in the interval f(x1) and f(x2).
There is no use to have a tolerance lower than the square root of the system's
float-precision.

findValueInAperture is similar to findValue, with the difference that
findValueInAperture searches the solution that is closest to the startx. If no
solution is found, startx is returned. The parameter dx specifies how big
intervals should be used when searching for a solution. When a solution is
found in an interval, a high precision search is started in that interval.
*/


bool findValue(const MathFunction&,float x1,float x2,float& res,
	       float targetval = 0,float tol=1e-5);

float findValueInAperture(const MathFunction&,float startx, 
	 	float aperture,float dx,float target=0,float tol=1e-5);


#endif
