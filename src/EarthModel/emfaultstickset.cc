/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emfaultstickset.h"

#include "emmanager.h"
#include "emrowcoliterator.h"
#include "emsurfacetr.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "posfilter.h"
#include "undo.h"


namespace EM {

mImplementEMObjFuncs(FaultStickSet,EMFaultStickSetTranslatorGroup::sGroupName())

FaultStickSet::FaultStickSet( EMManager& em )
    : Fault(em)
    , geometry_( *this )
{
    setPosAttrMarkerStyle( 0,
	MarkerStyle3D(MarkerStyle3D::Cube,3,OD::Color::Yellow()) );
}


FaultStickSet::~FaultStickSet()
{}


uiString FaultStickSet::getUserTypeStr() const
{ return EMFaultStickSetTranslatorGroup::sTypeName(); }


void FaultStickSet::apply( const Pos::Filter& pf )
{
    mDynamicCastGet(Geometry::FaultStickSet*,fssg,geometryElement())
    if ( !fssg )
	return;

    const StepInterval<int> rowrg = fssg->rowRange();
    if ( rowrg.isUdf() )
	return;

    RowCol rc;
    for ( rc.row()=rowrg.stop; rc.row()>=rowrg.start; rc.row()-=rowrg.step )
    {
	const StepInterval<int> colrg = fssg->colRange( rc.row() );
	if ( colrg.isUdf() ) continue;

	for ( rc.col()=colrg.stop; rc.col()>=colrg.start;
						    rc.col()-=colrg.step )
	{
	    const Coord3 pos = fssg->getKnot( rc );
	    if ( !pf.includes( (Coord) pos, (float) pos.z) )
		fssg->removeKnot( rc );
	}
    }
}


FaultStickSetGeometry& FaultStickSet::geometry()
{ return geometry_; }


const FaultStickSetGeometry& FaultStickSet::geometry() const
{ return geometry_; }


const IOObjContext& FaultStickSet::getIOObjContext() const
{
    return EMFaultStickSetTranslatorGroup::ioContext();
}


EMObjectIterator*
	FaultStickSet::createIterator( const TrcKeyZSampling* tkzs ) const
{
    return geometry_.createIterator( tkzs );
}


// FaultStickSetGeometry

FaultStickSetGeometry::StickInfo::StickInfo()
{}


FaultStickSetGeometry::FaultStickSetGeometry( Surface& surf )
    : FaultGeometry(surf)
{}


FaultStickSetGeometry::~FaultStickSetGeometry()
{}


Geometry::FaultStickSet* FaultStickSetGeometry::geometryElement()
{
    Geometry::Element* res = SurfaceGeometry::geometryElement();
    return sCast(Geometry::FaultStickSet*,res);
}


const Geometry::FaultStickSet* FaultStickSetGeometry::geometryElement() const
{
    const Geometry::Element* res = SurfaceGeometry::geometryElement();
    return sCast(const Geometry::FaultStickSet*,res);
}


Geometry::FaultStickSet* FaultStickSetGeometry::createGeometryElement() const
{ return new Geometry::FaultStickSet; }


EMObjectIterator* FaultStickSetGeometry::createIterator(
					const TrcKeyZSampling* tkzs ) const
{
    return new RowColIterator( surface_, tkzs );
}


int FaultStickSetGeometry::nrSticks() const
{
    const Geometry::FaultStickSet* fss = geometryElement();
    return fss ? fss->nrSticks(): 0;
}


int FaultStickSetGeometry::nrKnots( int sticknr ) const
{
    const Geometry::FaultStickSet* fss = geometryElement();
    return fss ? fss->nrKnots(sticknr) : 0;
}


#define mTriggerSurfaceChange( surf ) \
    surf.setChangedFlag(); \
    EMObjectCallbackData cbdata; \
    cbdata.event = EMObjectCallbackData::BurstAlert; \
    surf.change.trigger( cbdata );


bool FaultStickSetGeometry::insertStick( int sticknr,
					 int firstcol, const Coord3& pos,
					 const Coord3& editnormal,
					 bool addtohistory )
{
    return insertStick( sticknr, firstcol, pos, editnormal, nullptr, nullptr,
	    		addtohistory );
}


bool FaultStickSetGeometry::insertStick( int sticknr,
					 int firstcol, const Coord3& pos,
					 const Coord3& editnormal,
					 const MultiID* pickedmid,
					 const char* pickednm,
					 bool addtohistory )
{
    Geometry::FaultStickSet* fss = geometryElement();

    if ( !fss )
	return false;

    const bool firstrowchange = sticknr < fss->rowRange().start;

    if ( !fss->insertStick(pos,editnormal,sticknr,firstcol) )
	return false;

    for ( int idx=0; !firstrowchange && idx<stickinfo_.size(); idx++ )
    {
	if ( stickinfo_[idx]->sticknr>=sticknr )
	    stickinfo_[idx]->sticknr++;
    }

    stickinfo_.insertAt( new StickInfo, 0 );
    stickinfo_[0]->sticknr = sticknr;
    stickinfo_[0]->pickedmid = pickedmid ? *pickedmid : MultiID::udf();
    stickinfo_[0]->pickednm = pickednm;
    if ( addtohistory )
    {
	const PosID posid( surface_.id(), RowCol(sticknr,0) );
	UndoEvent* undo = new FaultStickUndoEvent( posid );
	EMM().undo(surface_.id()).addEvent( undo, 0 );
    }

    mTriggerSurfaceChange( surface_ );
    return true;
}


bool FaultStickSetGeometry::insertStick( int sticknr,
					 int firstcol, const Coord3& pos,
					 const Coord3& editnormal,
					 Pos::GeomID pickedgeomid,
					 bool addtohistory )
{
    Geometry::FaultStickSet* fss = geometryElement();

    if ( !fss )
	return false;

    const bool firstrowchange = sticknr < fss->rowRange().start;

    if ( !fss->insertStick(pos,editnormal,sticknr,firstcol) )
	return false;

    for ( int idx=0; !firstrowchange && idx<stickinfo_.size(); idx++ )
    {
	if ( stickinfo_[idx]->sticknr>=sticknr )
	    stickinfo_[idx]->sticknr++;
    }

    stickinfo_.insertAt( new StickInfo, 0 );
    stickinfo_[0]->sticknr = sticknr;
    stickinfo_[0]->pickedgeomid = pickedgeomid;
    if ( addtohistory )
    {
	const PosID posid( surface_.id(), RowCol(sticknr,0) );
	UndoEvent* undo = new FaultStickUndoEvent( posid );
	EMM().undo(surface_.id()).addEvent( undo, 0 );
    }

    mTriggerSurfaceChange( surface_ );
    return true;
}


bool FaultStickSetGeometry::removeStick( int sticknr,
					 bool addtohistory )
{
    Geometry::FaultStickSet* fss = geometryElement();
    if ( !fss )
	return false;

    const StepInterval<int> colrg = fss->colRange( sticknr );
    if ( colrg.isUdf() || colrg.width() )
	return false;

    const RowCol rc( sticknr, colrg.start );

    const Coord3 pos = fss->getKnot( rc );
    const Coord3 normal = getEditPlaneNormal( sticknr );
    if ( !normal.isDefined() || !pos.isDefined() )
	return false;

    if ( !fss->removeStick(sticknr) )
	return false;

    for ( int idx=stickinfo_.size()-1; idx>=0; idx-- )
    {
	if ( stickinfo_[idx]->sticknr == sticknr )
	{
	    delete stickinfo_.removeSingle( idx );
	    continue;
	}

	if ( sticknr>=fss->rowRange().start && stickinfo_[idx]->sticknr>sticknr)
	    stickinfo_[idx]->sticknr--;
    }

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), rc );
	UndoEvent* undo = new FaultStickUndoEvent( posid, pos, normal );
	EMM().undo(surface_.id()).addEvent( undo, 0 );
    }

    mTriggerSurfaceChange( surface_ );
    return true;
}


