/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: trackplane.cc,v 1.3 2005-03-02 18:40:19 cvskris Exp $";
   

#include "trackplane.h"
#include "mathfunc.h"
#include "survinfo.h"
#include "trigonometry.h"
#include <math.h>

namespace MPE
{


TrackPlane::TrackPlane( const BinID& start, const BinID& stop, float time )
{
    cubesampling.hrg.start = start;
    cubesampling.hrg.stop = stop;
    cubesampling.zrg.start = time;
    cubesampling.zrg.stop = time;
}


TrackPlane::TrackPlane( const BinID& start, const BinID& stop,
	      float starttime, float stoptime )
{
    cubesampling.hrg.start = start;
    cubesampling.hrg.stop = stop;
    cubesampling.zrg.start = starttime;
    cubesampling.zrg.stop = stoptime;
}


bool TrackPlane::isVertical() const
{
    return !mIsZero(cubesampling.zrg.width() ,mDefEps);
}


Coord3 TrackPlane::normal(const FloatMathFunction* mightbeusedlater) const
{
    if ( !isVertical() )
	return Coord3(0,0,1);

    const Coord start = SI().transform(cubesampling.hrg.start);
    const Coord stop = SI().transform(cubesampling.hrg.stop);

    const Coord dir = start-stop;
    return Coord3( Coord(dir.y,-dir.x), 0 ).normalize();
}


float TrackPlane::distance( const Coord3& pos,
			    const FloatMathFunction* t2d ) const
{
    if ( !isVertical() )
    {
	double ownz = t2d
	    ? t2d->getValue(cubesampling.zrg.start)
	    : cubesampling.zrg.start;
	double testz = t2d
	    ? t2d->getValue(pos.z)
	    : pos.z;
	return fabs(ownz-testz);
    }

    const Coord3 start = Coord3(SI().transform(cubesampling.hrg.start),pos.z);
    const Coord3 stop = Coord3(SI().transform(cubesampling.hrg.stop),pos.z);
    const Coord3 dir = start-stop;

    const Line3 line(start,dir);
    return line.distanceToPoint( pos );
}


void TrackPlane::setMotion( int inl, int crl, float z )
{
    motion_.binid.inl = inl;
    motion_.binid.crl = crl;
    motion_.value = z;
}


void TrackPlane::computePlane(Plane3& plane) const
{
    const Coord3 p0( SI().transform(cubesampling.hrg.start),
	    	     cubesampling.zrg.start );
    const Coord3 p1( SI().transform(cubesampling.hrg.start),
	    	     cubesampling.zrg.stop );
    const Coord3 p2( SI().transform(cubesampling.hrg.stop),
	    	     cubesampling.zrg.start );

    plane.set( p0, p1, p2 );
}

};  //namespace
