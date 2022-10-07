/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "ailayer.h"
#include "arrayndimpl.h"
#include "math2.h"
#include "mathfunc.h"
#include "ranges.h"


#define mIsValidThickness(val) \
( !mIsUdf(val) && validThicknessRange().includes(val,false) )
#define mIsValidDen(val) ( validDensityRange().includes(val,false) )
#define mIsValidVel(val) ( validVelocityRange().includes(val,false) )
#define mIsValidImp(val) ( validImpRange().includes(val,false) )


// RefLayer

mDefineEnumUtils(RefLayer,Type,"Layer Type")
{
    "Acoustic Layer",
    "Elastic Layer",
    "VTI Layer",
    "HTI Layer",
    nullptr
};


RefLayer* RefLayer::create( Type typ )
{
    if ( typ == Acoustic )
	return new AILayer( mUdf(float), mUdf(float), mUdf(float) );
    if ( typ == Elastic )
	return new ElasticLayer( mUdf(float), mUdf(float),
				 mUdf(float), mUdf(float) );
    //TODO: add others

    return nullptr;
}


RefLayer* RefLayer::clone( const RefLayer& layer, const Type* reqtyp )
{
    if ( !reqtyp || (reqtyp && layer.getType() == *reqtyp) )
	return layer.clone();

    auto* ret = create( reqtyp ? *reqtyp : layer.getType() );
    *ret = layer;
    if ( ret->isElastic() )
	ret->asElastic()->fillVsWithVp( true );
    if ( ret->isVTI() && !ret->isValidFraRho() )
	ret->setFracRho( 0.f );
    if ( ret->isHTI() && !ret->isValidFracAzi() )
	ret->setFracAzi( 0.f );

    return ret;
}


RefLayer::RefLayer()
{
}


RefLayer::~RefLayer()
{
}


RefLayer& RefLayer::operator =( const RefLayer& oth )
{
    if ( &oth == this )
	return *this;

    copyFrom( oth );
    return *this;
}


bool RefLayer::operator ==( const RefLayer& oth ) const
{
    if ( &oth == this )
	return true;

    if ( getThickness() != oth.getThickness() || getPVel() != oth.getPVel() ||
	 getDen() != oth.getDen() )
	return false;

    if ( isElastic() != oth.isElastic() ||
	(isElastic() && getSVel() != oth.getSVel()) )
	return false;

    if ( isVTI() != oth.isVTI() ||
	(isVTI() && getFracRho() != oth.getFracRho()) )
	return false;

    if ( isHTI() != oth.isHTI() ||
	(isHTI() && getFracAzi() != oth.getFracAzi()) )
	return false;

    return true;
}


bool RefLayer::operator !=( const RefLayer& oth ) const
{
    return !(*this == oth);
}


bool RefLayer::isOK( bool dodencheck, bool dosvelcheck,
		     bool dofracrhocheck, bool dofracazicheck ) const
{
    if ( !isValidThickness() || !isValidVel() )
	return false;

    if ( dodencheck && !isValidDen() )
	return false;

    if ( isElastic() && dosvelcheck && !isValidVs() )
	return false;

    if ( isVTI() && dofracrhocheck && !isValidFraRho() )
	return false;

    if ( isHTI() && dofracazicheck && !isValidFracAzi() )
	return false;

    return true;
}


// AILayer

AILayer::AILayer( float thkness, float vel, float den )
    : RefLayer()
    , thickness_(thkness)
    , den_(den)
    , vel_(vel)
{
}


AILayer::AILayer( float thkness, float ai, float den, bool needcompthkness )
    : RefLayer()
    , thickness_(thkness)
    , den_(den)
{
    const bool hasdensity = isValidDen();
    //compute vel_ using Gardner's equation vel = (den/a)^(1/(1+b))
    //with default values for C0 and C1 respectively 310 and 0.25
    vel_ = hasdensity ? ai / den : Math::PowerOf( ai/310.f, 0.8f );
    if ( !hasdensity )
	den_ = ai/vel_;

    if ( needcompthkness )
	thickness_ *= vel_ / 2.0f;
}


AILayer::AILayer( const RefLayer& oth )
    : RefLayer()
{
    AILayer::copyFrom( oth );
}


AILayer::~AILayer()
{
}


RefLayer* AILayer::clone() const
{
    return new AILayer( *this );
}


RefLayer& AILayer::operator =( const AILayer& oth )
{
    return RefLayer::operator=( oth );
}


void AILayer::copyFrom( const RefLayer& oth )
{
    thickness_ = oth.getThickness();
    den_ = oth.getDen();
    vel_ = oth.getPVel();
}


float AILayer::getAI() const
{
    return isValidDen() && isValidVel() ? den_ * vel_ : mUdf(float);
}


RefLayer& AILayer::setThickness( float thickness )
{
    thickness_ = thickness;
    return *this;
}


RefLayer& AILayer::setPVel( float vel )
{
    vel_ = vel;
    return *this;
}


RefLayer& AILayer::setDen( float den )
{
    den_ = den;
    return *this;
}


