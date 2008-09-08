/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          July 2008
 RCS:           $Id: empolygonbody.cc,v 1.1 2008-09-08 17:41:28 cvskris Exp $
________________________________________________________________________

-*/

#include "empolygonbody.h"

#include "emsurfacetr.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "undo.h"
#include "errh.h"
#include "rowcol.h"
#include "survinfo.h"
#include "sortedtable.h"
#include "embodytr.h"
#include "tabledef.h"
#include "unitofmeasure.h"

namespace EM {

mImplementEMObjFuncs( PolygonBody, polygonEMBodyTranslator::sKeyUserName() );

class PolygonBodyUndoEvent : public UndoEvent
{
public:

//Interface for insert
PolygonBodyUndoEvent( const EM::PosID& posid )
    : posid_( posid )
    , remove_( false )
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( PolygonBody*, polygon, emobj.ptr() );
    if ( !polygon ) return;

    pos_ = polygon->getPos( posid_ );
    const int row = RowCol(posid_.subID()).row;
    normal_ = polygon->geometry().getPolygonNormal( posid_.sectionID(), row );
}


//Interface for removal
PolygonBodyUndoEvent( const EM::PosID& posid, const Coord3& oldpos,
		  const Coord3& oldnormal )
    : posid_( posid )
    , pos_( oldpos )
    , normal_( oldnormal )
    , remove_( true )
{}


const char* getStandardDesc() const
{ return remove_ ? "Remove polygon" : "Insert polygon"; }


bool unDo()
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( PolygonBody*, polygon, emobj.ptr() );
    if ( !polygon ) return false;

    const int row = RowCol(posid_.subID()).row;

    return remove_
	? polygon->geometry().insertPolygon( posid_.sectionID(), row,
		RowCol(posid_.subID()).col, pos_, normal_, false )
	: polygon->geometry().removePolygon( posid_.sectionID(), row, false );
}


bool reDo()
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( PolygonBody*, polygon, emobj.ptr() );
    if ( !polygon ) return false;

    const int row = RowCol(posid_.subID()).row;

    return remove_
	? polygon->geometry().removePolygon( posid_.sectionID(), row, false )
	: polygon->geometry().insertPolygon( posid_.sectionID(), row,
		RowCol(posid_.subID()).col, pos_, normal_, false );
}

protected:

    Coord3	pos_;
    Coord3	normal_;
    EM::PosID	posid_;
    bool	remove_;
};

    
class PolygonBodyKnotUndoEvent : public UndoEvent
{
public:

//Interface for insert
PolygonBodyKnotUndoEvent( const EM::PosID& posid )
    : posid_( posid )
    , remove_( false )
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    if ( !emobj ) return;
    pos_ = emobj->getPos( posid_ );
}


//Interface for removal
PolygonBodyKnotUndoEvent( const EM::PosID& posid, const Coord3& oldpos )
    : posid_( posid )
    , pos_( oldpos )
    , remove_( true )
{ }


const char* getStandardDesc() const
{ return remove_ ? "Remove knot" : "Insert knot"; }


bool unDo()
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( PolygonBody*, polygon, emobj.ptr() );
    if ( !polygon ) return false;

    return remove_
	? polygon->geometry().insertKnot( posid_.sectionID(), posid_.subID(),
					  pos_, false )
	: polygon->geometry().removeKnot( posid_.sectionID(), posid_.subID(),
					  false );
}


bool reDo()
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( PolygonBody*, polygon, emobj.ptr() );
    if ( !polygon ) return false;

    return remove_
	? polygon->geometry().removeKnot( posid_.sectionID(), posid_.subID(),
					  false )
	: polygon->geometry().insertKnot( posid_.sectionID(), posid_.subID(),
					  pos_, false );
}

protected:

    Coord3	pos_;
    Coord3	normal_;
    EM::PosID	posid_;
    bool	remove_;
};

    
PolygonBody::PolygonBody( EMManager& em )
    : Surface( em )
    , geometry_( *this )
{
    geometry_.addSection( "", false );
}