bool FaultStickSetGeometry::insertKnot( const SubID& subid, const Coord3& pos,
					bool addtohistory )
{
    Geometry::FaultStickSet* fss = geometryElement();
    RowCol rc = RowCol::fromInt64( subid );
    if ( !fss || !fss->insertKnot(rc,pos) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), subid );
	UndoEvent* undo = new FaultKnotUndoEvent( posid );
	EMM().undo(surface_.id()).addEvent( undo, 0 );
    }

    mTriggerSurfaceChange( surface_ );
    return true;
}


bool FaultStickSetGeometry::removeKnot( const SubID& subid, bool addtohistory )
{
    Geometry::FaultStickSet* fss = geometryElement();
    if ( !fss ) return false;

    RowCol rc = RowCol::fromInt64( subid );
    const Coord3 pos = fss->getKnot( rc );

    if ( !pos.isDefined() || !fss->removeKnot(rc) )
	return false;

    if ( addtohistory )
    {
	const PosID posid( surface_.id(), subid );
	UndoEvent* undo = new FaultKnotUndoEvent( posid, pos );
	EMM().undo(surface_.id()).addEvent( undo, 0 );
    }

    mTriggerSurfaceChange( surface_ );
    return true;
}


bool FaultStickSetGeometry::pickedOnPlane( int sticknr ) const
{
    if ( pickedMultiID(sticknr) || pickedOn2DLine(sticknr) )
	return false;

    const Coord3& editnorm = getEditPlaneNormal( sticknr );
    return editnorm.isDefined();
}


