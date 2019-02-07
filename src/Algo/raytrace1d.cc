/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Kris / Bruno
Date:          2011
________________________________________________________________________

-*/




#include "raytrace1d.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "iopar.h"
#include "sorting.h"
#include "uistrings.h"
#include "velocitycalc.h"
#include "zoeppritzcoeff.h"



RayTracerData::RayTracerData( const ElasticModel& layers,
			      const TypeSet<float>& offsets )
    : offsets_(offsets)
    , zerooffstwt_(*new Array1DImpl<float>(0))
    , twt_(*new Array2DImpl<float>(0,0))
    , sini_(*new Array2DImpl<float>(0,0))
{
    init( layers );
}


RayTracerData::RayTracerData( const RayTracerData& oth )
    : zerooffstwt_(*new Array1DImpl<float>(0))
    , twt_(*new Array2DImpl<float>(0,0))
    , sini_(*new Array2DImpl<float>(0,0))
{
    *this = oth;
}


RayTracerData::~RayTracerData()
{
    delete &zerooffstwt_;
    delete &twt_;
    delete &sini_;
    delete reflectivity_;
    delete zerooffsett2dmodel_;
}


RayTracerData& RayTracerData::operator=( const RayTracerData& oth )
{
    if ( this == &oth )
	return *this;

    const_cast<TypeSet<float>& >( depths_ ) = oth.depths_;
    const_cast<TypeSet<float>& >( offsets_ ) = oth.offsets_;
    zerooffstwt_ = oth.zerooffstwt_;
    twt_ = oth.twt_;
    sini_ = oth.sini_;

    if ( !oth.reflectivity_ )
	deleteAndZeroPtr( reflectivity_ );
    else
    {
	if ( reflectivity_ )
	    *reflectivity_ = *oth.reflectivity_;
	else
	    reflectivity_ = new Array2DImpl<float_complex>( *oth.reflectivity_);
    }

    if ( !oth.zerooffsett2dmodel_ )
	deleteAndZeroPtr( zerooffsett2dmodel_ );
    else
    {
	if ( zerooffsett2dmodel_ )
	    *zerooffsett2dmodel_ = *oth.zerooffsett2dmodel_;
	else
	    zerooffsett2dmodel_ = new TimeDepthModel( *oth.zerooffsett2dmodel_);
    }

    t2dmodels_.setEmpty();
    for ( int idx=0; idx<oth.t2dmodels_.size(); idx++ )
	t2dmodels_.add( new TimeDepthModel(*oth.t2dmodels_.get(idx) ) );

    reflmodels_.setEmpty();
    for ( int idx=0; idx<oth.reflmodels_.size(); idx++ )
	reflmodels_.add( new ReflectivityModel(*oth.reflmodels_.get(idx) ) );

    return *this;
}


void RayTracerData::init( const ElasticModel& layers )
{
    const int nrlayers = layers.size();
    TypeSet<float>& depths = const_cast<TypeSet<float>& >( depths_ );
    if ( !depths.setSize(nrlayers,0.f) )
	return;

    const bool zinfeet = SI().zInFeet();
    double depth = nrlayers > 0 ? layers[0].thickness_ : 0.;
    if ( zinfeet ) depth *= mToFeetFactorD;
    if ( nrlayers > 0 ) depths[0] = mCast(float,depth);
    for ( int idx=1; idx<nrlayers; idx++ )
    {
	double thickness = layers[idx].thickness_;
	if ( zinfeet ) thickness *= mToFeetFactorD;
	depth += thickness;
	depths[idx] = mCast(float,depth);
    }

    const int nroffs = nrOffset();
    const Array1DInfoImpl layersz( nrlayers );
    const Array2DInfoImpl modelsz( nrlayers, nroffs );
    if ( (sini_.info() != modelsz && !sini_.setInfo(modelsz) ) ||
	 (twt_.info() != modelsz && !twt_.setInfo(modelsz) ) ||
	 (zerooffstwt_.info() != layersz && !zerooffstwt_.setInfo(layersz) ) )
	return;

    zerooffstwt_.setAll( mUdf(float) );
    twt_.setAll( mUdf(float) );
    sini_.setAll( 0.f );
}


