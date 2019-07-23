/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          November 2008
________________________________________________________________________

-*/

#include "emfaultstickset.h"

#include "emmanager.h"
#include "emrowcoliterator.h"
#include "emsurfacetr.h"
#include "ioobj.h"
#include "iopar.h"
#include "posfilter.h"
#include "undo.h"


namespace EM {

mImplementEMObjFuncs(FaultStickSet,
			     EMFaultStickSetTranslatorGroup::sGroupName())

FaultStickSet::FaultStickSet( const char* nm )
    : Fault(nm)
    , geometry_( *this )
{
    setPreferredMarkerStyle3D(
	OD::MarkerStyle3D(OD::MarkerStyle3D::Cube,3,Color::Yellow()) );
}

mImplMonitorableAssignment( FaultStickSet, Surface )


FaultStickSet::FaultStickSet( const FaultStickSet& oth )
    : Fault(oth)
    , geometry_(oth.geometry_)
{
    copyClassData( oth );
}


FaultStickSet::~FaultStickSet()
{}


void FaultStickSet::copyClassData( const FaultStickSet& oth )
{
    storageid_ = oth.storageid_;
    preferredcolor_ = oth.preferredcolor_;
    changed_ = oth.changed_;
    fullyloaded_ = oth.fullyloaded_;
    locked_ = oth.locked_;
    burstalertcount_ = oth.fullyloaded_;
    selremoving_ = oth.selremoving_;
    preferredlinestyle_ = oth.preferredlinestyle_;
    preferredmarkerstyle_ = oth.preferredmarkerstyle_;
    selectioncolor_ = oth.selectioncolor_;
    haslockednodes_ = oth.haslockednodes_;
}


Monitorable::ChangeType FaultStickSet::compareClassData(
					const FaultStickSet& oth ) const
{
    return cNoChange();
}


uiString FaultStickSet::getUserTypeStr() const
{ return EMFaultStickSetTranslatorGroup::sTypeName(); }


void FaultStickSet::apply( const Pos::Filter& pf )
{
    mDynamicCastGet( Geometry::FaultStickSet*, fssg, geometryElement() );
    if ( !fssg ) return;

    const StepInterval<int> rowrg = fssg->rowRange();
    if ( rowrg.isUdf() ) return;

    RowCol rc;
    for ( rc.row()=rowrg.stop; rc.row()>=rowrg.start; rc.row()-=rowrg.step )
    {
	const StepInterval<int> colrg = fssg->colRange( rc.row() );
	if ( colrg.isUdf() ) continue;

	for ( rc.col()=colrg.stop; rc.col()>=colrg.start;
						    rc.col()-=colrg.step )
	{
	    const Coord3 pos = fssg->getKnot( rc );
	    if ( !pf.includes( pos.getXY(), (float) pos.z_) )
		fssg->removeKnot( rc );
	}
    }

    // TODO: Handle case in which fault sticks become fragmented.
}


FaultStickSetGeometry& FaultStickSet::geometry()
{ return geometry_; }


const FaultStickSetGeometry& FaultStickSet::geometry() const
{ return geometry_; }


const IOObjContext& FaultStickSet::getIOObjContext() const
{ return EMFaultStickSetTranslatorGroup::ioContext(); }


bool FaultStickSet::pickedOnPlane( int row  ) const
{
    return geometry().pickedOnPlane( row );
}


bool FaultStickSet::pickedOn2DLine( int row  ) const
{
    return geometry().pickedOn2DLine( row );
}


Pos::GeomID FaultStickSet::pickedGeomID( int row  ) const
{
    return geometry().pickedGeomID( row );
}


const DBKey* FaultStickSet::pickedDBKey( int sticknr ) const
{
    return geometry().pickedDBKey( sticknr );
}


const char* FaultStickSet::pickedName( int sticknr ) const
{
    return geometry().pickedName( sticknr );
}


// ***** FaultStickSetGeometry *****


FaultStickSetGeometry::StickInfo::StickInfo()
{}


FaultStickSetGeometry::FaultStickSetGeometry( Surface& surf )
    : FaultGeometry(surf)
{}


FaultStickSetGeometry::~FaultStickSetGeometry()
{}


Geometry::FaultStickSet*
FaultStickSetGeometry::geometryElement()
{
    Geometry::Element* res = SurfaceGeometry::geometryElement();
    return (Geometry::FaultStickSet*) res;
}


const Geometry::FaultStickSet*
FaultStickSetGeometry::geometryElement() const
{
    const Geometry::Element* res = SurfaceGeometry::geometryElement();
    return (const Geometry::FaultStickSet*) res;
}


Geometry::FaultStickSet* FaultStickSetGeometry::createGeometryElement() const
{ return new Geometry::FaultStickSet; }


ObjectIterator* FaultStickSetGeometry::createIterator(
					const TrcKeyZSampling* cs ) const
{ return new RowColIterator( surface_, cs ); }


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


bool FaultStickSetGeometry::insertStick( int sticknr,
					 int firstcol, const Coord3& pos,
					 const Coord3& editnormal,
					 bool addtohistory )
{
    return insertStick( sticknr, firstcol, pos, editnormal, 0, 0,
			addtohistory );
}


bool FaultStickSetGeometry::insertStick( int sticknr,
					 int firstcol, const Coord3& pos,
					 const Coord3& editnormal,
					 const DBKey* pickeddbkey,
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
    stickinfo_[0]->pickeddbkey = pickeddbkey ? *pickeddbkey
					     : DBKey::getInvalid();
    stickinfo_[0]->pickednm = pickednm;
    if ( addtohistory )
    {
	const PosID posid = PosID::getFromRowCol( sticknr, 0 );
	auto undo = new FaultStickUndoEvent( posid );
	FSSMan().undo(surface_.id()).addEvent( undo, 0 );
    }

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
	const PosID posid = PosID::getFromRowCol( sticknr, 0 );
	auto undo = new FaultStickUndoEvent( posid );
	FSSMan().undo(surface_.id()).addEvent( undo, 0 );
    }

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
	const PosID posid = PosID::getFromRowCol( rc );
	auto undo = new FaultStickUndoEvent( posid, pos, normal );
	FSSMan().undo(surface_.id()).addEvent( undo, 0 );
    }

    return true;
}