bool AILayer::fillDenWithVp( bool onlyinvalid )
{
    if ( onlyinvalid && isValidDen() )
	return true;

    if ( !isValidVel() )
	return false;

    den_ = mCast( float, 310. * Math::PowerOf( (double)vel_, 0.25 ) );
    return isValidDen();
}


bool AILayer::isValidThickness() const
{
    return mIsValidThickness(thickness_);
}


bool AILayer::isValidVel() const
{
    return mIsValidVel(vel_);
}


bool AILayer::isValidDen() const
{
    return mIsValidDen(den_);
}


// ElasticLayer

ElasticLayer::ElasticLayer( float thkness, float pvel, float svel, float den )
    : AILayer(thkness,pvel,den)
    , svel_(svel)
{
}


ElasticLayer::ElasticLayer( float thkness, float ai, float si, float den,
			    bool needcompthkness )
    : AILayer(thkness,ai,den,needcompthkness)
{
    svel_ = mIsValidImp(si) && isValidDen() ? si / getDen() : mUdf(float);
}


ElasticLayer::ElasticLayer( const RefLayer& oth )
    : AILayer(oth.getThickness(),oth.getPVel(),oth.getDen())
    , svel_(mUdf(float))
{
    if ( oth.isElastic() )
	svel_ = oth.getSVel();
    else
	fillVsWithVp( true );
}


ElasticLayer::~ElasticLayer()
{
}


RefLayer* ElasticLayer::clone() const
{
    return new ElasticLayer( *this );
}


RefLayer& ElasticLayer::operator =( const ElasticLayer& oth )
{
    return RefLayer::operator=( oth );
}


void ElasticLayer::copyFrom( const RefLayer& oth )
{
    AILayer::copyFrom( oth );
    if ( oth.isElastic() )
	svel_ = oth.getSVel();
}


float ElasticLayer::getSI() const
{
    return isValidDen() && isValidVs() ? getDen() * svel_ : mUdf(float);
}


RefLayer& ElasticLayer::setSVel( float vel )
{
    svel_ = vel;
    return *this;
}


bool ElasticLayer::fillVsWithVp( bool onlyinvalid )
{
    if ( onlyinvalid && isValidVs() )
	return true;

    if ( !isValidVel() )
	return false;

    svel_ = mCast( float, 0.8619 * (double)getPVel() -1172. );
    return isValidVs();
}


bool ElasticLayer::isValidVs() const
{
    return mIsValidVel(svel_);
}


// ElasticModel

ElasticModel::ElasticModel()
    : ObjectSet<RefLayer>()
{
}


ElasticModel::ElasticModel( const ObjectSet<RefLayer>& oth )
    : ObjectSet<RefLayer>()
{
    deepCopyClone( *this, oth );
}


ElasticModel::ElasticModel( const ElasticModel& oth )
    : ObjectSet<RefLayer>()
{
    *this = oth;
}


ElasticModel::~ElasticModel()
{
    erase();
}


ElasticModel& ElasticModel::operator =( const ElasticModel& oth )
{
    ObjectSet<RefLayer>::operator= (oth );
    return *this;
}


ElasticModel& ElasticModel::operator -=( RefLayer* ptr )
{
    if ( ptr )
    {
	this->vec_.erase( (RefLayer*)ptr );
	delete ptr;
    }

    return *this;
}


void ElasticModel::append( const ObjectSet<RefLayer>& oth )
{
    const int sz = oth.size();
    this->vec_.setCapacity( this->size()+sz, true );
    for ( idx_type vidx=0; vidx<sz; vidx++ )
    {
	const RefLayer* obj = oth.get( vidx );
	if ( obj )
	    ObjectSet<RefLayer>::add( obj->clone() );
    }
}


void ElasticModel::erase()
{
    deepErase( *this );
}


RefLayer* ElasticModel::pop()
{
    delete ObjectSet<RefLayer>::pop();
    return nullptr;
}


RefLayer* ElasticModel::removeSingle( int vidx, bool kporder )
{
    delete ObjectSet<RefLayer>::removeSingle( vidx, kporder );
    return nullptr;
}


void ElasticModel::removeRange( int i1, int i2 )
{
    for ( int vidx=i1; vidx<=i2; vidx++ )
	delete this->get(vidx);

    ObjectSet<RefLayer>::removeRange( i1, i2 );
}


RefLayer* ElasticModel::replace( int vidx, RefLayer* ptr )
{
    delete ObjectSet<RefLayer>::replace( vidx, ptr );
    return nullptr;
}


RefLayer* ElasticModel::removeAndTake( int vidx, bool kporder )
{
    return ObjectSet<RefLayer>::removeSingle( vidx, kporder );
}


ElasticModel& ElasticModel::copyFrom( const ElasticModel& oth,
				      RefLayer::Type reqtyp )
{
    if ( oth.getMinType() >= reqtyp )
    {
	*this = oth;
	return *this;
    }

    const int sz = oth.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	auto* newlayer = RefLayer::clone( *oth.get(idx), &reqtyp );
	if ( validIdx(idx) )
	    replace( idx, newlayer );
	else
	    add( newlayer );
    }

    return *this;
}


