/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "raytrace1d.h"

#include "ailayer.h"
#include "keystrs.h"
#include "sorting.h"
#include "uistrings.h"
#include "zoeppritzcoeff.h"

mImplFactory(RayTracer1D,RayTracer1D::factory)


StepInterval<float> RayTracer1D::sDefOffsetRange( bool infeet )
{
    return infeet ? StepInterval<float>( 0.f, 18000.f, 300.f )
		  : StepInterval<float>( 0.f, 6000.f, 100.f );
}


// RayTracer1D::Setup

RayTracer1D::Setup::Setup()
    : pdown_(true)
    , pup_(true)
    , doreflectivity_(true)
    , starttime_(0.f)
    , startdepth_(0.f)
    , depthsinfeet_(false)
    , offsetsinfeet_(false)
{
}


RayTracer1D::Setup::~Setup()
{
}


bool RayTracer1D::Setup::usePar( const IOPar& par )
{
    par.getYN( sKeyWavetypes(), pdown_, pup_ );
    par.getYN( sKeyReflectivity(), doreflectivity_ );
    return true;
}


void RayTracer1D::Setup::fillPar( IOPar& par ) const
{
    par.setYN( sKeyWavetypes(), pdown_, pup_ );
    par.setYN( sKeyReflectivity(), doreflectivity_);
}


// RayTracer1D

RayTracer1D::RayTracer1D()
    : model_(*new ElasticModel())
{}


RayTracer1D::~RayTracer1D()
{
    delete [] velmax_;
    delete [] twt_;
    delete [] reflectivities_;
    delete &model_;
}


RayTracer1D* RayTracer1D::createInstance( const IOPar& par, uiString& errm,
					  const Setup* rtsu )
{
    return createInstance( par, nullptr, errm, rtsu );
}


RayTracer1D* RayTracer1D::createInstance( const IOPar& par,
					  const ElasticModel* model,
					  uiString& errm, const Setup* rtsu )
{
    BufferString typekey;
    par.get( sKey::Type(), typekey );

    const Factory<RayTracer1D>& rt1dfact = factory();
    if ( !rt1dfact.hasName(typekey) && !rt1dfact.isEmpty() )
    {
	const StringView defnm = rt1dfact.getDefaultName();
	typekey.set( defnm.isEmpty() ? rt1dfact.getNames().first()->buf()
				     : defnm.buf() );
    }

    RayTracer1D* raytracer = rt1dfact.create( typekey );
    if ( !raytracer )
    {
	errm = tr("Raytracer not found. Perhaps all plugins are not loaded");
	return nullptr;
    }

    if ( rtsu )
	raytracer->setup() = *rtsu;

    if ( !raytracer->usePar(par) )
    {
	errm = raytracer->uiMessage();
	delete raytracer;
	return nullptr;
    }

    if ( model && !raytracer->setModel(*model) )
    {
	errm = raytracer->uiMessage();
	delete raytracer;
	return nullptr;
    }

    return raytracer;
}


bool RayTracer1D::usePar( const IOPar& par )
{
    TypeSet<float> offsets;
    par.get( sKeyOffset(), offsets );
    if ( offsets.isEmpty() )
	offsets += 0.f;

    bool offsetisinfeet = false;
    par.getYN( sKeyOffsetInFeet(), offsetisinfeet );
    setOffsets( offsets, offsetisinfeet );

    return setup().usePar( par );
}


void RayTracer1D::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), factoryKeyword() );
    TypeSet<float> offsets;
    getOffsets( offsets );
    par.set( sKeyOffset(), offsets );
    if ( offsets.size() > 1 ||
	 (offsets.size() == 1 && !mIsZero(offsets.first(),1e-2f)) )
	par.setYN( sKeyOffsetInFeet(), offsetsInFeet() );

    setup().fillPar( par );
}


bool RayTracer1D::hasSameParams( const RayTracer1D& rt ) const
{
    TypeSet<float> rtoffsets;
    rt.getOffsets( rtoffsets );
    BufferString rtkeyword = rt.factoryKeyword();
    return rtkeyword==factoryKeyword() && setup().pdown_==rt.setup().pdown_ &&
	   setup().pup_==rt.setup().pup_ &&
	   setup().doreflectivity_==rt.setup().doreflectivity_ &&
	   offsets_==rtoffsets;
}


void RayTracer1D::setIOParsToZeroOffset( IOPar& par )
{
    TypeSet<float> emptyset; emptyset += 0.f;
    par.set( RayTracer1D::sKeyOffset(), emptyset );
    par.removeWithKey( RayTracer1D::sKeyOffsetInFeet() );
}


