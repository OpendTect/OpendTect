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
    if ( sourcelayer_>=layersize || receiverlayer_>=layersize )
    {
	errmsg_ = "Sourece or receiver is below lowest layer";
	return false;
    }

    float maxvel = 0;
    velmax_.erase();
    for ( int layer=0; layer<layersize; layer++ )
    {
	if ( !layer )
	{
	    velmax_ += 0;
	    continue;
	}

	if ( setup_.pdown_ || setup_.pup_ )
	{
	    if ( pmodel_[layer-1].vel_ > maxvel )
		maxvel = pmodel_[layer-1].vel_;
	}
	else if ( smodel_[layer-1].vel_ > maxvel )
	    maxvel = smodel_[layer-1].vel_;

	velmax_ += maxvel;
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

    return true;
}

class OffsetFromRayParam : public FloatMathFunction
{
public:
    			OffsetFromRayParam( const RayTracer1D& rt, int layer )
			    : raytracer_( rt ), layer_( layer ) {}  
    float 		getValue(float rayparam) const
			{ return raytracer_.getOffset( layer_, rayparam ); }

protected:

    const RayTracer1D&	raytracer_;
    int			layer_;
};



bool RayTracer1D::doWork( od_int64 start, od_int64 stop, int nrthreads )
{
    const int offsz = offsets_.size();
    const int firstreflection = 1 + mMAX(sourcelayer_,receiverlayer_);

    int zerooffsetidx = -1;
    for ( int osidx=offsz-1; osidx>=0; osidx-- )
    {
	if ( mIsZero(offsets_[osidx],1e-3) )
	{
	    zerooffsetidx = osidx;
	    break;
	}
    }

    for ( int layer=start; layer<=stop; layer++ )
    {
	const int usedlayer = mMAX( firstreflection, layer );
	OffsetFromRayParam function( *this, usedlayer );
	float maxrayparam = 0.9999999/velmax_[usedlayer];
	float prevoffset = offsets_[offsz-1];
	for ( int osidx=offsz-1; osidx>=0; osidx-- )
	{
	    float rayparam = 0;
	    const float offset = offsets_[osidx];
	    if ( osidx!=zerooffsetidx )
	    {
		rayparam = maxrayparam * offset/prevoffset * 0.9;
		if ( !findValue(function,0,maxrayparam,rayparam,offset,1e-10) )
		    continue;
	    }

	    maxrayparam = rayparam;
	    prevoffset = offset;

	    if ( !compute(layer, osidx, rayparam) )
		return false;
	}
    }

    return true;
}


#define mCalOffSetFactor( layers, prevdepth, dotwolayers ) \
	const AILayer& curlayer = layers[idx]; \
	const float sini = rayparam * curlayer.vel_;\
	const float depth = curlayer.depth_;  \
	const float thickness = depth - prevdepth; \
	prevdepth = depth; \
	result += dotwolayers ? 2 * thickness * sini / sqrt(1.0-sini*sini) : \
				thickness * sini / sqrt(1.0-sini*sini)


float RayTracer1D::getOffset( int layer, float rayparam ) const
{
    const TypeSet<AILayer>& dlayers = setup_.pdown_ ? pmodel_ : smodel_;
    const int firstlayer = mMIN( receiverlayer_, sourcelayer_ );
    float result = 0;
    int idx = firstlayer;
    float prevsourcedepth = 0;
    float prevreceiverdepth = 0;

    if ( setup_.pdown_==setup_.pup_ )
    {
	const int firstcommonlayer = mMAX( receiverlayer_, sourcelayer_ ) + 1;
	while ( idx<firstcommonlayer )
	{
	    if ( idx >= sourcelayer_ )
	    {
		mCalOffSetFactor( dlayers, prevsourcedepth, false );
	    }
	    
	    if ( idx >= receiverlayer_ )
	    {
		mCalOffSetFactor( dlayers, prevreceiverdepth, false );
	    }
	    idx++;
	}
	
	prevsourcedepth = dlayers[idx-1].depth_;
	while ( idx<layer )
	{
	    mCalOffSetFactor( dlayers, prevsourcedepth, true );
	    idx++;
	}
    }
    else
    {
	const TypeSet<AILayer>& ulayers = setup_.pup_ ? pmodel_ : smodel_; 
	while ( idx<layer )
	{
	    if ( idx >= sourcelayer_ )
	    {
		mCalOffSetFactor( dlayers, prevsourcedepth, false );
	    }
	    
	    if ( idx >= receiverlayer_ )
	    {
		mCalOffSetFactor( ulayers, prevreceiverdepth, false );
	    }

	    idx++;
	}
    } 

    return result;
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