PolygonBody::~PolygonBody()
{}


ImplicitBody* PolygonBody::createImplicitBody( TaskRunner* tr ) const
{
    return 0;
}
/*
    const EM::SectionID sid = sectionID( 0 );
    const Geometry::PolygonSurface* surf = geometry().sectionGeometry( sid );
    
    if ( !surf )
	return 0;

    Geometry::ExplPolygonSurface explpoly( surf );
    explpoly.display( false, true );

    if ( !update( true, tr ) )
	return 0;

    const ObjectSet<IndexedGeometry> indexedgeoms = explpoly.getGeometry();
    const Coord3List* xyzcoords = explpoly.coordList();
    if ( !indexedgeoms.size() || !xyzcoords )
	return 0;

    SortedTable<BinIDValue> bidvals;

    int curid = xyzcoords->nextID( -1 );
    while ( curid>=0 )
    {
	const Coord3 crd = xyzcoords->get( curid );
	if ( !crd.isDefined() )
	    return 0;

	bidvals.set( curid, BinIDValue( SI().transform(crd), crd.z ) );
	curid = xyzcoords->nextID( curid );
    }

    for ( int idx=0; idx<indexedgeoms.size(); idx++ )
    {
    }

    return 0;
}

*/


PolygonBodyGeometry& PolygonBody::geometry()
{ return geometry_; }


const PolygonBodyGeometry& PolygonBody::geometry() const
{ return geometry_; }


const IOObjContext& PolygonBody::getIOObjContext() const
{ return EMBodyTranslatorGroup::ioContext(); }


PolygonBodyGeometry::PolygonBodyGeometry( PolygonBody& polygon )
    : SurfaceGeometry(polygon)
{}


PolygonBodyGeometry::~PolygonBodyGeometry()
{}


Geometry::PolygonSurface* 
PolygonBodyGeometry::sectionGeometry( const SectionID& sid )
{
    Geometry::Element* res = SurfaceGeometry::sectionGeometry( sid );
    return (Geometry::PolygonSurface*) res;
}


const Geometry::PolygonSurface* PolygonBodyGeometry::sectionGeometry( 
	const SectionID& sid ) const
{
    const Geometry::Element* res = SurfaceGeometry::sectionGeometry( sid );
    return (const Geometry::PolygonSurface*) res;
}


Geometry::PolygonSurface* PolygonBodyGeometry::createSectionGeometry() const
{ return new Geometry::PolygonSurface; }


EMObjectIterator* PolygonBodyGeometry::createIterator( const SectionID& sid,
       	const CubeSampling* cs) const
{ return new RowColIterator( surface_, sid, cs ); }


int PolygonBodyGeometry::nrPolygons( const SectionID& sid ) const
{
    const Geometry::PolygonSurface* pol = sectionGeometry( sid );
    return (!pol || pol->isEmpty()) ? 0 : pol->rowRange().nrSteps()+1;
}


int PolygonBodyGeometry::nrKnots( const SectionID& sid, int polygonnr ) const
{
    const Geometry::PolygonSurface* pol = sectionGeometry( sid );
    return (!pol || pol->isEmpty()) ? -1 : pol->colRange(polygonnr).nrSteps()+1;
}


bool PolygonBodyGeometry::insertPolygon( const SectionID& sid, int polygonnr, 
				int firstknot, const Coord3& pos, 
				const Coord3& normal, bool addtohistory )
{
    Geometry::PolygonSurface* pol = sectionGeometry( sid );
    if ( !pol || !pol->insertPolygon(pos, normal, polygonnr, firstknot) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), sid, 
		RowCol(polygonnr,0).getSerialized() );
	UndoEvent* undo = new PolygonBodyUndoEvent( posid );
	EMM().undo().addEvent( undo, 0 );
    }

    surface_.setChangedFlag();
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