RefLayer::Type ElasticModel::getType() const
{
    if ( isHTI() )
	return RefLayer::HTI;
    if ( isVTI() )
	return RefLayer::VTI;
    if ( isElastic() )
	return RefLayer::Elastic;

    return RefLayer::Acoustic;
}


RefLayer::Type ElasticModel::getMinType() const
{
    RefLayer::Type ret = RefLayer::HTI;
    for ( const auto* layer : *this )
    {
	const RefLayer::Type typ = layer->getType();
	if ( typ < ret )
	    ret = typ;
	if ( ret == RefLayer::Acoustic )
	    break;
    }

    return ret;
}


bool ElasticModel::isElastic() const
{
    for ( const auto* layer : *this )
	if ( layer->isElastic() )
	    return true;

    return false;
}


bool ElasticModel::isVTI() const
{
    for ( const auto* layer : *this )
	if ( layer->isVTI() )
	    return true;

    return false;
}


bool ElasticModel::isHTI() const
{
    for ( const auto* layer : *this )
	if ( layer->isHTI() )
	    return true;

    return false;
}


int ElasticModel::isOK( bool dodencheck, bool dosvelcheck,
			bool dofracrhocheck, bool dofracazicheck ) const
{
    int idx = -1;
    for ( const auto* layer : *this )
    {
	idx++;
	if ( !layer->isOK(dodencheck,dosvelcheck,dofracrhocheck,dofracazicheck))
	    return idx;
    }

    return -1;
}


#define mRmLay(idx) \
{ \
    firsterroridx = idx; \
    removeSingle( idx ); \
    continue; \
}

void ElasticModel::checkAndClean( int& firsterroridx, bool dodencheck,
				  bool dosvelcheck, bool onlyinvalid )
{
    for ( int idx=size()-1; idx>=0; idx-- )
    {
	RefLayer& lay = *get(idx);
	if ( !lay.isOK(false,false) )
	    mRmLay(idx)

	if ( dodencheck && !lay.isValidDen() )
	{
	    if ( !lay.asAcoustic().fillDenWithVp(onlyinvalid) )
	    {
		mRmLay(idx)
		continue;
	    }
	}

	if ( dosvelcheck && lay.isElastic() && !lay.isValidVs() )
	{
	    if ( !lay.asElastic()->fillVsWithVp(onlyinvalid) )
		mRmLay(idx)
	}

	// Not removing the VTI/HTI props, only setting a neutral value
	if ( dosvelcheck && (lay.isVTI() || lay.isHTI()) &&
	     !lay.isValidFraRho() )
	    lay.setFracRho( 0.f );

	if ( dosvelcheck && lay.isHTI() && !lay.isValidFracAzi() )
	    lay.setFracAzi( 0.f );
    }
}


void ElasticModel::interpolate( bool dovp, bool doden, bool dovs )
{
    BoolTypeSet dointerpolate;
    dointerpolate += dovp;
    dointerpolate += doden;
    if ( isElastic() )
	dointerpolate += dovs;
    if ( isVTI() )
	dointerpolate += true;
    if ( isHTI() )
	dointerpolate += true;

    for ( int iprop=0; iprop<dointerpolate.size(); iprop++ )
    {
	if ( !dointerpolate[iprop] )
	    continue;

	BendPointBasedMathFunction<float,float> data;
	for ( int idx=0; idx<size(); idx++ )
	{
	    const RefLayer& layer = *get(idx);
	    float val = mUdf(float);
	    if ( iprop == 0 && layer.isValidVel() )
		val = layer.getPVel();
	    else if ( iprop == 1 && layer.isValidDen() )
		val = layer.getDen();
	    else if ( iprop == 2 && layer.isValidVs() )
		val = layer.getSVel();
	    else if ( iprop == 3 && layer.isValidFraRho() )
		val = layer.getFracRho();
	    else if ( iprop == 4 && layer.isValidFracAzi() )
		val = layer.getFracAzi();

	    if ( !mIsUdf(val) )
		data.add( (float)idx, val );
	}

	if ( data.isEmpty() )
	    continue;

	for ( int idx=0; idx<size(); idx++ )
	{
	    const float val = data.getValue( (float)idx );
	    RefLayer& layer = *get(idx);
	    if ( iprop == 0 )
		layer.setPVel( val );
	    else if ( iprop == 1 )
		layer.setDen( val );
	    else if ( iprop == 2 )
		layer.setSVel( val );
	    else if ( iprop == 3 )
		layer.setFracRho( val );
	    else if ( iprop == 4 )
		layer.setFracAzi( val );
	}
    }
}