bool RayTracerData::isOK() const
{
    if ( depths_.isEmpty() || offsets_.isEmpty() )
	return false;

    const int nrlayers = depths_.size();
    const int nroffsets = nrOffset();

    const Array2DInfoImpl modelsz( nrlayers, nroffsets );
    if ( twt_.info() != modelsz || sini_.info() != modelsz ||
	 !twt_.isOK() || !sini_.isOK() ||
	 zerooffstwt_.getSize(0) != nrlayers || !zerooffstwt_.isOK() )
	return false;

    if ( reflectivity_ )
    {
	const Array2DInfoImpl reflsz( nrlayers-1, nroffsets );
	if ( reflectivity_->info() != reflsz || !reflectivity_->isOK() )
	    return false;
    }

    return true;
}


bool RayTracerData::isFinalised() const
{
    if ( !isOK() )
	return false;

    const int nroffsets = nrOffset();
    return zerooffsett2dmodel_ && t2dmodels_.size() == nroffsets &&
	   (hasReflectivity() && reflmodels_.size() == nroffsets);
}


bool RayTracerData::setWithReflectivity( bool yn )
{
    const int nrinterfaces = nrLayers()-1;
    const int nroffs = nrOffset();
    if ( !yn || nrinterfaces <= 0 || nroffs <= 0 )
    {
	deleteAndZeroPtr( reflectivity_ );
	return true;
    }

    const Array2DInfoImpl reflsz( nrinterfaces, nroffs );
    if ( reflectivity_ && !reflectivity_->setInfo(reflsz) )
    {
	deleteAndZeroPtr( reflectivity_ );
	return false;
    }
    else
    {
	reflectivity_ = new Array2DImpl<float_complex>( reflsz );
	if ( !reflectivity_ || !reflectivity_->isOK() )
	    { deleteAndZeroPtr( reflectivity_ ); return false; }
    }

    reflectivity_->setAll( mUdf( float_complex ) );

    return true;
}


bool RayTracerData::validDepthIdx( int depth ) const
{
    return depths_.validIdx( depth );
}


bool RayTracerData::validOffsetIdx( int offset ) const
{
    return offsets_.validIdx( offset );
}


bool RayTracerData::hasZeroOffsetOnly() const
{
    return nrOffset() == 1 &&  mIsZero(offsets_[0],mDefEpsF);
}


