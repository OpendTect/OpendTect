/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: emhorizon2d.cc,v 1.15 2007-09-03 16:19:45 cvsjaap Exp $
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

const char* Horizon2DGeometry::lineidsstr_	 = "Line IDs";
const char* Horizon2DGeometry::linenamesstr_	 = "Line names";
const char* Horizon2DGeometry::linesetprefixstr_ = "Set ID of line ";

    
Horizon2DGeometry::Horizon2DGeometry( Surface& surface )
    : HorizonGeometry( surface )
    , synclineid_(-1)
{}


Geometry::Horizon2DLine*
Horizon2DGeometry::sectionGeometry( const SectionID& sid )
{
    return (Geometry::Horizon2DLine*)SurfaceGeometry::sectionGeometry( sid );
}


const Geometry::Horizon2DLine*
Horizon2DGeometry::sectionGeometry( const SectionID& sid ) const
{
    return (const Geometry::Horizon2DLine*)
	SurfaceGeometry::sectionGeometry( sid );
}


int Horizon2DGeometry::nrLines() const
{ return linenames_.size(); }


int Horizon2DGeometry::lineIndex( int lineid ) const
{ return lineids_.indexOf( lineid ); }


int Horizon2DGeometry::lineIndex( const char* linenm ) const
{ return linenames_.indexOf( linenm ); }


int Horizon2DGeometry::lineID( int idx ) const
{ return idx>=0 && idx<nrLines() ? lineids_[idx] : -1; }


const char* Horizon2DGeometry::lineName( int lid ) const
{
    const int idx = lineids_.indexOf( lid );
    if ( idx>=0 && idx<nrLines() )
    {
	const BufferString& str = linenames_.get(idx);
	return str.size() ? str.buf() : 0;
    }
    
    return 0;
}


const MultiID& Horizon2DGeometry::lineSet( int lid ) const
{
    const int idx = lineids_.indexOf( lid );
    if ( idx>=0 && idx<nrLines() )
	return linesets_[idx];

    static MultiID dummy(-1);
    return dummy;
}


int Horizon2DGeometry::addLine( const MultiID& linesetid, const char* line,
       				int step )
{
    linenames_.add( line );
    linesets_ += linesetid;

    for ( int idx=sections_.size()-1; idx>=0; idx-- )
    {
	Geometry::Horizon2DLine* section =
		reinterpret_cast<Geometry::Horizon2DLine*>( sections_[idx] );
	const int lineid = section->addUdfRow( 0, 0, step );
	if ( idx )
	    continue;

	lineids_ += lineid;
    }

    synclineid_ = lineids_[lineids_.size()-1];
    ((Horizon2D&) surface_).syncGeometry();
    synclineid_ = -1;

    return lineids_[lineids_.size()-1];
}
	    

int Horizon2DGeometry::addLine( const TypeSet<Coord>& path, int start, int step,
				const MultiID& linesetid, const char* line )
{
    linenames_.add( line );
    linesets_ += linesetid;

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


bool Horizon2DGeometry::syncLine( const MultiID& linesetid, const char* linenm,
				  const PosInfo::Line2DData& linegeom )
{
    for ( int lidx=0; lidx<linenames_.size(); lidx++ )
    {
	if ( linesets_[lidx]==linesetid && *linenames_[lidx]==linenm )
	{
	    for ( int idx=0; idx<sections_.size(); idx++ )
	    {
		Geometry::Horizon2DLine* section =
		    reinterpret_cast<Geometry::Horizon2DLine*>(sections_[idx]);
		section->syncRow( lineids_[lidx], linegeom );
	    }
	    return true;
	}
    }
    return false;
}


bool Horizon2DGeometry::syncBlocked( int lineid ) const
{ return synclineid_!=-1 && synclineid_!=lineid; } 


void Horizon2DGeometry::setLineInfo( int lineid, const char* line,
				     const MultiID& linesetid )
{
    const int lineidx = lineIndex( lineid );
    if ( lineidx < 0 )
    {
	lineids_ += lineid;
	linenames_.add( line );
	linesets_ += linesetid;
    }
    else
    {
	linenames_.get( lineidx ) = line;
	linesets_[lineidx] = linesetid;
    }
}


void Horizon2DGeometry::removeLine( int lid )
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


PosID Horizon2DGeometry::getNeighbor( const PosID& pid, bool nextcol,
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


int Horizon2DGeometry::getConnectedPos( const PosID& pid,
					TypeSet<PosID>* res ) const
{
    int nrres = 0;
    PosID neighborpid = getNeighbor( pid, true, true );
    if ( neighborpid.objectID()!=-1 )
    {
	nrres++;
	if ( res ) (*res) += neighborpid;
    }

    neighborpid = getNeighbor( pid, false, true );
    if ( neighborpid.objectID()!=-1 )
    {    
	nrres++;
	if ( res ) (*res) += neighborpid;
    }    

    return nrres;
}


bool Horizon2DGeometry::isAtEdge( const PosID& pid ) const
{
    return getConnectedPos( pid, 0 ) != 2;
}


Geometry::Horizon2DLine* Horizon2DGeometry::createSectionGeometry() const
{ return new Geometry::Horizon2DLine; }


void Horizon2DGeometry::fillPar( IOPar& par ) const
{
    par.set( lineidsstr_, lineids_ );
    par.set( linenamesstr_, linenames_ );

    for ( int idx=0; idx<linesets_.size(); idx++ )
    {
	BufferString linesetkey = linesetprefixstr_;
	linesetkey += idx;
	par.set( linesetkey, linesets_[idx] );
    }
}


bool Horizon2DGeometry::usePar( const IOPar& par )
{
    if ( !par.get(lineidsstr_,lineids_) )
	return false;
    if ( !par.get(linenamesstr_,linenames_)  )
     	return false;	

    for ( int idx=0; idx<lineids_.size(); idx++ )
    {
	BufferString linesetkey = linesetprefixstr_;
	linesetkey += idx;

	MultiID mid;
	linesets_ += par.get(linesetkey,mid) ? mid : MultiID(-1);
    }

    ((Horizon2D&) surface_).syncGeometry();
    return true;
}


mImplementEMObjFuncs( Horizon2D, EMHorizon2DTranslatorGroup::keyword )


Horizon2D::Horizon2D( EMManager& emm )
    : Horizon(emm)
    , geometry_(*this)
{
    geometry_.addSection( "", false );
}


Horizon2D::~Horizon2D()
{}


bool Horizon2D::unSetPos( const PosID& pid, bool addtoundo )
{
    Coord3 pos = getPos( pid );
    pos.z = mUdf(float);
    return setPos( pid, pos, addtoundo );
}


bool Horizon2D::unSetPos( const EM::SectionID& sid, const EM::SubID& subid,
			  bool addtoundo )
{
    Coord3 pos = getPos( sid, subid );
    pos.z = mUdf(float);
    return setPos( sid, subid, pos, addtoundo );
}


void Horizon2D::syncGeometry()
{ manager_.syncGeometry( id() ); }


const IOObjContext& Horizon2D::getIOObjContext() const
{ return EMHorizon2DTranslatorGroup::ioContext(); }

} // namespace EM
