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
#include "velocitycalc.h"
#include "zoeppritzcoeff.h"

mImplFactory(RayTracer1D,RayTracer1D::factory)


float RayTracer1D::cDefaultBlockRatio()
{
    return 0.01;
}


StepInterval<float> RayTracer1D::sDefOffsetRange()
{
    return SI().xyInFeet() ? StepInterval<float>( 0.f, 18000.f, 300.f )
			   : StepInterval<float>( 0.f, 6000.f, 100.f );
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
    : sini_( 0 )
    , twt_(0)
    , zerooffstwt_(0)
    , reflectivity_( 0 )
{}


RayTracer1D::~RayTracer1D()
{ delete sini_; delete twt_; delete zerooffstwt_; delete reflectivity_; }


RayTracer1D* RayTracer1D::createInstance( const IOPar& par, uiString& errm )
{
    BufferString typekey;
    par.get( sKey::Type(), typekey );

    RayTracer1D* raytracer = factory().create( typekey );
    if ( !raytracer && !factory().isEmpty() )
	raytracer = factory().create( factory().getNames().get(0) );
    if ( !raytracer )
    {
	errm = tr("Raytracer not found. Perhaps all plugins are not loaded");
	return 0;
    }

    if ( !raytracer->usePar(par) )
    {
	errm = raytracer->errMsg();
	delete raytracer;
	return 0;
    }

    return raytracer;
}


bool RayTracer1D::usePar( const IOPar& par )
{
    TypeSet<float> offsets;
    par.get( sKeyOffset(), offsets );
    if ( offsets.isEmpty() )
	offsets += 0;

    bool offsetisinfeet = false;
    if ( par.getYN(sKeyOffsetInFeet(),offsetisinfeet) && offsetisinfeet )
    {
	for ( int idx=0; idx<offsets.size(); idx++ )
	    offsets[idx] = offsets[idx] * mFromFeetFactorF;
    }

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


bool RayTracer1D::isPSWithoutZeroOffset() const
{
    return offsets_.size()>1 && !mIsZero(offsets_[0],mDefEps);
}


void RayTracer1D::setOffsets( const TypeSet<float>& offsets )
{
    offsets_ = offsets;
    sort( offsets_ );
    if ( SI().zInFeet() )
    {
	for ( int idx=0; idx<offsets_.size(); idx++ )
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
	errmsg_.append( tr("Cannot do raytracing." ), true );
	return false;
    }

    //Zero-offset: Vs is not required, density not either if !doreflectivity_
    const bool zerooffsetonly =
	offsets_.size()==1 && mIsZero(offsets_[0],mDefEps);

    model_ = lys;
    int firsterror = -1;
    model_.checkAndClean( firsterror, setup().doreflectivity_, !zerooffsetonly);

    if ( model_.isEmpty() )
	errmsg_ = tr( "Model is empty" );
    else if ( firsterror != -1 )
	errmsg_ = tr( "Model has invalid values on layer: %1" )
		      .arg( firsterror+1 );

    return !model_.isEmpty();
}


od_int64 RayTracer1D::nrIterations() const
{ return model_.size(); }


bool RayTracer1D::doPrepare( int nrthreads )
{
    const int layersize = mCast( int, nrIterations() );
    depths_.setSize( layersize, 0.f );
    velmax_.setSize( layersize, 0.f );
    const bool zinfeet = SI().zInFeet();
    for ( int idx=0; idx<layersize; idx++ )
    {
	float thickness = model_[idx].thickness_;
	if ( zinfeet ) thickness *= mToFeetFactorF;
	depths_[idx] = idx ? depths_[idx-1] + thickness : thickness;
	velmax_[idx] = model_[idx].vel_;
    }

    const int offsetsz = offsets_.size();
    if ( !sini_ )
	sini_ = new Array2DImpl<float>( layersize, offsetsz );
    else
	sini_->setSize( layersize, offsetsz );
    sini_->setAll( 0 );

    if ( !twt_ )
	twt_ = new Array2DImpl<float>( layersize, offsetsz );
    else
	twt_->setSize( layersize, offsetsz );
    twt_->setAll( mUdf(float) );
    if ( isPSWithoutZeroOffset() )
    {
	zerooffstwt_ = new Array1DImpl<float>( layersize );
	zerooffstwt_->setAll( mUdf(float) );
    }
    else
    {
	Array1DSlice<float>* zerooffstwtslice = new Array1DSlice<float>( *twt_);
	zerooffstwtslice->setDimMap( 0, 0 );
	zerooffstwtslice->setPos( 1, 0 );
	zerooffstwtslice->init();
	zerooffstwt_ = zerooffstwtslice;
    }

    if ( setup().doreflectivity_ )
    {
	if ( reflectivity_ && reflectivity_->isEmpty() )
	    { delete reflectivity_; reflectivity_ = 0; }

	const int dim0sz = layersize-1;
	if ( dim0sz < 1 || offsetsz < 1 )
	    { delete reflectivity_; reflectivity_ = 0; }
	else
	{
	    if ( !reflectivity_ )
		reflectivity_ =new Array2DImpl<float_complex>(dim0sz,offsetsz);
	    else
		reflectivity_->setSize( dim0sz, offsetsz );

	    reflectivity_->setAll( mUdf( float_complex ) );
	}
    }

    setZeroOffsetTWT();
    return true;
}


void RayTracer1D::setZeroOffsetTWT()
{
    const int layersize = mCast( int, nrIterations() );
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
	zerooffstwt_->set( lidx, twt );
    }
}


bool RayTracer1D::compute( int layer, int offsetidx, float rayparam )
{
    const ElasticLayer& ellayer = model_[layer];
    const float downvel = setup().pdown_ ? ellayer.vel_ : ellayer.svel_;

    const float sini = downvel * rayparam;
    sini_->set( layer, offsetidx, sini );

    if ( !reflectivity_ || layer>=model_.size()-1 )
	return true;

    const float off = offsets_[offsetidx];
    float_complex reflectivity = 0;
    const int nrinterfaces = layer+1;

    if ( !mIsZero(off,mDefEps) )
    {
	if ( rayparam*model_[layer].vel_ > 1 ||   // critical angle reached
	     rayparam*model_[layer+1].vel_ > 1 )  // no reflection
	{
	    reflectivity_->set( layer, offsetidx, reflectivity );
	    return true;
	}

	mAllocLargeVarLenArr( ZoeppritzCoeff, coefs, nrinterfaces );
        for ( int iidx=0; iidx<nrinterfaces; iidx++ )
	    coefs[iidx].setInterface( rayparam, model_[iidx], model_[iidx+1] );

	reflectivity = coefs[0].getCoeff( true, layer!=0, setup().pdown_,
				     layer==0 ? setup().pup_ : setup().pdown_ );

	if ( layer == 0 )
	{
	    reflectivity_->set( layer, offsetidx, reflectivity );
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
	const ElasticLayer& ail0 = model_[ layer ];
	const ElasticLayer& ail1 = model_[ layer+1 ];
	const float ai0 = ail0.vel_ * ail0.den_;
	const float ai1 = ail1.vel_ * ail1.den_;
	const float real =
	   mIsZero(ai1,mDefEpsF) && mIsZero(ai0,mDefEpsF) ? mUdf(float)
						          : (ai1-ai0)/(ai1+ai0);
	reflectivity = float_complex( real, 0 );
    }

    reflectivity_->set( layer, offsetidx, reflectivity );

    return true;
}


float RayTracer1D::getSinAngle( int layer, int offset ) const
{
    if ( !offsetpermutation_.validIdx( offset ) )
	return mUdf(float);

    const int offsetidx = offsetpermutation_[offset];

    if ( !sini_ || layer<0 || layer>=sini_->info().getSize(0) ||
	 offsetidx<0 || offsetidx>=sini_->info().getSize(1) )
	return mUdf(float);

    return sini_->get( layer, offsetidx );
}



bool RayTracer1D::getReflectivity( int offset, ReflectivityModel& model ) const
{
    if ( !reflectivity_ || !offsetpermutation_.validIdx( offset ) )
	return false;

    const int offsetidx = offsetpermutation_[offset];

    if ( offsetidx<0 || offsetidx>=reflectivity_->info().getSize(1) )
	return false;

    const int nrinterfaces = reflectivity_->info().getSize(0);

    model.erase();
    model.setCapacity( nrinterfaces, false );
    ReflectivitySpike spike;

    for ( int iidx=0; iidx<nrinterfaces; iidx++ )
    {
	spike.reflectivity_ = reflectivity_->get( iidx, offsetidx );
	spike.depth_ = depths_[iidx];
	spike.time_ = twt_->get( iidx, offsetidx );
	spike.correctedtime_ = zerooffstwt_->get( iidx );
	if ( !spike.isDefined()	)
	    continue;

	model += spike;
    }
    return true;
}


bool RayTracer1D::getZeroOffsTDModel( TimeDepthModel& d2tm ) const
{
    return getTDM( *zerooffstwt_, d2tm );
}


bool RayTracer1D::getTDModel( int offset, TimeDepthModel& d2tm ) const
{
    if ( !offsetpermutation_.validIdx( offset ) )
	return false;

    const int offsetidx = offsetpermutation_[offset];

    if ( !twt_ || offsetidx<0 || offsetidx>=twt_->info().getSize(1) )
	return false;
    Array1DSlice<float> offstwt( *twt_ );
    offstwt.setDimMap( 0, 0 );
    offstwt.setPos( 1, offsetidx );
    if ( !offstwt.init() )
	return false;
    return getTDM( offstwt, d2tm );
}


bool RayTracer1D::getTDM( const Array1D<float>& twt,
			  TimeDepthModel& d2tm ) const
{
    const int layersize = mCast( int, nrIterations() );

    TypeSet<float> times, depths;
    depths += 0;
    times += 0;
    for ( int lidx=0; lidx<layersize; lidx++ )
    {
	float time = twt.get( lidx );
	if ( mIsUdf( time ) ) time = times[times.size()-1];
	if ( time < times[times.size()-1] )
	    continue;

	depths += depths_[lidx];
	times += time;
    }

    return d2tm.setModel( depths.arr(), times.arr(), times.size() );
}


float RayTracer1D::getDepth( int layer ) const
{
    return depths_[layer];
}


float RayTracer1D::getTime( int layer, int offset ) const
{
    return twt_->get( layer, offset );
}


bool VrmsRayTracer1D::doWork( od_int64 start, od_int64 stop, int nrthreads )
{
    const int offsz = offsets_.size();

    for ( int layer=mCast(int,start); layer<=stop; layer++ )
    {
	addToNrDone( 1 );
	const ElasticLayer& ellayer = model_[layer];
	const float depth = 2*depths_[layer];
	const float vel = setup_.pdown_ ? ellayer.vel_ : ellayer.svel_;
	for ( int osidx=0; osidx<offsz; osidx++ )
	{
	    const float offset = offsets_[osidx];
	    const float angle = depth ? atan( offset / depth ) : 0;
	    const float rayparam = sin(angle) / vel;

	    if ( !compute( layer, osidx, rayparam ) )
	    {
		errmsg_ = tr( "Can not compute layer %1"
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
    const float tnmo = zerooffstwt_->get( layer );
    const float vrms = velmax_[layer];
    const float off = offsets_[offsetidx];
    float twt = tnmo;
    if ( vrms && !mIsUdf(tnmo) )
	twt = Math::Sqrt(off*off/(vrms*vrms) + tnmo*tnmo);

    twt_->set( layer, offsetidx, twt );

    return RayTracer1D::compute( layer, offsetidx, rayparam );
}
