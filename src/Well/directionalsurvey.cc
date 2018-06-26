/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Farrukh Qayyum
 Date:		August 2017
________________________________________________________________________

-*/


#include "directionalsurvey.h"

#include "geometry.h"
#include "math2.h"


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
    delta.x_ += dmd * (sini1*sina1 + sini2*sina2) * rf;
    delta.y_ += dmd * (sini1*cosa1 + sini2*cosa2) * rf;
    delta.z_ += dmd * (cosi1+cosi2) * rf;
}


// Tangential Method
static void getDevTangential( double md1, double md2,
			      double incl2, double azi2, Coord3& delta )
{
    delta.x_ += (md2-md1) * sin( Math::toRadians(incl2) )
			  * sin( Math::toRadians(azi2) );
    delta.y_ += (md2-md1) * sin( Math::toRadians(incl2) )
			  * cos( Math::toRadians(azi2) );
    delta.z_ += (md2-md1) * cos( Math::toRadians(incl2) );
}


void DirectionalSurvey::calcTrack( const TypeSet<double>& mds,
				   const TypeSet<double>& incls,
				   const TypeSet<double>& azis,
				   TypeSet<Coord3>& track )
{
    const Coord3 crd0( surfacecoord_, -kb_ );
    track.add( crd0 );
    Coord3 delta( 0, 0, 0 );
    for ( int idx=1; idx<mds.size(); idx++ )
    {
	if ( method_==MinCurv )
	    getDevMinCurv( mds[idx-1], mds[idx],
			   incls[idx-1], incls[idx],
			   azis[idx-1], azis[idx], delta );
	else
	    getDevTangential( mds[idx-1], mds[idx],
			      incls[idx], azis[idx], delta);

	const Coord3 newcrd = crd0 + delta;
	track.add( newcrd );
    }
}

} // namespace Well