float RayTracerData::getOffset( int offset ) const
{
#ifdef __debug__
    if ( !validOffsetIdx(offset) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    return offsets_[offset];
}


float RayTracerData::getDepth( int layer ) const
{
#ifdef __debug__
    if ( !validDepthIdx(layer) )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif
    return depths_[layer];
}


float RayTracerData::getTnmo( int layer ) const
{ return zerooffstwt_.get( layer ); }

float RayTracerData::getTime( int layer,int offset ) const
{ return twt_.get( layer, offset ); }

float RayTracerData::getSinAngle( int layer, int offset ) const
{ return sini_.get( layer, offset ); }


float_complex RayTracerData::getReflectivity( int layer,int offset ) const
{
    return reflectivity_ ? reflectivity_->get( layer, offset )
			 : mUdf( float_complex );
}


const TimeDepthModel& RayTracerData::getZeroOffsTDModel() const
{
    return *zerooffsett2dmodel_;
}


const TimeDepthModel& RayTracerData::getTDModel( int offset ) const
{
    return *t2dmodels_.get( offset );;
}


const ReflectivityModel& RayTracerData::getReflectivity( int offset ) const
{
    return *reflmodels_.get( offset );
}


bool RayTracerData::finalise()
{
    if ( !zerooffsett2dmodel_ )
	zerooffsett2dmodel_ = new TimeDepthModel;

    if ( !getZeroOffsTDModel(*zerooffsett2dmodel_) )
	return false;

    const int nroffsets = nrOffset();
    if ( !t2dmodels_.isEmpty() ) t2dmodels_.setEmpty();
    for ( int idx=0; idx<nroffsets; idx++ )
    {
	TimeDepthModel* td2model = new TimeDepthModel;
	if ( !getTDModel(idx,*td2model) )
	    { t2dmodels_.setEmpty(); return false; }

	t2dmodels_.add( td2model );
    }

    if ( !hasReflectivity() )
	return true;

    if ( !reflmodels_.isEmpty() ) reflmodels_.setEmpty();
    for ( int idx=0; idx<nroffsets; idx++ )
    {
	ReflectivityModel* refmodel = new ReflectivityModel;
	if ( !getReflectivity(idx,*refmodel) )
	    { reflmodels_.setEmpty(); return false; }

	reflmodels_.add( refmodel );
    }

    return true;
}


bool RayTracerData::getZeroOffsTDModel( TimeDepthModel& d2tm ) const
{
    return getTDM( zerooffstwt_, d2tm );
}


bool RayTracerData::getTDModel( int offset, TimeDepthModel& d2tm ) const
{
    if ( twt_.isEmpty() || !validOffsetIdx(offset) )
	return false;

    if ( offset == 0 && hasZeroOffsetOnly() )
	return getZeroOffsTDModel( d2tm );

    Array1DSlice<float> offstwt( twt_ );
    offstwt.setDimMap( 0, 0 );
    offstwt.setPos( 1, offset );
    if ( !offstwt.init() )
	return false;

    return getTDM( offstwt, d2tm );
}


bool RayTracerData::getTDM( const Array1D<float>& twt,
			    TimeDepthModel& d2tm ) const
{
    const int layersize = nrLayers();

    TypeSet<double> times, depths;
    depths += 0.;
    times += 0.;
    for ( int lidx=0; lidx<layersize; lidx++ )
    {
	double time = twt.get( lidx );
	if ( mIsUdf(time) ) time = times[times.size()-1];
	if ( time < times[times.size()-1] )
	    continue;

	depths += depths_[lidx];
	times += time;
    }

    return d2tm.setModel( depths.arr(), times.arr(), times.size() ).isOK();
}


bool RayTracerData::getReflectivity( int offset, ReflectivityModel& model) const
{
    if ( !reflectivity_ || !validOffsetIdx(offset) )
	return false;

    const int nrinterfaces = reflectivity_->getSize(0);
    if ( model.size() < nrinterfaces && !model.setCapacity(nrinterfaces,false) )
	return false;

    ReflectivitySpike spike;
    for ( int iidx=0; iidx<nrinterfaces; iidx++ )
    {
	spike.reflectivity_ = getReflectivity( iidx, offset );
	spike.depth_ = getDepth( iidx );
	spike.correctedtime_ = getTnmo( iidx );
	spike.time_ = getTime( iidx, offset );
	if ( !spike.isDefined() )
	    continue;

	model += spike;
    }

    return true;
}


void RayTracerData::setTnmo( int layer, float tnmo )
{ zerooffstwt_.set( layer, tnmo ); }

void RayTracerData::setTWT( int layer, int offset, float twt )
{ twt_.set( layer, offset, twt ); }


void RayTracerData::setSinAngle( int layer, int offset, float sini )
{ sini_.set( layer, offset, sini ); }

void RayTracerData::setReflectivity( int layer, int offset,
				     float_complex reflval )
{
    if ( reflectivity_ )
	reflectivity_->set( layer, offset, reflval );
}



mImplClassFactory(RayTracer1D,factory)


float RayTracer1D::cDefaultBlockRatio()
{
    return 0.01;
}


bool RayTracer1D::Setup::usePar( const IOPar& par )
{
    par.getYN( sKeyPWave(), pdown_, pup_);
    par.getYN( sKeyReflectivity(), doreflectivity_);
    return true;
}


void RayTracer1D::Setup::fillPar( IOPar& par ) const
{
    par.setYN( sKeyPWave(), pdown_, pup_ );
    par.setYN( sKeyReflectivity(), doreflectivity_);
}


RayTracer1D::RayTracer1D()
    : ParallelTask("RayTracer1D")
{}


RayTracer1D::~RayTracer1D()
{}


RayTracer1D* RayTracer1D::createInstance( const IOPar& par, uiString& errm )
{
    BufferString type;
    par.get( sKey::Type(), type );
    if ( type.isEmpty() || factory().indexOf(type) < 0 )
    {
	const int nravail = factory().size();
	if ( nravail < 1 )
	    { errm = mINTERNAL( "No Ray Tracers in factory" ); return 0; }

	    // last one is probably the 'best' one
	type = factory().getKeys().get( nravail-1 );
    }

    RayTracer1D* raytracer = factory().create( type );
    if ( !raytracer )
    {
	raytracer = factory().createAny();
	if ( !raytracer )
	    { errm = mINTERNAL("factory produces no Ray Tracer"); return 0; }
    }

    if ( !raytracer->usePar(par) )
    {
	errm = raytracer->errMsg();
	delete raytracer;
	return 0;
    }

    return raytracer;
}


uiString RayTracer1D::message() const
{
    return errmsg_.isEmpty() ? tr("Ray Tracing") : errmsg_;
}


uiString RayTracer1D::nrDoneText() const
{
    return tr("Layers handled");
}


bool RayTracer1D::usePar( const IOPar& par )
{
    TypeSet<float> offsets;
    par.get( sKeyOffset(), offsets );
    if ( offsets.isEmpty() )
	offsets += 0;

    setOffsets( offsets );
    return setup().usePar( par );
}


void RayTracer1D::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), factoryKeyword() );
    TypeSet<float> offsets;
    getOffsets( offsets );
    par.set( sKeyOffset(), offsets );
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
    TypeSet<float> emptyset; emptyset += 0;
    par.set( RayTracer1D::sKeyOffset(), emptyset );
}


