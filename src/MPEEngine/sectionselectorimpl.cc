/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: sectionselectorimpl.cc,v 1.3 2005-03-02 18:41:03 cvskris Exp $";

#include "sectionselectorimpl.h"

#include "binidsurface.h"
#include "emhorizon.h"
#include "geomtube.h"
#include "survinfo.h"
#include "trackplane.h"


namespace MPE 
{


BinIDSurfaceSourceSelector::BinIDSurfaceSourceSelector(
	const EM::Horizon& hor, const EM::SectionID& sid )
    : SectionSourceSelector( hor, sid )
    , surface( hor )
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
	if ( surface.geometry.isDefined(sectionid, currentbid) &&
	     surface.geometry.isDefined(sectionid,
		 			currentbid+plane.motion().binid) )
	{
	    selpos += currentbid.getSerialized();
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
	const EM::EMObject& obj_, const EM::SectionID& sid )
    : SectionSourceSelector( obj_, sid )
    , emobject( obj_ )
{}


void SurfaceSourceSelector::setTrackPlane( const MPE::TrackPlane& plane )
{
    if ( !plane.isVertical() )
	return;

    mDynamicCastGet( const Geometry::ParametricSurface*,surface,
		     emobject.getElement(sectionid));

    TypeSet<GeomPosID> allnodes;
    surface->getPosIDs( allnodes );

    Interval<int> inlrange(plane.boundingBox().hrg.start.inl,
	    		   plane.boundingBox().hrg.stop.inl );
    Interval<int> crlrange(plane.boundingBox().hrg.start.crl,
	    		   plane.boundingBox().hrg.stop.crl );

    inlrange.include( plane.boundingBox().hrg.start.inl+
	    	      plane.motion().binid.inl);
    crlrange.include( plane.boundingBox().hrg.start.crl+
	    	      plane.motion().binid.crl);

    for ( int idx=0; idx<allnodes.size(); idx++ )
    {
	const RowCol node(allnodes[idx]);
	const Coord3 pos = surface->getKnot(node);
	const BinID bid = SI().transform(pos);
	if ( !inlrange.includes(bid.inl) || !crlrange.includes(bid.crl) )
	    continue;

	if ( !surface->isAtEdge(node) )
	    continue;

	selpos += allnodes[idx];
    }
}


};
