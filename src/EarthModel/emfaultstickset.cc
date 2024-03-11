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
#include "keystrs.h"
#include "posfilter.h"
#include "survinfo.h"
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
	if ( colrg.isUdf() )
	    continue;

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


// FaultSSInfoHolder

FaultStickSetGeometry::GeomGroup::GeomGroup(
						const Pos::GeomID& geomid )
{
    tkzs_.hsamp_.setGeomID( geomid );
}


FaultStickSetGeometry::GeomGroup::~GeomGroup()
{}


const Pos::GeomID FaultStickSetGeometry::GeomGroup::geomID() const
{
    return tkzs_.hsamp_.getGeomID();
}


void FaultStickSetGeometry::GeomGroup::setTrcKeyZSampling(
					    const TrcKeyZSampling& tkzs )
{
    tkzs_ = tkzs;
    tkzs_.hsamp_.step_ = SI().sampling( true ).hsamp_.step_;
}


const TrcKeyZSampling&
	    FaultStickSetGeometry::GeomGroup::trcKeyZSampling() const
{
    return tkzs_;
}


int FaultStickSetGeometry::GeomGroup::size() const
{
    return sticknrs_.size();
}


bool FaultStickSetGeometry::GeomGroup::isPresent( int sticknr ) const
{
    return sticknrs_.indexOf(sticknr) >= 0;
}


const MultiID& FaultStickSetGeometry::GeomGroup::multiID() const
{
    return mid_;
}


void FaultStickSetGeometry::GeomGroup::setMultiID( const MultiID& mid )
{
    mid_ = mid;
}


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


void FaultStickSetGeometry::getPickedGeomIDs(
				    TypeSet<Pos::GeomID>& geomids ) const
{
    for ( auto* geomgrp : geomgroupset_ )
	geomids.add( geomgrp->geomID() );
}


void FaultStickSetGeometry::getStickNrsForGeomID( const Pos::GeomID& geomid,
						TypeSet<int>& sticknrs ) const
{
    for ( auto* geomgrp : geomgroupset_ )
    {
	if ( geomgrp->geomID() == geomid )
	{
	    sticknrs = geomgrp->sticknrs_;
	    break;
	}
    }
}


