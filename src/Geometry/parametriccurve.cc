/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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

    GeomPosID		next() override
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

    float	getValue( float p ) const override
		{ return (float) curve.computePosition(p).sqDistTo(pos); }
    float	getValue( const float* p ) const
		{ return getValue( *p ); }

protected:
    const ParametricCurve&	curve;
    Coord3			pos;
};



ParametricCurve::ParametricCurve()
{}


ParametricCurve::~ParametricCurve()
{}


bool ParametricCurve::findClosestPosition( float& p, const Coord3& pos,
					   float eps ) const
{
//    pErrMsg("This function is not tested, quality not assured (yet)");
    CurveSqDistanceFunction mfunc( *this, pos );
    const StepInterval<int> prange = parameterRange();
    if ( mIsUdf(p) || !prange.includes(p,false) )
    {
	float closestsqdist = mUdf(float);
	for ( int idx=prange.start_; idx<=prange.stop_; idx+=prange.step_ )
	{
	    const float sqdist = (float) getPosition(idx).sqDistTo(pos);
	    if ( sqdist<closestsqdist )
	    {
		closestsqdist = sqdist;
		p = mCast( float, idx );
	    }
	}
    }

    const Interval<float> limits( mCast(float,prange.start_),
				  mCast(float,prange.stop_) );
    ExtremeFinder1D finder( mfunc, false, 20, eps,
			    Interval<float>(mMAX(p-prange.step_,prange.start_),
					    mMIN(p+prange.step_,prange.stop_) ),
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
	for ( int idx=prange.start_; idx<=prange.stop_; idx+=prange.step_ )
	{
	    const Coord3 pos = getPosition(idx);
	    const float dist =
                    (float) (plane.A_*pos.x_+plane.B_*pos.y_+plane.C_*pos.z_+plane.D_);
	    if ( fabs(dist)<closestdist )
	    {
		closestdist = dist;
		p = mCast( float, idx );
	    }
	}
    }

    const Interval<float> limits( mCast(float,prange.start_),
				  mCast(float,prange.stop_) );
    for ( int idx=0; idx<20; idx++ )
    {
	const Coord3 pos = computePosition(p);
        float fp = (float)(plane.A_*pos.x_+plane.B_*pos.y_
                           +plane.C_*pos.z_+plane.D_);

	const Coord3 dir = computeTangent(p);
        float dp = (float)(plane.A_*dir.x_+plane.B_*dir.y_
                           +plane.C_*dir.z_+plane.D_);

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

    for ( int param=range.start_; param<=range.stop_; param += range.step_ )
    {
	if ( remudf && !isDefined(param) ) continue;
	ids += param;
    }
}


} // namespace Geometry
