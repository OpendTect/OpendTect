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
#include "survinfo.h"
#include "uistrings.h"
#include "undo.h"

namespace EM {


class PolygonBodyUndoEvent : public ::UndoEvent
{
public:

//Interface for insert
PolygonBodyUndoEvent( const EM::PosID& posid )
    : posid_( posid )
    , remove_( false )
{
    RefMan<Object> emobj = BodyMan().getObject( DBKey::getInvalid() );
    mDynamicCastGet( PolygonBody*, polygon, emobj.ptr() );
    if ( polygon )
    {
	pos_ = polygon->getPos( posid_ );
	const int row = posid_.getRowCol().row();
	normal_ = polygon->geometry().getPolygonNormal( row );
    }
}


//Interface for removal
PolygonBodyUndoEvent( const EM::PosID& posid, const Coord3& oldpos,
		      const Coord3& oldnormal )
    : posid_( posid )
    , pos_( oldpos )
    , normal_( oldnormal )
    , remove_( true )
{
}


const char* getStandardDesc() const
{ return remove_ ? "Remove polygon" : "Insert polygon"; }


bool unDo()
{
    RefMan<Object> emobj = BodyMan().getObject( DBKey::getInvalid() );
    mDynamicCastGet( PolygonBody*, polygon, emobj.ptr() );
    if ( !polygon ) return false;

    const int row = posid_.getRowCol().row();

    return remove_
	? polygon->geometry().insertPolygon( row,
		posid_.getRowCol().col(), pos_, normal_, false )
	: polygon->geometry().removePolygon( row, false );
}


bool reDo()
{
    RefMan<Object> emobj = BodyMan().getObject( DBKey::getInvalid() );
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
    EM::PosID	posid_;
    bool	remove_;
};


class PolygonBodyKnotUndoEvent : public ::UndoEvent
{
public:

//Interface for insert
PolygonBodyKnotUndoEvent( const EM::PosID& posid )
    : posid_( posid )
    , remove_( false )
{
    RefMan<Object> emobj = BodyMan().getObject( DBKey::getInvalid() );
    if ( emobj )
	pos_ = emobj->getPos( posid_ );
}


//Interface for removal
PolygonBodyKnotUndoEvent( const EM::PosID& posid, const Coord3& oldpos )
    : posid_( posid )
    , pos_( oldpos )
    , remove_( true )
{
}


const char* getStandardDesc() const
{ return remove_ ? "Remove knot" : "Insert knot"; }


bool unDo()
{
    RefMan<Object> emobj = BodyMan().getObject( DBKey::getInvalid() );
    mDynamicCastGet( PolygonBody*, polygon, emobj.ptr() );
    if ( !polygon ) return false;

    return remove_
	? polygon->geometry().insertKnot( posid_, pos_, false )
	: polygon->geometry().removeKnot( posid_, false );
}


bool reDo()
{
    RefMan<Object> emobj = BodyMan().getObject( DBKey::getInvalid() );
    mDynamicCastGet( PolygonBody*, polygon, emobj.ptr() );
    if ( !polygon ) return false;

    return remove_
	? polygon->geometry().removeKnot( posid_, false )
	: polygon->geometry().insertKnot( posid_, pos_, false );
}

protected:

    Coord3	pos_;
    Coord3	normal_;
    EM::PosID	posid_;
    bool	remove_;
};


mImplementEMObjFuncs( PolygonBody, "Polygon" );

PolygonBody::PolygonBody( const char* nm )
    : Surface(nm)
    , geometry_( *this )
{
}


PolygonBody::~PolygonBody()
{
}


ImplicitBody* PolygonBody::createImplicitBody( const TaskRunnerProvider& trprov,
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
     return bodyopt.createImplicitBody( pts, trprov );
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
	 cs.hsamp_.include( SI().transform(pts[idx].getXY()) );
	 if ( idx )
	     cs.zsamp_.include( (float) pts[idx].z_ );
	 else
	     cs.zsamp_.start = cs.zsamp_.stop = (float) pts[idx].z_;
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


DBKey PolygonBody::storageID() const
{ return Object::dbKey(); }


BufferString PolygonBody::storageName() const
{ return Object::name(); }


void EM::PolygonBody::refBody()
{ EM::Object::ref(); }


void EM::PolygonBody::unRefBody()
{ EM::Object::unRef(); }


bool PolygonBody::useBodyPar( const IOPar& par )
{
    if ( !EM::Object::usePar( par ) )
	return false;

    return geometry().usePar(par);
}


void PolygonBody::fillBodyPar( IOPar& par ) const
{
    EM::Object::fillPar( par );
    geometry().fillPar( par );
}


Executor* PolygonBody::saver()
{
    PtrMan<IOObj> ioobj = dbKey().getIOObj();
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
    PtrMan<IOObj> ioobj = dbKey().getIOObj();
    if ( !ioobj )
	{ errmsg_ = uiStrings::phrCannotFindObjInDB(); return 0; }

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
				      const DBKey* key )
{
    const DBKey dbky = key && key->isValid() ? *key : surface_.dbKey();
    PtrMan<IOObj> ioobj = dbky.getIOObj();
    if ( !ioobj )
	{ surface_.setErrMsg(uiStrings::phrCannotFindObjInDB()); return 0; }

    return surface_.saver( ioobj );
}


Executor* PolygonBodyGeometry::loader( const SurfaceIODataSelection* )
{ return surface_.loader(); }

Geometry::PolygonSurface*
PolygonBodyGeometry::geometryElement()
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


ObjectIterator* PolygonBodyGeometry::createIterator(
					const TrcKeyZSampling* cs) const
{ return new RowColIterator( surface_, cs ); }


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
	const PosID posid = PosID::getFromRowCol( polygonnr, 0 );
	auto undo = new PolygonBodyUndoEvent( posid );
	BodyMan().undo(surface_.id()).addEvent( undo, 0 );
    }

