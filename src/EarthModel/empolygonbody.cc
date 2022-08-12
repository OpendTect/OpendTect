/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          July 2008
________________________________________________________________________

-*/

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
PolygonBodyUndoEvent( const PosID& posid )
    : posid_( posid )
    , remove_( false )
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( PolygonBody*, polygon, emobj.ptr() );
    if ( !polygon ) return;

    pos_ = polygon->getPos( posid_ );
    const int row = posid_.getRowCol().row();
    normal_ = polygon->geometry().getPolygonNormal( row );
}


//Interface for removal
PolygonBodyUndoEvent( const PosID& posid, const Coord3& oldpos,
		  const Coord3& oldnormal )
    : posid_( posid )
    , pos_( oldpos )
    , normal_( oldnormal )
    , remove_( true )
{}


const char* getStandardDesc() const override
{ return remove_ ? "Remove polygon" : "Insert polygon"; }


bool unDo() override
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( PolygonBody*, polygon, emobj.ptr() );
    if ( !polygon ) return false;

    const int row = posid_.getRowCol().row();

    return remove_
	? polygon->geometry().insertPolygon( row,
		posid_.getRowCol().col(), pos_, normal_, false )
	: polygon->geometry().removePolygon( row, false );
}


bool reDo() override
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( PolygonBody*, polygon, emobj.ptr() );
    if ( !polygon ) return false;

    const int row = posid_.getRowCol().row();

    return remove_
	? polygon->geometry().removePolygon( row, false )
	: polygon->geometry().insertPolygon( row,
		posid_.getRowCol().col(), pos_, normal_, false );
}

protected:

    Coord3	pos_;
    Coord3	normal_;
    PosID	posid_;
    bool	remove_;
};


class PolygonBodyKnotUndoEvent : public UndoEvent
{
public:

//Interface for insert
PolygonBodyKnotUndoEvent( const PosID& posid )
    : posid_( posid )
    , remove_( false )
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    if ( !emobj ) return;
    pos_ = emobj->getPos( posid_ );
}


//Interface for removal
PolygonBodyKnotUndoEvent( const PosID& posid, const Coord3& oldpos )
    : posid_( posid )
    , pos_( oldpos )
    , remove_( true )
{ }


const char* getStandardDesc() const override
{ return remove_ ? "Remove knot" : "Insert knot"; }


bool unDo() override
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( PolygonBody*, polygon, emobj.ptr() );
    if ( !polygon ) return false;

    return remove_
	? polygon->geometry().insertKnot( posid_.subID(), pos_, false )
	: polygon->geometry().removeKnot( posid_.subID(), false );
}


bool reDo() override
{
    RefMan<EMObject> emobj = EMM().getObject( posid_.objectID() );
    mDynamicCastGet( PolygonBody*, polygon, emobj.ptr() );
    if ( !polygon ) return false;

    return remove_
	? polygon->geometry().removeKnot( posid_.subID(), false )
	: polygon->geometry().insertKnot( posid_.subID(), pos_, false );
}

protected:

    Coord3	pos_;
    Coord3	normal_;
    PosID	posid_;
    bool	remove_;
};


mImplementEMObjFuncs( PolygonBody, "Polygon" );

PolygonBody::PolygonBody( EMManager& em )
    : Surface( em )
    , geometry_( *this )
{
}


PolygonBody::~PolygonBody()
{}

ImplicitBody* PolygonBody::createImplicitBody( TaskRunner* taskrunner,
					       bool smooth ) const
{
    const Geometry::PolygonSurface* surf = geometry().geometryElement();
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
    const Geometry::PolygonSurface* surf = geometry().geometryElement();
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


void PolygonBody::refBody()
{ EMObject::ref(); }


void PolygonBody::unRefBody()
{ EMObject::unRef(); }


bool PolygonBody::useBodyPar( const IOPar& par )
{
    if ( !EMObject::usePar( par ) )
	return false;

    return geometry().usePar(par);
}


void PolygonBody::fillBodyPar( IOPar& par ) const
{
    EMObject::fillPar( par );
    geometry().fillPar( par );
}


Executor* PolygonBody::saver()
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    if ( !ioobj )
	return nullptr;

    return saver( ioobj );
}


Executor* PolygonBody::saver( IOObj* inpioobj )
{
    dgbSurfaceWriter* res =
	new dgbSurfaceWriter( inpioobj, typeStr(), *this, false );
    res->setWriteOnlyZ( false );
    return res;
}


Executor* PolygonBody::loader()
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    if ( !ioobj ) { errmsg_ = uiStrings::sCantFindSurf(); return 0; }

    dgbSurfaceReader* rd = new dgbSurfaceReader( *ioobj, typeStr() );
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
    const MultiID& mid = key && !key->isUdf() ? *key : surface_.multiID();
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
    {
	surface_.setErrMsg(uiStrings::sCantFindSurf() );
	return nullptr;
    }

    return surface_.saver( ioobj );
}


Executor* PolygonBodyGeometry::loader( const SurfaceIODataSelection* )
{ return surface_.loader(); }

Geometry::PolygonSurface* PolygonBodyGeometry::geometryElement()
{
    Geometry::Element* res = SurfaceGeometry::geometryElement();
    return (Geometry::PolygonSurface*) res;
}


