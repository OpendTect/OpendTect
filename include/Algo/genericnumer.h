#ifndef genericnumer_h
#define genericnumer_h

/*@+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: genericnumer.h,v 1.1 1999-11-16 16:05:41 kristofer Exp $
________________________________________________________________________


@$*/

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

#endif
