/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 1999
-*/


#include "raytrace1d.h"

#include "arrayndimpl.h"
#include "genericnumer.h"
#include "mathfunc.h"


mImplFactory( RayTracer1D, RayTracer1D::factory );


class InterfaceCoeff
{
public:		
    			InterfaceCoeff();

    void 		setInterface(float p,int layer,
	    			     const RayTracer1D::Layer& p_layer1, 
				     const RayTracer1D::Layer& p_layer2, 
				     const RayTracer1D::Layer& slayer1, 
				     const RayTracer1D::Layer& slayer2);	
    float_complex 	getCoeff(bool din,bool dout,bool pin,bool pout) const;

private:

    float_complex	pdn_pdn_;
    float_complex	pdn_sdn_;
    float_complex	pdn_pup_;
    float_complex	pdn_sup_;

    float_complex	sdn_pdn_;
    float_complex	sdn_sdn_;
    float_complex	sdn_pup_;
    float_complex	sdn_sup_;

    float_complex	pup_pdn_;
    float_complex	pup_sdn_;
    float_complex	pup_pup_;
    float_complex	pup_sup_;

    float_complex	sup_pdn_;
    float_complex	sup_sdn_;
    float_complex	sup_pup_;
    float_complex	sup_sup_;
};


RayTracer1D::RayTracer1D()
    : ownspmodel_( false )
    , ownssmodel_( false )
    , downisp_( true )
    , upisp_( true )
    , receiverlayer_( 0 )
    , sourcelayer_( 0 )
    , sourcedepth_( 0 )
    , relsourcedepth_( 0 )
    , receiverdepth_( 0 )
    , relreceiverdepth_( 0 )			
{}					       	


RayTracer1D::~RayTracer1D()
{
    if ( ownspmodel_ )
	deepErase( pmodel_ );

    if ( ownssmodel_ )
	deepErase( smodel_ );
}


void RayTracer1D::setModel( bool pwave, const ObjectSet<Layer>& lys,
			    OD::PtrPolicy policy )
{
    if ( pwave )
    {
	if ( ownspmodel_ )
    	    deepErase( pmodel_ );

	ownspmodel_ = policy==OD::CopyPtr;
	if ( ownspmodel_ )
	{
	    for ( int idx=0; idx<lys.size(); idx++ )
		pmodel_ += new Layer( *lys[idx] );
	}
	else
	    pmodel_ = lys;
    }
    else 
    {
	if ( ownssmodel_ )
    	    deepErase( smodel_ );

	ownssmodel_ = policy==OD::CopyPtr;
	if ( ownssmodel_ )
	{
	    for ( int idx=0; idx<lys.size(); idx++ )
		smodel_ += new Layer( *lys[idx] );
	}
	else
	    smodel_ = lys;
    }
}


void RayTracer1D::setOffsets( const TypeSet<float>& offsets )
{
    offsets_.erase();
    offsets_ = offsets;
}


od_int64 RayTracer1D::nrIterations() const
{
    const int psz = pmodel_.size();
    return psz ? psz : smodel_.size();
}


bool RayTracer1D::doPrepare( int nrthreads )
{
    const int layersize = downisp_ ? pmodel_.size() : smodel_.size();
    float maxvel = 0;
    velmax_.erase();
    for ( int layer=0; layer<layersize; layer++ )
    {
	if ( !layer )
	{
	    velmax_ += 0;
	    continue;
	}

	if ( downisp_ || upisp_ )
	{
	    if ( pmodel_[layer-1]->Vint_ > maxvel )
		maxvel = pmodel_[layer-1]->Vint_;
	}
	else if ( smodel_[layer-1]->Vint_ > maxvel )
	    maxvel = smodel_[layer-1]->Vint_;

	velmax_ += maxvel;
    }

    return true;
}