const Geometry::PolygonSurface* PolygonBodyGeometry::geometryElement() const
{
    const Geometry::Element* res = SurfaceGeometry::geometryElement();
    return (const Geometry::PolygonSurface*) res;
}


Geometry::PolygonSurface* PolygonBodyGeometry::createGeometryElement() const
{ return new Geometry::PolygonSurface; }


EMObjectIterator* PolygonBodyGeometry::createIterator(
					const TrcKeyZSampling* tkzs ) const
{ return new RowColIterator( surface_, tkzs ); }


int PolygonBodyGeometry::nrPolygons() const
{
    const Geometry::PolygonSurface* pol = geometryElement();
    return (!pol || pol->isEmpty()) ? 0 : pol->rowRange().nrSteps()+1;
}


int PolygonBodyGeometry::nrKnots( int polygonnr ) const
{
    const Geometry::PolygonSurface* pol = geometryElement();
    return (!pol || pol->isEmpty()) ? -1 : pol->colRange(polygonnr).nrSteps()+1;
}


bool PolygonBodyGeometry::insertPolygon( int polygonnr,
				int firstknot, const Coord3& pos,
				const Coord3& normal, bool addtohistory )
{
    Geometry::PolygonSurface* pol = geometryElement();
    if ( !pol || !pol->insertPolygon(pos, normal, polygonnr, firstknot) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), RowCol(polygonnr,0) );
	UndoEvent* undo = new PolygonBodyUndoEvent( posid );
	EMM().undo(surface_.id()).addEvent( undo, 0 );
    }

    surface_.setChangedFlag();
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


bool PolygonBodyGeometry::removePolygon( int polygonnr,
					 bool addtohistory )
{
    Geometry::PolygonSurface* pol = geometryElement();
    if ( !pol )	return false;

    const StepInterval<int> colrg = pol->colRange( polygonnr );
    if ( colrg.isUdf() || colrg.width() )
	return false;

    const RowCol rc( polygonnr, colrg.start );
    const Coord3 pos = pol->getKnot( rc );
    const Coord3 normal = getPolygonNormal( polygonnr );
    if ( !normal.isDefined() || !pos.isDefined() )
	return false;

    if ( !pol->removePolygon(polygonnr) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), rc );
	UndoEvent* undo = new PolygonBodyUndoEvent( posid, pos, normal );
	EMM().undo(surface_.id()).addEvent( undo, 0 );
    }

    surface_.setChangedFlag();
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


bool PolygonBodyGeometry::insertKnot( const SubID& subid,
				      const Coord3& pos, bool addtohistory )
{
    Geometry::PolygonSurface* pol = geometryElement();
    RowCol rc = RowCol::fromInt64( subid );
    if ( !pol || !pol->insertKnot(rc,pos) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), subid );
	UndoEvent* undo = new PolygonBodyKnotUndoEvent( posid );
	EMM().undo(surface_.id()).addEvent( undo, 0 );
    }

    surface_.setChangedFlag();
    EMObjectCallbackData cbdata;
    cbdata.event = EMObjectCallbackData::BurstAlert;
    surface_.change.trigger( cbdata );

    return true;
}


const Coord3& PolygonBodyGeometry::getPolygonNormal( int polygonnr ) const
{
    const Geometry::PolygonSurface* pol = geometryElement();
    return pol ? pol->getPolygonNormal(polygonnr) : Coord3::udf();
}


bool PolygonBodyGeometry::removeKnot( const SubID& subid,
				      bool addtohistory )
{
    Geometry::PolygonSurface* pol = geometryElement();
    if ( !pol ) return false;

    RowCol rc = RowCol::fromInt64( subid );
    const Coord3 pos = pol->getKnot( rc );

    if ( !pos.isDefined() || !pol->removeKnot(rc) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), subid );
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
    SectionID sid = SectionID::def();
    const Geometry::PolygonSurface* pol = geometryElement();
    if ( !pol ) return;

    mDefPlgBezierNr( bez, sid.asInt() );
    par.set( bez.buf(), pol->getBezierCurveSmoothness() );

    StepInterval<int> polygonrg = pol->rowRange();
    for ( int polygonnr=polygonrg.start; polygonnr<=polygonrg.stop;
	    polygonnr++ )
    {
	mDefEditNormalStr( editnormstr, sid.asInt(), polygonnr );
	par.set( editnormstr.buf(), pol->getPolygonNormal(polygonnr) );
    }
}


bool PolygonBodyGeometry::usePar( const IOPar& par )
{
    SectionID sid = SectionID::def();
    Geometry::PolygonSurface* pol = geometryElement();
    if ( !pol ) return false;

    int beziernr;
    mDefPlgBezierNr( bez, sid.asInt() );
    par.get( bez.buf(), beziernr );
    pol->setBezierCurveSmoothness( beziernr );

    StepInterval<int> polygonrg = pol->rowRange();
    for ( int polygonnr=polygonrg.start; polygonnr<=polygonrg.stop;
	    polygonnr++ )
    {
	mDefEditNormalStr( editnormstr, sid.asInt(), polygonnr );
	Coord3 editnormal( Coord3::udf() );
	par.get( editnormstr.buf(), editnormal );
	pol->addEditPlaneNormal( editnormal );
    }

    return true;
}

} // namespace EM