void ElasticModel::upscale( float maxthickness )
{
    if ( isEmpty() || maxthickness < cMinLayerThickness() )
	return;

    const ElasticModel orgmodl = *this;
    setEmpty();

    float totthickness = 0.f;
    ElasticModel curmodel;
    PtrMan<RefLayer> newlayer;
    for ( int lidx=0; lidx<orgmodl.size(); lidx++ )
    {
	PtrMan<RefLayer> curlayer = orgmodl.get(lidx)->clone();
	float thickness = curlayer->getThickness();
	if ( !curlayer->isValidThickness() || !curlayer->isValidVel() )
	    continue;

	if ( thickness > maxthickness-cMinLayerThickness() )
	{
	    if ( !curmodel.isEmpty() )
	    {
		newlayer = RefLayer::create( curmodel.getType() );
		if ( curmodel.getUpscaledBackus(*newlayer.ptr()) )
		    add( newlayer.release() );

		totthickness = 0.f;
		curmodel.setEmpty();
	    }

	    add( curlayer->clone() );
	    continue;
	}

	const bool lastlay = totthickness + thickness >
			     maxthickness - cMinLayerThickness();
	const float thicknesstoadd = lastlay ? maxthickness - totthickness
					     : thickness;
	totthickness += thicknesstoadd;
	if ( lastlay )
	{
	    thickness -= thicknesstoadd;
	    curlayer->setThickness( thicknesstoadd );
	}

	curmodel.add( curlayer->clone() );
	if ( lastlay )
	{
	    newlayer = RefLayer::create( curmodel.getType() );
	    if ( curmodel.getUpscaledBackus(*newlayer.ptr()) )
		add( newlayer.release() );

	    totthickness = thickness;
	    curmodel.setEmpty();
	    if ( thickness > cMinLayerThickness() )
	    {
		curlayer->setThickness( thickness );
		curmodel.add( curlayer->clone() );
	    }
	}
    }
    if ( totthickness > cMinLayerThickness() && !curmodel.isEmpty() )
    {
	newlayer = RefLayer::create( curmodel.getType() );
	if ( curmodel.getUpscaledBackus(*newlayer.ptr()) )
	    add( newlayer.release() );
    }
}


void ElasticModel::upscaleByN( int nbblock )
{
    if ( isEmpty() || nbblock < 2 )
	return;

    const ElasticModel orgmodl = *this;
    setEmpty();

    ElasticModel curmdl;
    PtrMan<RefLayer> newlayer;
    for ( int lidx=0; lidx<orgmodl.size(); lidx++ )
    {
	curmdl.add( orgmodl.get( lidx )->clone() );
	if ( (lidx+1) % nbblock == 0 )
	{
	    newlayer = RefLayer::create( curmdl.getType() );
	    if ( curmdl.getUpscaledBackus(*newlayer.ptr()) )
		add( newlayer->clone() );

	    curmdl.setEmpty();
	}
    }

    if ( !curmdl.isEmpty() )
    {
	newlayer = RefLayer::create( curmdl.getType() );
	if ( curmdl.getUpscaledBackus(*newlayer.ptr()) )
	    add( newlayer->clone() );
    }
}


void ElasticModel::block( float relthreshold, bool pvelonly )
{
    if ( isEmpty() || relthreshold < mDefEpsF || relthreshold > 1.f-mDefEpsF )
	return;

    TypeSet<Interval<int> > blocks;
    if ( !doBlocking(relthreshold,pvelonly,blocks) )
	return;

    const ElasticModel orgmodl = *this;
    setEmpty();
    for ( int lidx=0; lidx<blocks.size(); lidx++ )
    {
	ElasticModel blockmdl;
	const Interval<int> curblock = blocks[lidx];
	for ( int lidy=curblock.start; lidy<=curblock.stop; lidy++ )
	    blockmdl.add( orgmodl.get(lidy)->clone() );

	PtrMan<RefLayer> outlay = RefLayer::create( blockmdl.getType() );
	if ( !blockmdl.getUpscaledBackus(*outlay.ptr()) )
	    continue;

	add( outlay.release() );
    }
}


bool ElasticModel::getUpscaledByThicknessAvg( RefLayer& outlay ) const
{
    if ( isEmpty() )
	return false;

    outlay.setThickness( mUdf(float) );
    outlay.setPVel( mUdf(float) );
    outlay.setDen( mUdf(float) );
    outlay.setSVel( mUdf(float) );

    float totthickness=0.f, sonp=0.f, den=0.f, sson=0.f;
    float velpthickness=0.f, denthickness=0.f, svelthickness=0.f;
    for ( int lidx=0; lidx<size(); lidx++ )
    {
	const RefLayer& curlayer = *get(lidx);
	if ( !curlayer.isValidThickness() || !curlayer.isValidVel() )
	    continue;

	const float ldz = curlayer.getThickness();
	const float layinvelp = curlayer.getPVel();
	const float layinden = curlayer.getDen();
	const float layinsvel = curlayer.getSVel();

	totthickness += ldz;
	sonp += ldz / layinvelp;
	velpthickness += ldz;

	if ( curlayer.isValidDen() )
	{
	    den += layinden * ldz;
	    denthickness += ldz;
	}

	if ( curlayer.isElastic() && curlayer.isValidVs() )
	{
	    sson += ldz / layinsvel;
	    svelthickness += ldz;
	}
    }

    if ( totthickness<mDefEpsF || velpthickness<mDefEpsF || sonp<mDefEpsF )
	return false;

    const float velfinal = velpthickness / sonp;
    if ( !mIsValidVel(velfinal) )
	return false;

    outlay.setThickness( totthickness );
    outlay.setPVel( velfinal );
    if ( !mIsValidThickness(denthickness) || denthickness < mDefEpsF )
	outlay.setDen( mUdf(float) );
    else
    {
	const float denfinal = den / denthickness;
	outlay.setDen( mIsValidDen(denfinal) ? denfinal : mUdf(float) );
    }

    if ( outlay.isElastic() )
    {
	if ( !mIsValidThickness(svelthickness) || svelthickness < mDefEpsF )
	    outlay.setSVel( mUdf(float) );
	else
	{
	    const float svelfinal = svelthickness / sson;
	    outlay.setSVel( mIsValidVel(svelfinal) ? svelfinal : mUdf(float) );
	}
    }

    return true;
}


