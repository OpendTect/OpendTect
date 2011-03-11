/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 1999
-*/


#include "raytrace1d.h"

#include "arrayndimpl.h"
#include "genericnumer.h"
#include "idxable.h"
#include "iopar.h"
#include "mathfunc.h"
#include "sorting.h"
#include "zoeppritzcoeff.h"


bool RayTracer1D::Setup::usePar( const IOPar& par )
{
    return par.getYN( sKeyPWave(), pdown_, pup_ ) &&
	   par.get( sKeySRDepth(), sourcedepth_, receiverdepth_ );
}


void RayTracer1D::Setup::fillPar( IOPar& par ) const 
{
    par.setYN( sKeyPWave(), pdown_, pup_ );
    par.set( sKeySRDepth(), sourcedepth_, receiverdepth_ );
}


RayTracer1D::RayTracer1D( const RayTracer1D::Setup& s )
    : setup_( s )
    , receiverlayer_( 0 )
    , sourcelayer_( 0 )
    , sini_( 0 )
    , twt_(0)
    , reflectivity_( 0 )
{}					       	


RayTracer1D::~RayTracer1D()
{ delete sini_; delete twt_; delete reflectivity_; }


void RayTracer1D::setSetup( const RayTracer1D::Setup& s )
{ setup_ = s; }


void RayTracer1D::setModel( bool pwave, const AIModel& lys )
{
    if ( pwave )
	pmodel_ = lys;
    else 
	smodel_ = lys;
}


void RayTracer1D::setOffsets( const TypeSet<float>& offsets )
{ offsets_ = offsets; }


od_int64 RayTracer1D::nrIterations() const
{ return (pmodel_.size() ? pmodel_.size() : smodel_.size() )-1; }


int RayTracer1D::findLayer( const AIModel& model, float targetdepth )
{
    int res;
    for ( res=0; res<model.size(); res++ )
    {
	if ( targetdepth<model[res].depth_ )
	    break;
    }

    return res ? res-1 : 0;
}
	
#define mPvelToSvel 2.25
bool RayTracer1D::doPrepare( int nrthreads )
{
    //See if we can find zero-offset
    bool found = false;
    for ( int idx=0; idx<offsets_.size(); idx++ )
    {
	if ( mIsZero(offsets_[idx],1e-3) )
	{
	    found = true;
	    break;
	}
    }
    if ( !found )
	offsets_ += 0;

    const int psz = pmodel_.size();
    const int ssz = smodel_.size();

    if ( !ssz )
    {
	for ( int idx=0; idx<psz; idx++ )
	{
	    const AILayer& pl = pmodel_[idx];
	    smodel_ += AILayer( pl.depth_, pl.vel_/mPvelToSvel, pl.den_ );
	}
    }
    else if ( !psz )
    {
	for ( int idx=0; idx<psz; idx++ )
	{
	    const AILayer& sl = smodel_[idx];
	    pmodel_ += AILayer( sl.depth_, sl.vel_*mPvelToSvel , sl.den_ );
	}
    }

    if ( psz && ssz && psz!=ssz )
    {
	errmsg_ = "P and S model sizes must be identical";
	return false;
    }

    const AIModel& depthmodel = psz ? pmodel_ : smodel_;

    sourcelayer_ = findLayer( depthmodel, setup_.sourcedepth_ );
    receiverlayer_ = findLayer( depthmodel, setup_.receiverdepth_ );

    const int layersize = nrIterations();
    if ( sourcelayer_>=layersize || receiverlayer_>=layersize )
    {
	errmsg_ = "Source or receiver is below lowest layer";
	return false;
    }

    const int offsetsz = offsets_.size();
    TypeSet<int> offsetidx( offsetsz, 0 );
    for ( int idx=0; idx<offsetsz; idx++ )
	offsetidx[idx] = idx;

    sort_coupled( offsets_.arr(), offsetidx.arr(), offsetsz );
    offsetpermutation_.erase();
    for ( int idx=0; idx<offsetsz; idx++ )
	offsetpermutation_ += offsetidx.indexOf( idx );

    firstlayer_ = mMIN( receiverlayer_, sourcelayer_ );

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

    if ( !reflectivity_ )
	reflectivity_ = new Array2DImpl<float_complex>( layersize, offsetsz );
    else
	reflectivity_->setSize( layersize-1, offsetsz );

    reflectivity_->setAll( mUdf(float_complex) );

    return true;
}


bool RayTracer1D::doWork( od_int64 start, od_int64 stop, int nrthreads )
{
    const int offsz = offsets_.size();
    const AIModel& model = setup_.pdown_ ? pmodel_ : smodel_;

    for ( int layer=start; layer<=stop; layer++ )
    {
	if ( layer<firstlayer_ )
	    continue;

	const AILayer& ailayer = model[layer+1]; 
	const float raydepth = 2*( ailayer.depth_ - setup_.sourcedepth_ );

	float vrms2sum = 0; float zeroofftime = 0;
	for ( int idx=firstlayer_; idx<layer+1; idx++ )
	{
	    const float vel = model[idx].vel_;
	    const float thk = model[idx+1].depth_ - model[idx].depth_;
	    zeroofftime += 2 * thk / vel;
	    vrms2sum += vel * vel;
	}
	const float vrms  = sqrt( vrms2sum / (layer+1-firstlayer_) );

	for ( int osidx=offsz-1; osidx>=0; osidx-- )
	{
	    const bool iszerooff = mIsZero(offsets_[osidx],1e-3);
	    const float offsetdist = offsets_[osidx];
	    const float angle = raydepth ? atan( offsetdist / raydepth ) : 0;
	    const float raydist = raydepth / cos( angle );
	    const float toffset = iszerooff ? zeroofftime : raydist / vrms ;
	    twt_->set( layer, osidx, toffset );
	    sini_->set( layer, osidx, sin( angle ) );

	    const float rayparam = sin(angle) / vrms;

	    if ( !compute( layer+1, osidx, rayparam ) )
	    { 
		BufferString msg( "Can not compute layer " );
		msg += toString( layer+1 ); 
		errmsg_ = msg; return false; 
	    }
	}
    }
    return true;
}