void RayTracer1D::setOffsets( const TypeSet<float>& offsets,
			      bool offsetsinfeet )
{
    offsets_ = offsets;
    sort( offsets_ );

    if ( offsetsinfeet && !offsetsInFeet() )
    {
	for ( int idx=0; idx<offsets.size(); idx++ )
	    offsets_[idx] *= mFromFeetFactorF;
    }
    else if ( !offsetsinfeet && offsetsInFeet() )
    {
	for ( int idx=0; idx<offsets.size(); idx++ )
	    offsets_[idx] *= mToFeetFactorF;
    }

    const int offsetsz = offsets_.size();
    TypeSet<int> offsetidx( offsetsz, 0 );
    for ( int idx=0; idx<offsetsz; idx++ )
	offsetidx[idx] = idx;

    sort_coupled( offsets_.arr(), offsetidx.arr(), offsetsz );
    offsetpermutation_.erase();
    for ( int idx=0; idx<offsetsz; idx++ )
	offsetpermutation_ += offsetidx.indexOf( idx );
}


void RayTracer1D::getOffsets( TypeSet<float>& offsets ) const
{
    offsets = offsets_;
}


bool RayTracer1D::offsetsInFeet() const
{
    return setup().offsetsinfeet_;
}


bool RayTracer1D::needsSwave() const
{
    const bool zerooffsetonly =
		    offsets_.size()==1 && mIsZero(offsets_[0],mDefEps);
    return !(zerooffsetonly || !setup().doreflectivity_);
}


bool RayTracer1D::setModel( const ElasticModel& lys )
{
    msg_.setEmpty();
    if ( offsets_.isEmpty() )
    {
	msg_ = tr("Internal: Offsets must be set before the model");
	msg_.append( tr("Cannot do raytracing." ), true );
	return false;
    }

    const RefLayer::Type reqtyp = RefLayer::getType( needsSwave(), false,false);
    model_.copyFrom( lys, reqtyp );

    int firsterror = -1;
    model_.checkAndClean( firsterror, setup().doreflectivity_,
			  reqtyp >= RefLayer::Elastic );

    if ( model_.isEmpty() )
	msg_ = tr( "Model is empty" );
    else if ( firsterror != -1 )
	msg_ = tr( "Model has invalid values on layer: %1" )
		      .arg( firsterror+1 );

    return !model_.isEmpty();
}


od_int64 RayTracer1D::nrIterations() const
{ return model_.size(); }


bool RayTracer1D::doPrepare( int nrthreads )
{
    if ( !msg_.isEmpty() )
	return false;

    const int layersize = mCast( int, nrIterations() );
    delete [] velmax_;
    mTryAlloc( velmax_, float[layersize] );
    if ( !velmax_ )
    {
	msg_ = uiStrings::phrCannotAllocateMemory( layersize*sizeof(float) );
	return false;
    }

    const Setup& su = setup();
    OffsetReflectivityModel::Setup rmsu( true, su.doreflectivity_ );
    rmsu.pdown( su.pdown_ ).pup( su.pup_ ).starttime( su.starttime_ )
	.startdepth( su.startdepth_ ).depthsinfeet( su.depthsinfeet_ );
    rmsu.offsetsinfeet( offsetsInFeet() );
    refmodel_ = new OffsetReflectivityModel( getModel(), rmsu,
					     &offsets_, velmax_ );
    if ( !refmodel_ || !refmodel_->isOK() )
	return false;

    depths_ = refmodel_->getDefaultModel().getDepths()+1;
    const int nrmodels = refmodel_->nrModels();
    delete [] twt_;
    mTryAlloc( twt_, float*[nrmodels] );
    if ( !twt_ )
	return false;

    for ( int idx=0; idx<nrmodels; idx++ )
	twt_[idx] = refmodel_->get(idx)->getTimes()+1;
    zerooffstwt_ = refmodel_->getDefaultModel().getTimes()+1;

    sinarr_ = refmodel_->sinarr_;
    deleteAndNullArrPtr( reflectivities_ );
    if ( refmodel_->hasReflectivities() )
    {
	if ( refmodel_->nrRefModels() != nrmodels )
	    return false;

	mTryAlloc( reflectivities_, float_complex*[nrmodels] );
	if ( !reflectivities_ )
	    return false;

	for ( int idx=0; idx<nrmodels; idx++ )
	    reflectivities_[idx] = refmodel_->getRefs( idx );
    }

    return true;
}


bool RayTracer1D::doFinish( bool success )
{
    deleteAndNullArrPtr( velmax_ );
    if ( success )
	msg_.setEmpty();

    return success;
}