bool ElasticModel::getUpscaledBackus( RefLayer& outlay, float theta ) const
{
    if ( isEmpty() )
	return false;

    outlay.setThickness( mUdf(float) );
    outlay.setPVel( mUdf(float) );
    outlay.setDen( mUdf(float) );
    outlay.setSVel( mUdf(float) );

    float totthickness=0.f, den=0.f;
    float x=0.f, y=0.f, z=0.f, u=0.f, v=0.f, w=0.f;
    for ( int lidx=0; lidx<size(); lidx++ )
    {
	const RefLayer& curlayer = *get(lidx);
	const float ldz = curlayer.getThickness();
	const float layinvelp = curlayer.getPVel();
	const float layinden = curlayer.getDen();
	const float layinsvel = curlayer.getSVel();

	PtrMan<RefLayer> tmplay = curlayer.clone();
	tmplay->setThickness( ldz );
	tmplay->setPVel( layinvelp );
	tmplay->setDen( layinden );
	tmplay->setSVel( layinsvel );
	if ( !tmplay->isOK() )
	    continue;

	totthickness += ldz;
	const float vp2 = layinvelp * layinvelp;
	const float vs2 = layinsvel * layinsvel;
	const float mu = layinden * vs2;
	const float lam = layinden * ( vp2 - 2.f * vs2 );
	const float denomi = lam + 2.f * mu;
	den += ldz * layinden;
	x += ldz * mu * (lam + mu ) / denomi;
	y += ldz * mu * lam / denomi;
	z += ldz * lam / denomi;
	u += ldz / denomi;
	v += ldz / mu;
	w += ldz * mu;
    }

    if ( totthickness<mDefEpsF )
	return getUpscaledByThicknessAvg( outlay );

    den /= totthickness;
    x /= totthickness; y /= totthickness; z /= totthickness;
    u /= totthickness; v /= totthickness; w /= totthickness;
    outlay.setThickness( totthickness );
    outlay.setDen( den );

    const float c11 = 4.f * x + z * z / u;
    const float c12 = 2.f * y + z * z / u;
    const float c33 = 1.f / u;
    const float c13 = z / u;
    const float c44 = 1.f / v;
    const float c66 = w;
    const float c66qc = ( c11 - c12 ) / 2.f;

    if ( !mIsEqual(c66,c66qc,1e6f) )
	return getUpscaledByThicknessAvg( outlay );

    const bool zerooffset = mIsZero( theta, 1e-5f );
    const float s2 = zerooffset ? 0.f : sin( theta ) * sin( theta );
    const float c2 = zerooffset ? 1.f : cos( theta ) * cos( theta );
    const float s22 = zerooffset ? 0.f : sin( 2.f *theta ) * sin( 2.f *theta );

    const float mm = c11 * s2 + c33 * c2 + c44;
    const float mn = Math::Sqrt( ( ( c11 - c44 ) * s2 - ( c33 - c44 ) * c2 ) *
				 ( ( c11 - c44 ) * s2 - ( c33 - c44 ) * c2 )
				 + ( c13 + c44 ) *
				   ( c13 + c44 ) * s22 );
    const float mp2 = ( mm + mn ) / 2.f;
    const float msv2 = ( mm - mn ) / 2.f;

    outlay.setPVel( Math::Sqrt( mp2 / den ) );
    outlay.setSVel( Math::Sqrt( msv2 / den ) );

    return true;
}


void ElasticModel::setMaxThickness( float maxthickness )
{
    if ( isEmpty() || maxthickness < cMinLayerThickness() )
	return;

    const ElasticModel orgmodl = *this;
    setEmpty();
    const int initialsz = orgmodl.size();
    int nbinsert = mUdf(int);
    for ( int lidx=0; lidx<initialsz; lidx++ )
    {
	const float thickness = orgmodl.get(lidx)->getThickness();
	if ( !mIsValidThickness(thickness) )
	    continue;

	PtrMan<RefLayer> newlayer = orgmodl.get(lidx)->clone();
	nbinsert = 1;
	if ( thickness > maxthickness - cMinLayerThickness() )
	{
	    nbinsert = mCast( int, thickness/maxthickness ) + 1;
	    newlayer->setThickness( newlayer->getThickness() / nbinsert );
	}
	for ( int nlidx=0; nlidx<nbinsert; nlidx++ )
	    add( newlayer->clone() );
    }
}


