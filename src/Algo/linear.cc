/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/
 
static const char* rcsID = "$Id: linear.cc,v 1.2 2005-01-26 22:54:34 bert Exp $";


#include "linear.h"
#include <math.h>

LinePars* SecondOrderPoly::createDerivative() const
{
    return new LinePars( b, a*2 );
}

#define mArrVal(arr) (*(arr + idx * offs))

static void calcLS( LinStats2D& ls, const float* xvals, const float* yvals,
		    int nrpts, int offs )
{
    double sumx = 0, sumy = 0;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	sumx += mArrVal(xvals);
	sumy += mArrVal(yvals);
    }

    ls.lp.a0 = (float)sumx; ls.lp.ax = 0;
    ls.sd.a0 = 0; ls.sd.ax = mUndefValue;
    ls.corrcoeff = 1;
    if ( nrpts < 2 )
	return;

    float avgx = sumx / nrpts;
    float avgy = sumy / nrpts;
    double sumxy = 0, sumx2 = 0, sumy2 = 0;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	float xv = mArrVal(xvals) - avgx;
	float yv = mArrVal(yvals) - avgy;
	sumx2 += xv * xv;
	sumy2 += yv * yv;
	sumxy += xv * yv;
    }

    if ( sumx2 < 1e-30 )
    {
	// No x range
	ls.lp.a0 = 0;
	ls.lp.ax = mUndefValue;
	ls.sd.a0 = mUndefValue; ls.sd.ax = 0;
	ls.corrcoeff = 1;
	return;
    }
    else if ( sumy2 < 1e-30 )
    {
	// No y range
	ls.lp.a0 = avgy;
	ls.lp.ax = 0;
	ls.corrcoeff = 1;
	ls.sd.a0 = ls.sd.ax = 0;
	return;
    }

    ls.lp.ax = sumxy / sumx2;
    ls.lp.a0 = (sumy - sumx*ls.lp.ax) / nrpts;
    ls.corrcoeff = sumxy / (sqrt( sumx2 ) * sqrt( sumy2 ));

    double sumd2 = 0;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	const float ypred = ls.lp.a0 + ls.lp.ax * mArrVal(xvals);
	const float yd = mArrVal(yvals) - ypred;
	sumd2 = yd * yd;
    }
    ls.sd.ax = sqrt( sumd2 / ((nrpts-2) * sumx2) );
    ls.sd.a0 = sqrt( (sumx2 * sumd2) / (nrpts * (nrpts-2) * sumx2) );
}


void LinStats2D::use( const float* xvals, const float* yvals, int nrpts )
{
    calcLS( *this, xvals, yvals, nrpts, 1 );
}


void LinStats2D::use( const DataPoint2D* vals, int nrpts )
{
    if ( nrpts < 1 ) return;
    calcLS( *this, &vals[0].x, &vals[0].y, nrpts, 2 );
}