void RayTracer1D::setOffsets( const TypeSet<float>& offsets )
{
    offsets_ = offsets;
    sort( offsets_ );
    if ( SI().zInFeet() )
    {
	const int offsetsz = offsets_.size();
	for ( int idx=0; idx<offsetsz; idx++ )
	    offsets_[idx] *= mToFeetFactorF;
    }
}


void RayTracer1D::getOffsets( TypeSet<float>& offsets ) const
{
    offsets = offsets_;
    if ( SI().zInFeet() )
    {
	for ( int idx=0; idx<offsets.size(); idx++ )
	    offsets[idx] *= mFromFeetFactorF;
    }
}


bool RayTracer1D::setModel( const ElasticModel& lys )
{
    if ( offsets_.isEmpty() )
    {
	errmsg_ = tr("Internal: Offsets must be set before the model");
	errmsg_.appendPhrase( tr("Cannot do raytracing" ) );
	return false;
    }

    //Zero-offset: Vs is not required, density not either if !doreflectivity_
    const bool zerooffsetonly =
	offsets_.size()==1 && mIsZero(offsets_[0],mDefEps);

    model_ = lys;
    int firsterror = -1;
    model_.checkAndClean( firsterror, setup().doreflectivity_, !zerooffsetonly);

    if ( model_.isEmpty() )
	errmsg_ = tr("Model is empty");
    else if ( firsterror != -1 )
	errmsg_ = tr("Model has invalid values on layer: %1")
		      .arg( firsterror+1 );

    return !model_.isEmpty();
}


od_int64 RayTracer1D::nrIterations() const
{ return model_.size(); }


bool RayTracer1D::doPrepare( int )
{
    const int layersize = mCast( int, nrIterations() );
    if ( !velmax_.setSize(layersize,0.f) )
    {
	errmsg_ = uiStrings::phrCannotAllocateMemory( layersize );
	return false;
    }

    for ( int idx=0; idx<layersize; idx++ )
	velmax_[idx] = model_[idx].vel_;
    //Initial value only, may change

    result_ = new RayTracerData( model_, offsets_ );
    if ( result_ )
	result_->setWithReflectivity( setup().doreflectivity_ );

    if ( !result_ || !result_->isOK() )
    {
	result_ = 0;
	errmsg_ = uiStrings::phrCannotAllocateMemory();
	return false;
    }

    setZeroOffsetTWT();
    return true;
}


bool RayTracer1D::doFinish( bool success )
{
    if ( success )
	return result_->finalise();

    result_ = 0;

    return false;
}


void RayTracer1D::setZeroOffsetTWT()
{
    const int layersize = result_->nrLayers();
    float dnmotime, dvrmssum, unmotime, uvrmssum;
    float prevdnmotime, prevdvrmssum, prevunmotime, prevuvrmssum;
    prevdnmotime = prevdvrmssum = prevunmotime = prevuvrmssum = 0;
    for ( int lidx=0; lidx<layersize; lidx++ )
    {
	const ElasticLayer& layer = model_[lidx];
	const float dvel = setup().pdown_ ? layer.vel_ : layer.svel_;
	const float uvel = setup().pup_ ? layer.vel_ : layer.svel_;
	dnmotime = dvrmssum = unmotime = uvrmssum = 0;
	const float dz = layer.thickness_;

	dnmotime = dz / dvel;
	dvrmssum = dz * dvel;
	unmotime = dz / uvel;
	uvrmssum = dz * uvel;

	dvrmssum += prevdvrmssum;
	uvrmssum += prevuvrmssum;
	dnmotime += prevdnmotime;
	unmotime += prevunmotime;

	prevdvrmssum = dvrmssum;
	prevuvrmssum = uvrmssum;
	prevdnmotime = dnmotime;
	prevunmotime = unmotime;

	const float vrmssum = dvrmssum + uvrmssum;
	const float twt = unmotime + dnmotime;
	velmax_[lidx] = Math::Sqrt( vrmssum / twt );
	result_->setTnmo( lidx, twt );
    }
}


