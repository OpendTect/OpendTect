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
#include "emfaultstickset.h"
#include "emfsstofault3d.h"
#include "faultstickset.h"
#include "emmanager.h"
#include "emposid.h"
#include "emsurfaceauxdata.h"
#include "ioman.h"
#include "uistrings.h"
#include "survgeom.h"
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
			ZAxisTransform& zatf, EMObjectType objtype )
{
    if ( objtype == EMObjectType::Hor3D )
	return new Horizon3DT2DTransformer( datas, zatf );
    else if ( objtype == EMObjectType::Hor2D )
	return new Horizon2DT2DTransformer( datas, zatf );
    else if ( objtype == EMObjectType::Flt3D )
	return new FaultT2DTransformer( datas, zatf );

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


void SurfaceT2DTransformer::load3DTranformVol()
{
    Interval<float> zgate( mUdf(float), mUdf(float) );
    for ( const auto* data : datas_ )
    {
	const SurfaceIODataSelection& surfsel = data->surfsel_;
	const Interval<float>& datagate = surfsel.sd.zrg;
	zgate.include( datagate );
    }

    TrcKeyZSampling samp;
    samp.hsamp_ = datas_[0]->surfsel_.rg;
    if ( samp.zsamp_.stop > zgate.stop )
	samp.zsamp_.stop = samp.zsamp_.snap( zgate.stop, OD::SnapUpward );

    zatvoi_ = zatf_.addVolumeOfInterest( samp, false );
    TaskRunner tskr;
    if ( !zatf_.loadDataIfMissing(zatvoi_,&tskr) )
    {
	curmsg_ = tr("Cannot load data for z-transform");
	return;
    }
}


RefMan<Surface> SurfaceT2DTransformer::createSurface( const MultiID& outmid )
{
    PtrMan<IOObj> obj = IOM().get( outmid );
    if ( !obj )
	return nullptr;

    EM::EMManager& em = EM::EMM();
    ObjectID objid = em.getObjectID( outmid );
    if ( !objid.isValid() )
	objid = em.createObject( getTypeString(), obj->name());

    RefMan<EM::EMObject> emobj = em.getObject( objid );
    mDynamicCastGet(EM::Surface*,horizon,emobj.ptr())
    if ( !horizon )
	return nullptr;

    const MultiID midx = horizon->multiID();
    horizon->change.disable();
    horizon->setZDomain( zatf_.toZDomainInfo() );
    return horizon;
}


#define mErrRet(s) { curmsg_ = s; return false; }

void SurfaceT2DTransformer::postStepCB( CallBacker* )
{
}

RefMan<Surface> SurfaceT2DTransformer::getTransformedSurface(
						    const MultiID& mid ) const
{
    for ( auto* surf : outsurfs_ )
	if ( surf->multiID() == mid )
	    return surf;

    return nullptr;
}


void SurfaceT2DTransformer::unloadVolume()
{
    if ( zatvoi_ >= 0 )
	zatf_.removeVolumeOfInterest( zatvoi_ );
}


//Horizon3DT2DTransformer
Horizon3DT2DTransformer::Horizon3DT2DTransformer(
	const ObjectSet<SurfaceT2DTransfData>& datas, ZAxisTransform& zatf )
    : SurfaceT2DTransformer(datas,zatf)
{
    mAttachCB(prestep,Horizon3DT2DTransformer::preStepCB);
    mAttachCB(poststep,Horizon3DT2DTransformer::postStepCB);
}


Horizon3DT2DTransformer::~Horizon3DT2DTransformer()
{
    detachAllNotifiers();
}


const StringView Horizon3DT2DTransformer::getTypeString()
{
    return EM::Horizon3D::typeStr();
}


void Horizon3DT2DTransformer::preStepCB( CallBacker* )
{
    if ( (nrdone_==0) && zatf_.needsVolumeOfInterest() && (totnr_>0) )
	load3DTranformVol();
}


bool Horizon3DT2DTransformer::doHorizon( const SurfaceT2DTransfData& data )
{
    RefMan<Surface> surf = createSurface( data.outmid_ );
    if ( !surf )
	mErrRet( tr("Cannot access database") );

    const MultiID& inpmid = data.inpmid_;
    const IOObj* ioobj = IOM().get( inpmid );
    if ( !ioobj )
	mErrRet( tr("Cannot find input horizon in repository") );

    EM::EMManager& em = EM::EMM();
    TaskRunner tskr;
    PtrMan<Executor> loader = em.objectLoader( inpmid, &data.surfsel_ );
    if ( !loader || !loader->execute() )
	mErrRet( uiStrings::sCantCreateHor() )

    RefMan<EM::EMObject> emobj = em.getObject( inpmid );
    if ( !emobj )
	mErrRet( tr("Failed to load the surface") );

    mDynamicCastGet(EM::Horizon3D*,outhor3D,surf.ptr())
    if ( !outhor3D )
	mErrRet( tr("The output type is incompatible with input type, "
	    "3D Horizon object expected") );

    mDynamicCastGet(const EM::Horizon3D*,hor,emobj.ptr())
    if ( !hor )
	mErrRet( tr("Incorrect object selected, "
	    "3D horizon is expected for the workflow") );

    TrcKeyZSampling bbox;
    bbox.hsamp_ = hor->range();
    bbox.zsamp_.setFrom( hor->getZRange() );

    const StepInterval<int> inlrg = hor->geometry().rowRange();
    const StepInterval<int> crlrg = hor->geometry().colRange();
    outhor3D->enableGeometryChecks( false );
    const RowCol step( inlrg.step, crlrg.step );
    outhor3D->geometry().setStep( step, step );
    Array2D<float>* arrdata = hor->createArray2D( &zatf_ );
    if ( !arrdata )
	return false;

    outhor3D->setArray2D( arrdata, bbox.hsamp_.start_,
	bbox.hsamp_.step_, false );

    const ObjectSet<BinIDValueSet>& auxdatas = hor->auxdata.getData();
    if ( !auxdatas.isEmpty() )
    {
	BufferStringSet nms;
	const int nraux = hor->auxdata.nrAuxData();
	for ( int idx=0; idx<nraux; idx++ )
	    nms.add( hor->auxdata.auxDataName(idx) );

	outhor3D->auxdata.addAuxData( nms, *auxdatas.get(0) );
    }

    outhor3D->setStratLevelID( hor->stratLevelID() );
    outsurfs_.add( outhor3D );

    return true;
}


int Horizon3DT2DTransformer::nextStep()
{
    if ( nrdone_ == totnr_ )
	return Finished();

    for ( const auto* data : datas_ )
    {
	if ( !data )
	    continue;

	doHorizon( *data );

	nrdone_++;
    }

    return MoreToDo();
}


void Horizon3DT2DTransformer::postStepCB( CallBacker* )
{
    if ( nrdone_ == totnr_ )
	unloadVolume();
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


const StringView Horizon2DT2DTransformer::getTypeString()
{
    return EM::Horizon2D::typeStr();
}


void Horizon2DT2DTransformer::preStepCB( CallBacker* )
{
    EM::EMManager& em = EM::EMM();
    TaskRunner tskr;
    for ( const auto* data : datas_ )
    {
	RefMan<Surface> surf = createSurface( data->outmid_ );
	if ( !surf )
	    continue;

	mDynamicCastGet(EM::Horizon2D*,outhor2D,surf.ptr())
	if ( !outhor2D )
	    continue;

	const MultiID& inpmid = data->inpmid_;
	const IOObj* ioobj = IOM().get( inpmid );
	if ( !ioobj )
	    continue;

	PtrMan<Executor> loader = em.objectLoader( inpmid, &data->surfsel_ );
	if ( !loader || !loader->execute() )
	   continue;

	RefMan<EM::EMObject> emobj = em.getObject( inpmid );
	if ( !emobj )
	    continue;

	mDynamicCastGet(const EM::Horizon2D*,hor,emobj.ptr())
	if ( !hor )
	    continue;

	outsurfs_.add( outhor2D );
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
    StepInterval<int> trcrangeenvelop;
    for ( const auto& inpmid : inpmids )
    {
	const int horidx = preprocessesedinpmid_.indexOf( inpmid );
	const Surface* inpsurf = inpsurfs_.get( horidx );
	mDynamicCastGet(const Horizon2D*,inphor2D,inpsurf)
	if ( !inphor2D )
	    continue;

	const StepInterval<int>& trcrng =
				inphor2D->geometry().colRange( geomid );
	trcrangeenvelop.include( trcrng );

	horidxs.add( horidx );
    }

    if ( !load2DVelCubeTransf(geomid,trcrangeenvelop) )
    {
	curmsg_ = tr("Failed to preload the velocity volume");
	return false;
    }

    for ( const auto& horidx : horidxs )
    {
	const Surface* inpsurf = inpsurfs_.get( horidx );
	mDynamicCastGet(const Horizon2D*,inphor2D,inpsurf)
	Surface* outsurf = outsurfs_.get( horidx );
	mDynamicCastGet(Horizon2D*,outhor2D,outsurf)
	if ( !outsurf )
	    continue;

	outhor2D->enableGeometryChecks( false );
	outhor2D->setStratLevelID( inphor2D->stratLevelID() );
	const StepInterval<int>& trcrng =
				    inphor2D->geometry().colRange( geomid );
	PtrMan<Array1D<float>> array = inphor2D->createArray1D( geomid,
								    &zatf_ );
	if ( !array )
	    continue;

	outhor2D->setArray1D( *array, trcrng, geomid, false );
    }

    unload2DVelCubeTransf();
    return true;
}


int Horizon2DT2DTransformer::nextStep()
{
    if ( nrdone_ == totnr_ )
	return Finished();

    for ( const auto& data : dataset_ )
    {
	do2DHorizon( data );
	nrdone_++;
    }

    return MoreToDo();
}


bool Horizon2DT2DTransformer::load2DVelCubeTransf( const Pos::GeomID& geomid,
					    const StepInterval<int>& trcrg )
{
    if ( !zatf_.needsVolumeOfInterest() )
	return true;

    TrcKeyZSampling tkzs;
    tkzs.hsamp_.set( geomid, trcrg );
    zatvoi_ = zatf_.addVolumeOfInterest( tkzs );
    TaskRunner tskr;
    return zatf_.loadDataIfMissing( zatvoi_, &tskr );
}


void Horizon2DT2DTransformer::unload2DVelCubeTransf()
{
    unloadVolume();
}


//FaultT2DTransformer
FaultT2DTransformer::FaultT2DTransformer(
	const ObjectSet<SurfaceT2DTransfData>& data, ZAxisTransform& zatf )
    : SurfaceT2DTransformer(data,zatf)
{
    mAttachCB(prestep,FaultT2DTransformer::preStepCB);
    mAttachCB(poststep,FaultT2DTransformer::postStepCB);
}


FaultT2DTransformer::~FaultT2DTransformer()
{
    detachAllNotifiers();
}


void FaultT2DTransformer::preStepCB( CallBacker* )
{
    if ( (nrdone_==0) && zatf_.needsVolumeOfInterest() && (totnr_>0) )
	load3DTranformVol();
}


void FaultT2DTransformer::postStepCB( CallBacker* )
{
    if ( nrdone_ == totnr_ )
	unloadVolume();
}


const StringView FaultT2DTransformer::getTypeString()
{
    return EM::Fault3D::typeStr();
}


bool FaultT2DTransformer::doFault( const SurfaceT2DTransfData& data )
{
    RefMan<Surface> surf = createSurface( data.outmid_ );
    if ( !surf )
	mErrRet( tr("Cannot access database") );

    const MultiID& inpmid = data.inpmid_;
    const IOObj* ioobj = IOM().get( inpmid );
    if ( !ioobj )
	mErrRet( tr("Cannot find input horizon in repository") );

    EM::EMManager& em = EM::EMM();
    TaskRunner tskr;
    PtrMan<Executor> loader = em.objectLoader( inpmid, &data.surfsel_ );
    if ( !loader || !loader->execute() )
	mErrRet( uiStrings::sCantCreateHor() )

    RefMan<EM::EMObject> emobj = em.getObject( inpmid );
    if ( !emobj )
	mErrRet( tr("Failed to load the surface") );

    mDynamicCastGet(EM::Fault3D*,outfault3d,surf.ptr())
    if ( !outfault3d )
	mErrRet( tr("The output type is incompatible with input type, "
						"Fault object expected") );

    mDynamicCastGet(EM::Fault3D*,flt,emobj.ptr())
    if ( !flt )
	mErrRet( tr("Incorrect object selected, "
				"3D horizon is expected for the workflow") );

    outfault3d->enableGeometryChecks( false );
    const EM::Fault3DGeometry& fltgeom = flt->geometry();
    const Geometry::FaultStickSurface* fssurf = fltgeom.geometryElement();
    if ( !fssurf )
	mErrRet( tr("Fault is empty") );

    const int nrsticks = fltgeom.nrSticks();
    const ZSampling zint = zatf_.getZInterval( true );
    EM::EMObject* tmpemobj = EM::FaultStickSet::create(EM::EMM());
    mDynamicCastGet( EM::FaultStickSet*, tmpfltss, tmpemobj )
    if ( tmpfltss )
	tmpfltss->enableGeometryChecks( false );

    for ( int idx=0; idx<nrsticks; idx++ )
    {
	const Geometry::FaultStick* stick = fssurf->getStick( idx );
	if ( !stick || stick->locs_.isEmpty() )
	    continue;

	const int sz = stick->size();
	const Coord3 firstcrd = stick->getCoordAtIndex( 0 );
	const Coord3 lastcrd = stick->getCoordAtIndex( sz-1 );
	if ( !zint.includes(firstcrd.z,false) ||
					    !zint.includes(lastcrd.z,false) )
	    continue;

	const Coord3& editnormal = stick->getNormal();
	for ( int crdidx=0; crdidx<sz; crdidx++ )
	{
	    Coord3 outcrd( stick->getCoordAtIndex(crdidx) );
	    outcrd.z = zatf_.transformTrc( stick->locs_[crdidx].trcKey(),
								    outcrd.z );
	    if ( crdidx == 0 )
	    {
		if ( !tmpfltss->geometry().insertStick(idx,0,outcrd,
						    editnormal,false) )
		    break;

		continue;
	    }

	    const RowCol rc( idx, crdidx );
	    tmpfltss->geometry().insertKnot( rc.toInt64(), outcrd, false);
	}
    }

    //Following is necessary step to convert FSS to Fault, taking care of
    //un-twisting Stick and resolvinf normals

    EM::FSStoFault3DConverter::Setup setup;
    setup.addtohistory_ = false;
    setup.pickplanedir_ = EM::FSStoFault3DConverter::Setup::Auto;
    EM::FSStoFault3DConverter fsstof3d( setup, *tmpfltss, *outfault3d );
    fsstof3d.convert( false );
    outsurfs_.add( outfault3d );
    return true;
}


int FaultT2DTransformer::nextStep()
{
    if ( nrdone_ == totnr_ )
	return Finished();

    for ( const auto* data : datas_ )
    {
	if ( !data )
	    continue;

	doFault( *data );
	nrdone_++;
    }

    return MoreToDo();
}

}
