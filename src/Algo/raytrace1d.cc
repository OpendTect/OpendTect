/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 1999
-*/


#include "raytrace1d.h"

#include "arrayndimpl.h"

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
    , downisp_( false )
    , upisp_( false )
    , receiverlayer_( 0 )
    , sourcelayer_( 0 )
    , sourcedepth_( 0 )
    , relsourcedepth_( 0 )
    , receiverdepth_( 0 )
    , relreceiverdepth_( 0 )			 
    , sini_( 0 )
    , reflectivity_( 0 )		
    , geomspread_( GeomSpread::None )
    , angleonly_( false )				    
    , projectincwave_( false )							{}


RayTracer1D::~RayTracer1D()
{
    delete reflectivity_;
    delete sini_;

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
	    pmodel_.copy( lys );
	else
	    pmodel_ = lys;
    }
    else 
    {
	if ( ownssmodel_ )
    	    deepErase( smodel_ );

	ownssmodel_ = policy==OD::CopyPtr;
	if ( ownssmodel_ )
	    smodel_.copy( lys );
	else
	    smodel_ = lys;
    }
}


void RayTracer1D::setOffsets( const TypeSet<float>& offsets )
{
    offsets_.erase();
    offsets_ = offsets;
}


float_complex RayTracer1D::getReflectivity( int layer, int offset ) const
{
    if ( layer<0 || layer>=reflectivity_->info().getSize(0) || 
	 offset<0 || offset>=reflectivity_->info().getSize(1) )
    return mUdf(float);

    return reflectivity_->get( layer, offset );
}


float RayTracer1D::getSinAngle( int layer, int offset ) const
{
    if ( layer<0 || layer>=sini_->info().getSize(0) || 
	 offset<0 || offset>=sini_->info().getSize(1) )
    return mUdf(float);
    
    return sini_->get( layer, offset );
}


od_int64 RayTracer1D::nrIterations() const
{
    return downisp_ ? pmodel_.size() : smodel_.size();
}


bool RayTracer1D::doPrepare( int )
{
    const int psz = pmodel_.size();
    const int ssz = smodel_.size();
    const int layersize = downisp_ ? psz : ssz;
    const int offsetsz = offsets_.size();
    if ( layersize<2 || !offsetsz )
	return false;

    if ( !angleonly_ && (!psz || !ssz) )
	return false;

    if ( sini_ ) delete sini_;
    sini_ = new Array2DImpl<float>( layersize, offsetsz );
    sini_->setAll( 0 );

    if ( reflectivity_ ) delete reflectivity_;
    reflectivity_ = new Array2DImpl<float_complex>( layersize, offsetsz );
    reflectivity_->setAll( 0 );

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


bool RayTracer1D::doWork( od_int64 start, od_int64 stop, int nrthreads )
{
    const ObjectSet<Layer>& dlayers = downisp_ ? pmodel_ : smodel_;
    for ( int layer=start; layer<=stop; layer++ )
    {
	float rayparam = 0;
	for ( int osidx=0; osidx<offsets_.size(); osidx++ )
	{
	    rayparam = findRayParam( layer, offsets_[osidx], rayparam );

	    if ( mIsUdf(rayparam) ) 
		continue;

	    const float sinival = layer ? dlayers[layer-1]->Vint_*rayparam : 0;
	    sini_->set( layer, osidx, sinival );

	    float_complex ref;
	    if ( !angleonly_ && computeReflectivity( layer, rayparam, ref ) )
    		reflectivity_->set( layer, osidx, ref );
	}
    }

    return true;
}


float RayTracer1D::findRayParam( int layer, float offset, float seed ) const
{
    float a=0, b=0.9999999/velmax_[layer], c = seed, d, e;
    float fa = offset - getOffset( layer, a );
    float fb = offset - getOffset( layer, b );
    float fc = offset - getOffset( layer, c );

    if ( fa * fb > 0 )
	return mUdf(float);

#define ITMAX 100
    for ( int iter=1; iter<=ITMAX; iter++ )
    {
	if ( fb * fc > 0 )
	{
	    c = a;
	    fc = fa;
	    e = d = b-a;
	}
	
	if ( fabs(fc) < fabs(fb) ) 
	{
	    a = b;
	    b = c;
	    c = a;
	    fa = fb;
	    fb = fc;
	    fc = fa;
	}

	const float tol1 = 6.0e-10 * fabs(b) + 0.5e-10;
	const float xm = 0.5 * (c-b);
	if ( fabs(xm) <= tol1 || fb == 0.0 )
	    return b;

	if ( fabs(e) >= tol1 && fabs(fa) > fabs(fb) )  
	{
	    float s = fb / fa;
	    float p,q;
	    if ( a == c ) 
	    {
		p = 2.0 * xm * s;
		q = 1.0 - s;
	    } 
	    else 
	    {
		q = fa / fc;
		const float r = fb / fc;
		p = s * (2.0 * xm * q * (q-r) - (b-a) * (r-1.0) );
		q = (q-1.0) * (r-1.0) * (s-1.0);
	    }

	    if ( p > 0.0 ) q = -q;
	    p = fabs(p);
	    const float min1 = 3.0 * xm * q - fabs(tol1*q);
	    const float min2 = fabs(e*q);
	    if ( 2.0*p < (min1 < min2 ? min1 : min2) ) 
	    {
		e = d;
		d = p / q;
	    } 
	    else 
	    {
		d = xm;	
		e = d;
	    }
	} 
	else 
	{
	    d = xm;
	    e = d;
	}
		
	a = b;
	fa = fb;
	if ( fabs(d) > tol1 )
	    b += d;
	else
	    b += (xm > 0.0 ? fabs(tol1) : -fabs(tol1));

	fb = offset - getOffset( layer, b );
    }

    return mUdf(float);
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


#define mRetFalseIfZero(vel) \
    if ( mIsZero(vel,mDefEps) ) return false;


#define mGetSpreadDenominator \
    if ( geomspread_ == GeomSpread::Distance ) \
	spreadingdenominator += dist; \
    else if ( geomspread_ == GeomSpread::Vint ) \
	spreadingdenominator += vel * dist

bool RayTracer1D::computeReflectivity( int layer, float rayparam, 
	float_complex& reflectivity ) const
{
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
    reflectivity = coefs[lidx].getCoeff( true, lidx!=layer-1, downisp_, 
	    lidx==layer-1? upisp_ : downisp_ );

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

    return true;
}


RayTracer1D::Layer::Layer()
{
    d0_ = 0;
    Vint_ = 0; 
    delta_ = 0;
    epsilon_ = 0;
    eta_ = 0;
    dip_ = 0;
    density_ = 0;
}


RayTracer1D::Layer& RayTracer1D::Layer::operator=(const RayTracer1D::Layer& l )
{
    d0_ = l.d0_;  
    Vint_ = l.Vint_;
    delta_ = l.delta_;  
    epsilon_ = l.epsilon_;
    eta_ = l.eta_;  
    dip_ = l.dip_;
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

