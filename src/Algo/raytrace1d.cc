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

#include "hiddenparam.h"

mImplFactory(RayTracer1D,RayTracer1D::factory)


StepInterval<float> RayTracer1D::sDefOffsetRange()
{
    return sDefOffsetRange( SI().xyInFeet() ? Seis::OffsetType::OffsetFeet
					    : Seis::OffsetType::OffsetMeter );
}


StepInterval<float> RayTracer1D::sDefOffsetRange( Seis::OffsetType typ )
{
    if ( !Seis::isOffsetDist(typ) )
	return StepInterval<float>::udf();

    return typ == Seis::OffsetType::OffsetMeter
		? StepInterval<float>( 0.f, 6000.f, 100.f )
		: StepInterval<float>( 0.f, 18000.f, 300.f );
}


// RayTracer1D::Setup

class RayTracer1DHP
{
public:
				RayTracer1DHP()		{}

    RayTracer1DHP&		starttime( float t0 )
				{ starttime_ = t0; return *this; }
    RayTracer1DHP&		startdepth( float z0 )
				{ startdepth_ = z0; return *this; }
    RayTracer1DHP&		offsettype( Seis::OffsetType typ )
				{ offsettype_ = typ; return *this; }
    RayTracer1DHP&		depthtype( ZDomain::DepthType typ )
				{ depthtype_ = typ; return *this; }

    float starttime_ = 0.f;
    float startdepth_ = 0.f;
    Seis::OffsetType offsettype_ = Seis::OffsetType::OffsetMeter;
    ZDomain::DepthType depthtype_ = ZDomain::DepthType::Meter;
};

static HiddenParam<RayTracer1D::Setup,RayTracer1DHP*>
						raytracesuparsmgr_(nullptr);

RayTracer1D::Setup::Setup()
    : pdown_(true)
    , pup_(true)
    , doreflectivity_(true)
{
    raytracesuparsmgr_.setParam( this, new RayTracer1DHP() );
}


RayTracer1D::Setup::Setup( const Setup& oth )
{
    raytracesuparsmgr_.setParam( this, new RayTracer1DHP() );
    *this = oth;
}


RayTracer1D::Setup::~Setup()
{
    raytracesuparsmgr_.removeAndDeleteParam( this );
}


RayTracer1D::Setup& RayTracer1D::Setup::operator =( const Setup& oth )
{
    if ( &oth == this )
	return *this;

    pdown_ = oth.pdown_;
    pup_ = oth.pup_;
    doreflectivity_ = oth.doreflectivity_;
    *raytracesuparsmgr_.getParam( this ) = *raytracesuparsmgr_.getParam( &oth );

    return *this;
}


RayTracer1D::Setup& RayTracer1D::Setup::starttime( float val )
{
    raytracesuparsmgr_.getParam( this )->starttime( val );
    return *this;
}


RayTracer1D::Setup& RayTracer1D::Setup::startdepth( float val )
{
    raytracesuparsmgr_.getParam( this )->startdepth( val );
    return *this;
}


RayTracer1D::Setup& RayTracer1D::Setup::depthtype( ZDomain::DepthType typ )
{
    raytracesuparsmgr_.getParam( this )->depthtype( typ );
    return *this;
}


float RayTracer1D::Setup::getStartTime() const
{
    return raytracesuparsmgr_.getParam( this )->starttime_;
}


float RayTracer1D::Setup::getStartDepth() const
{
    return raytracesuparsmgr_.getParam( this )->startdepth_;
}


Seis::OffsetType RayTracer1D::Setup::offsetType() const
{
    return raytracesuparsmgr_.getParam( this )->offsettype_;
}


