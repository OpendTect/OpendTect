/*+
________________________________________________________________________

Copyright:	(C) 1995-2022 dGB Beheer B.V.
License:	https://dgbes.com/licensing
________________________________________________________________________

-*/


#include "emsurft2dtransformer.h"

#include "arraynd.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emfault3d.h"
#include "emfaultauxdata.h"
#include "emfaultset3d.h"
#include "emfaultstickset.h"
#include "emfsstofault3d.h"
#include "faultstickset.h"
#include "emmanager.h"
#include "emposid.h"
#include "emsurfaceauxdata.h"
#include "ioman.h"
#include "uistrings.h"
#include "survgeom.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "zaxistransform.h"

namespace EM
{

SurfaceT2DTransfData::SurfaceT2DTransfData( const SurfaceIOData& surfdata )
    : surfsel_(surfdata)
{}

SurfaceT2DTransformer::SurfaceT2DTransformer(
			const ObjectSet<SurfaceT2DTransfData>& datas,
			ZAxisTransform& zatf )
    : Executor("Surface Domain Transformation")
    , datas_(datas)
    , zatf_(zatf)
{
    totnr_ = datas_.size();
}


Executor* SurfaceT2DTransformer::createExecutor(
			const ObjectSet<SurfaceT2DTransfData>& datas,
			ZAxisTransform& zatf, ObjectType objtype )
{
    if ( objtype == ObjectType::Hor3D )
	return new Horizon3DT2DTransformer( datas, zatf );
    else if ( objtype == ObjectType::Hor2D )
	return new Horizon2DT2DTransformer( datas, zatf );
    else if ( objtype == ObjectType::Flt3D )
	return new FaultT2DTransformer( datas, zatf );
    else if ( objtype == ObjectType::FltSet )
	return new FaultSetT2DTransformer( datas, zatf );
    else if ( objtype == ObjectType::FltSS3D )
	return new FaultStickSetT2DTransformer( datas, zatf, false );
    else if ( objtype == ObjectType::FltSS2D )
	return new FaultStickSetT2DTransformer( datas, zatf, true );

    return nullptr;
}


SurfaceT2DTransformer::~SurfaceT2DTransformer()
{
    detachAllNotifiers();
}


od_int64 SurfaceT2DTransformer::totalNr() const
{
    return totnr_;
}


uiString SurfaceT2DTransformer::uiNrDoneText() const
{
    return tr("Transforming horizons");
}


void SurfaceT2DTransformer::preStepCB( CallBacker* )
{
}


void SurfaceT2DTransformer::load3DTranformVol( const TrcKeyZSampling* tkzsext )
{
    if ( tkzsext )
	addVolumeOfInterest( *tkzsext, SI().zDomainInfo() );
}


bool SurfaceT2DTransformer::load2DVelCubeTransf( const Pos::GeomID& geomid,
					const StepInterval<int>& trcrg )
{
    TrcKeyZSampling tkzs;
    tkzs.hsamp_.set( geomid, trcrg );
    return addVolumeOfInterest( tkzs, SI().zDomainInfo() );
}


bool SurfaceT2DTransformer::addVolumeOfInterest( const TrcKeyZSampling& tkzsext,
						 const ZDomain::Info& zinfo )
{
    const ZDomain::Info& fromzinfo = zatf_.fromZDomainInfo();
    const ZDomain::Info& tozinfo = zatf_.toZDomainInfo();
    if ( !zinfo.isCompatibleWith(fromzinfo) &&
	 !zinfo.isCompatibleWith(tozinfo) )
    {
	errmsg_ = tr("Incompatible ZDomains");
	return false;
    }

    const bool fromzdomain = zinfo.isCompatibleWith( fromzinfo );
    TrcKeyZSampling tkzs( tkzsext );
    tkzs.zsamp_ = zatf_.getZInterval( fromzdomain );
    if ( fromzdomain && mIsUdf(tkzsext.zsamp_.stop_) )
    {
	tkzs.zsamp_.stop_ = tkzs.zsamp_.snap( tkzsext.zsamp_.stop_,
					      OD::SnapUpward );
    }

    zatvoi_ = zatf_.addVolumeOfInterest( tkzs, !fromzdomain );
    if ( !zatf_.loadDataIfMissing(zatvoi_) )
    {
	errmsg_.add( tr("Failed to preload the velocity volume") );
	return false;
    }

    return true;
}


RefMan<EMObject> SurfaceT2DTransformer::createObject( const MultiID& outmid,
						const MultiID& inpmid ) const
{
    EM::EMObject* emobj = nullptr;
    EM::EMManager& em = EM::EMM();
    if ( outmid.isInMemoryObjID() )
    {
	PtrMan<IOObj> inpioobj = IOM().get( inpmid );
	if ( !inpioobj )
	    return nullptr;

	emobj = em.createTempObject( inpioobj->group() );
	emobj->setMultiID( outmid );
    }
    else
    {
	PtrMan<IOObj> obj = IOM().get( outmid );
	if ( !obj )
	    return nullptr;

	ObjectID objid = em.getObjectID( outmid );
	if ( !objid.isValid() )
	    objid = em.createObject( getTypeString(), obj->name());

	emobj = em.getObject( objid );
    }

    emobj->change.disable();
    emobj->setZDomain( zatf_.toZDomainInfo() );
    return emobj;
}


#define mErrRet(s) { errmsg_.add(s); return false; }

void SurfaceT2DTransformer::postStepCB( CallBacker* )
{
}

RefMan<EMObject> SurfaceT2DTransformer::getTransformedSurface(
						    const MultiID& mid ) const
{
    for ( auto* surf : outsurfs_ )
    {
	if ( surf->multiID() == mid )
	    return surf;
    }

    return nullptr;
}


void SurfaceT2DTransformer::unloadModel()
{
    if ( zatvoi_ >= 0 )
	zatf_.removeVolumeOfInterest( zatvoi_ );
}


//Horizon3DT2DTransformer
Horizon3DT2DTransformer::Horizon3DT2DTransformer(
	const ObjectSet<SurfaceT2DTransfData>& datas, ZAxisTransform& zatf )
    : SurfaceT2DTransformer(datas,zatf)
{
    msg_ = tr("Transforming 3D Horizon");
    mAttachCB(prestep,Horizon3DT2DTransformer::preStepCB);
    mAttachCB(poststep,Horizon3DT2DTransformer::postStepCB);
}


Horizon3DT2DTransformer::~Horizon3DT2DTransformer()
{
    detachAllNotifiers();
}


const StringView Horizon3DT2DTransformer::getTypeString() const
{
    return EM::Horizon3D::typeStr();
}


void Horizon3DT2DTransformer::preStepCB( CallBacker* )
{
    if ( nrdone_>0 || totnr_<=0 || !zatf_.needsVolumeOfInterest() ||
	 datas_.isEmpty() )
	return;

    TrcKeyZSampling tkzs;
    tkzs.hsamp_ = datas_.first()->surfsel_.rg;
    ZSampling& zsamp = tkzs.zsamp_;
    zsamp.step_ = mUdf(float);
    const ZDomain::Info* zinfo = nullptr;
    for ( const auto* data : datas_ )
    {
	const IOObjInfo objinfo( data->inpmid_ );
	if ( objinfo.isOK() )
	{
	    if ( zinfo && &objinfo.zDomain() != zinfo )
	    {
		pErrMsg("Should not mix various zDomains in one execution");
	    }
	    else if ( !zinfo )
		zinfo = &objinfo.zDomain();
	}

	const SurfaceIODataSelection& surfsel = data->surfsel_;
	const Interval<float>& datagate = surfsel.sd.zrg;
	zsamp.include( datagate );
    }

    if ( !zinfo )
	zinfo = &SI().zDomainInfo();

    if ( !addVolumeOfInterest(tkzs,*zinfo) )
	return; // Will become an error
}


bool Horizon3DT2DTransformer::doHorizon( const SurfaceT2DTransfData& data )
{
    const MultiID& inpmid = data.inpmid_;
    const IOObj* ioobj = IOM().get( inpmid );
    if ( !ioobj )
	mErrRet( tr("Cannot find input horizon in repository") );

    EM::EMManager& em = EM::EMM();
    PtrMan<Executor> loader = em.objectLoader( inpmid, &data.surfsel_ );
    if ( !loader || !loader->execute() )
	mErrRet( uiStrings::sCantCreateHor() )

    ConstRefMan<EM::EMObject> emobj = em.getObject( inpmid );
    if ( !emobj )
	mErrRet( tr("Failed to load the surface") );

    mDynamicCastGet(const EM::Horizon3D*,hor,emobj.ptr())
    if ( !hor )
	mErrRet( tr("Incorrect object selected, "
	    "3D horizon is expected for the workflow") );

    RefMan<EMObject> surf = createObject( data.outmid_, inpmid );
    if ( !surf )
	mErrRet( tr("Cannot access database") );

    mDynamicCastGet(EM::Horizon3D*,outhor3d,surf.ptr())
    if ( !outhor3d )
	mErrRet( tr("The output type is incompatible with input type, "
	    "3D Horizon object expected") );

    TrcKeyZSampling bbox;
    bbox.hsamp_ = hor->range();
    bbox.zsamp_.setFrom( hor->getZRange() );

    const StepInterval<int> inlrg = hor->geometry().rowRange();
    const StepInterval<int> crlrg = hor->geometry().colRange();
    outhor3d->enableGeometryChecks( false );
    const RowCol step( inlrg.step_, crlrg.step_ );
    outhor3d->geometry().setStep( step, step );
    PtrMan<Array2D<float> > arrdata = hor->createArray2D( &zatf_ );
    if ( !arrdata )
	return false;

    outhor3d->setArray2D( arrdata.release(), bbox.hsamp_.start_,
			  bbox.hsamp_.step_ );

    const ObjectSet<BinIDValueSet>& auxdatas = hor->auxdata.getData();
    if ( !auxdatas.isEmpty() )
    {
	BufferStringSet nms;
	const int nraux = hor->auxdata.nrAuxData();
	for ( int idx=0; idx<nraux; idx++ )
	    nms.add( hor->auxdata.auxDataName(idx) );

	outhor3d->auxdata.addAuxData( nms, *auxdatas.get(0) );
    }

    outhor3d->setStratLevelID( hor->stratLevelID() );
    outsurfs_.add( outhor3d );

    return true;
}


int Horizon3DT2DTransformer::nextStep()
{
    for ( const auto* data : datas_ )
    {
	if ( !data )
	    continue;

	doHorizon( *data );
	nrdone_++;
    }

    return nrDone() >= totalNr()
		? ( errmsg_.isOK() ? Finished() : ErrorOccurred() )
		: MoreToDo();
}


void Horizon3DT2DTransformer::postStepCB( CallBacker* )
{
    if ( nrdone_ == totnr_ && zatf_.needsVolumeOfInterest() )
	unloadModel();
}


//Horizon2DDataHolder
Horizon2DDataHolder::Horizon2DDataHolder( const Pos::GeomID& geomid )
    : geomid_(geomid)
{}


Horizon2DDataHolder& Horizon2DDataHolder::operator =(
					    const Horizon2DDataHolder& oth )
{
    if ( *this == oth )
	return *this;

    geomid_ = oth.geomid_;
    mids_ = oth.mids_;

    return *this;
}


bool Horizon2DDataHolder::operator ==( const Horizon2DDataHolder& oth ) const
{
    return geomid_ == oth.geomid_ && mids_ == oth.mids_;
}


void Horizon2DDataHolder::addHorMID( const MultiID& mid )
{
    mids_.addIfNew( mid );
}


const Pos::GeomID& Horizon2DDataHolder::getGeomID() const
{
    return geomid_;
}


const TypeSet<MultiID>& Horizon2DDataHolder::getMIDSet() const
{
    return mids_;
}


//Horizon2DDataHolderSet
Horizon2DDataHolderSet::Horizon2DDataHolderSet()
{}


void Horizon2DDataHolderSet::addData( const Pos::GeomID& geomid,
				      const MultiID& mid )
{
    for ( auto& data : *this )
    {
	if ( data.getGeomID() == geomid )
	{
	    data.addHorMID( mid );
	    return;
	}
    }

    Horizon2DDataHolder hd( geomid );
    hd.addHorMID( mid );
    this->add( hd );
}



//Horizon2DT2DTransformer
Horizon2DT2DTransformer::Horizon2DT2DTransformer(
    const ObjectSet<SurfaceT2DTransfData>& datas, ZAxisTransform& zatf )
    : SurfaceT2DTransformer(datas,zatf)
{
    msg_ = tr("Transforming 2D Horizon");
    for ( const auto* data : datas )
    {
	const BufferStringSet& linenms = data->surfsel_.sd.linenames;
	for ( const auto* linenm : linenms )
	{
	    const Pos::GeomID geomid = Survey::GM().getGeomID( linenm->buf() );
	    if ( !geomid.isUdf() )
		dataset_.addData( geomid, data->inpmid_ );
	}
    }

    mAttachCB(prestep,Horizon2DT2DTransformer::preStepCB);
    totnr_ = dataset_.size();
}


Horizon2DT2DTransformer::~Horizon2DT2DTransformer()
{
    detachAllNotifiers();
}


const StringView Horizon2DT2DTransformer::getTypeString() const
{
    return EM::Horizon2D::typeStr();
}


void Horizon2DT2DTransformer::preStepCB( CallBacker* )
{
    EM::EMManager& em = EM::EMM();
    for ( const auto* data : datas_ )
    {
	const MultiID& inpmid = data->inpmid_;
	const IOObj* ioobj = IOM().get( inpmid );
	if ( !ioobj )
	    continue;

	PtrMan<Executor> loader = em.objectLoader( inpmid, &data->surfsel_ );
	if ( !loader || !loader->execute() )
	   continue;

	ConstRefMan<EM::EMObject> emobj = em.getObject( inpmid );
	if ( !emobj )
	    continue;

	mDynamicCastGet(const EM::Horizon2D*,hor,emobj.ptr())
	if ( !hor )
	    continue;


	RefMan<EMObject> surf = createObject( data->outmid_, inpmid );
	if ( !surf )
	    continue;

	mDynamicCastGet(EM::Horizon2D*,outhor2d,surf.ptr())
	if ( !outhor2d )
	    continue;

	outsurfs_.add( outhor2d );
	inpsurfs_.add( hor );
	preprocessesedinpmid_.add( hor->multiID() );
    }
}



bool Horizon2DT2DTransformer::do2DHorizon( const
				    Horizon2DDataHolder& dataholder  )
{
    const Pos::GeomID& geomid = dataholder.getGeomID();
    const TypeSet<MultiID>& inpmids = dataholder.getMIDSet();
    TypeSet<int> horidxs;
    StepInterval<int> trcrangeenvelop( Interval<int>::udf(), 1 );
    const ZDomain::Info* zinfo = nullptr;
    for ( const auto& inpmid : inpmids )
    {
	const int horidx = preprocessesedinpmid_.indexOf( inpmid );
	const Surface* inpsurf = inpsurfs_.get( horidx );
	mDynamicCastGet(const Horizon2D*,inphor2D,inpsurf)
	if ( !inphor2D )
	    continue;

	if ( zinfo && &inpsurf->zDomain() != zinfo )
	{
	    errmsg_ = tr("Cannot transform horizons from different ZDomains");
	    return false;
	}
	else if ( !zinfo )
	    zinfo = &inpsurf->zDomain();

	const StepInterval<int>& trcrng =
				inphor2D->geometry().colRange( geomid );
	trcrangeenvelop.include( trcrng );

	horidxs.add( horidx );
    }

    if ( !zinfo )
    {
	errmsg_.add( tr("Cannot determine horizon ZDomain") );
	return false;
    }

    TrcKeyZSampling tkzs( geomid );
    tkzs.hsamp_.setTrcRange( trcrangeenvelop );
    tkzs.zsamp_ = ZSampling::udf();
    if ( zatf_.needsVolumeOfInterest() && !addVolumeOfInterest(tkzs,*zinfo) )
	return false;

    for ( const auto& horidx : horidxs )
    {
	const Surface* inpsurf = inpsurfs_.get( horidx );
	mDynamicCastGet(const Horizon2D*,inphor2D,inpsurf)
	EMObject* outsurf = outsurfs_.get( horidx );
	mDynamicCastGet(Horizon2D*,outhor2d,outsurf)
	if ( !outsurf )
	    continue;

	outhor2d->enableGeometryChecks( false );
	outhor2d->setStratLevelID( inphor2D->stratLevelID() );
	const StepInterval<int>& trcrng =
				    inphor2D->geometry().colRange( geomid );
	PtrMan<Array1D<float>> array =
				inphor2D->createArray1D( geomid, &zatf_ );
	if ( array )
	    outhor2d->setArray1D( *array.ptr(), trcrng, geomid, false );
    }

    if ( zatf_.needsVolumeOfInterest() )
	unloadModel();

    return true;
}


int Horizon2DT2DTransformer::nextStep()
{
    for ( const auto& data : dataset_ )
    {
	do2DHorizon( data );
	nrdone_++;
    }

    return nrDone() >= totalNr()
		? ( errmsg_.isOK() ? Finished() : ErrorOccurred() )
		: MoreToDo();
}


//FaultT2DTransformer
FaultT2DTransformer::FaultT2DTransformer(
	const ObjectSet<SurfaceT2DTransfData>& data, ZAxisTransform& zatf )
    : SurfaceT2DTransformer(data,zatf)
{
    msg_ = tr("Transforming Fault");
    mAttachCB(prestep,FaultT2DTransformer::preStepCB);
    mAttachCB(poststep,FaultT2DTransformer::postStepCB);
}


FaultT2DTransformer::~FaultT2DTransformer()
{
    detachAllNotifiers();
}


const StringView FaultT2DTransformer::getTypeString() const
{
    return EM::Fault3D::typeStr();
}


bool FaultT2DTransformer::doFault( const SurfaceT2DTransfData& data )
{
    const MultiID& inpmid = data.inpmid_;
    const IOObj* ioobj = IOM().get( inpmid );
    if ( !ioobj )
	mErrRet( tr("Cannot find input fault in repository") );

    EM::EMManager& em = EM::EMM();
    ConstRefMan<EM::EMObject> emobj = em.loadIfNotFullyLoaded( inpmid );
    if ( !emobj )
	mErrRet( tr("Failed to load the fault") );

    mDynamicCastGet(const EM::Fault3D*,flt,emobj.ptr())
    if ( !flt )
	mErrRet( tr("Incorrect object selected, "
				"3D fault is expected for the workflow") );

    const EM::Fault3DGeometry& fltgeom = flt->geometry();
    const Geometry::FaultStickSurface* fssurf = fltgeom.geometryElement();
    if ( !fssurf )
	mErrRet( tr("Fault is empty") );

    RefMan<EMObject> surf = createObject( data.outmid_, inpmid );
    if ( !surf )
	mErrRet( tr("Cannot access database") );

    mDynamicCastGet(EM::Fault3D*,outfault3d,surf.ptr())
    if ( !outfault3d )
	mErrRet( tr("The output type is incompatible with input type, "
						"Fault object expected") );

    outfault3d->enableGeometryChecks( false );

    const int nrsticks = fltgeom.nrSticks();
    const TrcKeyZSampling tkzs = fltgeom.getEnvelope();
    const ZDomain::Info& info = emobj->zDomain();
    if ( zatf_.needsVolumeOfInterest() && !addVolumeOfInterest(tkzs,info) )
	return false;

    int stickidx = 0;
    for ( int idx=0; idx<nrsticks; idx++ )
    {
	const Geometry::FaultStick* stick = fssurf->getStick( idx );
	if ( !stick || stick->locs_.isEmpty() )
	    continue;

	const int sz = stick->size();
	const Coord3& editnormal = stick->getNormal();
	bool stickinserted = false;
	int colidx=1;
	for ( int crdidx=0; crdidx<sz; crdidx++ )
	{
	    Coord3 outcrd( stick->getCoordAtIndex(crdidx) );
            outcrd.z_ = zatf_.transformTrc( stick->locs_[crdidx].trcKey(),
                                            outcrd.z_ );
	    if ( mIsUdf(outcrd.z_) )
		continue;

	    if ( !stickinserted )
	    {
		if ( !outfault3d->geometry().insertStick(stickidx,0,outcrd,
						    editnormal,false) )
		    break;

		stickinserted = true;
		continue;
	    }

	    const RowCol rc( stickidx, colidx );
	    if ( outfault3d->geometry().insertKnot(rc.toInt64(),outcrd,false) )
		colidx++;
	}

	if ( stickinserted )
	    stickidx++;
    }

    if ( zatf_.needsVolumeOfInterest() )
	unloadModel();

    outsurfs_.add( outfault3d );
    return true;
}


int FaultT2DTransformer::nextStep()
{
    for ( const auto* data : datas_ )
    {
	if ( !data )
	    continue;

	doFault( *data );
	nrdone_++;
    }

    return nrDone() >= totalNr()
		? ( errmsg_.isOK() ? Finished() : ErrorOccurred() )
		: MoreToDo();
}


//FaultSetT2DTransformer class
FaultSetT2DTransformer::FaultSetT2DTransformer(
	const ObjectSet<SurfaceT2DTransfData>& data, ZAxisTransform& zatf)
    : SurfaceT2DTransformer(data, zatf)
{
    msg_ = tr("Transforming FaultSet");
    mAttachCB(prestep,FaultSetT2DTransformer::preStepCB);
    mAttachCB(poststep,FaultSetT2DTransformer::postStepCB);
}

FaultSetT2DTransformer::~FaultSetT2DTransformer()
{
    detachAllNotifiers();
}


const StringView FaultSetT2DTransformer::getTypeString() const
{
    return EM::FaultSet3D::typeStr();
}


bool FaultSetT2DTransformer::doFaultSet( const SurfaceT2DTransfData& data )
{
    const MultiID& inpmid = data.inpmid_;
    const IOObj* ioobj = IOM().get( inpmid );
    if ( !ioobj )
	mErrRet( tr("Cannot find input FaultSet in repository") );

    EM::EMManager& em = EM::EMM();
    ConstRefMan<EM::EMObject> inpemobj = em.loadIfNotFullyLoaded( inpmid );
    if ( !inpemobj )
	mErrRet( tr("Failed to load the faultset") );

    mDynamicCastGet(const EM::FaultSet3D*,inpfltset,inpemobj.ptr())
    if ( !inpfltset )
	mErrRet( tr("Cannot access database") );

    RefMan<EMObject> outfltsetobj = createObject( data.outmid_, inpmid );
    if ( !outfltsetobj )
	mErrRet( tr("Cannot access database") );

    mDynamicCastGet(EM::FaultSet3D*,outfltset,outfltsetobj.ptr())
    if ( !outfltset )
	mErrRet( tr("The output type is incompatible with input type, "
	    "FaultSet object expected") );

    const int nrfaults = inpfltset->nrFaults();
    const TrcKeyZSampling tkzs = inpfltset->getEnvelope();
    const ZDomain::Info& zinfo = inpfltset->zDomain();
    if ( zatf_.needsVolumeOfInterest() && !addVolumeOfInterest(tkzs,zinfo) )
	return false;

    for ( int fltidx=0; fltidx<nrfaults; fltidx++ )
    {
	const EM::FaultID fltid = inpfltset->getFaultID( fltidx );
	ConstRefMan<EM::EMObject> fltobj = inpfltset->getFault3D( fltid );
	mDynamicCastGet(const EM::Fault3D*,flt,fltobj.ptr())
	    const EM::Fault3DGeometry& fltgeom = flt->geometry();
	const Geometry::FaultStickSurface* fssurf = fltgeom.geometryElement();
	if ( !fssurf )
	    mErrRet( tr("FaultSet is empty") );

	const int nrsticks = fltgeom.nrSticks();

	RefMan<EM::EMObject> fltemobj = em.createTempObject(
						    EM::Fault3D::typeStr() );
	mDynamicCastGet(EM::Fault3D*,outfault3d,fltemobj.ptr());
	int stickidx = 0;
	for ( int idx=0; idx<nrsticks; idx++ )
	{
	    const Geometry::FaultStick* stick = fssurf->getStick( idx );
	    if ( !stick || stick->locs_.isEmpty() )
		continue;

	    const int sz = stick->size();
	    const Coord3& editnormal = stick->getNormal();
	    bool stickinserted = false;
	    int colidx = 1;
	    for ( int crdidx=0; crdidx<sz; crdidx++ )
	    {
		Coord3 outcrd( stick->getCoordAtIndex(crdidx) );
                outcrd.z_ = zatf_.transformTrc( stick->locs_[crdidx].trcKey(),
                                                outcrd.z_ );
		if ( mIsUdf(outcrd.z_) )
		    continue;

		if ( !stickinserted )
		{
		    if ( !outfault3d->geometry().insertStick(stickidx,0,outcrd,
			editnormal,false) )
			break;

		    stickinserted = true;
		    continue;
		}

		const RowCol rc( stickidx, colidx );
		if ( outfault3d->geometry().insertKnot(rc.toInt64(),outcrd,
								    false) )
		    colidx++;
	    }

	    if ( stickinserted )
		stickidx++;
	}

	outfltset->addFault( outfault3d );
    }

    if ( zatf_.needsVolumeOfInterest() )
	unloadModel();

    outsurfs_.add( outfltset );
    return true;
}


int FaultSetT2DTransformer::nextStep()
{
    for ( const auto* data : datas_ )
    {
	if ( !data )
	    continue;

	doFaultSet( *data );
	nrdone_++;
    }

    return nrDone() >= totalNr()
		? ( errmsg_.isOK() ? Finished() : ErrorOccurred() )
		: MoreToDo();
}


//FaultTStickSet2DTransformer
FaultStickSetT2DTransformer::FaultStickSetT2DTransformer(
    const ObjectSet<SurfaceT2DTransfData>& data, ZAxisTransform& zatf,
								bool is2d )
    : SurfaceT2DTransformer(data,zatf)
    , is2d_(is2d)
{
    msg_ = tr("Transforming %1 FaultStickSet").arg( is2d ? uiStrings::s2D()
							 : uiStrings::s3D() );
    mAttachCB(prestep,FaultStickSetT2DTransformer::preStepCB);
    mAttachCB(poststep,FaultStickSetT2DTransformer::postStepCB);
}


FaultStickSetT2DTransformer::~FaultStickSetT2DTransformer()
{
    detachAllNotifiers();
}


const StringView FaultStickSetT2DTransformer::getTypeString() const
{
    return EM::FaultStickSet::typeStr();
}


bool FaultStickSetT2DTransformer::doTransformation(
		const Geometry::FaultStick* stick, int sticknr,
		EM::FaultStickSet& outfault3d, const Pos::GeomID& geomid )
{
    if ( !stick || stick->locs_.isEmpty() )
	return false;

    const int sz = stick->size();
    const Coord3& editnormal = stick->getNormal();
    bool stickinserted = false;
    int colidx=1;
    for ( int crdidx = 0; crdidx<sz; crdidx++ )
    {
	Coord3 outcrd( stick->getCoordAtIndex(crdidx) );
        outcrd.z_ = zatf_.transformTrc( stick->locs_[crdidx].trcKey(),
                                        outcrd.z_ );
	if ( !stickinserted )
	{
	    if ( !outfault3d.geometry().insertStick(sticknr,0,outcrd,
						    editnormal,geomid,false) )
		break;

	    stickinserted = true;
	    continue;
	}

	const RowCol rc( sticknr, colidx );
	if ( outfault3d.geometry().insertKnot(rc.toInt64(),outcrd,false) )
	    colidx++;
    }

    return true;
}


bool FaultStickSetT2DTransformer::handle2DTransformation(
				const EM::FaultStickSetGeometry& fssgeom,
				EM::FaultStickSet& outfss )
{
    return false;
}


bool FaultStickSetT2DTransformer::handle2DTransformation(
				const EM::FaultStickSetGeometry& fssgeom,
				const ZDomain::Info& zinfo,
				EM::FaultStickSet& outfss )
{
    TypeSet<Pos::GeomID> geomids;
    fssgeom.getPickedGeomIDs( geomids );
    for ( const auto& geomid : geomids )
    {
	TrcKeyZSampling tkzs( geomid );
	fssgeom.getTrcKeyZSamplingForGeomID( geomid, tkzs );
	if ( zatf_.needsVolumeOfInterest() &&
	     !addVolumeOfInterest(tkzs,zinfo) )
	    return false;

	TypeSet<int> sticknrs;
	fssgeom.getStickNrsForGeomID( geomid, sticknrs );
	for ( const auto& sticknr : sticknrs )
	{
	    doTransformation( fssgeom.geometryElement()->getStick(sticknr,
					true), sticknr, outfss, geomid );
	}

	if ( zatf_.needsVolumeOfInterest() )
	    unloadModel();
    }

    return true;
}


bool FaultStickSetT2DTransformer::handle3DTransformation(
				    const EM::FaultStickSetGeometry& fssgeom,
				    EM::FaultStickSet& outfss )
{
    return false;
}


bool FaultStickSetT2DTransformer::handle3DTransformation(
				    const EM::FaultStickSetGeometry& fssgeom,
				    const ZDomain::Info& zinfo,
				    EM::FaultStickSet& outfss )
{
    const Geometry::FaultStickSet* fss = fssgeom.geometryElement();
    if ( !fss )
	mErrRet( tr("FaultStickSet is empty") );

    const int nrsticks = fssgeom.nrSticks();
    TypeSet<Pos::GeomID> geomids;
    fssgeom.getPickedGeomIDs( geomids );
    for ( const auto& geomid : geomids )
    {
	TrcKeyZSampling tkzs( geomid );
	fssgeom.getTrcKeyZSamplingForGeomID( geomid, tkzs );

	if ( zatf_.needsVolumeOfInterest() &&
	     !addVolumeOfInterest(tkzs,zinfo) )
	    return false;

	for ( int idx=0; idx<nrsticks; idx++ )
	{
	    const Geometry::FaultStick* stick = fss->getStick( idx );
	    doTransformation( stick, idx, outfss );
	}

	if ( zatf_.needsVolumeOfInterest() )
	    unloadModel();
    }

    return true;
}


bool FaultStickSetT2DTransformer::doFaultStickSet(
				const SurfaceT2DTransfData& data )
{
    const MultiID& inpmid = data.inpmid_;
    const IOObj* ioobj = IOM().get( inpmid );
    if ( !ioobj )
	mErrRet( tr("Cannot find input FaultStickSet in repository") );

    EM::EMManager& em = EM::EMM();
    ConstRefMan<EM::EMObject> emobj = em.loadIfNotFullyLoaded( inpmid );
    if ( !emobj )
	mErrRet( tr("Failed to load the fault") );

    mDynamicCastGet(const EM::FaultStickSet*,fltss,emobj.ptr())
    if ( !fltss )
	mErrRet( tr("Incorrect object selected, "
		"3D FaultStickSet is expected for the workflow") );

    RefMan<EMObject> surf = createObject( data.outmid_, inpmid );
    if ( !surf )
	mErrRet( tr("Cannot access database") );

    mDynamicCastGet(EM::FaultStickSet*,outfss,surf.ptr())
    if ( !outfss)
	mErrRet( tr("The output type is incompatible with input type, "
		"FaultStickSet object expected") );

    outfss->enableGeometryChecks( false );
    const EM::FaultStickSetGeometry& fssgeom = fltss->geometry();
    const EM::ObjectType objtype = fssgeom.FSSObjType();
    const bool reqtypecheck = objtype != EM::ObjectType::FltSS2D3D;
    if ( reqtypecheck && zatf_.needsVolumeOfInterest() )
    {
	const EM::ObjectType expectedtype = is2d_ ? EM::ObjectType::FltSS2D
						   : EM::ObjectType::FltSS3D;
	if ( objtype != expectedtype )
	    mErrRet( tr("Incorrect type of opbject provided.\n"
		"Process is expecting a %1 FaultStickSet,\n"
		"a %2 FaultStickSet provided")
		.arg(is2d_ ? uiStrings::s2D() : uiStrings::s3D())
		.arg(is2d_ ? uiStrings::s3D() : uiStrings::s2D()) );
    }

    const ZDomain::Info& zinfo = emobj->zDomain();
    if ( is2d_ )
	handle2DTransformation( fssgeom, zinfo, *outfss );
    else
	handle3DTransformation( fssgeom, zinfo, *outfss );

    outsurfs_.add( outfss );
    return true;
}


int FaultStickSetT2DTransformer::nextStep()
{
    for ( const auto* data : datas_ )
    {
	if ( !data )
	    continue;

	doFaultStickSet( *data );
	nrdone_++;
    }

    return nrDone() >= totalNr()
		? ( errmsg_.isOK() ? Finished() : ErrorOccurred() )
		: MoreToDo();
}

}
