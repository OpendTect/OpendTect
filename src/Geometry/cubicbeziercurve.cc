/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2004
-*/

static const char* rcsID mUsedVar = "$Id$";

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
    , directioninfluence ( step_/4.0f )
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

#define cubicDeCasteljauPrep \
    const StepInterval<int> range = parameterRange();\
    int previdx = range.getIndex(param);\
\
    if ( previdx<0 || previdx>positions.size()-1 )\
	return Coord3::udf();\
    else if ( previdx==positions.size()-1 )\
    {\
	if ( range.atIndex(previdx)<=param )\
	    previdx--;\
	else\
	    return Coord3::udf();\
    }\
\
    const GeomPosID prevparam = range.atIndex(previdx);\
\
    const int nextidx = previdx+1;\
    const GeomPosID nextparam = range.atIndex(nextidx);\
\
    const float u = (param-prevparam)/range.step;\
    Coord3 temppos[] = { positions[previdx], getBezierVertex(prevparam,false),\
			 getBezierVertex(nextparam,true),\
			 positions[previdx+1] }

Coord3 CubicBezierCurve::computePosition(float param) const
{
    const StepInterval<int> range = parameterRange();
    int previdx = range.getIndex(param);
    int nextidx = previdx+1;

    if ( previdx<0 || previdx>positions.size()-1 )
	return Coord3::udf();
    else if ( previdx==positions.size()-1 )
    {
	if ( range.atIndex(previdx)<=param )
	{
	    if ( !isCircular() )
	    {
		previdx--;
		nextidx--;
	    }
	    else
		nextidx = 0;
	}
	else
	    return Coord3::udf();
    }

    const GeomPosID prevparam = range.atIndex(previdx);
    const GeomPosID nextparam = range.atIndex(nextidx);
    Coord3 temppos[] = { positions[previdx], getBezierVertex(prevparam,false),
			 getBezierVertex(nextparam,true),
			 positions[nextidx] };

    return cubicDeCasteljau( temppos, 0, 1, (param-prevparam)/range.step );
}


Coord3 CubicBezierCurve::computeTangent( float param ) const
{
    cubicDeCasteljauPrep;
    return cubicDeCasteljauTangent( temppos, 0, 1, u );
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

    const Coord3 dir = getTangent(pid,true);
    if ( !dir.isDefined() ) return dir;
    
    if ( before ) 
	return basepos-dir.normalize()*directioninfluence;
    else
    	return basepos+dir.normalize()*directioninfluence;
}


Coord3 CubicBezierCurve::getTangent( GeomPosID param, bool computeifudf ) const
{
    const int idx = getIndex(param);

    if ( idx<0||idx>=directions.size() )
	return Coord3::udf();

    if ( !directions[idx].isDefined() && computeifudf )
	return computeTangent( param );

    return directions[idx];
}


bool CubicBezierCurve::setTangent( GeomPosID param, const Coord3& np )
{
    const int idx = getIndex(param);

    if ( idx<0||idx>=positions.size() )
	mRetErr("No corresponding position", false );

    directions[idx] = np;

    triggerMovement( param );
    return true;
}


bool CubicBezierCurve::unsetTangent( GeomPosID param )
{ return setTangent( param, Coord3::udf()); }


bool CubicBezierCurve::isTangentDefined( GeomPosID param ) const
{
    const int index = getIndex( param );
    return index>=0 && index<positions.size() && directions[index].isDefined();
}


float CubicBezierCurve::directionInfluence() const { return directioninfluence;}


void CubicBezierCurve::setTangentInfluence( float ndi )
{
    directioninfluence=ndi;
    triggerMovement();
}


bool CubicBezierCurve::isCircular() const
{ return iscircular && positions.size()>2; }


bool CubicBezierCurve::setCircular(bool yn)
{
    if ( positions.size()<3 ) return false;

    if ( iscircular != yn )
    {
	iscircular=yn;
	TypeSet<GeomPosID> affectedpids;
	affectedpids += firstparam;
	affectedpids += firstparam + (positions.size()-1)*paramstep;
	triggerMovement( affectedpids );
    }

    return true;
}


Coord3 CubicBezierCurve::computeTangent( GeomPosID param ) const
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

    if ( positions.size()==3 )
	diff = 1;
   
    const Coord3& c0 = positions[idx0];
    const Coord3& c1 = positions[idx1];

    if ( c0.distTo(c1)<mEPS )
	return Coord3::udf();

    return (c1-c0)/diff;
}


}; //Namespace