void ElasticModel::mergeSameLayers()
{
    if ( isEmpty() )
	return;

    const ElasticModel orgmodl = *this;
    setEmpty();
    const int initialsz = orgmodl.size();
    bool havemerges = false;
    float totthickness = 0.f;
    PtrMan<RefLayer> prevlayer = orgmodl.first()->clone();
    for ( int lidx=1; lidx<initialsz; lidx++ )
    {
	const RefLayer& curlayer = *orgmodl.get(lidx);
	if ( mIsEqual(curlayer.getPVel(),prevlayer->getPVel(),1e-2f) &&
	     mIsEqual(curlayer.getDen(),prevlayer->getDen(),1e-2f) &&
	     mIsEqual(curlayer.getSVel(),prevlayer->getSVel(),1e-2f) )
	{
	    if ( havemerges == false )
		totthickness = prevlayer->getThickness();

	    havemerges = true;
	    totthickness += curlayer.getThickness();
	}
	else
	{
	    if ( havemerges )
	    {
		prevlayer->setThickness( totthickness );
		havemerges = false;
	    }

	    add( prevlayer.release() );
	    prevlayer = curlayer.clone();
	}
    }
    if ( havemerges )
	prevlayer->setThickness( totthickness );

    add( prevlayer.release() );
}


bool ElasticModel::createFromVel( const StepInterval<float>& zrange,
				  const float* pvel, const float* svel,
				  const float* den )
{
    if ( !pvel )
	return false;

    setEmpty();
    const bool zit =  SI().zDomain().isTime();
    const bool zinfeet	= SI().depthsInFeet();
    const int zsize = zrange.nrSteps()+1;

    const float srddepth = -1.0f * (float) SI().seismicReferenceDatum();

    int firstidx = 0; float firstlayerthickness;
    const float firstvel = zinfeet ? pvel[firstidx] * mFromFeetFactorF
				   : pvel[firstidx];
    if ( zit )
    {
	firstlayerthickness = zrange.start<0.f ? 0.0f : zrange.start;
	firstlayerthickness *= firstvel / 2.0f;
    }
    else
    {
	if ( zrange.start < srddepth )
	{
	    firstidx = zrange.getIndex( srddepth );
	    if ( firstidx < 0 )
		firstidx = 0;
	}

	firstlayerthickness = zrange.atIndex( firstidx ) - srddepth;
	if ( zinfeet ) firstlayerthickness *= mFromFeetFactorF;
    }

    const float firstden = den ? den[firstidx] : mUdf(float);
    const float firstsvel = svel ? ( zinfeet ? svel[firstidx] * mFromFeetFactorF
					     : svel[firstidx] )
				 : mUdf(float);
    add( svel ? new ElasticLayer( firstlayerthickness, firstvel, firstsvel,
				  firstden )
	     : new AILayer( firstlayerthickness, firstvel, firstden ) );

    for ( int idx=firstidx+1; idx<zsize; idx++ )
    {
	const float velp = zinfeet ? pvel[idx] * mFromFeetFactorF : pvel[idx];
	const float layerthickness = zit ? zrange.step * velp / 2.0f
				 : ( zinfeet ? zrange.step * mFromFeetFactorF
					     : zrange.step );
	const float rhob = den ? den[idx] : mUdf(float);
	if ( svel )
	{
	    const float vels = zinfeet ? svel[idx] * mFromFeetFactorF
				       : svel[idx];
	    add( new ElasticLayer( layerthickness, velp, vels, rhob ) );
	}
	else
	    add( new AILayer( layerthickness, velp, rhob ) );
    }

    if ( isEmpty() )
	return false;

    mergeSameLayers();
    removeSpuriousLayers( zrange.step );

    return true;
}


bool ElasticModel::createFromAI( const StepInterval<float>& zrange,
				 const float* ai, const float* si,
				 const float* den )
{
    if ( !ai )
	return false;

    setEmpty();
    const bool zit =  SI().zDomain().isTime();
    const bool zinfeet	= SI().depthsInFeet();
    const int zsize = zrange.nrSteps()+1;

    const float srddepth = -1.0f * (float) SI().seismicReferenceDatum();

    int firstidx = 0; float firstlayerthickness;
    if ( zit )
	firstlayerthickness = zrange.start<0.f ? 0.0f : zrange.start;
    else
    {
	if ( zrange.start < srddepth )
	{
	    firstidx = zrange.getIndex( srddepth );
	    if ( firstidx < 0 )
		firstidx = 0;
	}

	firstlayerthickness = zrange.atIndex( firstidx ) - srddepth;
	if ( zinfeet ) firstlayerthickness *= mFromFeetFactorF;
    }

    const float firstden = den ? den[firstidx] : mUdf(float);
    if ( si )
	add( new ElasticLayer( firstlayerthickness, ai[firstidx], si[firstidx],
			       firstden, zit ) );
    else
	add( new AILayer( firstlayerthickness, ai[firstidx], firstden, zit ) );

    for ( int idx=firstidx+1; idx<zsize; idx++ )
    {
	const float rhob = den ? den[idx] : mUdf(float);
	if ( si )
	    add( new ElasticLayer(zrange.step, ai[idx], si[idx], rhob, zit) );
	else
	    add( new AILayer(zrange.step, ai[idx], rhob, zit) );
    }

    if ( isEmpty() )
	return false;

    mergeSameLayers();
    removeSpuriousLayers( zrange.step );

    return true;
}


