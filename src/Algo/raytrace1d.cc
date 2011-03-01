/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 1999
-*/


#include "raytrace1d.h"

#include "arrayndimpl.h"
#include "iopar.h"
#include "genericnumer.h"
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


void RayTracer1D::setModel( bool pwave, const TypeSet<AILayer>& lys )
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


int RayTracer1D::findLayer( const TypeSet<AILayer>& model, float targetdepth )
{
    int res;
    for ( res=0; res<model.size(); res++ )
    {
	if ( targetdepth<model[res].depth_ )
	    break;
    }

    return res ? res-1 : 0;
}
	

bool RayTracer1D::doPrepare( int nrthreads )
{
    const int psz = pmodel_.size();
    const int ssz = smodel_.size();
    if ( psz && ssz && psz!=ssz )
    {
	pErrMsg("P and S model sizes must be identical" );
	return false;
    }

    const TypeSet<AILayer>& depthmodel = psz ? pmodel_ : smodel_;

    sourcelayer_ = findLayer( depthmodel, setup_.sourcedepth_ );
    receiverlayer_ = findLayer( depthmodel, setup_.receiverdepth_ );

    const int layersize = nrIterations()+1;
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
	reflectivity_->setSize( layersize, offsetsz );

    if ( !reflectivity_ || !reflectivity_->isOK() )
	return false;

    reflectivity_->setAll( mUdf(float_complex) );

    return true;
}


#define mRetFalseIfZero(vel) \
    if ( mIsZero(vel,mDefEps) ) return false;

bool RayTracer1D::compute( int layer, int offsetidx, float rayparam )
{
    const TypeSet<AILayer>& dlayers = setup_.pdown_ ? pmodel_ : smodel_;
    const TypeSet<AILayer>& ulayers = setup_.pup_ ? pmodel_ : smodel_;

    mAllocVarLenArr( ZoeppritzCoeff, coefs, layer );
    const int toplayer = mMIN(sourcelayer_,receiverlayer_ );

    for ( int idx=toplayer; idx<layer; idx++ )
	coefs[idx].setInterface( rayparam, idx, pmodel_[idx], pmodel_[idx+1],
				smodel_[idx], smodel_[idx+1] );

    int lidx = sourcelayer_;
    float vel = dlayers[lidx].vel_;
    mRetFalseIfZero(vel);

    float sini = rayparam * vel;
    float dist = (dlayers[lidx].depth_-setup_.sourcedepth_)/sqrt(1.0-sini*sini);
    float_complex reflectivity =
	coefs[lidx].getCoeff( true, lidx!=layer-1, setup_.pdown_,
			    lidx==layer-1? setup_.pup_ : setup_.pdown_ );

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
	dist =  (ulayers[lidx].depth_-(lidx==receiverlayer_ ?
	setup_.receiverdepth_ : 0)) / cos( asin(sini) );

	if ( lidx>receiverlayer_ )
	{
	    reflectivity *=
	    coefs[lidx-1].getCoeff(false,false,setup_.pup_,setup_.pup_);
	}
    }

    reflectivity_->set( layer, offsetidx, reflectivity );
    return true;
}



bool RayTracer1D::doWork( od_int64 start, od_int64 stop, int nrthreads )
{
    const int offsz = offsets_.size();
    const int firstreflection = 1 + mMAX(sourcelayer_,receiverlayer_);

    //TODO different down/upgoing model
    for ( int layer=start; layer<=stop; layer++ )
    {
	const TypeSet<AILayer>& model = setup_.pdown_ ? pmodel_ : smodel_;
	const AILayer& ailayer = pmodel_[layer]; 
	const float thickness = ailayer.depth_ - setup_.sourcedepth_;

	float vrms2sum = 0;
	for ( int idx=start; idx<layer+1; idx++ )
	{
	    const float vel = model[idx].vel_;
	    vrms2sum += vel*vel;
	}
	const float vrms  = sqrt( vrms2sum / (layer+1) );

	for ( int osidx=offsz-1; osidx>=0; osidx-- )
	{
	    const float offsetdist = offsets_[osidx];
	    const float angle = atan( offsetdist /thickness );
	    const float raydist = thickness / cos( angle );
	    const float toffset = raydist / vrms;
	    twt_->set( layer, osidx, toffset );
	    sini_->set( layer, osidx, sin( angle ) );
	    const float rayparam = sin(angle) / vrms;
	    if ( !compute(layer, osidx, rayparam) )
		return false;
	}
    }
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


bool RayTracer1D::getReflectivity(int offset,
	                                         ReflectivityModel& model) const
{
    const int offsetidx = offsetpermutation_[offset];
    if ( offsetidx<0 || offsetidx>=reflectivity_->info().getSize(1) )
	return false;

    const int nrlayers = reflectivity_->info().getSize(0);

    model.erase();

    model.setCapacity( nrlayers );
    ReflectivitySpike spike;

    const TypeSet<AILayer>& layers = pmodel_.size() ? pmodel_ : smodel_;

    for ( int idx=0; idx<nrlayers; idx++ )
    {
	spike.reflectivity_ = reflectivity_->get( idx, offsetidx );
	spike.depth_ = layers[idx+1].depth_;
	spike.time_ = twt_->get( idx, offsetidx );
	spike.correctedtime_ = twt_->get( idx, 0 );
    }

    return true;
}