bool FaultStickSetGeometry::pickedOnHorizon( int sticknr ) const
{
    const Coord3& editnorm = getEditPlaneNormal( sticknr );
    return !pickedOnPlane(sticknr) &&
	   editnorm.isDefined() && fabs(editnorm.z)>0.5;
}


bool FaultStickSetGeometry::pickedOn2DLine( int sticknr ) const
{
    return pickedGeomID(sticknr).isValid();
}


const MultiID* FaultStickSetGeometry::pickedMultiID(
						     int sticknr) const
{
    int idx = indexOf(sticknr);
    if ( idx >= 0 )
    {
	const MultiID& pickedmid = stickinfo_[idx]->pickedmid;
	return pickedmid.isUdf() ? nullptr : &pickedmid;
    }

    return nullptr;
}


const char* FaultStickSetGeometry::pickedName(
					       int sticknr) const
{
    int idx = indexOf(sticknr);
    if ( idx >= 0 )
    {
        const char* pickednm = stickinfo_[idx]->pickednm.buf();
	return *pickednm ? pickednm : nullptr;
    }

    return nullptr;
}


Pos::GeomID FaultStickSetGeometry::pickedGeomID(
						 int sticknr) const
{
    int idx = indexOf(sticknr);
    if ( idx >= 0 )
	return stickinfo_[idx]->pickedgeomid;

    return Survey::GeometryManager::cUndefGeomID();
}


#define mDefStickInfoStr( prefixstr, stickinfostr, sticknr ) \
    BufferString stickinfostr(prefixstr); \
    stickinfostr.add( " sticknr " ).add( sticknr );

#define mDefEditNormalStr( editnormalstr, sticknr ) \
    mDefStickInfoStr( "Edit normal", editnormalstr, sticknr )
#define mDefLineSetStr( linesetstr, sticknr ) \
    mDefStickInfoStr( "Line set", linesetstr, sticknr )
#define mDefLineNameStr( linenamestr, sticknr ) \
    mDefStickInfoStr( "Line name", linenamestr, sticknr )
#define mDefPickedMultiIDStr( pickedmidstr, sticknr ) \
    mDefStickInfoStr( "Picked MultiID", pickedmidstr, sticknr )
#define mDefPickedNameStr( pickednmstr, sticknr ) \
    mDefStickInfoStr( "Picked name", pickednmstr, sticknr )
#define mDefPickedGeomIDStr( pickedgeomidstr, sticknr ) \
	mDefStickInfoStr( "Picked GeomID", pickedgeomidstr, sticknr )
#define mDefL2DKeyStr( l2dkeystr, sticknr ) \
	mDefStickInfoStr( "GeomID", l2dkeystr, sticknr )



