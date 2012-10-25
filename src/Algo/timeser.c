/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Dave Hale / A.H. Bril
 * DATE     : 24-3-1996
 * FUNCTION : Time series manipulation

 Only AntiAlias and ReSample are Bert's, the rest is Dave's (albeit with
 another interface).

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "timeser.h"
#include "mallocdefs.h"
#include <math.h>
#include <string.h>


void BFhighpass( int npoles, float f3db,
		 int n, const float* arrin, float* arrout )
{
    int jpair, j;
    float r, scale, theta, a, b1, b2, pj, pjm1, pjm2, qjm1, qjm2;

    r = (float) ( 2.0 * tan( M_PI * fabs(f3db)) );
    if ( ! (npoles%2) )
	for ( j=0; j<n; j++ ) arrout[j] = arrin[j];
    else
    {
	scale = r + 2;
	a = 2 / scale;
	b1 = (r - 2) / scale;
	pj = qjm1 = 0;
	for ( j=0; j<n; j++ )
	{
	    pjm1 = pj;
	    pj = arrin[j];
	    arrout[j] = a * (pj-pjm1) - b1*qjm1;
	    qjm1 = arrout[j];
	}
    }

    for ( jpair=0; jpair<npoles/2; jpair++ )
    {
	theta = (float) ( M_PI * (2*jpair+1) / (2*npoles) );
	scale = (float) ( 4 + 4*r*sin(theta) + r*r );
	a = 4 / scale;
	b1 = (2*r*r - 8) / scale;
	b2 = (float) ( (4 - 4*r*sin(theta) + r*r) / scale );
	pjm1 = pj = qjm2 = qjm1 = 0;
	for (j=0; j<n; j++)
	{
	    pjm2 = pjm1;
	    pjm1 = pj;
	    pj = arrout[j];
	    arrout[j] = a * (pj-2*pjm1+pjm2) - b1*qjm1 - b2*qjm2;
	    qjm2 = qjm1;
	    qjm1 = arrout[j];
	}
    }
}


void BFlowpass( int npoles, float f3db,
		int n, const float* arrin, float* arrout )
{
    int jpair,j;
    float r,scale,theta,a,b1,b2,pj,pjm1,pjm2,qjm1,qjm2;

    r = (float) ( 2.0*tan(M_PI*fabs(f3db)) );
    if ( !(npoles%2) )
	for ( j=0; j<n; j++ ) arrout[j] = arrin[j];
    else
    {
	scale = r + 2;
	a = r / scale;
	b1 = (r - 2) / scale;
	pj = qjm1 = 0;
	for ( j=0; j<n; j++ )
	{
	    pjm1 = pj;
	    pj = arrin[j];
	    arrout[j] = a * (pj+pjm1) - b1*qjm1;
	    qjm1 = arrout[j];
	}
    }

    for ( jpair=0; jpair<npoles/2; jpair++ )
    {
	theta = (float) ( M_PI * (2*jpair+1) / (2*npoles) );
	scale = (float) ( 4 + 4 * r * sin(theta) + r*r );
	a = r * r / scale;
	b1 = (2*r*r - 8) / scale;
	b2 = (float) ( (4 - 4*r*sin(theta) + r*r)/scale );
	pjm1 = pj = qjm2 = qjm1 = 0;
	for (j=0; j<n; j++)
	{
	    pjm2 = pjm1;
	    pjm1 = pj;
	    pj = arrout[j];
	    arrout[j] = a * (pj+2.0f*pjm1+pjm2) - b1*qjm1 - b2*qjm2;
	    qjm2 = qjm1;
	    qjm1 = arrout[j];
	}
    }
}


void AntiAlias( float frac, int sz, const float* arrin, float* arrout )
{
    int iwt, ival, istart, width, hwidth, inpidx;
    float* wts;

    if ( frac < 0 ) frac = -frac;
    width = mCast(int,2 * (1.f / frac + .5f));
    if ( width < 2 || frac >= 1 )
    {
	memcpy( arrout, arrin, sz * sizeof(float) );
	return;
    }
    if ( (width % 2) == 0 )
	width++;
    hwidth = width / 2;

    wts = mMALLOC(width,float);
    for ( iwt=0; iwt<width; iwt++ )
	wts[iwt] = (float) ( 0.5 * frac * 
				(1 + cos( M_PI * iwt / hwidth - M_PI )) );

    for ( ival=0; ival<sz; ival++ )
    {
	arrout[ival] = 0;
	istart = ival - hwidth;
	for ( iwt=0; iwt<width; iwt++ )
	{
	    inpidx = istart + iwt;
	    if ( inpidx < 0 ) inpidx = 0;
	    if ( inpidx >= sz ) inpidx = sz - 1;
	    arrout[ival] += wts[iwt] * arrin[inpidx];
	}
    }

    mFREE(wts);
}


void Convolve( int lx, int ifx, const float* x, int ly, int ify, const float* y,
	       int lz, int ifz, float* z )
/*
Compute z = x convolved with y; i.e.,

           ifx+lx-1
    z[i] =   sum    x[j]*y[i-j]  ;  i = ifz,...,ifz+lz-1
            j=ifx
*/
{
    int ilx=ifx+lx-1, ily=ify+ly-1, ilz=ifz+lz-1, i, j, jlow, jhigh;
    float sum;
    
    x -= ifx;  y -= ify;  z -= ifz;
    for ( i=ifz; i<=ilz; ++i )
    {
	jlow = i-ily; if ( jlow < ifx ) jlow = ifx;
	jhigh = i-ify; if ( jhigh > ilx ) jhigh = ilx;
	for ( j=jlow, sum=0; j<=jhigh; ++j )
	    sum += x[j]*y[i-j];
	z[i] = sum;
    }
}


void Resample( int stepsize, int n, const float* arrin, float* arrout )
{
    int isamp;
    int nsamp = n / stepsize;

    for ( isamp=0;isamp < nsamp; isamp++ )
        arrout[isamp]= arrin[ isamp*stepsize ];
}


#define mHilbertLength 2*mHalfHilbertLength+1

void Hilbert( int n, float* x, float* y )
{
    static int madeh = 0;
    static float h[mHilbertLength];
    static int hlen = mHalfHilbertLength;
    int i;
    float taper;

    if ( !madeh )
    {
	h[hlen] = 0;
	for ( i=1; i<=hlen; i++ )
	{
	    taper = (float) ( 0.54 + 0.46 * 
				    cos( M_PI*(float)i / (float)(hlen) ) );
	    h[hlen+i] = (float) ( taper * 
				( -(float)(i%2)*2.0 / (M_PI*(float)(i))) );
	    h[hlen-i] = -h[hlen+i];
	}
	madeh = 1;
    }

    Convolve( mHilbertLength, -hlen, h, n, 0, x, n, 0, y );
}
