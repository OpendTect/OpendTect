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
{}					       	


RayTracer1D::~RayTracer1D()
{ delete sini_; }


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

    const int layersize = nrIterations();
    firstlayer_ = mMIN( receiverlayer_, sourcelayer_ );
    if ( sourcelayer_>=layersize || receiverlayer_>=layersize )
    {
	errmsg_ = "Source or receiver is below lowest layer";
	return false;
    }

    const int offsetsz = offsets_.isEmpty();
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

    return true;
}


bool RayTracer1D::doWork( od_int64 start, od_int64 stop, int nrthreads )
{
    //TODO simpel ray model
    return true;
}


#define mCalOffSetFactor() \
        const float sini = rayparam * velvals_[idx];\
        const float thickness = thicknesses_[idx]; \
        result += thicknesses * sini / sqrt(1.0-sini*sini)

float RayTracer1D::getOffset( int layer,float rayparam ) const
{
    //TODO
    return -1;
}


float RayTracer1D::getSinAngle( int layer, int offset ) const
{
    const int offsetidx = offsetpermutation_[offset];

    if ( !sini_ || layer<0 || layer>=sini_->info().getSize(0) || 
	 offsetidx<0 || offsetidx>=sini_->info().getSize(1) )
	return mUdf(float);
    
    return sini_->get( layer, offsetidx );
}


bool RayTracer1D::compute( int layer, int offsetidx, float rayparam )
{
    const TypeSet<AILayer>& dlayers = setup_.pdown_ ? pmodel_ : smodel_;
    const float sinival = layer ? dlayers[layer-1].vel_ * rayparam : 0;
    sini_->set( layer, offsetidx, sinival );

    return true;
}


