/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: emhorizon2d.cc,v 1.7 2006-12-22 10:13:35 cvsjaap Exp $
________________________________________________________________________

-*/

#include "emhorizon2d.h"

#include "emsurfacetr.h"
#include "emmanager.h"
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


const MultiID& Horizon2Geometry::lineSet( int lid ) const
{
    const int idx = lineids_.indexOf( lid );
    if ( idx>=0 && idx<nrLines() )
	return linesets_[idx];

    static MultiID dummy(-1);
    return dummy;
}


int Horizon2Geometry::addLine( const TypeSet<Coord>& path, int start, int step,
			 const MultiID& lineset, const char* line )
{
    linenames_.add( line );
    linesets_ +=  lineset;

    for ( int idx=sections_.size()-1; idx>=0; idx-- )
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
    linesets_.remove( lidx );
    lineids_.remove( lidx );
    for ( int idx=sections_.size()-1; idx>=0; idx-- )
    {
	Geometry::Horizon2DLine* section =
	    reinterpret_cast<Geometry::Horizon2DLine*>(sections_[idx]);
	section->removeRow( lid );
    }
}


PosID Horizon2Geometry::getNeighbor( const PosID& pid, bool nextcol,
				     bool retundef ) const
{
    const RowCol rc( pid.subID() );
    TypeSet<PosID> aliases;
    getLinkedPos( pid, aliases );
    aliases += pid;

    const int nraliases = aliases.size();
    for ( int idx=0; idx<nraliases; idx++ )
    {
	const SectionID sid = aliases[idx].sectionID();
	const RowCol ownrc( aliases[idx].subID() );
	const int colstep = colRange( sid, ownrc.row ).step;
	const RowCol neighborrc( ownrc.row,
		nextcol ? ownrc.col+colstep : ownrc.col-colstep );

	if ( surface_.isDefined( sid, neighborrc.getSerialized() ) ||
	     (!retundef && idx==nraliases-1) )
	    return PosID( surface_.id(), sid, neighborrc.getSerialized() );
    }

    return PosID::udf();
}


int Horizon2Geometry::getConnectedPos(const PosID& pid,
				      TypeSet<PosID>* res) const
{
    int nrres = 0;
    PosID neighborpid = getNeighbor( pid, true, true);
    if ( neighborpid.objectID()!=-1 )
    {
	nrres++;
	if ( res ) (*res) += neighborpid;
    }

    neighborpid = getNeighbor( pid, false, true);
    if ( neighborpid.objectID()!=-1 )
    {    
	nrres++;
	if ( res ) (*res) += neighborpid;
    }    

    return nrres;
}


bool Horizon2Geometry::isAtEdge( const PosID& pid ) const
{
    return getConnectedPos( pid, 0 )!=2;
}


Geometry::Horizon2DLine* Horizon2Geometry::createSectionGeometry() const
{ return new Geometry::Horizon2DLine; }


const char* Horizon2D::typeStr() { return EMHorizon2DTranslatorGroup::keyword; }


EMObject* Horizon2D::create( EMManager& emm )
{ return new Horizon2D( emm ); }


void Horizon2D::initClass(EMManager& emm)
{
    ObjectFactory* no = new ObjectFactory( create,
	    EMHorizon2DTranslatorGroup::ioContext(), typeStr());
    emm.addFactory( no );
}


Horizon2D::Horizon2D( EMManager& man )
    : Surface( man )
    , geometry_( *this )
{
    geometry_.addSection( "", false );
}


const char* Horizon2D::getTypeStr() const
{ return typeStr(); }


Horizon2Geometry& Horizon2D::geometry()
{ return geometry_; }


const Horizon2Geometry& Horizon2D::geometry() const
{ return geometry_; }


const IOObjContext& Horizon2D::getIOObjContext() const
{ return EMHorizon2DTranslatorGroup::ioContext(); }  //Fix onw tr

    

}; //namespace