void FaultStickSetGeometry::getTrcKeyZSamplingForGeomID(
		    const Pos::GeomID& geomid, TrcKeyZSampling& tkzs ) const
{
    for ( auto* geomgrp : geomgroupset_ )
    {
	if ( geomgrp->geomID() == geomid )
	{
	    tkzs = geomgrp->trcKeyZSampling();
	    break;
	}
    }
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


FaultStickSetGeometry::GeomGroup* FaultStickSetGeometry::getGeomGroup(
				    const Pos::GeomID& pickedgeomid )
{
    for ( auto* currgeomgrp : geomgroupset_ )
    {
	if ( currgeomgrp->geomID() == pickedgeomid )
	    return currgeomgrp;
    }

    return nullptr;
}


FaultStickSetGeometry::GeomGroup* FaultStickSetGeometry::getGeomGroup(
					    const MultiID& pickedmid )
{
    for ( auto* currgeomgrp : geomgroupset_ )
    {
	if ( currgeomgrp->multiID() == pickedmid )
	    return currgeomgrp;
    }

    return nullptr;
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
    const Pos::GeomID pickedgeomid = Survey::GM().getGeomID( pickednm );
    MultiID mid = pickedmid ? *pickedmid : MultiID::udf();
    return insertStick( sticknr, firstcol, pos, editnormal, mid, pickedgeomid,
							    addtohistory );

}


bool FaultStickSetGeometry::insertStick( int sticknr,
					 int firstcol, const Coord3& pos,
					 const Coord3& editnormal,
					 Pos::GeomID pickedgeomid,
					 bool addtohistory )
{
    return insertStick( sticknr, firstcol, pos, editnormal, MultiID::udf(),
	pickedgeomid, addtohistory );
}


bool FaultStickSetGeometry::insertStick(int sticknr, int firstcol,
	const Coord3& pos, const Coord3& editnormal, const MultiID& pickedmid,
	const Pos::GeomID& pickedgeomid, bool addtohistory )
{
    Geometry::FaultStickSet* fss = geometryElement();

    if ( !fss )
	return false;

    const bool firstrowchange = sticknr < fss->rowRange().start;

    GeomGroup* geomgrp = nullptr;

    if ( !pickedmid.isUdf() )
	geomgrp = getGeomGroup( pickedmid );
    else
	geomgrp = getGeomGroup( pickedgeomid );

    if ( !geomgrp )
    {
	geomgrp = new GeomGroup( pickedgeomid.isValid() ? pickedgeomid :
						    Pos::GeomID(OD::Geom3D) );
	geomgrp->setMultiID( pickedmid );
	geomgroupset_.add( geomgrp );
    }

    if ( !fss->insertStick(pos,editnormal,sticknr,firstcol) )
	return false;

    if ( !firstrowchange )
    {
	for ( auto* currgeomgrp : geomgroupset_ )
	{
	    const int sz = currgeomgrp->size();
	    for ( int kidx=0; kidx<sz; kidx++ )
	    {
		if ( currgeomgrp->sticknrs_[kidx]>=sticknr)
		    currgeomgrp->sticknrs_[kidx]++;
	    }

	}
    }

    geomgrp->sticknrs_.add( sticknr );
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

    const bool forrem = sticknr >= fss->rowRange().start;
    for ( int idx=0; idx<geomgroupset_.size(); idx++ )
    {
	auto* geomgrp = geomgroupset_.get( idx );
	const int sz = geomgrp->size();
	for ( int kidx=sz-1; kidx>=0; kidx-- )
	{
	    if ( forrem && geomgrp->sticknrs_[kidx]>sticknr)
		geomgrp->sticknrs_[kidx]--;
	    if ( geomgrp->sticknrs_[kidx] == sticknr )
	    {
		geomgrp->sticknrs_.removeSingle(kidx);
		continue;
	    }
	}

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


const MultiID* FaultStickSetGeometry::pickedMultiID( int sticknr) const
{
    for ( auto* geomgrp : geomgroupset_ )
    {
	const int idx = geomgrp->sticknrs_.indexOf( sticknr );
	if ( idx >= 0 )
	{
	    const MultiID& pickedmid = geomgrp->multiID();
	    return pickedmid.isUdf() ? nullptr : &pickedmid;
	}
    }

    return nullptr;
}


const char* FaultStickSetGeometry::pickedName( int sticknr) const
{
    const Pos::GeomID& geomid = pickedGeomID( sticknr );
    if ( geomid.isValid() )
	return Survey::GM().getName( geomid );

    return nullptr;
}


Pos::GeomID FaultStickSetGeometry::pickedGeomID( int sticknr ) const
{
    for ( auto* geomgrp : geomgroupset_ )
    {
	const int idx = geomgrp->sticknrs_.indexOf( sticknr );
	if ( geomgrp->isPresent(sticknr) )
	    return geomgrp->geomID();
    }

    return Survey::GeometryManager::cUndefGeomID();
}


static BufferString getKey( const char* prefix, int sticknr )
{
    return BufferString( prefix, " sticknr ", ::toString(sticknr) );
}


EM::ObjectType FaultStickSetGeometry::FSSObjType() const
{
    const int nrdistinct = geomgroupset_.size();
    int count2d = 0;
    for ( auto* geomgrp : geomgroupset_ )
    {
	const int nrstricks = geomgrp->sticknrs_.size();
	const Pos::GeomID& geomid = geomgrp->geomID();
	if ( geomid.isValid() && geomid.is2D() )
	    count2d++;
    }

    EM::ObjectType objtype = EM::ObjectType::FltSS2D3D;
    if ( count2d == nrdistinct )
	objtype = EM::ObjectType::FltSS2D;
    else if ( count2d == 0 )
	objtype = EM::ObjectType::FltSS3D;

    return objtype;
}

void FaultStickSetGeometry::fillPar( IOPar& par ) const
{
    const Geometry::FaultStickSet* fss = geometryElement();
    if ( !fss )
	return;

    StepInterval<int> stickrg = fss->rowRange();
    for ( int sticknr=stickrg.start; sticknr<=stickrg.stop; sticknr++ )
    {
	const BufferString editnormalstr = getKey( "Edit normal", sticknr );
	par.set( editnormalstr.buf(), fss->getEditPlaneNormal(sticknr) );
	const MultiID* pickedmid = pickedMultiID( sticknr );
	if ( pickedmid )
	{
	    const BufferString pickedmidstr =
					getKey( "Picked MultiID", sticknr );
	    par.set( pickedmidstr.buf(), *pickedmid );
	}

	const char* pickednm = pickedName( sticknr );
	if ( pickednm )
	{
	    const BufferString pickednmstr = getKey( "Picked name" , sticknr );
	    par.set( pickednmstr.buf(), pickednm );
	}

	Pos::GeomID geomid = pickedGeomID( sticknr );
	if ( geomid != Survey::GeometryManager::cUndefGeomID() )
	{
	    const BufferString pickedgeomidstr =
					getKey( "Picked GeomID", sticknr );
	    par.set( pickedgeomidstr.buf(), geomid );
	}
    }
}


bool FaultStickSetGeometry::usePar( const IOPar& par )
{
    Geometry::FaultStickSet* fss = geometryElement();
    if ( !fss )
	return false;

    par.get( sKey::ZRange(), zgate_ );
    StepInterval<int> stickrg = fss->rowRange();
    for ( int sticknr=stickrg.start; sticknr<=stickrg.stop; sticknr++ )
    {
	BufferString editnormstr = getKey( "Edit normal", sticknr );
	if ( !par.hasKey(editnormstr.buf()) )
	    editnormstr.set("Edit normal of section 0 sticknr ").add( sticknr );

	Coord3 editnormal( Coord3::udf() );
	par.get( editnormstr.buf(), editnormal );
	fss->addEditPlaneNormal( editnormal, sticknr );

	Pos::GeomID geomid;
	MultiID mid;
	BufferString geomidstr = getKey( "Picked GeomID", sticknr );
	if ( !par.get(geomidstr.buf(),geomid) )
	{
	    geomidstr.set("Picked GeomID of section 0 sticknr ").add(sticknr);
	    if ( !par.get(geomidstr.buf(),geomid) )
	    {
		const BufferString l2dkeystr = getKey( "GeomID", sticknr );
		BufferString keybuf;
		if ( par.get(l2dkeystr.buf(),keybuf) )
		{
		    PosInfo::Line2DKey l2dkey;
		    l2dkey.fromString( keybuf );
		    if ( S2DPOS().curLineSetID() != l2dkey.lsID() )
			S2DPOS().setCurLineSet( l2dkey.lsID() );

		    geomid = Survey::GM().getGeomID(
				    S2DPOS().getLineSet(l2dkey.lsID()),
				    S2DPOS().getLineName(l2dkey.lineID()) );

		}
		else
		{
		    const BufferString pickedmidstr = getKey( "Picked MultiID",
							    sticknr );
		    if ( !par.get(pickedmidstr.buf(),mid) )
		    {
			BufferString linesetstr = getKey( "Line set",
								    sticknr );
			if ( !par.hasKey(linesetstr.buf()) )
			    linesetstr.set("Line set of section 0 sticknr ").
								add( sticknr );

			par.get( linesetstr.buf(), mid );
		    }

		    BufferString pickednm;
		    const BufferString pickednmstr = getKey( "Picked name",
								sticknr );
		    if ( !par.get(pickednmstr.buf(),pickednm) )
		    {
			BufferString linenamestr = getKey( "Line name",
								    sticknr );
			if ( !par.hasKey(linenamestr.buf()) )
			    linenamestr.set("Line name of section 0 sticknr ").
								add(sticknr);

			par.get( linenamestr.buf(), pickednm );
		    }

		    geomid = Survey::GM().getGeomID( pickednm );
		    PtrMan<IOObj> pickedioobj = IOM().get( mid );
		    if ( pickedioobj )
			geomid = Survey::GM().getGeomID( pickedioobj->name(),
								    pickednm );
		}
	    }
	}

	GeomGroup* geomgrp = nullptr;
	for ( auto* currgeomgrp : geomgroupset_ )
	{
	    if ( currgeomgrp->geomID() == geomid )
	    {
		geomgrp = currgeomgrp;
		break;
	    }
	}

	if ( !geomgrp )
	{
	    geomgrp = new GeomGroup( geomid );
	    geomgrp->setMultiID( mid );
	    geomgroupset_.add( geomgrp );
	}

	geomgrp->sticknrs_.add( sticknr );
    }

    GeomGroupUpdater geomgroupupdater( *fss, geomgroupset_ );
    return geomgroupupdater.execute();
}


//FaultStickDataUpdater
FaultStickSetGeometry::GeomGroupUpdater::GeomGroupUpdater(
	    Geometry::FaultStickSet& fss, ObjectSet<GeomGroup>& geomgrps )
    : ParallelTask("FaultStickSet Data Updater")
    , faultstickset_(fss)
    , geomgroupset_(geomgrps)
{
    totnr_ = geomgroupset_.size();
}


FaultStickSetGeometry::GeomGroupUpdater::~GeomGroupUpdater()
{}


od_int64 FaultStickSetGeometry::GeomGroupUpdater::nrIterations() const
{
    return totnr_;
}


bool FaultStickSetGeometry::GeomGroupUpdater::doWork(
				    od_int64 start, od_int64 stop, int /**/ )
{
    for ( int index=start; index<=stop; index++ )
    {
	GeomGroup* geomgrp = geomgroupset_[index];
	if ( !geomgrp )
	    return true;

	const int size = geomgrp->sticknrs_.size();
	const Pos::GeomID& geomid = geomgrp->geomID();
	TrcKey trckey;

	trckey.setGeomID( geomid );
	TrcKeyZSampling tkzs;
	tkzs.setEmpty();
	if ( geomid.isValid() && geomid.is2D() )
	{
	    tkzs.hsamp_.setGeomID( geomid );
	    trckey.setGeomSystem( OD::Geom2D );
	}
	else
	{
	    tkzs.hsamp_.setGeomID( Pos::GeomID(OD::Geom3D) );
	    trckey.setGeomSystem( OD::Geom3D );
	    tkzs.hsamp_.step_ = SI().sampling(true).hsamp_.step_;
	}

	for ( int idx=0; idx<size; idx++ )
	{
	    const int sticknr = geomgrp->sticknrs_[idx];

	    Geometry::FaultStick* fss = faultstickset_.getStick( sticknr );
	    const int fsssz = fss->size();
	    for ( int kidx=0; kidx<fsssz; kidx++ )
	    {
		LocationBase& loc = fss->locs_[kidx];
		trckey.setFrom( loc.pos() );
		loc.setTrcKey( trckey );
		tkzs.hsamp_.include( trckey );
		tkzs.zsamp_.include( loc.pos().z );
	    }
	}

	geomgrp->setTrcKeyZSampling( tkzs );
    }

    return true;
}

} // namespace EM
