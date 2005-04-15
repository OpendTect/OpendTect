/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2005
 RCS:           $Id: surfacecutter.cc,v 1.3 2005-04-15 15:35:04 cvsnanne Exp $
________________________________________________________________________

-*/

#include "surfacecutter.h"
#include "emmanager.h"
#include "emhistory.h"
#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "emsurfacerelations.h"
#include "cubesampling.h"
#include "cubicbeziersurface.h"
#include "consistencychecker.h"

#include "emtracker.h"
#include "mpeengine.h"
#include "trackplane.h"
#include "trigonometry.h"
#include "survinfo.h"
#include "cubesampling.h"


namespace MPE
{

SurfaceCutter::SurfaceCutter()
    : cuttingobjid_(-1)
    , cuttingsectionid_(-1)
    , cuttedobjid_(-1)
    , cuttedsectionid_(-1)
{
}


SurfaceCutter::~SurfaceCutter()
{
}


void SurfaceCutter::setCuttingObj( EM::ObjectID objid, EM::SectionID sid )
{
    cuttingobjid_ = objid;
    cuttingsectionid_ = sid;
}


void SurfaceCutter::setCuttedObj( EM::ObjectID objid, EM::SectionID sid )
{
    cuttedobjid_ = objid;
    cuttedsectionid_ = sid;
}


bool SurfaceCutter::doTerminate( bool positiveside )
{
    mDynamicCastGet(EM::Surface*,cuttedsurf,EM::EMM().getObject(cuttedobjid_))
    if ( !cuttedsurf || !cuttedsurf->geometry.hasSection(cuttedsectionid_) )
	return false;

    mDynamicCastGet(EM::Surface*,cuttingsurf,EM::EMM().getObject(cuttingobjid_))
    if ( !cuttingsurf || !cuttingsurf->geometry.hasSection(cuttingsectionid_) )
	return false;

    const int relation = cuttedsurf->relations.setRelation(
	    cuttedsectionid_, cuttingobjid_, cuttingsectionid_, positiveside );

    RowCol dir;
    if ( !getSurfaceDir(dir) ) return false;

    CubeSampling cs;
    getBoundingBox( cs );

    EM::PosID posid( cuttedsurf->id(), cuttedsectionid_ );
    ConsistencyChecker checker( *cuttedsurf );
    const Geometry::ParametricSurface* psurf = 
			cuttedsurf->geometry.getSurface( cuttedsectionid_ );
    const int nrnodes = psurf->nrKnots();
    for ( int idy=0; idy<nrnodes; idy++ )
    {
	const RowCol rc = psurf->getKnotRowCol( idy );
	if ( cs.hrg.includes(rc) )
	{
	    cuttedsurf->geometry.setPos( cuttedsectionid_, rc, 
		    			 Coord3(0,0,mUndefValue), true );
	    continue;
	}
	
	posid.setSubID( rc.getSerialized() );
	checker.addNodeToCheck( posid );
    }

    checker.nextStep();

//  reTrack();
    return true;
}


bool SurfaceCutter::doCut()
{
    mDynamicCastGet(EM::Surface*,cuttedsurf,EM::EMM().getObject(cuttedobjid_))
    if ( !cuttedsurf || !cuttedsurf->geometry.hasSection(cuttedsectionid_) )
	return false;

    mDynamicCastGet(EM::Surface*,cuttingsurf,EM::EMM().getObject(cuttingobjid_))
    if ( !cuttingsurf || !cuttingsurf->geometry.hasSection(cuttingsectionid_) )
	return false;

    const int initialhistnr = EM::EMM().history().currentEventNr();
    const EM::SectionID newsectionid =
		    cuttedsurf->geometry.cloneSection( cuttedsectionid_ );
    if ( newsectionid == -1 )
	return false;

    if ( doTerminate(true) )
    {
	cuttedsectionid_ = newsectionid;
	bool res = doTerminate( false );
	return res;
    }

    while ( EM::EMM().history().canUnDo() &&
	    EM::EMM().history().currentEventNr()!=initialhistnr )
    {
	bool res = EM::EMM().history().unDo(1);
	if ( !res ) break;
    }

    EM::EMM().history().setCurrentEventAsLast();
    return false;
}


bool SurfaceCutter::reTrack()
{
    mDynamicCastGet(EM::Surface*,cuttedsurf,EM::EMM().getObject(cuttedobjid_))
    if ( !cuttedsurf || !cuttedsurf->geometry.hasSection(cuttedsectionid_) )
	return false;

    const int trackeridx = engine().getTrackerByObject( cuttedobjid_ );
    EMTracker* tracker = engine().getTracker( trackeridx );
    if ( !tracker ) return false;

    BinID start;
    BinID stop;
    BinID step( 2, 0 );
    float zstart, zstop;
    TrackPlane plane( start, stop, zstart, zstop );
    plane.setMotion( step.inl, step.crl, 0 );
    while ( tracker->trackSections(plane) )
    {
	CubeSampling& cs = plane.boundingBox();
	cs.hrg.start.inl += step.inl;
	cs.hrg.stop.inl = cs.hrg.start.inl;
    }

    return true;
}


#define mConv2IC(crd) \
{ BinID bid = SI().transform( crd ); crd.x = bid.inl; crd.y = bid.crl; }

bool SurfaceCutter::getSurfaceDir( RowCol& dir )
{
    mDynamicCastGet(EM::Surface*,cuttingsurf,EM::EMM().getObject(cuttingobjid_))
    if ( !cuttingsurf || !cuttingsurf->geometry.hasSection(cuttingsectionid_) )
	return false;
    
    mDynamicCastGet(const Geometry::CubicBezierSurface*,
		    cbsurf,cuttingsurf->geometry.getSurface(0))
    if ( !cbsurf ) return false;
    
    StepInterval<int> rowrg = cbsurf->rowRange();
    StepInterval<int> colrg = cbsurf->colRange();
    Coord3 crd1 = cbsurf->getKnot( RowCol(rowrg.start,colrg.start));
    Coord3 crd2 = cbsurf->getKnot( RowCol(rowrg.start,colrg.stop) );
    Coord3 crd3 = cbsurf->getKnot( RowCol(rowrg.stop,colrg.start) );
    mConv2IC( crd1 ); mConv2IC( crd2 ); mConv2IC( crd3 );
    Plane3 plane( crd1, crd2, crd3 );
    Coord3 normal = plane.normal();
    normal.z = 0; 
    normal = normal.normalize();

    dir.row = fabs(normal.x) >= 0.5 ? 1 : 0;
    dir.col = fabs(normal.y) > 0.5 ? 1 : 0;

    return true;
}


void SurfaceCutter::getBoundingBox( CubeSampling& cs )
{
    cs.hrg.set( Interval<int>(mUdf(int),-mUdf(int)), 
	    	Interval<int>(mUdf(int),-mUdf(int)) );
    mDynamicCastGet(EM::Surface*,cuttingsurf,EM::EMM().getObject(cuttingobjid_))
    IntervalND<float> bb =
	cuttingsurf->geometry.getSurface(cuttingsectionid_)->boundingBox(true);

    const Interval<float>& xrange = bb.getRange(0);
    const Interval<float>& yrange = bb.getRange(1);
    cs.hrg.include( SI().transform(Coord(xrange.start,yrange.start)) );
    cs.hrg.include( SI().transform(Coord(xrange.start,yrange.stop)) );
    cs.hrg.include( SI().transform(Coord(xrange.stop,yrange.start)) );
    cs.hrg.include( SI().transform(Coord(xrange.stop,yrange.stop)) );
    assign( cs.zrg, bb.getRange(2) );
}


};