void ElasticModel::removeSpuriousLayers( float zrgstep )
{
    if ( SI().zInFeet() ) zrgstep *= mFromFeetFactorF;

    const bool zistime = SI().zIsTime();
    for ( int idx=size()-2; idx>0; idx-- )
    {
	const float layervel = get(idx)->getPVel();
	const float layerthickness = get(idx)->getThickness();
	const float layertwtthickness = 2.f * layerthickness / layervel;
	if ( ( zistime && !mIsEqual(layertwtthickness,zrgstep,1e-2f) ) ||
	     (!zistime && !mIsEqual(layerthickness,zrgstep,1e-2f) ) )
	    continue;

	const float velabove = get(idx-1)->getPVel();
	const float velbelow = get(idx+1)->getPVel();
	const float layerthicknessabove = get(idx-1)->getThickness();
	const float layerthicknessbelow = get(idx+1)->getThickness();
	const float twtthicknessabove = 2.f * layerthicknessabove / velabove;
	const float twtthicknessbelow = 2.f * layerthicknessbelow / velbelow;
	if ( zistime )
	{
	    if ( mIsEqual(twtthicknessabove,zrgstep,1e-2f) ||
		 mIsEqual(twtthicknessbelow,zrgstep,1e-2f) )
		continue;
	}
	else
	{
	    if ( mIsEqual(layerthicknessabove,zrgstep,1e-2f) ||
		 mIsEqual(layerthicknessbelow,zrgstep,1e-2f) )
		continue;
	}

	const float twtbelow = layertwtthickness * ( layervel-velabove )
						 / ( velbelow-velabove );
	const float twtabove = layertwtthickness - twtbelow;
	get(idx-1)->setThickness( get(idx-1)->getThickness() +
				  twtabove * velabove / 2.f );
	get(idx+1)->setThickness( get(idx+1)->getThickness() +
				  twtbelow * velbelow / 2.f );
	removeSingle( idx );
    }
}


bool ElasticModel::getValues( bool isden, bool issvel,
			      TypeSet<float>& vals ) const
{
    const int sz = size();
    vals.setEmpty();

    for ( int idx=0; idx<sz; idx++ )
    {
	const RefLayer& layer = *get(idx);
	const bool isvalid = isden ? layer.isValidDen()
				   : (issvel ? layer.isValidVel()
					     : layer.isValidVs());
	if ( !isvalid )
	    return false;

	vals += isden ? layer.getDen()
		      : ( issvel ? layer.getSVel() : layer.getPVel() );
    }

    return true;
}

#define mGetVals(dofill,isden,issvel,data) \
{ \
    if ( dofill ) \
    { \
	if ( !getValues(isden,issvel,data) ) \
		return false; \
	icomp++; \
    } \
}

#define mFillArr(dofill,data) \
{ \
    if ( dofill ) \
    { \
	for ( int idx=0; idx<sz; idx++ ) \
		vals.set( icomp, idx, data[idx] ); \
	icomp++; \
    } \
}

bool ElasticModel::getValues( bool vel, bool den, bool svel,
			      Array2D<float>& vals ) const
{
    const int sz = size();
    TypeSet<float> velvals, denvals, svelvals;

    int icomp = 0;
    mGetVals(vel,false,false,velvals);
    mGetVals(den,true,false,denvals);
    mGetVals(svel,false,true,svelvals);

    const Array2DInfoImpl info2d( icomp, sz );
    if ( !vals.setInfo(info2d) )
	return false;

    icomp = 0;
    mFillArr(vel,velvals);
    mFillArr(den,denvals);
    mFillArr(svel,svelvals);

    return true;
}


bool ElasticModel::getRatioValues( bool vel, bool den, bool svel,
				   Array2D<float>& ratiovals,
				   Array2D<float>& vals ) const
{
    if ( !getValues(vel,den,svel,vals) )
	return false;

    const int sz = size();
    const int nrcomp = vals.info().getSize( 0 );
    if ( nrcomp == 0 )
	return false;

    const Array2DInfoImpl info2d( nrcomp, sz );
    if ( !ratiovals.setInfo(info2d) )
	return false;

    for ( int icomp=0; icomp<nrcomp; icomp++ )
    {
	float prevval = vals.get( icomp, 0 );
	ratiovals.set( icomp, 0, 0.f );
	for ( int idx=1; idx<sz; idx++ )
	{
	    const float curval = vals.get( icomp, idx );
	    const float val = curval < prevval
			    ? prevval / curval - 1.f
			    : curval / prevval - 1.f;
	    ratiovals.set( icomp, idx, val );
	    prevval = curval;
	}
    }

    return true;
}