void FaultStickSetGeometry::fillPar( IOPar& par ) const
{
    const Geometry::FaultStickSet* fss = geometryElement();
    if ( !fss )
	return;

    StepInterval<int> stickrg = fss->rowRange();
    for ( int sticknr=stickrg.start; sticknr<=stickrg.stop; sticknr++ )
    {
	mDefEditNormalStr( editnormalstr, sticknr );
	par.set( editnormalstr.buf(), fss->getEditPlaneNormal(sticknr) );
	const MultiID* pickedmid = pickedMultiID( sticknr );
	if ( pickedmid )
	{
	    mDefPickedMultiIDStr( pickedmidstr, sticknr );
	    par.set( pickedmidstr.buf(), *pickedmid );
	}

	const char* pickednm = pickedName( sticknr );
	if ( pickednm )
	{
	    mDefPickedNameStr( pickednmstr, sticknr );
	    par.set( pickednmstr.buf(), pickednm );
	}

	Pos::GeomID geomid = pickedGeomID( sticknr );
	if ( geomid != Survey::GeometryManager::cUndefGeomID() )
	{
	    mDefPickedGeomIDStr( pickedgeomidstr, sticknr );
	    par.set( pickedgeomidstr.buf(), geomid );
	}
    }
}


bool FaultStickSetGeometry::usePar( const IOPar& par )
{
    Geometry::FaultStickSet* fss = geometryElement();
    if ( !fss ) return false;

    StepInterval<int> stickrg = fss->rowRange();
    for ( int sticknr=stickrg.start; sticknr<=stickrg.stop; sticknr++ )
    {
	mDefEditNormalStr( editnormstr, sticknr );
	Coord3 editnormal( Coord3::udf() );
	par.get( editnormstr.buf(), editnormal );
	fss->addEditPlaneNormal( editnormal );

	stickinfo_.insertAt( new StickInfo, 0 );
	stickinfo_[0]->sticknr = sticknr;

	mDefPickedGeomIDStr( pickedgeomidstr, sticknr );
	if ( par.get(pickedgeomidstr.buf(), stickinfo_[0]->pickedgeomid) )
	    continue;

	mDefL2DKeyStr( l2dkeystr, sticknr );
	BufferString keybuf;
	if ( par.get(l2dkeystr.buf(),keybuf) )
	{
	    PosInfo::Line2DKey l2dkey;
	    l2dkey.fromString( keybuf );
	    if ( S2DPOS().curLineSetID() != l2dkey.lsID() )
		S2DPOS().setCurLineSet( l2dkey.lsID() );

	    stickinfo_[0]->pickedgeomid = Survey::GM().getGeomID(
				S2DPOS().getLineSet(l2dkey.lsID()),
				S2DPOS().getLineName(l2dkey.lineID()) );
	    continue;
	}

	mDefPickedMultiIDStr( pickedmidstr, sticknr );
	mDefLineSetStr( linesetstr, sticknr );
	if ( !par.get(pickedmidstr.buf(), stickinfo_[0]->pickedmid))
	    par.get( linesetstr.buf(), stickinfo_[0]->pickedmid );
	mDefPickedNameStr( pickednmstr, sticknr );
	mDefLineNameStr( linenamestr, sticknr );

	if ( !par.get(pickednmstr.buf(), stickinfo_[0]->pickednm) )
	    par.get( linenamestr.buf(), stickinfo_[0]->pickednm );

	PtrMan<IOObj> pickedioobj = IOM().get( stickinfo_[0]->pickedmid );
	if ( pickedioobj )
	    stickinfo_[0]->pickedgeomid =
		    Survey::GM().getGeomID( pickedioobj->name(),
					    stickinfo_[0]->pickednm );
    }

    return true;
}


int FaultStickSetGeometry::indexOf( int sticknr ) const
{
    for ( int idx=0; idx<stickinfo_.size(); idx++ )
    {
	if ( stickinfo_[idx]->sticknr==sticknr )
	    return idx;
    }

    return -1;
}

} // namespace EM
