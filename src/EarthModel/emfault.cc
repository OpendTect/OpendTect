/*
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : N. Fredman
 * DATE     : Sep 2002
-*/

static const char* rcsID = "$Id: emfault.cc,v 1.18 2004-07-23 12:54:49 kristofer Exp $";

#include "emfault.h"
#include "emsurfacetr.h"
#include "geommeshsurfaceimpl.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "survinfo.h"


EM::Fault::Fault( EM::EMManager& em_, const EM::ObjectID& mid_ )
    : Surface(em_,mid_)
{}


Geometry::MeshSurface* EM::Fault::createSectionSurface( const SectionID& pid ) const
{
    return new Geometry::MeshSurfaceImpl;
}


const IOObjContext& EM::Fault::getIOObjContext() const
{ return EMFaultTranslatorGroup::ioContext(); }


bool EM::Fault::createFromStick( const TypeSet<Coord3>& stick, float velocity )
{
    if ( stick.size() < 2 ) return false;

    if ( !nrSections() ) addSection( "", true );
    setTranslatorData( RowCol(1,1), RowCol(1,1), RowCol(0,0), 0, 0 );
    const EM::SectionID sectionid = sectionID(0);
    const float idealdistance = 25; // TODO set this in some intelligent way
    RowCol rowcol(0,0);

    bool istimestick = mIsEqual(stick[0].z,stick[stick.size()-1].z,1e-6); 

    for ( int idx=0; idx<stick.size()-1; idx++ )
    {
	const Coord3 startpos( stick[idx], stick[idx].z*velocity/2 );
	const Coord3 stoppos( stick[idx+1], stick[idx+1].z*velocity/2 );

	if ( !startpos.isDefined() || !stoppos.isDefined() )
	    break;

	const BinID startbid = SI().transform( startpos );
	const BinID stopbid = SI().transform( stoppos );

	const float distance = startpos.distance(stoppos);
	const Coord3 vector = (stoppos-startpos).normalize();
	const int nrofsegments = mNINT(distance/idealdistance);
	const float segmentlength = distance/nrofsegments;

	for ( int idy=1; idy<nrofsegments; idy++ )
	{
	    const Coord3 newrelpos( vector.x*segmentlength*idy,
				    vector.y*segmentlength*idy,
				    vector.z*segmentlength*idy );

	    const Coord3 newprojectedpos = startpos+newrelpos;
	    const BinID newprojectedbid = SI().transform( newprojectedpos );
	    const Coord3 newpos( SI().transform(newprojectedbid),
				 newprojectedpos.z/(velocity/2) );
	    setPos( sectionid, rowcol, newpos, false, true );
	    istimestick ? rowcol.row++ : rowcol.col++;
	}

	Coord3 crd( SI().transform(stopbid), stoppos.z/(velocity/2) );
	setPos( sectionid, rowcol, crd, false, true );
	istimestick ? rowcol.row++ : rowcol.col++;
    }

    return true;
}
