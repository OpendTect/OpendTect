/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2004
-*/

static const char* rcsID = "$Id: parametriccurve.cc,v 1.2 2005-03-18 11:21:27 cvskris Exp $";

#include "parametriccurve.h"

#include "extremefinder.h"
#include "mathfunc.h"
#include "sets.h"
#include "undefval.h"

namespace Geometry
{


class CurveSqDistanceFunction : public FloatMathFunction
{
public:
    			CurveSqDistanceFunction( const ParametricCurve& pc,
						 const Coord3& ppos )
			   : curve( pc ) 
			   , pos( ppos )
		       {}

    float		getValue( float p ) const
			{ return curve.computePosition(p).sqDistance(pos); }

protected:
    const ParametricCurve&	curve;
    Coord3			pos;
};



bool ParametricCurve::findClosestPosition( float& p, const Coord3& pos,
					   float eps ) const
{
    CurveSqDistanceFunction mfunc( *this, pos );
    StepInterval<int> prange = parameterRange();
    if ( Values::isUdf(p) || !prange.includes(p,false) )
    {
	float closestsqdist = mUdf(float);
	for ( int idx=prange.start; idx<=prange.stop; idx+=prange.step )
	{
	    const float sqdist = getPosition(idx).sqDistance(pos);
	    if ( sqdist<closestsqdist )
	    {
		closestsqdist = sqdist;
		p = idx;
	    }
	}
    }

    const Interval<float> limits( prange.start, prange.stop );
    ExtremeFinder1D finder( mfunc, false, 20, eps,
	    		    Interval<float>(mMAX(p-prange.step,prange.start),
					    mMIN(p+prange.step,prange.stop) ),
			    &limits );

    int res;
    while ( (res=finder.nextStep())==1 ) ;

    if ( !res ) p = finder.extremePos();

    return res!=-1;
}


    
void ParametricCurve::getPosIDs( TypeSet<GeomPosID>& ids ) const
{
    ids.erase();
    const StepInterval<int> range = parameterRange();

    for ( int param=range.start; param<=range.stop; param += range.step )
	ids += param;
}


}; //Namespace