bool RayTracer1D::doWork( od_int64 start, od_int64 stop, int nrthreads )
{
    const int offsz = offsets_.size();
    const int firstreflection = 1 +
	(sourcelayer_ > receiverlayer_ ? sourcelayer_ : receiverlayer_);
    for ( int layer=start; layer<=stop; layer++ )
    {
	const int usedlayer = firstreflection > layer ? firstreflection : layer;
	float rayparam = 0;
	for ( int osidx=0; osidx<offsz; osidx++ )
	{
	    rayparam = findRayParam( usedlayer, offsets_[osidx], rayparam );
	    if ( mIsUdf(rayparam) ) 
		continue;

	    if ( !compute(layer, osidx, rayparam) )
		return false;
	}
    }

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


float RayTracer1D::findRayParam( int layer, float offset, float seed ) const
{
    OffsetFromRayParam function( *this, layer );

    if ( !findValue(function,0,0.9999999/velmax_[layer],seed,offset,1e-10) )
	return mUdf(float);

    return seed;
}


float RayTracer1D::getOffset( int layer, float rayparam) const
{
    const ObjectSet<Layer>& dlayers = downisp_ ? pmodel_ : smodel_;
    const ObjectSet<Layer>& ulayers = upisp_ ? pmodel_ : smodel_; 
    
    float xtot = 0;
    int idx = sourcelayer_ < receiverlayer_ ? sourcelayer_ : receiverlayer_;
    while ( idx<layer )
    {
	if ( idx >= sourcelayer_ )
	{
	    const float sinidn = rayparam * dlayers[idx]->Vint_;
	    const float factor = sinidn/sqrt(1.0-sinidn*sinidn);
	    const float rd = idx==sourcelayer_ ? relsourcedepth_ : 0;
	    xtot += ( dlayers[idx]->d0_ - rd ) * factor;
	}
	
	if ( idx >= receiverlayer_ )
	{
	    const float siniup = rayparam * ulayers[idx]->Vint_;
	    const float factor = siniup/sqrt(1-siniup*siniup);
	    const float rd = idx==receiverlayer_ ? relreceiverdepth_ : 0;
	    xtot += ( ulayers[idx]->d0_ - rd ) * factor; 
	}

	idx++;
    }
    
    return xtot;
}


RayTracer1D::Layer::Layer()
{
    d0_ = 0;
    Vint_ = 0; 
    density_ = 0;
}


RayTracer1D::Layer::Layer(const RayTracer1D::Layer& l )
{
    d0_ = l.d0_;  
    Vint_ = l.Vint_;
    density_ = l.density_; 
}


RayTracer1D::Layer& RayTracer1D::Layer::operator=(const RayTracer1D::Layer& l )
{
    d0_ = l.d0_;  
    Vint_ = l.Vint_;
    density_ = l.density_; 
    
    return *this;
}


InterfaceCoeff::InterfaceCoeff()
    : pdn_pdn_( 0, 0 )
    , pdn_sdn_( 0, 0 )
    , pdn_pup_( 0, 0 )
    , pdn_sup_( 0, 0 )
    , sdn_pdn_( 0, 0 )
    , sdn_sdn_( 0, 0 )
    , sdn_pup_( 0, 0 )
    , sdn_sup_( 0, 0 )
    , pup_pdn_( 0, 0 )
    , pup_sdn_( 0, 0 )
    , pup_pup_( 0, 0 )
    , pup_sup_( 0, 0 )
    , sup_pdn_( 0, 0 )
    , sup_sdn_( 0, 0 )
    , sup_pup_( 0, 0 )
    , sup_sup_( 0, 0 )
{}


void InterfaceCoeff::setInterface( float p, int layer,
 	const RayTracer1D::Layer& pl1, const RayTracer1D::Layer& pl2, 
	const RayTracer1D::Layer& sl1, const RayTracer1D::Layer& sl2 )	
{
    float p2 = p * p; 
    float pvel1 = pl1.Vint_;
    float pvel2 = pl2.Vint_;
    float svel1 = sl1.Vint_;
    float svel2 = sl2.Vint_;
    
    bool waterabove = mIsZero(svel1,mDefEps);	//Detect water
    bool waterbelow = mIsZero(svel2,mDefEps);

    if ( waterabove ) svel1 = 0.1;	// Set small values to make eqns work
    if ( waterbelow ) svel2 = 0.1;

    float l1s2 = svel1 * svel1;
    float l2s2 = svel2 * svel2;
    float l1p2 = pvel1 * pvel1;
    float l2p2 = pvel2 * pvel2;

    float_complex a = pl2.density_ * (1 -  2 * l2s2 * p2) -
	      pl1.density_ * (1 -  2 * l1s2 * p2);	
    float_complex b = pl2.density_ * (1 -  2 * l2s2 * p2) +
	      pl1.density_ * 2 * l1s2 * p2;	
    float_complex c = pl1.density_ * (1 -  2 * l1s2 * p2) +
	      pl2.density_ * 2 * l2s2 * p2;	
    float_complex d = 2 * (pl2.density_ * l2s2 - pl1.density_ * l1s2);

    float_complex pzi1 = sqrt( float_complex( 1/l1p2 - p2, 0) );
    float_complex pzi2 = sqrt( float_complex( 1/l2p2 - p2, 0) );

    float_complex pzj1 = sqrt( float_complex( 1/l1s2 - p2, 0) );
    float_complex pzj2 = sqrt( float_complex( 1/l2s2 - p2, 0) );

    float_complex ee = b * pzi1 + c * pzi2;
    float_complex ff = b * pzj1 + c * pzj2;
    float_complex gg = a - d * pzi1 * pzj2;
    float_complex hh = a - d * pzi2 * pzj1;
    float_complex dd = ee*ff + gg*hh * p2;

    const float f2 = (float)2;

    pdn_pup_ = ( (b*pzi1 - c*pzi2) * ff - 
				(a + d * pzi1 * pzj2) * hh * p2) / dd;
    pdn_pdn_ = 2 * pl1.density_ * pzi1 * ff * pvel1/(pvel2*dd);

    pdn_sup_ = -f2 * pzi1 * ( a*b + c*d * pzi2*pzj2 ) 
				* p * pvel1 /(svel2 *dd);
    pdn_sdn_ = 2 * pl1.density_ * pzi1 * hh * p * pvel1/(svel2*dd);

    
    sdn_pup_ = -f2 * pzj1 * (a*b + c*d * pzi2*pzj2) * p * svel1/(pvel1*dd);
    sdn_pdn_ = -2 * pl1.density_ * pzj1 * gg * 
				p * svel1/(pvel2*dd);
    sdn_sup_ = -( (b*pzj1 - c*pzj2) * ee - 
				(a + d*pzi2*pzj1) * gg*p2) /dd;
    sdn_sdn_ = 2 * pl1.density_ * pzj1 * ee * svel1/(svel2*dd);
    pup_pup_ = 2 * pl2.density_ * pzi2 * ff * pvel2/(pvel1*dd);
    pup_pdn_ = -( ( b*pzi1 - c*pzi2 ) * ff + 
				(a + d*pzi2*pzj1) * gg * p2)/dd;
    pup_sup_ = -2 * pl2.density_ * pzi2 * gg * p * pvel2/(svel1*dd);
    pup_sdn_ = f2 * pzi2 * ( a*c + b*d*pzi1*pzj1) 
				* p * pvel2/(svel2*dd);

    sup_pup_ = 2 * pl2.density_ * pzj2 * hh * p * svel2/(pvel1*dd);
    sup_pdn_ = f2 * pzj2 * (a*c + b*d * pzi1 * pzj1)  
				* p * svel2/(pvel2*dd);
    sup_sup_ = 2 * pl2.density_ * pzj2 * ee * svel2/(svel1*dd);
    sup_sdn_ = ( (b*pzj1 - c*pzj2) * ee + 
				(a + d*pzi1*pzj2) * hh*p2)/dd;

    if ( waterabove )
    {
	sdn_pdn_ = 0;
	sdn_sdn_ = 0;
	sdn_pup_ = 0;
	sdn_sup_ = 0;

	pdn_sup_ = 0;
	pup_sup_ = 0;
	sup_sup_ = 0;
    }

    if ( waterbelow )
    {
	sdn_pdn_ = 0;
	sdn_sdn_ = 0;
	sdn_pup_ = 0;
	sdn_sup_ = 0;

	pdn_sdn_ = 0;
	pup_sdn_ = 0;
	sup_sdn_ = 0;
    }
} 


float_complex InterfaceCoeff::getCoeff( bool downin, bool downout,
					bool pin, bool pout ) const
{
    if ( downin )
    {
	if ( pin )
	{
	    if ( downout )
	    {
		if ( pout )
		    return pdn_pdn_;

		return pdn_sdn_;
	    }

	    if ( pout )
		return pdn_pup_;

	    return pdn_sup_;
	}
	
	if ( downout )
	{
	    if ( pout )
		return sdn_pdn_;

	    return sdn_sdn_;
	}
	
	if ( pout )
	    return sdn_pup_;

	return sdn_sup_;
    }

    if ( pin )
    {
	if ( downout )
	{
	    if ( pout )
		return pup_pdn_;

	    return pup_sdn_;
	}

	if ( pout )
	    return pup_pup_;

	return pup_sup_;
    }

    if ( downout )
    {
	if ( pout )
	    return sup_pdn_;

	return sup_sdn_;
    }

    if ( pout )
	return sup_pup_;

    return sup_sup_;
}


AngleRayTracer::AngleRayTracer()
    : sini_( 0 )
{}


AngleRayTracer::~AngleRayTracer()
{ delete sini_; }


float AngleRayTracer::getSinAngle( int layer, int offset ) const
{
    if ( !sini_ || layer<0 || layer>=sini_->info().getSize(0) || 
	 offset<0 || offset>=sini_->info().getSize(1) )
    return mUdf(float);
    
    return sini_->get( layer, offset );
}


float* AngleRayTracer::getSinAngleData() const
{ return sini_ ? sini_->getData() : 0; }


bool AngleRayTracer::doPrepare( int nrthreads )
{
    const int layersize = downisp_ ? pmodel_.size() : smodel_.size();
    const int offsetsz = offsets_.size();
    if ( layersize<2 || !offsetsz || !RayTracer1D::doPrepare(nrthreads) )
	return false;

    sini_ = new Array2DImpl<float>( layersize, offsetsz );
    sini_->setAll( 0 );

    return true;
}


bool AngleRayTracer::compute( int layer, int offsetidx, float rayparam )
{
    if ( !sini_ )
	return false;

    const ObjectSet<Layer>& dlayers = downisp_ ? pmodel_ : smodel_;
    const float sinival = layer ? dlayers[layer-1]->Vint_ * rayparam : 0;
    sini_->set( layer, offsetidx, sinival );

    return true;
}



IsotropicRayTracer::IsotropicRayTracer()
    : reflectivity_( 0 )		
    , geomspread_( GeomSpread::None )
    , projectincwave_( false )
{}


IsotropicRayTracer::~IsotropicRayTracer()
{ delete reflectivity_; }


float_complex IsotropicRayTracer::getReflectivity( int layer, int offset ) const
{
    if ( layer<0 || layer>=reflectivity_->info().getSize(0) || 
	 offset<0 || offset>=reflectivity_->info().getSize(1) )
    return mUdf(float);

    return reflectivity_->get( layer, offset );
}


bool IsotropicRayTracer::doPrepare( int nrthreads )
{
    const int layersize = smodel_.size();
    if ( !AngleRayTracer::doPrepare(nrthreads) || !layersize )
	return false;

    reflectivity_ = new Array2DImpl<float_complex>(layersize,offsets_.size());
    reflectivity_->setAll( 0 );

    float ztot = 0;
    sourcelayer_ = 0;
    ObjectSet<Layer>& dlayers = downisp_ ? pmodel_ : smodel_;
    while ( ztot+dlayers[sourcelayer_]->d0_ < sourcedepth_ ||
	    mIsEqual(sourcedepth_,ztot+dlayers[sourcelayer_]->d0_,mDefEps) )
    {
	ztot += dlayers[sourcelayer_]->d0_;
	sourcelayer_++;
	
	if ( sourcelayer_ >= dlayers.size() )
	    return false;
    }
    
    relsourcedepth_ = sourcedepth_ - ztot;
    
    receiverlayer_ = 0;
    ztot = 0;
    ObjectSet<Layer>& ulayers = upisp_ ? pmodel_ : smodel_;
    while ( ztot+ulayers[receiverlayer_]->d0_ < receiverdepth_ ||
	    mIsEqual(receiverdepth_,ztot+ulayers[receiverlayer_]->d0_,mDefEps) )
    {
	ztot += ulayers[receiverlayer_]->d0_;
	receiverlayer_++;
	
	if ( receiverlayer_>=ulayers.size() )
	    return false;
    }
    
    relreceiverdepth_ = receiverdepth_ - ztot;
    return true;
}


#define mRetFalseIfZero(vel) \
    if ( mIsZero(vel,mDefEps) ) return false;


#define mGetSpreadDenominator \
    if ( geomspread_ == GeomSpread::Distance ) \
	spreadingdenominator += dist; \
    else if ( geomspread_ == GeomSpread::Vint ) \
	spreadingdenominator += vel * dist

bool IsotropicRayTracer::compute( int layer, int offsetidx, float rayparam )
{
    if ( !AngleRayTracer::compute(layer,offsetidx,rayparam) || !reflectivity_ )
	return false;
    
    const ObjectSet<Layer>& dlayers = downisp_ ? pmodel_ : smodel_;
    const ObjectSet<Layer>& ulayers = upisp_ ? pmodel_ : smodel_; 

    mAllocVarLenArr( InterfaceCoeff, coefs, layer );
    int toplayer = sourcelayer_<receiverlayer_ ? sourcelayer_ : receiverlayer_;
    for ( int idx=toplayer; idx<layer; idx++ )
	coefs[idx].setInterface( rayparam, idx, *pmodel_[idx], *pmodel_[idx+1],
		*smodel_[idx], *smodel_[idx+1] ); 

    int lidx = sourcelayer_;
    float vel = dlayers[lidx]->Vint_;
    mRetFalseIfZero(vel);

    float sini = rayparam * vel;
    float dist = ( dlayers[lidx]->d0_ - relsourcedepth_ ) / sqrt(1.0-sini*sini);
    float_complex reflectivity = coefs[lidx].getCoeff( true, lidx!=layer-1, 
	    downisp_, lidx==layer-1? upisp_ : downisp_ );

    float spreadingdenominator = 0;
    mGetSpreadDenominator;
    lidx++;

    while ( lidx < layer )
    {
	vel = dlayers[lidx]->Vint_;
	mRetFalseIfZero(vel);
	sini = rayparam * vel;
	dist = dlayers[lidx]->d0_ / cos( asin(sini) );
	reflectivity *= coefs[lidx].getCoeff( true, lidx!=layer-1,
		downisp_, lidx==layer-1? upisp_ : downisp_ );
	mGetSpreadDenominator;
        lidx++;
    }

    for ( lidx=layer-1; lidx>=receiverlayer_; lidx--)
    {
	vel = ulayers[lidx]->Vint_;
	mRetFalseIfZero(vel);
	sini = rayparam * vel;
	dist =  (ulayers[lidx]->d0_-(lidx==receiverlayer_ ? 
		    relreceiverdepth_ : 0)) / cos( asin(sini) );	

	if ( lidx>receiverlayer_ )
	    reflectivity *= coefs[lidx-1].getCoeff(false,false,upisp_,upisp_);
	
	if ( projectincwave_ && lidx==receiverlayer_ ) 
	    reflectivity *= cos( asin(sini) );
	
	mGetSpreadDenominator;
    }
 
    if ( geomspread_ )
	reflectivity /= spreadingdenominator;

    reflectivity_->set( layer, offsetidx, reflectivity );
    return true;
}


