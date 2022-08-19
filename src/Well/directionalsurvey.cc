/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "directionalsurvey.h"

#include "geometry.h"
#include "math2.h"
#include "survinfo.h"


namespace Well
{

DirectionalSurvey::DirectionalSurvey( const Coord& crd, double kb )
    : surfacecoord_(crd)
    , kb_(kb)
    , method_(MinCurv)
{
}


DirectionalSurvey::~DirectionalSurvey()
{
}


// Minimum Curvature Method
// From: http://www.drillingformulas.com/tag/directional-drilling-calculations/
static void getDevMinCurv( double md1, double md2,
			   double incl1, double incl2,
			   double azi1, double azi2, Coord3& delta )
{
    const double sini1 = sin( Math::toRadians(incl1) );
    const double sini2 = sin( Math::toRadians(incl2) );
    const double sina1 = sin( Math::toRadians(azi1) );
    const double sina2 = sin( Math::toRadians(azi2) );

    const double cosi1 = cos( Math::toRadians(incl1) );
    const double cosi2 = cos( Math::toRadians(incl2) );
    const double cosa1 = cos( Math::toRadians(azi1) );
    const double cosa2 = cos( Math::toRadians(azi2) );

    const double cosi2mi1 = cos( Math::toRadians(incl2-incl1) );
    const double cosa2ma1 = cos( Math::toRadians(azi2-azi1) );

    double rf = 1.0; //Ratio factor
    const double beta = acos( cosi2mi1 - (sini1 * sini2 * (1.0 - cosa2ma1)) );
    if ( mIsZero(beta,mDefEps) )
	rf = 1.0;
    else
	rf = (2./beta) * tan(beta/2.);

    const double dmd = (md2-md1) / 2.;
    delta.x = dmd * (sini1*sina1 + sini2*sina2) * rf;
    delta.y = dmd * (sini1*cosa1 + sini2*cosa2) * rf;
    delta.z = dmd * (cosi1+cosi2) * rf;
}


// Tangential Method
static void getDevTangential( double md1, double md2,
			      double incl2, double azi2, Coord3& delta )
{
    delta.x = (md2-md1) * sin( Math::toRadians(incl2) )
			* sin( Math::toRadians(azi2) );
    delta.y = (md2-md1) * sin( Math::toRadians(incl2) )
			* cos( Math::toRadians(azi2) );
    delta.z = (md2-md1) * cos( Math::toRadians(incl2) );
}


void DirectionalSurvey::calcTrack( const TypeSet<double>& mds,
				   const TypeSet<double>& incls,
				   const TypeSet<double>& azis,
				   TypeSet<Coord3>& track )
{
    Coord3 curpos( surfacecoord_, -kb_ );
    track.add( curpos );
    const bool xyinfeet = SI().xyInFeet();
    const bool mdinfeet = SI().zInFeet();
    for ( int idx=1; idx<mds.size(); idx++ )
    {
	Coord3 delta( 0, 0, 0 );
	if ( method_==MinCurv )
	    getDevMinCurv( mds[idx-1], mds[idx],
			   incls[idx-1], incls[idx],
			   azis[idx-1], azis[idx], delta );
	else
	    getDevTangential( mds[idx-1], mds[idx],
			      incls[idx], azis[idx], delta);

	if ( xyinfeet && !mdinfeet )
	{
	    delta.x *= mToFeetFactorD;
	    delta.y *= mToFeetFactorD;
	}
	else if ( !xyinfeet && mdinfeet )
	{
	    delta.x *= mFromFeetFactorD;
	    delta.y *= mFromFeetFactorD;
	}

	curpos += delta;
	track.add( curpos );
    }
}

} // namespace Well