bool FaultStickSetGeometry::insertKnot( const PosID& posid, const Coord3& pos,
					bool addtohistory )
{
    Geometry::FaultStickSet* fss = geometryElement();
    RowCol rc =  posid .getRowCol();
    if ( !fss || !fss->insertKnot(rc,pos) )
	return false;

    if ( addtohistory )
    {
	auto undo = new FaultKnotUndoEvent( posid );
	FSSMan().undo(surface_.id()).addEvent( undo, 0 );
    }

    return true;
}


bool FaultStickSetGeometry::removeKnot( const PosID& posid, bool addtohistory )
{
    Geometry::FaultStickSet* fss = geometryElement();
    if ( !fss ) return false;

    RowCol rc =  posid .getRowCol();
    const Coord3 pos = fss->getKnot( rc );

    if ( !pos.isDefined() || !fss->removeKnot(rc) )
	return false;

    if ( addtohistory )
    {
	auto undo = new FaultKnotUndoEvent( posid, pos );
	FSSMan().undo(surface_.id()).addEvent( undo, 0 );
    }

    return true;
}


bool FaultStickSetGeometry::pickedOnPlane( int sticknr ) const
{
    if ( pickedDBKey(sticknr) || pickedOn2DLine(sticknr) )
	return false;

    const Coord3& editnorm = getEditPlaneNormal( sticknr );
    return editnorm.isDefined();
}


bool FaultStickSetGeometry::pickedOnHorizon( int sticknr ) const
{
    const Coord3& editnorm = getEditPlaneNormal( sticknr );
    return !pickedOnPlane(sticknr) &&
	   editnorm.isDefined() && fabs(editnorm.z_)>0.5;
}


bool FaultStickSetGeometry::pickedOn2DLine( int sticknr ) const
{
    return pickedGeomID(sticknr).isValid();
}


const DBKey* FaultStickSetGeometry::pickedDBKey( int sticknr) const
{
    int idx = indexOf(sticknr);
    if ( idx >= 0 )
    {
	const DBKey& pickeddbkey = stickinfo_[idx]->pickeddbkey;
	return pickeddbkey==DBKey::getInvalid() ? 0 : &pickeddbkey;
    }

    return 0;
}


