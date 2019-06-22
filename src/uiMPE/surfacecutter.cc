/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2005
________________________________________________________________________

-*/

#include "surfacecutter.h"
#include "emmanager.h"
#include "undo.h"
#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "emsurfacerelations.h"
#include "cubicbeziersurface.h"
#include "consistencychecker.h"

#include "emtracker.h"
#include "sectiontracker.h"
#include "sectionselector.h"
#include "sectionextender.h"
#include "sectionadjuster.h"
#include "mpeengine.h"
#include "trackplane.h"
#include "trigonometry.h"
#include "survinfo.h"
#include "cubesampling.h"


namespace MPE
{

SurfaceCutter::SurfaceCutter()
    : cuttingobjid_(-1)
    , cuttedobjid_(-1)
{
}


SurfaceCutter::~SurfaceCutter()
{
}


void SurfaceCutter::setCuttingObj( const DBKey& objid )
{
    cuttingobjid_ = objid;
}


void SurfaceCutter::setCuttedObj( const DBKey& objid )
{
    cuttedobjid_ = objid;
}


bool SurfaceCutter::doTerminate( bool positiveside )
{
    /*
    mDynamicCastGet(EM::Surface*,cuttedsurf,EM::MGR().getObject(cuttedobjid_))
    if ( !cuttedsurf || cuttedsurf->sectionIndex(cuttedsectionid_)!=-1 )
	return false;

    mDynamicCastGet(EM::Surface*,cuttingsurf,EM::MGR().getObject(cuttingobjid_))
    if ( !cuttingsurf || cuttingsurf->sectionIndex(cuttingsectionid_)!=-1 )
	return false;

    const int relation = cuttedsurf->relations.setRelation(
	    cuttedsectionid_, cuttingobjid_, cuttingsectionid_, positiveside );

    RowCol dir;
    if ( !getSurfaceDir(dir) ) return false;

    CubeSampling cs;
    getBoundingBox( cs );

    ConsistencyChecker checker( *cuttedsurf );
    const Geometry::ParametricSurface* psurf =
			cuttedsurf->geometry().geometryElement( cuttedsectionid_ );
    const int nrnodes = psurf->nrKnots();
    for ( int idy=0; idy<nrnodes; idy++ )
    {
	const RowCol rc = psurf->getKnotRowCol( idy );
	if ( cs.hrg.includes(rc) )
	{
	    cuttedsurf->setPos( cuttedsectionid_, rc.toInt64(),
					 Coord3(0,0,mUdf(double)), true );
	    continue;
	}

	posid = EM::PosID::getFromRowCol( rc );
	checker.addNodeToCheck( posid );
    }

    checker.nextStep();
    cuttedsurf->geometry().checkSections();

    reTrack();
    */
    return true;
}


bool SurfaceCutter::doCut()
{
    mDynamicCastGet(EM::Surface*,cuttedsurf,EM::MGR().getObject(cuttedobjid_))
    if ( !cuttedsurf )
	return false;

    mDynamicCastGet(EM::Surface*,cuttingsurf,EM::MGR().getObject(cuttingobjid_))
    if ( !cuttingsurf )
	return false;

    const int initialhistnr = EM::MGR().undo().currentEventID();
    if ( doTerminate(true) )
    {
	bool res = doTerminate( false );
	return res;
    }

    while ( EM::MGR().undo().canUnDo() &&
	    EM::MGR().undo().currentEventID()!=initialhistnr )
    {
	bool res = EM::MGR().undo().unDo(1);
	if ( !res ) break;
    }

    EM::MGR().undo().removeAllAfterCurrentEvent();
    return false;
}


bool SurfaceCutter::reTrack()
{
    mDynamicCastGet(EM::Surface*,cuttedsurf,EM::MGR().getObject(cuttedobjid_))
    if ( !cuttedsurf )
	return false;

    const int trackeridx = engine().getTrackerByObject( cuttedobjid_ );
    EMTracker* tracker = engine().getTracker( trackeridx );
    if ( !tracker ) return false;

    CubeSampling cs;
    getBoundingBox( cs );

    RowCol dir;
    getSurfaceDir( dir );

    BinID start, stop, step;
    if ( dir.row > 0 )
    {
	BinID start0 = cs.hrg.start; start0.inl += cs.hrg.step.inl;
	BinID start1 = cs.hrg.start; start1.inl -= cs.hrg.step.inl;
	BinID start2(cs.hrg.stop.inl+cs.hrg.step.inl,cs.hrg.start.crl);
	BinID start3(cs.hrg.stop.inl-cs.hrg.step.inl,cs.hrg.start.crl);
	stop = cs.hrg.stop;
	step = cs.hrg.step; step.inl = 0;
	if ( cuttedsurf->isDefined(EM::PosID::getFromRowCol(start0)) )
	{ start = start0; stop.inl = start0.inl; step *= -1; }
	else if ( cuttedsurf->isDefined(EM::PosID::getFromRowCol(start1)) )
	{ start = start1; stop.inl = start1.inl; }
	else if ( cuttedsurf->isDefined(EM::PosID::getFromRowCol(start2)) )
	{ start = start2; stop.inl = start2.inl; step *= -1; }
	else if ( cuttedsurf->isDefined(EM::PosID::getFromRowCol(start3)) )
	{ start = start3; stop.inl = start3.inl; }
    }
    else
    {
	BinID start0 = cs.hrg.start; start0.crl += cs.hrg.step.crl;
	BinID start1 = cs.hrg.start; start1.crl -= cs.hrg.step.crl;
	BinID start2(cs.hrg.start.inl,cs.hrg.stop.crl+cs.hrg.step.crl);
	BinID start3(cs.hrg.start.inl,cs.hrg.stop.crl-cs.hrg.step.crl);
	stop = cs.hrg.stop;
	step = cs.hrg.step; step.inl = 0;
	if ( cuttedsurf->isDefined(EM::PosID::getFromRowCol(start0)) )
	{ start = start0; stop.crl = start0.crl; step *= -1; }
	else if ( cuttedsurf->isDefined(EM::PosID::getFromRowCol(start1)) )
	{ start = start1; stop.crl = start1.crl; }
	else if ( cuttedsurf->isDefined(EM::PosID::getFromRowCol(start2)) )
	{ start = start2; stop.crl = start2.crl; step *= -1; }
	else if ( cuttedsurf->isDefined(EM::PosID::getFromRowCol(start3)) )
	{ start = start3; stop.crl = start3.crl; }
    }

    float zstart = cs.zrg.start;
    float zstop = cs.zrg.stop;
    start += step;
    stop += step;
    TrackPlane plane( start, stop, zstart, zstop );
    plane.setMotion( step.inl, step.crl, 0 );
    while ( true )
    {
	SectionTracker* sectiontracker = tracker->getSectionTracker( true );
	if ( !sectiontracker ) return true;

	sectiontracker->reset();
	sectiontracker->selector()->setTrackPlane( plane );
	if ( sectiontracker->selector()->selectedPositions().size() == 0 )
	    break;

	sectiontracker->extender()->setDirection( plane.motion() );
	sectiontracker->select();
	if ( sectiontracker->extend() && sectiontracker->adjusterUsed() )
	    sectiontracker->adjust();

	ConsistencyChecker checker( *cuttedsurf );
	const TypeSet<EM::PosID>& addedpos =
				sectiontracker->extender()->getAddedPositions();
	for ( int posidx=0; posidx<addedpos.size(); posidx++ )
	    checker.addNodeToCheck( addedpos[posidx] );

	checker.nextStep();

	CubeSampling& planecs = plane.boundingBox();
	planecs.hrg.start += step;
	planecs.hrg.stop += step;
    }

    return true;
}


#define mConv2IC(crd) \
{ BinID bid = SI().transform( crd ); crd.x = bid.inl; crd.y = bid.crl; }

bool SurfaceCutter::getSurfaceDir( RowCol& dir )
{
    mDynamicCastGet(EM::Surface*,cuttingsurf,EM::MGR().getObject(cuttingobjid_))
    if ( !cuttingsurf )
	return false;

    mDynamicCastGet(const Geometry::CubicBezierSurface*,
		    cbsurf,cuttingsurf->geometry().geometryElement())
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
    mDynamicCastGet(EM::Surface*,cuttingsurf,EM::MGR().getObject(cuttingobjid_))
    IntervalND<float> bb =
	cuttingsurf->geometry().geometryElement()->boundingBox(true);

    const Interval<float>& xrange = bb.getRange(0);
    const Interval<float>& yrange = bb.getRange(1);
    cs.hrg.include( SI().transform(Coord(xrange.start,yrange.start)) );
    cs.hrg.include( SI().transform(Coord(xrange.start,yrange.stop)) );
    cs.hrg.include( SI().transform(Coord(xrange.stop,yrange.start)) );
    cs.hrg.include( SI().transform(Coord(xrange.stop,yrange.stop)) );
    cs.zrg.setFrom( bb.getRange(2) );
}


};