    mSendEMSurfNotif( EM::Object::cBurstAlert() );
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

    mSendEMSurfNotif( EM::Object::cBurstAlert() );
    return true;
}


bool PolygonBodyGeometry::insertKnot( const PosID& posid,
				      const Coord3& pos, bool addtohistory )
{
    Geometry::PolygonSurface* pol = geometryElement();
    RowCol rc =  posid .getRowCol();
    if ( !pol || !pol->insertKnot(rc,pos) )
	return false;

    mSendEMSurfNotif( EM::Object::cBurstAlert() );
    return true;
}


Coord3 PolygonBodyGeometry::getPolygonNormal( int polygonnr ) const
{
    const Geometry::PolygonSurface* pol = geometryElement();
    return pol ? pol->getPolygonNormal(polygonnr) : Coord3::udf();
}


bool PolygonBodyGeometry::removeKnot( const PosID& posid,
				      bool addtohistory )
{
    Geometry::PolygonSurface* pol = geometryElement();
    if ( !pol ) return false;

    RowCol rc =  posid .getRowCol();
    const Coord3 pos = pol->getKnot( rc );

    if ( !pos.isDefined() || !pol->removeKnot(rc) )
	return false;

    mSendEMSurfNotif( EM::Object::cBurstAlert() );
    return true;
}


#define mDefEditNormalStr( editnormstr, polygonnr ) \
     BufferString editnormstr("Edit normal of section "); \
     editnormstr += 0; editnormstr += " polygonnr "; editnormstr += polygonnr;


#define mDefKnotsStr( knotstr, polygonnr, knotidx ) \
    BufferString knotstr("Edit knots of section "); knotstr += 0; \
    knotstr += " polygon "; knotstr += polygonnr; \
    knotstr += " knot "; knotstr += knotidx;

#define mDefPlgBezierNr( bez ) \
     BufferString bez("BezierCurve nr of section "); bez += 0;


void PolygonBodyGeometry::fillPar( IOPar& par ) const
{
    const Geometry::PolygonSurface* pol = geometryElement();
    if ( !pol ) return;

    mDefPlgBezierNr( bez );
    par.set( bez.buf(), pol->getBezierCurveSmoothness() );

    StepInterval<int> polygonrg = pol->rowRange();
    for ( int polygonnr=polygonrg.start; polygonnr<=polygonrg.stop;
	    polygonnr++ )
    {
	mDefEditNormalStr( editnormstr, polygonnr );
	par.set( editnormstr.buf(), pol->getPolygonNormal(polygonnr) );
    }
}


bool PolygonBodyGeometry::usePar( const IOPar& par )
{
    Geometry::PolygonSurface* pol = geometryElement();
    if ( !pol ) return false;

    int beziernr;
    mDefPlgBezierNr( bez );
    par.get( bez.buf(), beziernr );
    pol->setBezierCurveSmoothness( beziernr );

    StepInterval<int> polygonrg = pol->rowRange();
    for ( int polygonnr=polygonrg.start; polygonnr<=polygonrg.stop;
	    polygonnr++ )
    {
	mDefEditNormalStr( editnormstr, polygonnr );
	Coord3 editnormal( Coord3::udf() );
	par.get( editnormstr.buf(), editnormal );
	pol->addEditPlaneNormal( editnormal );
    }

    return true;
}


} // namespace EM
