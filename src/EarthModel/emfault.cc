/*
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : N. Fredman
 * DATE     : Sep 2002
-*/

static const char* rcsID = "$Id: emfault.cc,v 1.23 2004-09-17 12:43:16 kristofer Exp $";

#include "emfault.h"
#include "emsurfacetr.h"
#include "geommeshsurfaceimpl.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "survinfo.h"

namespace EM {


Fault::Fault( EMManager& em_, const ObjectID& mid_ )
    : Surface(em_,mid_, *new FaultGeometry(*this) )
{}


const IOObjContext& Fault::getIOObjContext() const
{ return EMFaultTranslatorGroup::ioContext(); }


FaultGeometry::FaultGeometry(Fault& flt)
    : SurfaceGeometry( flt )
{}


bool FaultGeometry::createFromStick( const TypeSet<Coord3>& stick, 
				     const SectionID& sid, float velocity )
{
    if ( stick.size() < 2 ) return false;

    SectionID sectionid = sid;
    if ( !nrSections() || !hasSection(sid) ) 
	sectionid = addSection( "", true );

    setTranslatorData( RowCol(1,1), RowCol(1,1), RowCol(0,0), 0, 0 );
    const float idealdistance = 25; // TODO set this in some intelligent way
    RowCol rowcol(0,0);

    bool istimestick = mIsEqual(stick[0].z,stick[stick.size()-1].z,1e-6); 

    Coord3 stoppos;
    BinID prevbid(-1,-1);
    for ( int idx=0; idx<stick.size()-1; idx++ )
    {
	const Coord3 startpos( SI().transform(SI().transform(stick[idx])),
				stick[idx].z*velocity/2 );
	stoppos = Coord3( SI().transform(SI().transform(stick[idx+1])),
				stick[idx+1].z*velocity/2 );

	if ( !startpos.isDefined() || !stoppos.isDefined() )
	    break;

	const BinID startbid = SI().transform( startpos );
	const BinID stopbid = SI().transform( stoppos );

	const float distance = startpos.distance(stoppos);
	const Coord3 vector = (stoppos-startpos).normalize();
	const int nrofsegments = mNINT(distance/idealdistance);
	const float segmentlength = distance/nrofsegments;

	for ( int idy=0; idy<nrofsegments; idy++ )
	{
	    const Coord3 newrelpos( vector.x*segmentlength*idy,
				    vector.y*segmentlength*idy,
				    vector.z*segmentlength*idy );

	    const Coord3 newprojectedpos = startpos+newrelpos;
	    const BinID newprojectedbid = SI().transform( newprojectedpos );
	    if ( istimestick && newprojectedbid==prevbid )
		continue; 

	    prevbid = newprojectedbid;

	    const Coord3 newpos( SI().transform(newprojectedbid),
				 newprojectedpos.z/(velocity/2) );
	    setPos( sectionid, rowcol, newpos, false, true );
	    if ( !idy )
	    {
		surface.setPosAttrib(
			PosID(surface.id(), sectionid, rowCol2SubID(rowcol)),
			EMObject::sPermanentControlNode, true);
	    }
	    istimestick ? rowcol.col++ : rowcol.row++;
	}
    }

    Coord3 crd( SI().transform(SI().transform(stick[stick.size()-1])),
	    	stick[stick.size()-1].z );
    setPos( sectionid, rowcol, crd, false, true );
    surface.setPosAttrib( PosID(surface.id(), sectionid,
			  rowCol2SubID(rowcol)),
			  EMObject::sPermanentControlNode, true);

    return true;
}


Geometry::MeshSurface*
EM::FaultGeometry::createSectionSurface( const SectionID& pid ) const
{
    return new Geometry::MeshSurfaceImpl;
}



}; //namespace
