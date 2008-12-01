/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          July 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: empolygonbody.cc,v 1.7 2008-12-01 15:32:27 cvsyuancheng Exp $";

#include "empolygonbody.h"

#include "arraynd.h"
#include "arrayndimpl.h"
#include "delaunay3d.h"
#include "embodytr.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "emsurfacetr.h"
#include "errh.h"
#include "explpolygonsurface.h"
#include "indexedshape.h"
#include "ioman.h"
#include "mousecursor.h"
#include "positionlist.h"
#include "rowcol.h"
#include "survinfo.h"
#include "sortedtable.h"
#include "tabledef.h"
#include "trigonometry.h"
#include "undo.h"
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
    const EM::SectionID sid = sectionID( 0 );
    const Geometry::PolygonSurface* surf = geometry().sectionGeometry( sid );
    if ( !surf || surf->nrPolygons()<2 )
	return 0;

     const StepInterval<int> rrg = surf->rowRange();
     if ( rrg.isUdf() )
	 return 0;

     const float zscale = SI().zFactor();
     TypeSet<Coord3> pts;
     for ( int plg=rrg.start; plg<=rrg.stop; plg += rrg.step )
	 surf->getCubicBezierCurve( plg, pts, zscale );
     
    Interval<float> zrg;
    StepInterval<int> inlrg, crlrg;
    inlrg.step = SI().inlStep();
    crlrg.step = SI().crlStep();
   
    for ( int idx=0; idx<pts.size(); idx++ )
    {
	const BinID bid = SI().transform( pts[idx] );
	if ( !idx )
	{
	    inlrg.start = inlrg.stop = bid.inl;
	    crlrg.start = crlrg.stop = bid.crl;
	    zrg.start = zrg.stop = pts[idx].z;
	}
	else
	{
	    inlrg.include( bid.inl ); 
	    crlrg.include( bid.crl );
	    zrg.include( pts[idx].z );
	}
    }

    for ( int idx=0; idx<pts.size(); idx++ )
	 pts[idx].z *= zscale;

     DAGTetrahedraTree dagtree;
     if ( !dagtree.setCoordList( pts, false ) )
	 return 0;

     ParallelDTetrahedralator triangulator( dagtree );
     if ( !triangulator.execute(true) )
	 return 0;

    mDeclareAndTryAlloc( Array3D<char>*, intarr, 
	    		 Array3DImpl<char>(inlrg.nrSteps()+1,crlrg.nrSteps()+1,
			     		   mNINT(zrg.width()/SI().zStep())+1) );
    if ( !intarr )
	return 0;
    
    mDeclareAndTryAlloc( ImplicitBody*, res, ImplicitBody );
    if ( !res )
    {
 	delete intarr;
	return 0;
    }

    //Initial all the points outside the body.
    memset( intarr->getData(), 1, sizeof(char)*intarr->info().getTotalSz() );
    
    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    PtrMan<Explicit2ImplicitBodyExtracter> extractor = 
	new Explicit2ImplicitBodyExtracter(dagtree, inlrg, crlrg, zrg, *intarr);

    Array3D<float>* arr = new Array3DConv<float,char>(intarr);
    if ( !arr )
    {
	delete intarr;
	delete res;
	return 0;
    }

    res->arr_ = arr;
    res->threshold_ = 0;
    res->inlsampling_.start = inlrg.start;
    res->inlsampling_.step = inlrg.step;
    res->crlsampling_.start = crlrg.start;
    res->crlsampling_.step = crlrg.step;    
    res->zsampling_.start = zrg.start;
    res->zsampling_.step = SI().zStep();

    if ( !extractor->execute() )
	res = 0;
    
    cursorchanger.restore();
    return res;
}


PolygonBodyGeometry& PolygonBody::geometry()
{ return geometry_; }


const PolygonBodyGeometry& PolygonBody::geometry() const
{ return geometry_; }


const IOObjContext& PolygonBody::getIOObjContext() const
{ return EMBodyTranslatorGroup::ioContext(); }


Executor* PolygonBody::saver()
{
    return saver( 0 );
}
    

Executor* PolygonBody::saver( IOObj* inpioobj )
{
    PtrMan<IOObj> myioobj = 0;
    IOObj* ioobj = 0;
    if ( inpioobj )
	ioobj = inpioobj;
    else
    {
	myioobj = IOM().get( multiID() );
	ioobj = myioobj;
    }

    if ( !ioobj )
    { 
	errmsg_ = "Cannot find surface"; 
	return 0; 
    }

    mDynamicCastGet( polygonEMBodyTranslator*, tr, ioobj->getTranslator() );
    if ( !tr )
    {
	errmsg_ = "No Translator";
	return 0;
    }

    Executor* exec = tr->writer( *this, *ioobj );
    if ( !exec )
    {
	errmsg_ = tr->errMsg();
	delete tr;

	return 0;
    }

    return exec;
}


Executor* PolygonBody::loader()
{
    PtrMan<IOObj> ioobj = IOM().get( multiID() );
    if ( !ioobj ) { errmsg_ = "Cannot find surface"; return 0; }

    mDynamicCastGet( polygonEMBodyTranslator*, tr, ioobj->getTranslator() );
    if ( !tr )
    {
	errmsg_ = "No Translator";
	return 0;
    }

    Executor* exec = tr->reader( *ioobj, *this );
    if ( !exec )
    {
	errmsg_ = tr->errMsg();
	delete tr;

	return 0;
    }

    return exec;
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
    if ( !ioobj ) { surface_.setErrMsg("Cannot find surface" ); return 0; }

    return surface_.saver( ioobj );
}


Executor* PolygonBodyGeometry::loader( const SurfaceIODataSelection* )
{
    return surface_.loader();
}

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