bool PolygonBodyGeometry::removePolygon( const SectionID& sid, int polygonnr,
					 bool addtohistory )
{
    Geometry::PolygonSurface* pol = sectionGeometry( sid );
    if ( !pol )	return false;

    const StepInterval<int> colrg = pol->colRange( polygonnr );
    if ( colrg.isUdf() || colrg.width() )
	return false;

    const RowCol rc( polygonnr, colrg.start );
    const Coord3 pos = pol->getKnot( rc );
    const Coord3 normal = getPolygonNormal( sid, polygonnr );
    if ( !normal.isDefined() || !pos.isDefined() )
	return false;
    
    if ( !pol->removePolygon(polygonnr) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), sid, rc.getSerialized() );
	UndoEvent* undo = new PolygonBodyUndoEvent( posid, pos, normal );
	EMM().undo().addEvent( undo, 0 );
    }

    surface_.setChangedFlag();
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


bool PolygonBodyGeometry::insertKnot( const SectionID& sid, const SubID& subid,
				      const Coord3& pos, bool addtohistory )
{
    Geometry::PolygonSurface* pol = sectionGeometry( sid );
    RowCol rc;
    rc.setSerialized( subid );
    if ( !pol || !pol->insertKnot(rc,pos) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), sid, subid );
	UndoEvent* undo = new PolygonBodyKnotUndoEvent( posid );
	EMM().undo().addEvent( undo, 0 );
    }

    surface_.setChangedFlag();
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


const Coord3& PolygonBodyGeometry::getPolygonNormal( const SectionID& sid,
						     int polygonnr ) const
{
    const Geometry::PolygonSurface* pol = sectionGeometry( sid );
    return pol ? pol->getPolygonNormal(polygonnr) : Coord3::udf();
}


bool PolygonBodyGeometry::removeKnot( const SectionID& sid, const SubID& subid,
				      bool addtohistory )
{
    Geometry::PolygonSurface* pol = sectionGeometry( sid );
    if ( !pol ) return false;

    RowCol rc;
    rc.setSerialized( subid );
    const Coord3 pos = pol->getKnot( rc );

    if ( !pos.isDefined() || !pol->removeKnot(rc) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), sid, subid );
	UndoEvent* undo = new PolygonBodyKnotUndoEvent( posid, pos );
	EMM().undo().addEvent( undo, 0 );
    }

    surface_.setChangedFlag();
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


#define mDefEditNormalStr( editnormstr, sid, polygonnr ) \
    BufferString editnormstr("Edit normal of section "); \
    editnormstr += sid; editnormstr += " polygonnr "; editnormstr += polygonnr; 


#define mDefKnotsStr( knotstr, sid, polygonnr, knotidx ) \
    BufferString knotstr("Edit knots of section "); knotstr += sid; \
    knotstr += " polygon "; knotstr += polygonnr; \
    knotstr += " knot "; knotstr += knotidx;

void PolygonBodyGeometry::fillPar( IOPar& par ) const
{
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	int sid = sectionID( idx );
	const Geometry::PolygonSurface* pol = sectionGeometry( sid );
	if ( !pol ) continue;

	StepInterval<int> polygonrg = pol->rowRange();
	for ( int polygonnr=polygonrg.start; polygonnr<=polygonrg.stop; 
		polygonnr++ )
	{
	    mDefEditNormalStr( editnormstr, sid, polygonnr );
	    par.set( editnormstr.buf(), pol->getPolygonNormal(polygonnr) );
	}
    }
}


bool PolygonBodyGeometry::usePar( const IOPar& par )
{
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	int sid = sectionID( idx );
	Geometry::PolygonSurface* pol = sectionGeometry( sid );
	if ( !pol ) return false;

	StepInterval<int> polygonrg = pol->rowRange();
	for ( int polygonnr=polygonrg.start; polygonnr<=polygonrg.stop; 
		polygonnr++ )
	{
	    mDefEditNormalStr( editnormstr, sid, polygonnr );
	    Coord3 editnormal( Coord3::udf() ); 
	    par.get( editnormstr.buf(), editnormal ); 
	    pol->addEditPlaneNormal( editnormal );
	}
    }

    return true;
}


} // namespace EM
