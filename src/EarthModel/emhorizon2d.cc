/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: emhorizon2d.cc,v 1.2 2006-04-26 21:14:32 cvskris Exp $
________________________________________________________________________

-*/

#include "emhorizon2d.h"

#include "emsurfacetr.h"

#include "emrowcoliterator.h"
#include "errh.h"
#include "horizon2dline.h"
#include "ioman.h"

namespace EM
{

Horizon2Geometry::Horizon2Geometry( Surface& hor2 )
    : RowColSurfaceGeometry( hor2 )
{}


Geometry::Horizon2DLine*
Horizon2Geometry::sectionGeometry( const SectionID& sid )
{
    return (Geometry::Horizon2DLine*) SurfaceGeometry::sectionGeometry(sid);
}


const Geometry::Horizon2DLine*
Horizon2Geometry::sectionGeometry( const SectionID& sid ) const
{
    return (const Geometry::Horizon2DLine*)
	SurfaceGeometry::sectionGeometry(sid);
}


int Horizon2Geometry::nrLines() const
{ return linenames_.size(); }


int Horizon2Geometry::lineID( int idx ) const
{ return idx>=0 && idx<nrLines() ? lineids_[idx] : -1; }


const char* Horizon2Geometry::lineName( int lid ) const
{
    const int idx = lineids_.indexOf( lid );
    if ( idx>=0 && idx<nrLines() )
    {
	const BufferString& str = linenames_.get(idx);
	return str.size() ? str.buf() : 0;
    }
    
    return 0;
}


int Horizon2Geometry::addLine( const TypeSet<Coord>& path, int start, int step,
			 const char* nm )
{
    linenames_.add( nm );

    for ( int idx=sections_.size(); idx>=0; idx-- )
    {
	Geometry::Horizon2DLine* section =
	    reinterpret_cast<Geometry::Horizon2DLine*>(sections_[idx]);
	const int lineid = section->addRow( path, start, step );
	if ( idx )
	    continue;

	lineids_ += lineid;
    }

    return lineids_[lineids_.size()-1];
}


void Horizon2Geometry::removeLine( int lid )
{
    const int lidx = lineids_.indexOf( lid );
    if ( lidx<0 || lidx>=linenames_.size() )
	return;

    linenames_.remove( lidx );
    lineids_.remove( lidx );
    for ( int idx=sections_.size(); idx>=0; idx-- )
    {
	Geometry::Horizon2DLine* section =
	    reinterpret_cast<Geometry::Horizon2DLine*>(sections_[idx]);
	section->removeRow( lid );
    }
}


bool Horizon2Geometry::isAtEdge( const PosID& pid ) const
{
    const int sidx = sids_.indexOf( pid.sectionID() );
    if ( sidx==-1 ) return false;

    const RowCol rc( pid.subID() );
    const StepInterval<int> colrange = colRange( rc.row );
    return rc.col==colrange.start || rc.col==colrange.stop;
}


Geometry::Horizon2DLine* Horizon2Geometry::createSectionGeometry() const
{ return new Geometry::Horizon2DLine; }


Horizon2D::Horizon2D( EMManager& man )
    : Surface( man )
    , geometry_( *this )
{ }


const char* Horizon2D::getTypeStr() const { return "2D Horizon"; }


Horizon2Geometry& Horizon2D::geometry()
{ return geometry_; }


const Horizon2Geometry& Horizon2D::geometry() const
{ return geometry_; }


const IOObjContext& Horizon2D::getIOObjContext() const
{ return EMHorizonTranslatorGroup::ioContext(); }  //Fix onw tr

    

}; //namespace
