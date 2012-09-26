/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/
 
static const char* rcsID mUsedVar = "$Id$";


#include "linear.h"
#include "undefval.h"
#include "math2.h"
//#include <math.h>

#define mArrVal(arr) (*(float*)(arr + idx * offs))

static void calcLS( LinStats2D& ls, const char* xvals, const char* yvals,
		    int nrpts, int offs )
{
    double sumx = 0, sumy = 0;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	sumx += mArrVal(xvals);
	sumy += mArrVal(yvals);
    }

    ls.lp.a0 = (float)sumx; ls.lp.ax = 0;
    Values::setUdf(ls.sd.a0); Values::setUdf(ls.sd.ax);
    ls.corrcoeff = 0;
    if ( nrpts < 2 )
	return;

    float avgx = (float) ( sumx / nrpts ); 
    float avgy = (float) ( sumy / nrpts );
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
	ls.lp.a0 = avgx;
	Values::setUdf(ls.lp.ax);
	ls.sd.ax = 0; ls.corrcoeff = 1;
	return;
    }
    else if ( sumy2 < 1e-30 )
    {
	// No y range
	ls.lp.a0 = avgy;
	ls.lp.ax = 0;
	ls.sd.a0 = ls.sd.ax = 0;
	ls.corrcoeff = 1;
	return;
    }

    ls.lp.ax = (float) ( sumxy / sumx2 );
    ls.lp.a0 = (float) ( (sumy - sumx*ls.lp.ax) / nrpts );
    ls.corrcoeff = (float) (sumxy / (Math::Sqrt(sumx2) * Math::Sqrt(sumy2)));

    double sumd2 = 0;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	const float ypred = ls.lp.a0 + ls.lp.ax * mArrVal(xvals);
	const float yd = mArrVal(yvals) - ypred;
	sumd2 = yd * yd;
    }
    if ( nrpts < 3 )
	ls.sd.ax = ls.sd.a0 = 0;
    else
    {
	ls.sd.ax = (float) ( Math::Sqrt( sumd2 / ((nrpts-2) * sumx2) ) );
	ls.sd.a0 = (float) ( Math::Sqrt( (sumx2 * sumd2) / 
									 (nrpts * (nrpts-2) * sumx2) ) );
    }
}


void LinStats2D::use( const float* xvals, const float* yvals, int nrpts )
{
    calcLS( *this, (const char*) xvals, (const char*) yvals,
	    nrpts, sizeof(float) );
}


void LinStats2D::use( const Geom::Point2D<float>* vals, int nrpts )
{
    if ( nrpts < 1 ) return;
    calcLS( *this, (const char*) &vals[0].x, (const char*) &vals[0].y, nrpts,
	    sizeof(Geom::Point2D<float>) );
}
