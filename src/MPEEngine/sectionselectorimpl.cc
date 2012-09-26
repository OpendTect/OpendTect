/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "sectionselectorimpl.h"

#include "binidsurface.h"
#include "emhorizon3d.h"
#include "survinfo.h"
#include "trackplane.h"


namespace MPE 
{


BinIDSurfaceSourceSelector::BinIDSurfaceSourceSelector(
	const EM::Horizon3D& hor, const EM::SectionID& sid )
    : SectionSourceSelector( sid )
    , surface_( hor )
{}


void BinIDSurfaceSourceSelector::setTrackPlane( const MPE::TrackPlane& plane )
{
    if ( !plane.isVertical() )
	return;

    const BinID& startbid = plane.boundingBox().hrg.start;
    const BinID& stopbid = plane.boundingBox().hrg.stop;
    const BinID& step = plane.boundingBox().hrg.step;

    BinID currentbid( startbid );
    while ( true )
    {
	const BinID prevbid = currentbid-plane.motion().binid;
	const EM::SubID curnode = currentbid.toInt64();
	const EM::SubID prevnode = prevbid.toInt64();
	const bool curnodedef = surface_.isDefined( sectionid_, curnode );
	const bool prevnodedef = surface_.isDefined( sectionid_, prevnode );

	if ( plane.getTrackMode() == TrackPlane::Erase ||
	     plane.getTrackMode() == TrackPlane::ReTrack )
	{
	    if ( prevnodedef )
		selpos_ += prevnode;
	}
	else if ( plane.getTrackMode()==TrackPlane::Extend )
	{
	    if ( prevnodedef && !curnodedef )
		selpos_ += prevnode;
	}

	if ( startbid.inl==stopbid.inl )
	{
	    currentbid.crl += step.crl;
	    if ( currentbid.crl>stopbid.crl ) break;
	}
	else 
	{
	    currentbid.inl += step.inl;
	    if ( currentbid.inl>stopbid.inl ) break;
	}
    }
}


SurfaceSourceSelector::SurfaceSourceSelector(
	const EM::EMObject& obj, const EM::SectionID& sid )
    : SectionSourceSelector( sid )
    , emobject_( obj )
{}


void SurfaceSourceSelector::setTrackPlane( const MPE::TrackPlane& plane )
{
    mDynamicCastGet( const Geometry::ParametricSurface*,surface,
		     emobject_.sectionGeometry(sectionid_));

    TypeSet<GeomPosID> allnodes;
    surface->getPosIDs( allnodes );

    Interval<int> inlrange( plane.boundingBox().hrg.start.inl,
	    		    plane.boundingBox().hrg.stop.inl );
    Interval<int> crlrange( plane.boundingBox().hrg.start.crl,
	    		    plane.boundingBox().hrg.stop.crl );
    Interval<float> zrange( plane.boundingBox().zrg.start,
	    		    plane.boundingBox().zrg.stop );

    inlrange.include( plane.boundingBox().hrg.start.inl-
	    	      plane.motion().binid.inl );
    crlrange.include( plane.boundingBox().hrg.start.crl-
	    	      plane.motion().binid.crl );
    zrange.include( plane.boundingBox().zrg.start-plane.motion().value );

    for ( int idx=0; idx<allnodes.size(); idx++ )
    {
	const RowCol node = RowCol::fromInt64(allnodes[idx]);
	const Coord3 pos = surface->getKnot(node);
	const BinID bid = SI().transform(pos);
	if ( !inlrange.includes(bid.inl,true) ||
	     !crlrange.includes(bid.crl,true) ||
	     !zrange.includes(pos.z,true) )
	    continue;

	if ( !surface->isAtEdge(node) )
	    continue;

	selpos_ += allnodes[idx];
    }
}


};
