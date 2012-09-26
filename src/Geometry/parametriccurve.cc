/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2004
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "parametriccurve.h"

#include "extremefinder.h"
#include "mathfunc.h"
#include "typeset.h"
#include "trigonometry.h"
#include "undefval.h"

namespace Geometry
{


class ParametricCurveIterator : public Iterator
{
public:
    			ParametricCurveIterator( const ParametricCurve& pc )
			    : range_( pc.parameterRange() )
			    , curpos_( -1 )
			{}

    GeomPosID		next()
    {
	GeomPosID newid = curpos_++;
	if ( !range_.includes( newid, false ) )
	    return -1;

	return newid;
    }

public:

    StepInterval<int>		range_;
    Threads::Atomic<od_int64>	curpos_;
};


class CurveSqDistanceFunction : public FloatMathFunction
{
public:
    			CurveSqDistanceFunction( const ParametricCurve& pc,
						 const Coord3& ppos )
			   : curve( pc ) 
			   , pos( ppos )
		       {}

    float		getValue( float p ) const
			{ return (float) curve.computePosition(p).sqDistTo(pos); }
    float		getValue( const float* p ) const
			{ return getValue( *p ); }

protected:
    const ParametricCurve&	curve;
    Coord3			pos;
};



bool ParametricCurve::findClosestPosition( float& p, const Coord3& pos,
					   float eps ) const
{
//    pErrMsg("This function is not tested, quality not assured (yet)");
    CurveSqDistanceFunction mfunc( *this, pos );
    const StepInterval<int> prange = parameterRange();
    if ( mIsUdf(p) || !prange.includes(p,false) )
    {
	float closestsqdist = mUdf(float);
	for ( int idx=prange.start; idx<=prange.stop; idx+=prange.step )
	{
	    const float sqdist = (float) getPosition(idx).sqDistTo(pos);
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
    p = finder.extremePos();

    return res!=-1;
}


bool ParametricCurve::findClosestIntersection( float& p, const Plane3& plane,
					       float eps ) const
{
    pErrMsg("This function is not tested, quality not assured (yet)");
    const StepInterval<int> prange = parameterRange();
    if ( mIsUdf(p) || !prange.includes(p,false) )
    {
	float closestdist = mUdf(float);
	for ( int idx=prange.start; idx<=prange.stop; idx+=prange.step )
	{
	    const Coord3 pos = getPosition(idx);
	    const float dist =
		(float) (plane.A_*pos.x+plane.B_*pos.y+plane.C_*pos.z+plane.D_);
	    if ( fabs(dist)<closestdist )
	    {
		closestdist = dist;
		p = idx;
	    }
	}
    }

    const Interval<float> limits( prange.start, prange.stop );
    for ( int idx=0; idx<20; idx++ )
    {
	const Coord3 pos = computePosition(p);
	float fp = (float)(plane.A_*pos.x+plane.B_*pos.y+plane.C_*pos.z+plane.D_);

	const Coord3 dir = computeTangent(p);
	float dp = (float)(plane.A_*dir.x+plane.B_*dir.y+plane.C_*dir.z+plane.D_);

	const float diff = dp/fp;
	p = p - diff;
	if ( fabs(diff)<eps )
	    return true;

	if ( !prange.includes(p,false) )
	    return false;
    }

    return false;
}


Iterator* ParametricCurve::createIterator() const
{ return new ParametricCurveIterator( *this ); }


void ParametricCurve::getPosIDs( TypeSet<GeomPosID>& ids, bool remudf ) const
{
    ids.erase();
    const StepInterval<int> range = parameterRange();

    for ( int param=range.start; param<=range.stop; param += range.step )
    {
	if ( remudf && !isDefined(param) ) continue;
	ids += param;
    }
}


}; //Namespace