bool RayTracer1D::compute( int layer, int offsetidx, float rayparam )
{
    const ElasticLayer& ellayer = model_[layer];
    const float downvel = setup().pdown_ ? ellayer.vel_ : ellayer.svel_;
    const float sini = downvel * rayparam;

    result_->setSinAngle( layer, offsetidx, sini );

    if ( !result_->hasReflectivity() || layer>=model_.size()-1 )
	return true;

    const float off = result_->getOffset( offsetidx );
    float_complex reflectivityval = 0;
    const int nrinterfaces = layer+1;

    if ( !mIsZero(off,mDefEps) )
    {
	if ( rayparam*model_[layer].vel_ > 1 ||   // critical angle reached
	     rayparam*model_[layer+1].vel_ > 1 )  // no reflection
	{
	    result_->setReflectivity( layer, offsetidx, reflectivityval );
	    return true;
	}

	mAllocLargeVarLenArr( ZoeppritzCoeff, coefs, nrinterfaces );
        for ( int iidx=0; iidx<nrinterfaces; iidx++ )
	    coefs[iidx].setInterface( rayparam, model_[iidx], model_[iidx+1] );

	reflectivityval = coefs[0].getCoeff( true, layer!=0, setup().pdown_,
				     layer==0 ? setup().pup_ : setup().pdown_ );

	if ( layer == 0 )
	{
	    result_->setReflectivity( layer, offsetidx, reflectivityval );
	    return true;
	}

	for ( int iidx=1; iidx<nrinterfaces; iidx++ )
	{
	    reflectivityval *= coefs[iidx].getCoeff( true, iidx!=layer,
						 setup().pdown_, iidx==layer ?
						 setup().pup_ : setup().pdown_);
	}

	for ( int iidx=nrinterfaces-2; iidx>=0; iidx--)
	{
	    reflectivityval *= coefs[iidx].getCoeff( false, false, setup().pup_,
								setup().pup_);
	}
    }
    else
    {
	const ElasticLayer& ail0 = model_[ layer ];
	const ElasticLayer& ail1 = model_[ layer+1 ];
	const float ai0 = ail0.vel_ * ail0.den_;
	const float ai1 = ail1.vel_ * ail1.den_;
	const float real =
	   mIsZero(ai1,mDefEpsF) && mIsZero(ai0,mDefEpsF) ? mUdf(float)
						          : (ai1-ai0)/(ai1+ai0);
	reflectivityval = float_complex( real, 0 );
    }

    result_->setReflectivity( layer, offsetidx, reflectivityval );

    return true;
}



bool VrmsRayTracer1D::doWork( od_int64 start, od_int64 stop, int )
{
    const int offsz = result_->nrOffset();

    for ( int layer=mCast(int,start); layer<=stop; layer++, addToNrDone(1) )
    {
	const ElasticLayer& ellayer = model_[layer];
	const double depth = 2. * result_->getDepth( layer );
	const float vel = setup_.pdown_ ? ellayer.vel_ : ellayer.svel_;
	for ( int osidx=0; osidx<offsz; osidx++ )
	{
	    const double offset = result_->getOffset( osidx );
	    const double angle = depth ? atan( offset / depth ) : 0.;
	    const float rayparam = mCast(float,sin(angle) / vel );

	    if ( !compute(layer,osidx,rayparam) )
	    {
		errmsg_ = tr("Can not compute layer %1"
			     "\n most probably the velocity is not correct")
			.arg( layer );
		return false;
	    }
	}
    }

    return true;
}


bool VrmsRayTracer1D::compute( int layer, int offsetidx, float rayparam )
{
    const float tnmo = result_->getTnmo( layer );
    const float vrms = velmax_[layer];
    const float off = offsets_[offsetidx];
    float twt = tnmo;
    if ( vrms && !mIsUdf(tnmo) )
	twt = Math::Sqrt(off*off/(vrms*vrms) + tnmo*tnmo);

    result_->setTWT( layer, offsetidx, twt );

    return RayTracer1D::compute( layer, offsetidx, rayparam );
}