#define mRetFalseIfZero(vel) \
    if ( mIsZero(vel,mDefEps) ) return false;

bool RayTracer1D::compute( int layer, int offsetidx, float rayparam )
{
    const AIModel& dlayers = setup_.pdown_ ? pmodel_ : smodel_;
    const AIModel& ulayers = setup_.pup_ ? pmodel_ : smodel_;

    mAllocVarLenArr( ZoeppritzCoeff, coefs, layer );

    for ( int idx=firstlayer_; idx<layer; idx++ )
	coefs[idx].setInterface( rayparam, idx, pmodel_[idx], pmodel_[idx+1],
				smodel_[idx], smodel_[idx+1] );

    int lidx = sourcelayer_;
    float vel = dlayers[lidx].vel_;
    mRetFalseIfZero(vel);

    float sini = rayparam * vel;
    float dist = (dlayers[lidx].depth_-setup_.sourcedepth_)
	       / sqrt(1.0-sini*sini);
    float_complex reflectivity =
	coefs[lidx].getCoeff( true, lidx!=layer-1, setup_.pdown_,
			    lidx==layer-1? setup_.pup_ : setup_.pdown_ );

    if ( !layer )
	reflectivity = float_complex( 0, 0 );

    lidx++;

    while ( lidx < layer )
    {
	vel = dlayers[lidx].vel_;
	mRetFalseIfZero(vel);
	sini = rayparam * vel;
	dist = dlayers[lidx].depth_ / cos( asin(sini) );
	reflectivity *= coefs[lidx].getCoeff( true, lidx!=layer-1,
	setup_.pdown_, lidx==layer-1? setup_.pup_ : setup_.pdown_ );
	lidx++;
    }

    for ( lidx=layer-1; lidx>=receiverlayer_; lidx--)
    {
	vel = ulayers[lidx].vel_;
	mRetFalseIfZero(vel);
	sini = rayparam * vel;
	dist =  (ulayers[lidx].depth_-(
		    lidx==receiverlayer_ ? setup_.receiverdepth_ : 0) ) 
		    / cos( asin(sini) );

	if ( lidx>receiverlayer_ )
	{
	    reflectivity *=
		coefs[lidx-1].getCoeff(false,false,setup_.pup_,setup_.pup_);
	}
    }
    reflectivity_->set( layer-1, offsetidx, reflectivity );
    return true;
}


float RayTracer1D::getSinAngle( int layer, int offset ) const
{
    const int offsetidx = offsetpermutation_[offset];

    if ( !sini_ || layer<0 || layer>=sini_->info().getSize(0) || 
	 offsetidx<0 || offsetidx>=sini_->info().getSize(1) )
	return mUdf(float);
    
    return sini_->get( layer, offsetidx );
}


float RayTracer1D::getTWT( int layer, int offsetidx ) const
{
    if ( !twt_ || layer<0 || layer>=twt_->info().getSize(0) || 
	 offsetidx<0 || offsetidx>=twt_->info().getSize(1) )
	return mUdf(float);

    return twt_->get( layer, offsetidx );
}


bool RayTracer1D::getReflectivity( int offset, ReflectivityModel& model ) const
{
    const int offsetidx = offsetpermutation_[offset];
    if ( offsetidx<0 || offsetidx>=reflectivity_->info().getSize(1) )
	return false;

    const int nrlayers = reflectivity_->info().getSize(0);

    model.erase();
    model.setCapacity( nrlayers );
    ReflectivitySpike spike;

    const AIModel& layers = pmodel_.size() ? pmodel_ : smodel_;
    for ( int idx=0; idx<nrlayers; idx++ )
    {
	spike.reflectivity_ = reflectivity_->get( idx, offsetidx );
	spike.depth_ = layers[idx+1].depth_;
	spike.time_ = twt_->get( idx, offsetidx );
	spike.correctedtime_ = twt_->get( idx, 0 );
	model += spike;
    }
    return true;
}


//TODO move this to a d2t model class
float RayTracer1D::convertTo( float z, int offset, bool targetistime ) const
{
    const AIModel& layers = pmodel_.size() ? pmodel_ : smodel_;
    const int sz = nrIterations();
    if ( sz == 0 ) return 0;
    if ( sz == 1 )
	return targetistime ? twt_->get(0, offset) : layers[0].depth_;

    TypeSet<float> zvals;
    for ( int layer=0; layer<sz; layer++ )
	zvals += targetistime ? layers[layer].depth_ : getTWT(layer,offset);

    int idx;
    IdxAble::findFPPos( zvals, sz, z, -1, idx );
    if ( !zvals.validIdx( idx ) ) 
	return targetistime ? z / layers[0].vel_ : z*layers[0].vel_/2;
    const float remainder = idx > 0 ? z - zvals[idx] : 0;

    if ( targetistime )
    {
	const float time =  idx > 0 ? getTWT(idx-1,offset) : 0; 
	return time + remainder / layers[idx].vel_;
    }
    else
	return layers[idx+1].depth_ + remainder * layers[idx].vel_/2;
}
