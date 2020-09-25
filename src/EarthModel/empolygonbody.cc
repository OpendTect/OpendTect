/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          July 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "empolygonbody.h"

#include "embodyoperator.h"
#include "embodytr.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "emsurfaceio.h"
#include "ioman.h"
#include "survinfo.h"
#include "uistrings.h"
#include "undo.h"

namespace EM {


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
    const int row = posid_.getRowCol().row();
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

    const int row = posid_.getRowCol().row();

    return remove_
	? polygon->geometry().insertPolygon( posid_.sectionID(), row,
		posid_.getRowCol().col(), pos_, normal_, false )
	: polygon->geometry().removePolygon( posid_.sectionID(), row, false );
}


bool reDo()
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( PolygonBody*, polygon, emobj.ptr() );
    if ( !polygon ) return false;

    const int row = posid_.getRowCol().row();

    return remove_
	? polygon->geometry().removePolygon( posid_.sectionID(), row, false )
	: polygon->geometry().insertPolygon( posid_.sectionID(), row,
		posid_.getRowCol().col(), pos_, normal_, false );
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


mImplementEMObjFuncs( PolygonBody, "Polygon" );

PolygonBody::PolygonBody( EMManager& em )
    : Surface( em )
    , geometry_( *this )
{
    geometry_.addSection( "", false );
}


PolygonBody::~PolygonBody()
{}

ImplicitBody* PolygonBody::createImplicitBody( TaskRunner* taskrunner,
					       bool smooth ) const
{
    const EM::SectionID sid = sectionID( 0 );
    const Geometry::PolygonSurface* surf = geometry().sectionGeometry( sid );
    if ( !surf || surf->bodyDimension()<3 )
	return 0;

     const StepInterval<int> rrg = surf->rowRange();
     if ( rrg.isUdf() )
	 return 0;

     TypeSet<Coord3> pts;
     for ( int plg=rrg.start; plg<=rrg.stop; plg += rrg.step )
	 surf->getCubicBezierCurve( plg, pts,
				    mCast(float,SI().zDomain().userFactor()) );

     BodyOperator bodyopt;
     return bodyopt.createImplicitBody( pts, taskrunner );
}


bool PolygonBody::getBodyRange( TrcKeyZSampling& cs )
{
    const Geometry::PolygonSurface* surf =
	geometry().sectionGeometry( sectionID(0) );
    if ( !surf )
	return false;

     TypeSet<Coord3> pts;
     surf->getAllKnots( pts );
     for ( int idx=0; idx<pts.size(); idx++ )
     {
	 cs.hsamp_.include( SI().transform(pts[idx]) );
	 if ( idx )
	     cs.zsamp_.include( (float) pts[idx].z );
	 else
	     cs.zsamp_.start = cs.zsamp_.stop = (float) pts[idx].z;
     }

     return pts.size();
}


PolygonBodyGeometry& PolygonBody::geometry()
{ return geometry_; }


const PolygonBodyGeometry& PolygonBody::geometry() const
{ return geometry_; }


const IOObjContext& PolygonBody::getIOObjContext() const
{
    mDefineStaticLocalObject( PtrMan<IOObjContext>, res, = 0 );
    if ( !res )
    {
	IOObjContext* newres =
	    new IOObjContext(EMBodyTranslatorGroup::ioContext() );
	newres->fixTranslator( typeStr() );

	res.setIfNull(newres,true);
    }

    return *res;
}


MultiID PolygonBody::storageID() const
{ return EMObject::multiID(); }


BufferString PolygonBody::storageName() const
{ return EMObject::name(); }


void EM::PolygonBody::refBody()
{ EM::EMObject::ref(); }


void EM::PolygonBody::unRefBody()
{ EM::EMObject::unRef(); }


bool PolygonBody::useBodyPar( const IOPar& par )
{
    if ( !EM::EMObject::usePar( par ) )
	return false;

    return geometry().usePar(par);
}


void PolygonBody::fillBodyPar( IOPar& par ) const
{
    EM::EMObject::fillPar( par );
    geometry().fillPar( par );
}


Executor* PolygonBody::saver()
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    return saver( ioobj );
}


Executor* PolygonBody::saver( IOObj* inpioobj )
{
    EM::dgbSurfaceWriter* res =
	new EM::dgbSurfaceWriter( inpioobj, typeStr(), *this, false );
    res->setWriteOnlyZ( false );
    return res;
}


Executor* PolygonBody::loader()
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    if ( !ioobj ) { errmsg_ = uiStrings::sCantFindSurf(); return 0; }

    EM::dgbSurfaceReader* rd = new EM::dgbSurfaceReader( *ioobj, typeStr() );
    if ( !rd->isOK() )
    {
	delete rd;
	return 0;
    }

    rd->setOutput( *this );
    rd->setReadOnlyZ( false );
    return rd;
}


PolygonBodyGeometry::PolygonBodyGeometry( PolygonBody& polygon )
    : SurfaceGeometry(polygon)
{}


PolygonBodyGeometry::~PolygonBodyGeometry()
{}


Executor* PolygonBodyGeometry::saver( const SurfaceIODataSelection* newsel,
				      const MultiID* key )
{
    const MultiID& mid = key && !(*key=="") ? *key : surface_.multiID();
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj ) { surface_.setErrMsg(uiStrings::sCantFindSurf() ); return 0; }

    return surface_.saver( ioobj );
}


Executor* PolygonBodyGeometry::loader( const SurfaceIODataSelection* )
{ return surface_.loader(); }

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
	const TrcKeyZSampling* cs) const
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
		RowCol(polygonnr,0).toInt64() );
	UndoEvent* undo = new PolygonBodyUndoEvent( posid );
	EMM().undo(surface_.id()).addEvent( undo, 0 );
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
	const PosID posid( surface_.id(), sid, rc.toInt64() );
	UndoEvent* undo = new PolygonBodyUndoEvent( posid, pos, normal );
	EMM().undo(surface_.id()).addEvent( undo, 0 );
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
    RowCol rc = RowCol::fromInt64( subid );
    if ( !pol || !pol->insertKnot(rc,pos) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), sid, subid );
	UndoEvent* undo = new PolygonBodyKnotUndoEvent( posid );
	EMM().undo(surface_.id()).addEvent( undo, 0 );
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

    RowCol rc = RowCol::fromInt64( subid );
    const Coord3 pos = pol->getKnot( rc );

    if ( !pos.isDefined() || !pol->removeKnot(rc) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), sid, subid );
	UndoEvent* undo = new PolygonBodyKnotUndoEvent( posid, pos );
	EMM().undo(surface_.id()).addEvent( undo, 0 );
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

#define mDefPlgBezierNr( bez, sid ) \
     BufferString bez("BezierCurve nr of section "); bez += sid;


void PolygonBodyGeometry::fillPar( IOPar& par ) const
{
    for ( int idx=0; idx<nrSections(); idx++ )
    {
	EM::SectionID sid = sectionID( idx );
	const Geometry::PolygonSurface* pol = sectionGeometry( sid );
	if ( !pol ) continue;

	mDefPlgBezierNr( bez, sid );
	par.set( bez.buf(), pol->getBezierCurveSmoothness() );

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
	EM::SectionID sid = sectionID( idx );
	Geometry::PolygonSurface* pol = sectionGeometry( sid );
	if ( !pol ) return false;

	int beziernr;
	mDefPlgBezierNr( bez, sid );
	par.get( bez.buf(), beziernr );
	pol->setBezierCurveSmoothness( beziernr );

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
