/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2004
-*/

static const char* rcsID = "$Id: cubicbeziercurve.cc,v 1.5 2005-02-20 13:42:26 cvskris Exp $";

#include "cubicbeziercurve.h"

#include "errh.h"

#define mRetErr( msg, retval ) { errmsg()=msg; return retval; }

#ifndef mEPS
#define mEPS 1e-10
#endif

namespace Geometry
{

CubicBezierCurve::CubicBezierCurve( const Coord3& c0, const Coord3& c1,
				    int fp, int step_ )
    : firstparam( fp )
    , paramstep( step_ )
    , directioninfluence( step_/3.0 )
    , iscircular(false)
{
    if ( !c0.isDefined() || !c1.isDefined() )
    {
	pErrMsg("Object must be initiated with valid coords" );
	return;
    }

    positions += c0;
    directions += Coord3::udf();
    positions += c1;
    directions += Coord3::udf();
}


CubicBezierCurve* CubicBezierCurve::clone() const
{ return new CubicBezierCurve(*this); }


IntervalND<float> CubicBezierCurve::boundingBox(bool approx) const
{
    if ( approx )
	return ParametricCurve::boundingBox(approx);

    pErrMsg("Not implemnted");
    IntervalND<float> res(3);
    return res;
}


Coord3 CubicBezierCurve::computePosition( float param) const
{
    const StepInterval<int> range = parameterRange();
    int previdx = range.getIndex(param);

    if ( previdx<0 || previdx>positions.size()-1 )
	return Coord3::udf();
    else if ( previdx==positions.size()-1 )
    {
	if ( range.atIndex(previdx)<=param )
	    previdx--;
	else
	    return Coord3::udf();
    }

    const GeomPosID prevparam = range.atIndex(previdx);

    const int nextidx = previdx+1;
    const GeomPosID nextparam = range.atIndex(nextidx);

    const float u = (param-prevparam)/range.step;
    return cubicDeCasteljau( positions[previdx],
	    		     getBezierVertex(prevparam,false),
			     getBezierVertex(nextparam,true),
			     positions[previdx+1], u);
}


Coord3 CubicBezierCurve::computeDirection( float param ) const
{
    pErrMsg("Not implemented");
    return Coord3::udf();
}


StepInterval<int> CubicBezierCurve::parameterRange() const
{
    return StepInterval<int>( firstparam,
			      firstparam+(positions.size()-1)*paramstep,
			      paramstep);
}


Coord3 CubicBezierCurve::getPosition( GeomPosID param ) const
{
    const int idx = getIndex(param);

    if ( idx<0||idx>=positions.size() )
	return Coord3::udf();

    return positions[idx];
}


bool CubicBezierCurve::setPosition( GeomPosID param, const Coord3& np )
{
    if ( !np.isDefined() ) return unsetPosition( param );

    const int idx = getIndex(param);

    if ( idx<-1||idx>positions.size() )
	mRetErr("Cannot add position that is not a neighbor to an existing"
		" position", false );

    if ( idx==-1 )
    {
	positions.insert( 0, np );
	directions.insert( 0, Coord3::udf() );
	firstparam = param;
	triggerNrPosCh( param );
    }
    else if ( idx==positions.size() )
    {
	positions += np;
	directions += Coord3::udf();
	triggerNrPosCh( param );
    }
    else
    {
	positions[idx] = np;
	triggerMovement( param );
    }

    return true;
}


bool CubicBezierCurve::insertPosition( GeomPosID param, const Coord3& np )
{
    if ( !np.isDefined() ) 
	mRetErr("Cannot insert undefined position", false );

    const int idx = getIndex(param);
    if ( idx<0 || idx>=positions.size() )
	return setPosition(param, np );

    positions.insert( idx, np );
    directions.insert( idx, Coord3::udf() );

    TypeSet<GeomPosID> changedpids;
    for ( int idy=idx; idy<positions.size()-1; idy ++ )
	changedpids += firstparam+idy*paramstep;

    triggerMovement( changedpids );
    triggerNrPosCh( firstparam+(positions.size()-1)*paramstep);
    return true;
}


bool CubicBezierCurve::removePosition( GeomPosID param )
{
    const int idx = getIndex(param);
    if ( idx<0 || idx>=positions.size() )
	mRetErr("Cannot remove non-existing position", false );

    if ( idx==0 || idx==positions.size()-1 )
	return unsetPosition(param);

    positions.remove( idx );
    directions.remove( idx );

    TypeSet<GeomPosID> changedpids;
    for ( int idy=idx; idy<positions.size(); idy ++ )
	changedpids += firstparam+idy*paramstep;

    triggerMovement( changedpids );
    triggerNrPosCh( firstparam+positions.size()*paramstep);
    return true;
}


bool CubicBezierCurve::unsetPosition( GeomPosID param )
{
    const int idx = getIndex( param );

    if ( positions.size()<3 )
	mRetErr("Cannot remove positions since too few positions will be left",
		false );

    if ( !idx || idx==positions.size()-1)
    {
	if ( !idx ) firstparam += paramstep;
	positions.remove(idx);
	directions.remove(idx);
	triggerNrPosCh( param );
	return true;
    }

    mRetErr("Cannot remove positions in the middle of a curve, since that "
	    "would split the curve.", false );
}


bool CubicBezierCurve::isDefined( GeomPosID param ) const
{
    const int index = getIndex( param );
    return index>=0 && index<positions.size();
}


Coord3 CubicBezierCurve::getBezierVertex( GeomPosID pid, bool before ) const
{
    const Coord3 basepos = getPosition( pid );
    if ( !basepos.isDefined() ) return basepos;

    const Coord3 dir = getDirection(pid,true);
    if ( !dir.isDefined() ) return dir;
    
    if ( before ) return basepos-dir.normalize()*directioninfluence;
    return basepos+dir.normalize()*directioninfluence;
}


Coord3 CubicBezierCurve::getDirection(	GeomPosID param,
					bool computeifudf ) const
{
    const int idx = getIndex(param);

    if ( idx<0||idx>=directions.size() )
	return Coord3::udf();

    if ( !directions[idx].isDefined() && computeifudf )
	return computeDirection( param );

    return directions[idx];
}


bool CubicBezierCurve::setDirection( GeomPosID param, const Coord3& np )
{
    const int idx = getIndex(param);

    if ( idx<0||idx>=positions.size() )
	mRetErr("No corresponding position", false );

    directions[idx] = np;

    triggerMovement( param );
    return true;
}


bool CubicBezierCurve::unsetDirection( GeomPosID param )
{ return setDirection( param, Coord3::udf()); }


bool CubicBezierCurve::isDirectionDefined( GeomPosID param ) const
{
    const int index = getIndex( param );
    return index>=0 && index<positions.size() && directions[index].isDefined();
}


float CubicBezierCurve::directionInfluence() const { return directioninfluence;}


void CubicBezierCurve::setDirectionInfluence( float ndi )
{
    directioninfluence=ndi;
    triggerMovement();
}


bool CubicBezierCurve::isCircular() const
{ return iscircular && positions.size()>2; }


bool CubicBezierCurve::setCircular(bool yn)
{
    if ( positions.size()<3 ) return false;
    if ( iscircular!=yn )
    {
	iscircular=yn;
	TypeSet<GeomPosID> affectedpids;
	affectedpids += firstparam;
	affectedpids += firstparam + (positions.size()-1)*paramstep;
	triggerMovement( affectedpids );
    }

    return true;
}


Coord3 CubicBezierCurve::computeDirection( GeomPosID param ) const
{
    const int idx = getIndex( param );
    if ( idx<0||idx>=positions.size() )
    {
	pErrMsg("Outside range");
	return Coord3::udf();
    }

    int idx0 = idx-1, idx1 = idx+1;
    int diff = 2;
    if ( !idx )
    {
	idx0 = isCircular() ? positions.size()-1 : 0;
	diff--;
    }
    else if ( idx==positions.size()-1 )
    {
	idx1 = isCircular() ? 0 : positions.size()-1;
	diff--;
    }
   
    const Coord3& c0 = positions[idx0];
    const Coord3& c1 = positions[idx1];

    if ( c0.distance(c1)<mEPS )
	return Coord3::udf();

    return (c1-c0)/(diff)*directioninfluence;
}


/*! Implementation of deCastaleau's algoritm. For more info, refer to
 *  "The NURBS book", figure 1.17. */


Coord3 cubicDeCasteljau( const Coord3& p0, const Coord3& p1,
			 const Coord3& p2, const Coord3& p3, float u )
{
    const float one_minus_u = 1-u;
    Coord3 interpolpos1 = 	p1*one_minus_u+p2*u;

    const Coord3 interpolpos0 = (p0*one_minus_u+p1*u)	* one_minus_u +
				interpolpos1		* u;

    interpolpos1 =		interpolpos1		* one_minus_u +
				(p2*one_minus_u+p3*u) 	* u;

    return interpolpos0*one_minus_u+interpolpos1*u;
}

}; //Namespace

