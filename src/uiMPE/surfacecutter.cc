/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2005
 RCS:           $Id: surfacecutter.cc,v 1.1 2005-03-17 14:59:17 cvsnanne Exp $
________________________________________________________________________

-*/

#include "surfacecutter.h"
#include "emmanager.h"
#include "emhistory.h"
#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "emsurfacerelations.h"
#include "cubesampling.h"

#include "emtracker.h"
#include "mpeengine.h"
#include "trackplane.h"


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

// TODO removeNodes;

    reTrack();

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
	cuttingsectionid_ = newsectionid;
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


};
