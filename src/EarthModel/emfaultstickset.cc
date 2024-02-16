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

    const Pos::GeomID geomid = Survey::GM().getGeomID( pickednm );
    if ( !fss->insertStick(pos,editnormal,sticknr,firstcol,geomid) )
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


static BufferString getKey( const char* prefix, int sticknr )
{
    return BufferString( prefix, " sticknr ", ::toString(sticknr) );
}


EM::ObjectType FaultStickSetGeometry::FSSObjType() const
{
    const int nrsticks = nrSticks();
    int count2d = 0;
    for ( int idx=0; idx<nrsticks; idx++ )
    {
	const Pos::GeomID& geomid = stickinfo_[idx]->pickedgeomid;
	if ( geomid.isValid() && geomid.is2D() )
	    count2d++;
    }

    EM::ObjectType objtype = EM::ObjectType::FltSS2D3D;
    if ( count2d == nrsticks )
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

	stickinfo_.insertAt( new StickInfo, 0 );
	stickinfo_[0]->sticknr = sticknr;

	BufferString geomidstr = getKey( "Picked GeomID", sticknr );
	if ( par.get(geomidstr.buf(), stickinfo_[0]->pickedgeomid) )
	    continue;

	geomidstr.set("Picked GeomID of section 0 sticknr ").add(sticknr);
	if ( par.get(geomidstr.buf(), stickinfo_[0]->pickedgeomid) )
	    continue;

	const BufferString l2dkeystr = getKey( "GeomID", sticknr );
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

	const BufferString pickedmidstr = getKey( "Picked MultiID", sticknr );
	if ( !par.get(pickedmidstr.buf(), stickinfo_[0]->pickedmid) )
	{
	    BufferString linesetstr = getKey( "Line set", sticknr );
	    if ( !par.hasKey(linesetstr.buf()) )
		linesetstr.set("Line set of section 0 sticknr ").add( sticknr );

	    par.get( linesetstr.buf(), stickinfo_[0]->pickedmid );
	}

	const BufferString pickednmstr = getKey( "Picked name" , sticknr );
	if ( !par.get(pickednmstr.buf(), stickinfo_[0]->pickednm) )
	{
	    BufferString linenamestr = getKey( "Line name", sticknr );
	    if ( !par.hasKey(linenamestr.buf()) )
		linenamestr.set("Line name of section 0 sticknr ").add(sticknr);

	    par.get( linenamestr.buf(), stickinfo_[0]->pickednm );
	}

	PtrMan<IOObj> pickedioobj = IOM().get( stickinfo_[0]->pickedmid );
	if ( pickedioobj )
	    stickinfo_[0]->pickedgeomid =
		    Survey::GM().getGeomID( pickedioobj->name(),
					    stickinfo_[0]->pickednm );
    }

    FaultStickSetDataOrganiser fssdataorganiser( *this, dataholders_ );
    if ( !fssdataorganiser.execute() )
	return false;

    FaultStickSetDataUpdater fssupdater( *fss, dataholders_ );
    return fssupdater.execute();
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


//DataHolder
FaultSSDataHolder::FaultSSDataHolder()
{}


FaultSSDataHolder::~FaultSSDataHolder()
{}


//FaultStickSetDataOrganiser
FaultStickSetDataOrganiser::FaultStickSetDataOrganiser(
				const FaultStickSetGeometry& fssgeom,
				ObjectSet<FaultSSDataHolder>& dataholders )
    : Executor("FaultStickSet Data Grouping")
    , fssgeom_(fssgeom)
    , dataholders_(dataholders)
{
    const Geometry::FaultStickSet* fss = fssgeom_.geometryElement();
    totnr_ = fss ? fssgeom.nrSticks() : 0;
}


FaultStickSetDataOrganiser::~FaultStickSetDataOrganiser()
{}


uiString FaultStickSetDataOrganiser::uiNrDoneText() const
{
    return tr("Grouping sticks");
}

int FaultStickSetDataOrganiser::nextStep()
{
    if ( nrdone_ >= totnr_ )
	return Finished();

    const Geometry::FaultStickSet* fss = fssgeom_.geometryElement();
    if ( !fss )
	return ErrorOccurred();


    const Pos::GeomID geomid = fssgeom_.pickedGeomID( nrdone_ );
    const Geometry::FaultStick* stick = fss->getStick( nrdone_ );
    if ( !stick || stick->locs_.isEmpty() )
    {
	nrdone_++;
	return MoreToDo();
    }

    if ( processedgeomids_.addIfNew(geomid) )
    {
	auto* dh = new FaultSSDataHolder();
	dh->geomid_ = geomid;
	dh->sticknr_.add( nrdone_ );
	dh->sticks_.add( stick );
	dh->tkzs_.zsamp_.setFrom( fssgeom_.getZRange() );
	dataholders_.add( dh );
    }
    else
    {
	const int gidx = processedgeomids_.indexOf( geomid );
	auto* dh = dataholders_.get( gidx );
	dh->sticknr_.add( nrdone_ );
	dh->sticks_.add( stick );
    }

    nrdone_++;
    return MoreToDo();
}


//FaultStickDataUpdater
FaultStickSetDataUpdater::FaultStickSetDataUpdater(
	    Geometry::FaultStickSet& fss, ObjectSet<FaultSSDataHolder>& dhs )
    : ParallelTask("FaultStickSet Data Updater")
    , faultstickset_(fss)
    , dataholders_(dhs)
{
    totnr_ = dataholders_.size();
}


FaultStickSetDataUpdater::~FaultStickSetDataUpdater()
{}


od_int64 FaultStickSetDataUpdater::nrIterations() const
{
    return totnr_;
}


bool FaultStickSetDataUpdater::doWork( od_int64 index, od_int64/**/, int /**/ )
{
    FaultSSDataHolder* dh = dataholders_[index];
    if ( !dh )
	return true;

    const int size = dh->sticks_.size();
    const Pos::GeomID& geomid = dh->geomid_;
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
	Geometry::FaultStick* fss =
		    const_cast<Geometry::FaultStick*>( dh->sticks_.get(idx) );
	const int fsssz = fss->size();

	for ( int kidx=0; kidx<fsssz; kidx++ )
	{
	    LocationBase& loc = fss->locs_[kidx];
	    trckey.setFrom( loc.pos() );
	    loc.setTrcKey( trckey );
	    tkzs.hsamp_.include( trckey );
	}
    }

    dh->tkzs_.hsamp_ = tkzs.hsamp_;
    dh->tkzs_.hsamp_.step_.second = dh->tkzs_.hsamp_.step_.first;
    return true;
}

} // namespace EM