bool RayTracer1D::compute( int layer, int offsetidx, float rayparam )
{
    const RefLayer& ailayer = *model_.get( layer );
    const float downvel = setup().pdown_ ? ailayer.getPVel()
					 : ailayer.getSVel();

    if ( mIsUdf(downvel) || mIsUdf(rayparam) ||
	 !Math::IsNormalNumber(downvel) || !Math::IsNormalNumber(rayparam) )
    {
	pErrMsg("Invalid angle");
	DBG::forceCrash(false);
    }

    const float sini = downvel * rayparam;
    if ( mIsUdf(sini) || !Math::IsNormalNumber(sini) )
    {
	pErrMsg("Invalid angle");
	DBG::forceCrash(false);
    }

    sinarr_[offsetidx][layer] = sini;

    if ( !reflectivities_ || layer>=model_.size()-1 )
	return true;

    const float off = offsets_[offsetidx];
    float_complex reflectivity = 0;
    const int nrinterfaces = layer+1;

    if ( !mIsZero(off,mDefEps) )
    {
						 // critical angle reached
	if ( rayparam*model_.get(layer)->getPVel() > 1 ||
	     rayparam*model_.get(layer+1)->getPVel() > 1 )  // no reflection
	{
	    reflectivities_[offsetidx][layer] = reflectivity;
	    return true;
	}

	mAllocLargeVarLenArr( ZoeppritzCoeff, coefs, nrinterfaces );
        for ( int iidx=0; iidx<nrinterfaces; iidx++ )
	{
	    const ElasticLayer* elay0 = model_.get( iidx )->asElastic();
	    const ElasticLayer* elay1 = model_.get( iidx+1 )->asElastic();
	    if ( !elay0 || !elay1 )
		return false;

	    coefs[iidx].setInterface( rayparam, *elay0, *elay1 );
	}

	reflectivity = coefs[0].getCoeff( true, layer!=0, setup().pdown_,
				     layer==0 ? setup().pup_ : setup().pdown_ );

	if ( layer == 0 )
	{
	    reflectivities_[offsetidx][layer] = reflectivity;
	    return true;
	}

	for ( int iidx=1; iidx<nrinterfaces; iidx++ )
	{
	    reflectivity *= coefs[iidx].getCoeff( true, iidx!=layer,
						 setup().pdown_, iidx==layer ?
						 setup().pup_ : setup().pdown_);
	}

	for ( int iidx=nrinterfaces-2; iidx>=0; iidx--)
	{
	    reflectivity *= coefs[iidx].getCoeff( false, false, setup().pup_,
								setup().pup_);
	}
    }
    else
    {
	const float ai0 = model_.get( layer )->getAI();
	const float ai1 = model_.get( layer+1 )->getAI();
	const float real =
	   mIsZero(ai1,mDefEpsF) && mIsZero(ai0,mDefEpsF) ? mUdf(float)
						          : (ai1-ai0)/(ai1+ai0);
	reflectivity = float_complex( real, 0.f );
    }

    reflectivities_[offsetidx][layer] = reflectivity;

    return true;
}


ConstRefMan<OffsetReflectivityModel> RayTracer1D::getRefModel() const
{
    return refmodel_;
}


float RayTracer1D::getDepth( int layer ) const
{
    return depths_[layer];
}


float RayTracer1D::getTime( int layer, int offset ) const
{
    return twt_[offset][layer];
}


// VrmsRayTracer1D

bool VrmsRayTracer1D::doWork( od_int64 start, od_int64 stop, int nrthreads )
{
    const int offsz = offsets_.size();

    const float startdepth = setup().startdepth_;
    const bool depthsinfeet = setup().depthsinfeet_;
    const bool offsetsinfeet = setup().offsetsinfeet_;
    for ( int layer=mCast(int,start); layer<=stop; layer++, addToNrDone(1) )
    {
	const RefLayer& ellayer = *model_.get( layer );
	if ( !setup_.pdown_ && !ellayer.isElastic() )
	    return false;

	float depth = 2.f * (depths_[layer] - startdepth);
	if ( depthsinfeet )
	    depth *= mFromFeetFactorF;

	const float vel = setup_.pdown_ ? ellayer.getPVel() : ellayer.getSVel();
	for ( int osidx=0; osidx<offsz; osidx++ )
	{
	    float offset = offsets_[osidx];
	    if ( offsetsinfeet )
		offset *= mFromFeetFactorF;

	    const float angle = depth ? atan( offset / depth ) : 0.f;
	    const float rayparam = sin(angle) / vel;

	    if ( !compute(layer,osidx,rayparam) )
	    {
		msg_ = tr( "Can not compute layer %1"
			      "\n most probably the velocity is not correct" )
			.arg( layer );
		return false;
	    }
	}
    }

    return true;
}


bool VrmsRayTracer1D::compute( int layer, int offsetidx, float rayparam )
{
    const float tnmo = zerooffstwt_[layer];
    float vrms = velmax_[layer];
    if ( setup().depthsinfeet_ )
	vrms *= mFromFeetFactorF;

    float off = offsets_[offsetidx];
    if ( setup().offsetsinfeet_ )
	off *= mFromFeetFactorF;

    float twt = tnmo;
    if ( vrms && !mIsUdf(tnmo) )
	twt = Math::Sqrt(off*off/(vrms*vrms) + tnmo*tnmo);

    twt_[offsetidx][layer] = twt;

    return RayTracer1D::compute( layer, offsetidx, rayparam );
}