const char* FaultStickSetGeometry::pickedName( int sticknr ) const
{
    int idx = indexOf(sticknr);
    if ( idx >= 0 )
    {
        const char* pickednm = stickinfo_[idx]->pickednm.buf();
        return *pickednm ? pickednm : 0;
    }

    return 0;
}


Pos::GeomID FaultStickSetGeometry::pickedGeomID( int sticknr) const
{
    int idx = indexOf(sticknr);
    if ( idx >= 0 )
	return stickinfo_[idx]->pickedgeomid;

    return Pos::GeomID();
}


#define mDefStickInfoStr( prefixstr, stickinfostr, sticknr ) \
    BufferString stickinfostr(prefixstr); stickinfostr += " of section "; \
    stickinfostr += 0; stickinfostr += " sticknr "; stickinfostr += sticknr;

#define mDefEditNormalStr( editnormalstr, sticknr ) \
    mDefStickInfoStr( "Edit normal", editnormalstr, sticknr )
#define mDefLineSetStr( linesetstr, sticknr ) \
    mDefStickInfoStr( "Line set", linesetstr, sticknr )
#define mDefLineNameStr( linenamestr, sticknr ) \
    mDefStickInfoStr( "Line name", linenamestr, sticknr )
#define mDefPickedDBKeyStr( pickeddbkeystr, sticknr ) \
    mDefStickInfoStr( "Picked DBKey", pickeddbkeystr, sticknr )
#define mDefPickedNameStr( pickednmstr, sticknr ) \
    mDefStickInfoStr( "Picked name", pickednmstr, sticknr )
#define mDefPickedGeomIDStr( pickedgeomidstr, sticknr ) \
	mDefStickInfoStr( "Picked GeomID", pickedgeomidstr, sticknr )
#define mDefL2DKeyStr( l2dkeystr, sticknr ) \
	mDefStickInfoStr( "GeomID", l2dkeystr, sticknr )



void FaultStickSetGeometry::fillPar( IOPar& par ) const
{
    const Geometry::FaultStickSet* fss = geometryElement();
    if ( !fss ) return;

    StepInterval<int> stickrg = fss->rowRange();
    for ( int sticknr=stickrg.start; sticknr<=stickrg.stop; sticknr++ )
    {
	mDefEditNormalStr( editnormalstr, sticknr );
	par.set( editnormalstr.buf(), fss->getEditPlaneNormal(sticknr) );
	const DBKey* pickeddbkey = pickedDBKey( sticknr );
	if ( pickeddbkey )
	{
	    mDefPickedDBKeyStr( pickeddbkeystr, sticknr );
	    par.set( pickeddbkeystr.buf(), *pickeddbkey );
	}

	const char* pickednm = pickedName( sticknr );
	if ( pickednm )
	{
	    mDefPickedNameStr( pickednmstr, sticknr );
	    par.set( pickednmstr.buf(), pickednm );
	}

	Pos::GeomID geomid = pickedGeomID( sticknr );
	if ( geomid.isValid() )
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
	fss->setEditPlaneNormal( sticknr, editnormal );

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

	    const BufferString oldlnm( S2DPOS().getLineSet(l2dkey.lsID()),
		    "-", S2DPOS().getLineName(l2dkey.lineID()) );
	    stickinfo_[0]->pickedgeomid = SurvGeom::getGeomID( oldlnm );
	    continue;
	}

	mDefPickedDBKeyStr( pickeddbkeystr, sticknr );
	mDefLineSetStr( linesetstr, sticknr );
	if ( !par.get(pickeddbkeystr.buf(), stickinfo_[0]->pickeddbkey))
	    par.get( linesetstr.buf(), stickinfo_[0]->pickeddbkey );
	mDefPickedNameStr( pickednmstr, sticknr );
	mDefLineNameStr( linenamestr, sticknr );

	if ( !par.get(pickednmstr.buf(), stickinfo_[0]->pickednm) )
	    par.get( linenamestr.buf(), stickinfo_[0]->pickednm );

	PtrMan<IOObj> pickedioobj = stickinfo_[0]->pickeddbkey.getIOObj();
	if ( pickedioobj )
	{
	    const BufferString oldlnm( pickedioobj->name(),
				       stickinfo_[0]->pickednm );
	    stickinfo_[0]->pickedgeomid = SurvGeom::getGeomID( oldlnm );
	}
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
