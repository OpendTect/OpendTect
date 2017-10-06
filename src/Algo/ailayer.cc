/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Arnaud Huck
Date:          5 March 2013
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


AILayer::AILayer( float thkness, float ai, float den, bool needcompthkness )
    : thickness_(thkness)
    , den_(den)
{
    const bool hasdensity = mIsValidDen( den );
    //compute vel_ using Gardner's equation vel = (den/a)^(1/(1+b))
    //with default values for C0 and C1 respectively 310 and 0.25
    vel_ = hasdensity ? ai / den : Math::PowerOf( ai/310.f, 0.8f );
    if ( !hasdensity )
	den_ = ai/vel_;

    if ( needcompthkness )
	thickness_ *= vel_ / 2.0f;
}


float AILayer::getAI() const
{
    return mIsValidVel(vel_) && mIsValidDen(den_) ? vel_ * den_ : mUdf(float);
}


bool AILayer::isOK( bool dodencheck ) const
{
    if ( !mIsValidThickness(thickness_) )
	return false;

    if ( !mIsValidVel(vel_) )
	return false;

    return dodencheck ? mIsValidDen(den_) : true;
}


bool AILayer::isValidVel() const
{ return mIsValidVel(vel_); }


bool AILayer::isValidDen() const
{ return mIsValidDen(den_); }


bool AILayer::fillDenWithVp( bool onlyinvalid )
{
    if ( onlyinvalid && mIsValidDen(den_) )
	return true;

    den_ = mCast( float, 310. * Math::PowerOf( (double)vel_, 0.25 ) );
    return mIsValidDen( den_ );
}



float getLayerDepth( const AIModel& mod, int layer )
{
    float depth = 0;
    for ( int idx=0; idx<layer+1; idx++ )
	depth += mod[idx].thickness_;

    return depth;
}


// ElasticLayer
ElasticLayer::ElasticLayer( float thkness, float pvel, float svel, float den )
    : AILayer(thkness,pvel,den)
    , svel_(svel)
{}


ElasticLayer::ElasticLayer( const AILayer& ailayer )
    : AILayer(ailayer)
    , svel_(mUdf(float))
{}


ElasticLayer::ElasticLayer( float thkness, float ai, float si, float den,
			    bool needcompthkness )
    : AILayer( thkness, ai, den, needcompthkness )
{
    svel_ = mIsValidImp(si) && mIsValidDen(den) ? si / den_ : mUdf(float);
}


float ElasticLayer::getSI() const
{
    return mIsValidVel(svel_) && mIsValidDen(den_) ? svel_ * den_ : mUdf(float);
}


bool ElasticLayer::isOK( bool dodencheck, bool dosvelcheck ) const
{
    if ( !mIsValidThickness(thickness_) )
	return false;

    if ( !mIsValidVel(vel_) )
	return false;

    if ( ( dodencheck && !mIsValidDen(den_) ) ||
	 ( dosvelcheck && !mIsValidVel(svel_) ) )
	return false;

    return true;
}


bool ElasticLayer::isValidVs() const
{ return mIsValidVel(svel_); }


bool ElasticLayer::fillVsWithVp( bool onlyinvalid )
{
    if ( onlyinvalid && mIsValidVel(svel_) )
	return true;

    svel_ = mCast( float, 0.8619 * (double)vel_ -1172. );
    return mIsValidVel( svel_ );
}



int ElasticModel::isOK( bool dodencheck, bool dosvelcheck ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const ElasticLayer& lay = (*this)[idx];
	if ( !lay.isOK(dodencheck,dosvelcheck) )
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
	ElasticLayer& lay = (*this)[idx];
	if ( !lay.isOK(false,false) )
	    mRmLay(idx)

	if ( dodencheck && !lay.isValidDen() )
	{
	    if ( !lay.fillDenWithVp(onlyinvalid) )
	    {
		mRmLay(idx)
		continue;
	    }
	}

	if ( dosvelcheck && !lay.isValidVs() )
	{
	    if ( !lay.fillVsWithVp(onlyinvalid) )
		mRmLay(idx)
	}
    }
}


