/*+
________________________________________________________________________

Copyright:	(C) 1995-2022 dGB Beheer B.V.
License:	https://dgbes.com/licensing
________________________________________________________________________

-*/


#include "emsurft2dtransformer.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emposid.h"
#include "emsurfaceauxdata.h"
#include "ioman.h"
#include "uistrings.h"
#include "zaxistransform.h"

namespace EM
{

SurfaceT2DTransfData::SurfaceT2DTransfData( const SurfaceIOData& surfdata )
    : surfsel_(surfdata)
{}

SurfaceT2DTransformer::SurfaceT2DTransformer(
			const ObjectSet<SurfaceT2DTransfData>& datas,
			ZAxisTransform& zatf,
			IOObjInfo::ObjectType objtype )
    : Executor("Surface Domain Transformation")
    , datas_(datas)
    , objtype_(objtype)
    , zatf_(zatf)
    , zinfo_(new ZDomain::Info(SI().zDomainInfo()))
{
    totnr_ = datas_.size();
    mAttachCB(prestep,SurfaceT2DTransformer::preStepCB);
    mAttachCB(poststep,SurfaceT2DTransformer::postStepCB);
}


SurfaceT2DTransformer::~SurfaceT2DTransformer()
{
    detachAllNotifiers();
    delete zinfo_;
}


od_int64 SurfaceT2DTransformer::totalNr() const
{
    return totnr_;
}


uiString SurfaceT2DTransformer::uiNrDoneText() const
{
    return tr("Transforming horizons");
}


void SurfaceT2DTransformer::setZDomain( const ZDomain::Info& zinfo )
{
    if ( zinfo_->isCompatibleWith(zinfo) )
	return;

    delete zinfo_;
    zinfo_ = new ZDomain::Info( zinfo );
}


void SurfaceT2DTransformer::preStepCB(CallBacker*)
{
    if ( (nrdone_==0) && zatf_.needsVolumeOfInterest() && (totnr_>0) )
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

    return;
}


RefMan<Surface> SurfaceT2DTransformer::createSurface( const MultiID& outmid )
{
    PtrMan<IOObj> obj = IOM().get( outmid );
    if ( !obj )
	return nullptr;

    EM::EMManager& em = EM::EMM();
    ObjectID objid = em.getObjectID( outmid );
    if ( !objid.isValid() )
	objid = em.createObject( EM::Horizon3D::typeStr(), obj->name() );

    RefMan<EM::EMObject> emobj = em.getObject( objid );
    mDynamicCastGet(EM::Surface*,horizon,emobj.ptr())
    if ( !horizon )
	return nullptr;

    const MultiID midx = horizon->multiID();
    horizon->change.disable();
    horizon->setZDomain( *zinfo_ );
    return horizon;
}


#define mErrRet(s) { curmsg_ = s; return false; }

bool SurfaceT2DTransformer::doHorizon( const SurfaceT2DTransfData& data )
{
    RefMan<Surface> surf = createSurface( data.outmid_ );
    mDynamicCastGet(EM::Horizon3D*,outhor3D,surf.ptr())
    if ( !outhor3D )
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

    mDynamicCastGet(EM::Horizon3D*,hor,emobj.ptr())
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
    surfs_.add( outhor3D );

    return true;
}


int SurfaceT2DTransformer::nextStep()
{
    if ( nrdone_ == totnr_ )
	return Finished();

    for ( const auto* data : datas_ )
    {
	if ( !data )
	    continue;

	if ( objtype_ == IOObjInfo::Horizon3D )
	    doHorizon( *data );

	nrdone_++;
    }

    return MoreToDo();
}


void SurfaceT2DTransformer::postStepCB( CallBacker* )
{
    if ( nrdone_ == totnr_ )
    {
	if ( zatvoi_ >= 0 )
	    zatf_.removeVolumeOfInterest( zatvoi_ );
    }
}

RefMan<Surface> SurfaceT2DTransformer::getTransformedSurface(
						    const MultiID& mid ) const
{
    for ( auto* surf : surfs_ )
	if ( surf->multiID() == mid )
	    return surf;

    return nullptr;
}

}