#define mAddBlock( block ) \
    if ( !blocks.isEmpty() && block.start<blocks[0].start ) \
	blocks.insert( 0, block ); \
    else \
	blocks += block


bool ElasticModel::doBlocking( float relthreshold, bool pvelonly,
			       TypeSet<Interval<int> >& blocks ) const
{
    blocks.setEmpty();

    Array2DImpl<float> vals( 1, size() );
    Array2DImpl<float> ratiovals( 1, size() );
    if ( !getRatioValues(true,!pvelonly,!pvelonly,ratiovals,vals) )
	return false;

    const int nrcomp = vals.info().getSize( 0 );
    const int modelsize = vals.info().getSize( 1 );

    TypeSet<Interval<int> > investigationqueue;
    investigationqueue += Interval<int>( 0, modelsize-1 );
    while ( investigationqueue.size() )
    {
	Interval<int> curblock = investigationqueue.pop();

	while ( true )
	{
	    const int width = curblock.width();
	    if ( width==0 )
	    {
		mAddBlock( curblock );
		break;
	    }

	    TypeSet<int> bendpoints;
	    const int lastblock = curblock.start + width;
	    TypeSet<float> firstval( nrcomp, mUdf(float) );
	    TypeSet< Interval<float> > valranges;
	    float maxvalratio = 0;
	    for ( int icomp=0; icomp<nrcomp; icomp++ )
	    {
		firstval[icomp] = vals.get( icomp, curblock.start );
		Interval<float> valrange(  firstval[icomp],  firstval[icomp] );
		valranges += valrange;
	    }

	    for ( int lidx=curblock.start+1; lidx<=lastblock; lidx++ )
	    {
		for ( int icomp=0; icomp<nrcomp; icomp++ )
		{
		    const float curval = vals.get( icomp, lidx );
		    valranges[icomp].include( curval );

		    const float valratio = ratiovals.get( icomp, lidx );
		    if ( valratio >= maxvalratio )
		    {
			if ( !mIsEqual(valratio,maxvalratio,1e-5f) )
			    bendpoints.erase();

			bendpoints += lidx;
			maxvalratio = valratio;
		    }
		}
	    }

	    if ( maxvalratio<=relthreshold )
	    {
		mAddBlock( curblock );
		break;
	    }

	    int bendpoint = curblock.center();
	    if ( bendpoints.isEmpty() )
	    {
		pFreeFnErrMsg("Should never happen");
	    }
	    else if ( bendpoints.size()==1 )
	    {
		bendpoint = bendpoints[0];
	    }
	    else
	    {
		const int middle = bendpoints.size()/2;
		bendpoint = bendpoints[middle];
	    }

	    investigationqueue += Interval<int>( curblock.start, bendpoint-1);
	    curblock = Interval<int>( bendpoint, curblock.stop );
	}
    }

    return !blocks.isEmpty() && ( blocks.size() < modelsize );
}


float ElasticModel::getLayerDepth( int ilayer ) const
{
    float depth = 0.f;
    if ( ilayer >= size() )
	ilayer = size();

    for ( int idx=0; idx<ilayer; idx++ )
	depth += get(idx)->getThickness();

    if ( ilayer < size() )
	depth += get(ilayer)->getThickness() / 2.f;

    return depth;
}


Interval<float> ElasticModel::getTimeSampling( bool usevs ) const
{
    Interval<float> ret( 0.f, 0.f );
    for ( const auto* layer : *this )
    {
	if ( !layer->isOK(false,usevs) )
	    continue;

	const float vel = usevs ? layer->getSVel() : layer->getPVel();
	ret.stop += layer->getThickness() / vel;
    }

    ret.stop *= 2.f; // TWT needed
    return ret;
}


// ElasticModelSet

ElasticModelSet::ElasticModelSet()
    : ManagedObjectSet<ElasticModel>()
{
}


ElasticModelSet::~ElasticModelSet()
{
}


bool ElasticModelSet::setSize( int nrmdls )
{
    const int oldsz = size();
    if ( nrmdls == oldsz )
	return true;

    if ( nrmdls <= 0 )
	setEmpty();
    else if ( nrmdls < oldsz )
	removeRange( nrmdls, oldsz );
    else // nrmdls > oldsz
    {
	while ( size() < nrmdls )
	    add( new ElasticModel );
    }

    return true;
}


bool ElasticModelSet::getTimeSampling( Interval<float>& timerg,
				       bool usevs ) const
{
    if ( isEmpty() )
	return false;

    timerg.set( mUdf(float), -mUdf(float) );
    for ( const auto* model : *this )
    {
	const Interval<float> tsampling = model->getTimeSampling( usevs );
	timerg.include( tsampling, false );
    }

    return !timerg.isUdf();
}