void ElasticModel::interpolate( bool dovp, bool doden, bool dovs )
{
    BoolTypeSet dointerpolate;
    dointerpolate += dovp;
    dointerpolate += doden;
    dointerpolate += dovs;

    for ( int iprop=0; iprop<dointerpolate.size(); iprop++ )
    {
	if ( !dointerpolate[iprop] )
	    continue;

	BendPointBasedMathFunction<float,float> data;
	for ( int idx=0; idx<size(); idx++ )
	{
	    const ElasticLayer& layer = (*this)[idx];
	    float val = mUdf(float);
	    if ( iprop == 0 && mIsValidVel(layer.vel_) )
		val = layer.vel_;
	    else if ( iprop == 1 && mIsValidDen(layer.den_) )
		val = layer.den_;
	    else if ( iprop == 2 && mIsValidVel(layer.svel_) )
		val = layer.svel_;

	    if ( !mIsUdf(val) )
		data.add( (float)idx, val );
	}
	if ( !data.size() )
	    continue;

	for ( int idx=0; idx<size(); idx++ )
	{
	    ElasticLayer& layer = (*this)[idx];
	    float& val = iprop==0 ? layer.vel_
				  : ( iprop==1 ? layer.den_ : layer.svel_ );
	    val = data.getValue( (float)idx );
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
    ElasticLayer newlayer( mUdf(float), mUdf(float), mUdf(float), mUdf(float) );
    for ( int lidx=0; lidx<orgmodl.size(); lidx++ )
    {
	ElasticLayer curlayer = orgmodl[lidx];
	float thickness = curlayer.thickness_;
	const float pvel = curlayer.vel_;
	if ( !mIsValidThickness(thickness) || !mIsValidVel(pvel) )
	    continue;

	if ( thickness > maxthickness-cMinLayerThickness() )
	{
	    if ( !curmodel.isEmpty() )
	    {
		if ( curmodel.getUpscaledBackus(newlayer) )
		    *this += newlayer;

		totthickness = 0.f;
		curmodel.setEmpty();
	    }

	    newlayer = curlayer;
	    *this += newlayer;
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
	    curlayer.thickness_ = thicknesstoadd;
	}

	curmodel += curlayer;
	if ( lastlay )
	{
	    if ( curmodel.getUpscaledBackus(newlayer) )
		*this += newlayer;

	    totthickness = thickness;
	    curmodel.setEmpty();
	    if ( thickness > cMinLayerThickness() )
	    {
		curlayer.thickness_ = thickness;
		curmodel += curlayer;
	    }
	}
    }
    if ( totthickness > cMinLayerThickness() && !curmodel.isEmpty() )
	if ( curmodel.getUpscaledBackus(newlayer) )
	    *this += newlayer;
}


void ElasticModel::upscaleByN( int nbblock )
{
    if ( isEmpty() || nbblock < 2 )
	return;

    const ElasticModel orgmodl = *this;
    setEmpty();

    ElasticModel curmdl;
    ElasticLayer newlayer( mUdf(float), mUdf(float), mUdf(float), mUdf(float) );
    for ( int lidx=0; lidx<orgmodl.size(); lidx++ )
    {
	curmdl += orgmodl[lidx];
	if ( (lidx+1) % nbblock == 0 )
	{
	    if ( curmdl.getUpscaledBackus(newlayer) )
		*this += newlayer;

	    curmdl.setEmpty();
	}
    }

    if ( !curmdl.isEmpty() )
	if ( curmdl.getUpscaledBackus(newlayer) )
	    *this += newlayer;
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
	    blockmdl += orgmodl[lidy];

	ElasticLayer outlay( mUdf(float),mUdf(float),mUdf(float),mUdf(float) );
	if ( !blockmdl.getUpscaledBackus(outlay) )
	    continue;

	*this += outlay;
    }
}


bool ElasticModel::getUpscaledByThicknessAvg( ElasticLayer& outlay ) const
{
    if ( isEmpty() )
	return false;

    outlay.thickness_ = mUdf(float);
    outlay.vel_ = mUdf(float);
    outlay.den_ = mUdf(float);
    outlay.svel_ = mUdf(float);

    float totthickness=0.f, sonp=0.f, den=0.f, sson=0.f;
    float velpthickness=0.f, denthickness=0.f, svelthickness=0.f;
    for ( int lidx=0; lidx<size(); lidx++ )
    {
	const ElasticLayer& curlayer = (*this)[lidx];
	const float ldz = curlayer.thickness_;
	const float layinvelp = curlayer.vel_;
	const float layinden = curlayer.den_;
	const float layinsvel = curlayer.svel_;

	if ( !mIsValidThickness(ldz) || !mIsValidVel(layinvelp) )
	    continue;

	totthickness += ldz;
	sonp += ldz / layinvelp;
	velpthickness += ldz;

	if ( mIsValidDen(layinden) )
	{
	    den += layinden * ldz;
	    denthickness += ldz;
	}

	if ( mIsValidVel(layinsvel) )
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

    outlay.thickness_ = totthickness;
    outlay.vel_ = velfinal;
    if ( !mIsValidThickness(denthickness) || denthickness < mDefEpsF )
	outlay.den_ = mUdf(float);
    else
    {
	const float denfinal = den / denthickness;
	outlay.den_ = mIsValidDen(denfinal) ? denfinal : mUdf(float);
    }
    if ( !mIsValidThickness(svelthickness) || svelthickness < mDefEpsF )
	outlay.svel_ = mUdf(float);
    else
    {
	const float svelfinal = svelthickness / sson;
	outlay.svel_ = mIsValidVel(svelfinal) ? svelfinal : mUdf(float);
    }

    return true;
}


bool ElasticModel::getUpscaledBackus( ElasticLayer& outlay, float theta ) const
{
    if ( isEmpty() )
	return false;

    outlay.thickness_ = mUdf(float);
    outlay.vel_ = mUdf(float);
    outlay.den_ = mUdf(float);
    outlay.svel_ = mUdf(float);

    float totthickness=0.f, den=0.f;
    float x=0.f, y=0.f, z=0.f, u=0.f, v=0.f, w=0.f;
    for ( int lidx=0; lidx<size(); lidx++ )
    {
	const ElasticLayer& curlayer = (*this)[lidx];
	const float ldz = curlayer.thickness_;
	const float layinvelp = curlayer.vel_;
	const float layinden = curlayer.den_;
	const float layinsvel = curlayer.svel_;
	ElasticLayer tmplay( ldz, layinvelp, layinsvel, layinden );

	if ( !tmplay.isOK() )
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
    outlay.thickness_ = totthickness;
    outlay.den_ = den;

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

    outlay.vel_ = Math::Sqrt( mp2 / den );
    outlay.svel_ = Math::Sqrt( msv2 / den );

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
	const float thickness = orgmodl[lidx].thickness_;
	if ( !mIsValidThickness(thickness) )
	    continue;

	ElasticLayer newlayer = orgmodl[lidx];
	nbinsert = 1;
	if ( thickness > maxthickness - cMinLayerThickness() )
	{
	    nbinsert = mCast( int, thickness/maxthickness ) + 1;
	    newlayer.thickness_ /= (float)nbinsert;
	}
	for ( int nlidx=0; nlidx<nbinsert; nlidx++ )
	    *this += newlayer;
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
    ElasticLayer prevlayer = orgmodl[0];
    for ( int lidx=1; lidx<initialsz; lidx++ )
    {
	const ElasticLayer& curlayer = orgmodl[lidx];
	if ( mIsEqual(curlayer.vel_,prevlayer.vel_,1e-2f) &&
	     mIsEqual(curlayer.den_,prevlayer.den_,1e-2f) &&
	     mIsEqual(curlayer.svel_,prevlayer.svel_,1e-2f) )
	{
	    if ( havemerges == false ) totthickness = prevlayer.thickness_;
	    havemerges = true;
	    totthickness += curlayer.thickness_;
	}
	else
	{
	    if ( havemerges )
	    {
		prevlayer.thickness_ = totthickness;
		havemerges = false;
	    }

	    *this += prevlayer;
	    prevlayer = curlayer;
	}
    }
    if ( havemerges )
	prevlayer.thickness_ = totthickness;

    *this += prevlayer;
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

    const float firstsvel = svel ? ( zinfeet ? svel[firstidx] * mFromFeetFactorF
					     : svel[firstidx] )
				 : mUdf(float);
    const ElasticLayer firstlayer( firstlayerthickness, firstvel, firstsvel,
				   den ? den[firstidx] : mUdf(float));
    *this += firstlayer;

    for ( int idx=firstidx+1; idx<zsize; idx++ )
    {
	const float velp = zinfeet ? pvel[idx] * mFromFeetFactorF : pvel[idx];
	const float layerthickness = zit ? zrange.step * velp / 2.0f
				 : ( zinfeet ? zrange.step * mFromFeetFactorF
					     : zrange.step );

	const float vels = svel ? ( zinfeet ? svel[idx] * mFromFeetFactorF
					    : svel[idx] )
				: mUdf(float);
	const ElasticLayer elayer( layerthickness, velp, vels,
				   den ? den[idx] : mUdf(float) );
	*this += elayer;
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

    const ElasticLayer firstlayer( firstlayerthickness, ai[firstidx],
				   si ? si[firstidx] : mUdf(float),
				   den ? den[firstidx] : mUdf(float), zit );
    *this += firstlayer;

    for ( int idx=firstidx+1; idx<zsize; idx++ )
    {
	const ElasticLayer elayer( zrange.step, ai[idx],
				   si ? si[idx] : mUdf(float),
				   den ? den[idx] : mUdf(float), zit );

	*this += elayer;
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
	const float layervel = (*this)[idx].vel_;
	const float layerthickness = (*this)[idx].thickness_;
	const float layertwtthickness = 2.f * layerthickness / layervel;
	if ( ( zistime && !mIsEqual(layertwtthickness,zrgstep,1e-2f) ) ||
	     (!zistime && !mIsEqual(layerthickness,zrgstep,1e-2f) ) )
	    continue;

	const float velabove = (*this)[idx-1].vel_;
	const float velbelow = (*this)[idx+1].vel_;
	const float layerthicknessabove = (*this)[idx-1].thickness_;
	const float layerthicknessbelow = (*this)[idx+1].thickness_;
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
	(*this)[idx-1].thickness_ += twtabove * velabove / 2.f;
	(*this)[idx+1].thickness_ += twtbelow * velbelow / 2.f;
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
	const ElasticLayer& layer = (*this)[idx];
	const float val = isden ? layer.den_
				: ( issvel ? layer.svel_ : layer.vel_ );
	const bool isvalid = isden ? mIsValidDen(val) : mIsValidVel(val);
	if ( !isvalid )
	    return false;

	vals += val;
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
			      Array2DImpl<float>& vals) const
{
    const int sz = size();
    TypeSet<float> velvals, denvals, svelvals;

    int icomp = 0;
    mGetVals(vel,false,false,velvals);
    mGetVals(den,true,false,denvals);
    mGetVals(svel,false,true,svelvals);

    if ( !vals.setSize(icomp,sz) )
	return false;

    icomp = 0;
    mFillArr(vel,velvals);
    mFillArr(den,denvals);
    mFillArr(svel,svelvals);

    return true;
}

#define mRet(act) \
{ \
    if ( !hasvals ) delete vals; \
    act; \
}

bool ElasticModel::getRatioValues( bool vel, bool den, bool svel,
				   Array2DImpl<float>& ratiovals,
				   Array2DImpl<float>* vals ) const
{
    const int sz = size();
    bool hasvals = vals;
    if ( !hasvals )
	vals = new Array2DImpl<float> ( 1, sz );

    if ( !getValues(vel,den,svel,*vals) )
	mRet(return false)

    const int nrcomp = vals->info().getSize( 0 );
    if ( nrcomp == 0 || !ratiovals.setSize(nrcomp,sz) )
	mRet(return false)

    for ( int icomp=0; icomp<nrcomp; icomp++ )
    {
	float prevval = vals->get( icomp, 0 );
	ratiovals.set( icomp, 0, 0.f );
	for ( int idx=1; idx<sz; idx++ )
	{
	    const float curval = vals->get( icomp, idx );
	    const float val = curval < prevval
			    ? prevval / curval - 1.f
			    : curval / prevval - 1.f;
	    ratiovals.set( icomp, idx, val );
	    prevval = curval;
	}
    }

    mRet(return true)

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
    if ( !getRatioValues(true,!pvelonly,!pvelonly,ratiovals,&vals) )
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
    float depth = 0;
    if ( ilayer >= size() )
	ilayer = size();

    for ( int idx=0; idx<ilayer; idx++ )
	depth += (*this)[idx].thickness_;

    if ( ilayer < size() )
	depth += (*this)[ilayer].thickness_ / 2.f;

    return depth;
}


bool ElasticModel::getTimeSampling( const TypeSet<ElasticModel>& models,
				    Interval<float>& timerg, bool usevs )
{
    if ( models.isEmpty() )
	return false;

    timerg.set( mUdf(float), -mUdf(float) );
    for ( int imod=0; imod<models.size(); imod++ )
    {
	if ( !models.validIdx(imod) )
	    continue;

	const ElasticModel& model = models[imod];
	Interval<float> tsampling;
	model.getTimeSampling( tsampling, usevs );
	timerg.include( tsampling, false );
    }

    return !timerg.isUdf();
}


void ElasticModel::getTimeSampling( Interval<float>& timerg, bool usevs ) const
{
    timerg.set( 0.f, 0.f );
    for ( int ilay=0; ilay<size(); ilay++ )
    {
	const ElasticLayer& layer = (*this)[ilay];
	if ( !layer.isOK(false,usevs) )
	    continue;

	const float vel = usevs ? layer.svel_ : layer.vel_;
	timerg.stop += layer.thickness_ / vel;
    }

    timerg.stop *= 2.f;
}