ZDomain::DepthType RayTracer1D::Setup::depthType() const
{
    return raytracesuparsmgr_.getParam( this )->depthtype_;
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


RayTracer1D::Setup& RayTracer1D::Setup::offsettype( Seis::OffsetType typ )
{
    if ( Seis::isOffsetDist(typ) )
	raytracesuparsmgr_.getParam( this )->offsettype( typ );

    return *this;
}


bool RayTracer1D::Setup::areOffsetsInFeet() const
{
    return offsetType() == Seis::OffsetType::OffsetFeet;
}


bool RayTracer1D::Setup::areDepthsInFeet() const
{
    return depthType() == ZDomain::DepthType::Feet;
}


// RayTracer1D

RayTracer1D::RayTracer1D()
    : model_(*new ElasticModelOv())
{}


RayTracer1D::~RayTracer1D()
{
    delete [] velmax_;
    delete [] twt_;
    delete [] reflectivities_;
    delete &model_;
}


RayTracer1D* RayTracer1D::createInstance( const IOPar& par, uiString& errm )
{
    return createInstance( par, nullptr, errm, nullptr );
}


RayTracer1D* RayTracer1D::createInstance( const IOPar& par,
					  const ElasticModel* model,
					  uiString& errm )
{
    return createInstance( par, model, errm, nullptr );
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
    setOffsets( offsets, offsetisinfeet ? Seis::OffsetType::OffsetFeet
					: Seis::OffsetType::OffsetMeter );

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
	par.setYN( sKeyOffsetInFeet(), areOffsetsInFeet() );

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
    static TypeSet<float> emptyset( 1, 0.f );
    par.set( RayTracer1D::sKeyOffset(), emptyset );
    par.removeWithKey( RayTracer1D::sKeyOffsetInFeet() );
}


void RayTracer1D::setOffsets( const TypeSet<float>& offsets )
{
    setOffsets( offsets, SI().zInFeet() ? Seis::OffsetType::OffsetFeet
					: Seis::OffsetType::OffsetMeter);
}


void RayTracer1D::setOffsets( const TypeSet<float>& offsets,
			      Seis::OffsetType offstype )
{
    offsets_ = offsets;
    sort( offsets_ );

    const bool offsetsinfeet = offstype == Seis::OffsetType::OffsetFeet;
    if ( offsetsinfeet && !areOffsetsInFeet() )
    {
	for ( int idx=0; idx<offsets.size(); idx++ )
	    offsets_[idx] *= mFromFeetFactorF;
    }
    else if ( !offsetsinfeet && areOffsetsInFeet() )
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


bool RayTracer1D::areOffsetsInFeet() const
{
    return setup().areOffsetsInFeet();
}


bool RayTracer1D::areDepthsInFeet() const
{
    return setup().areDepthsInFeet();
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
    rmsu.offsettype( su.offsetType() ).depthtype( su.depthType() )
	.starttime( su.getStartTime() ).startdepth( su.getStartDepth() )
	.pdown( su.pdown_ ).pup( su.pup_ );
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
    const RefLayer& reflayer0 = *model_.get( layer );
    const bool pdown = setup().pdown_;
    const bool pup = setup().pup_;
    const float downvel = pdown ? reflayer0.getPVel() : reflayer0.getSVel();

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
    const RefLayer& reflayer1 = *model_.get( nrinterfaces );

    if ( mIsZero(off,mDefEps) )
    {
	const float ai0 = reflayer0.getAI();
	const float ai1 = reflayer1.getAI();
	const float real = mIsUdf(ai0) || mIsUdf(ai1) ||
			   (mIsZero(ai1,mDefEpsF) && mIsZero(ai0,mDefEpsF))
			 ? mUdf(float) : (ai1-ai0)/(ai1+ai0);
	reflectivity = float_complex( real, 0.f );
    }
    else
    {
						 // critical angle reached
	if ( rayparam*reflayer0.getPVel() > 1 ||
	     rayparam*reflayer1.getPVel() > 1 )  // no reflection
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

	reflectivity = coefs[0].getCoeff( true, layer!=0, pdown,
					  layer==0 ? pup : pdown );

	if ( layer == 0 )
	{
	    reflectivities_[offsetidx][layer] = reflectivity;
	    return true;
	}

	for ( int iidx=1; iidx<nrinterfaces; iidx++ )
	{
	    reflectivity *= coefs[iidx].getCoeff( true, iidx!=layer,
						  pdown, iidx==layer ?
						  pup : pdown);
	}

	for ( int iidx=nrinterfaces-2; iidx>=0; iidx--)
	{
	    reflectivity *= coefs[iidx].getCoeff( false, false, pup, pup );
	}
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

    const bool pdown = setup().pdown_;
    const float startdepth = setup().getStartDepth();
    const bool offsetsinfeet = areOffsetsInFeet();
    const bool depthsinfeet = areDepthsInFeet();
    for ( int layer=mCast(int,start); layer<=stop; layer++, addToNrDone(1) )
    {
	const RefLayer& ellayer = *model_.get( layer );
	if ( !pdown && !ellayer.isElastic() )
	    return false;

	float depth = 2.f * (depths_[layer] - startdepth);
	if ( depthsinfeet )
	    depth *= mFromFeetFactorF;

	const float vel = pdown ? ellayer.getPVel() : ellayer.getSVel();
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
    if ( areDepthsInFeet() )
	vrms *= mFromFeetFactorF;

    float off = offsets_[offsetidx];
    if ( areOffsetsInFeet() )
	off *= mFromFeetFactorF;

    float twt = tnmo;
    if ( vrms && !mIsUdf(tnmo) )
	twt = Math::Sqrt(off*off/(vrms*vrms) + tnmo*tnmo);

    twt_[offsetidx][layer] = twt;

    return RayTracer1D::compute( layer, offsetidx, rayparam );
}
