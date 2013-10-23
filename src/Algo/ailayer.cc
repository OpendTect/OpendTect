/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Arnaud Huck
Date:          5 March 2013
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "ailayer.h"
#include "math2.h"
#include "ranges.h"

float AILayer::getAI() const
{
    return !mIsUdf(vel_)&&!mIsUdf(den_) ? vel_ * den_ : mUdf(float);
}


float getLayerDepth( const AIModel& mod, int layer )
{
    float depth = 0;
    for ( int idx=0; idx<layer+1; idx++ )
	depth += mod[idx].thickness_;

    return depth;
}


float ElasticLayer::getSI() const
{
    return !mIsUdf(svel_)&&!mIsUdf(den_) ? svel_ * den_ : mUdf(float);
}


#define mDefEpsf 1e-3f
#define mIsValid(val) ( !mIsUdf(val) && val > mDefEpsF )


void ElasticModel::upscale( float maxthickness )
{
    if ( isEmpty() || maxthickness < mDefEpsf )
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
	if ( !mIsValid(thickness) || !mIsValid(pvel) )
	    continue;

	if ( thickness > maxthickness-mDefEpsf )
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

	const bool lastlay = totthickness+thickness > maxthickness-mDefEpsf;
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
	    if ( thickness>mDefEpsf )
	    {
		curlayer.thickness_ = thickness;
		curmodel += curlayer;
	    }
	}
    }
    if ( totthickness>mDefEpsf && !curmodel.isEmpty() )
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


#define mAddBlock( block ) \
    if ( !blocks.isEmpty() && block.start<blocks[0].start ) \
	blocks.insert( 0, block ); \
    else \
	blocks += block


void ElasticModel::block( float relthreshold, bool pvelonly )
{
    if ( isEmpty() || relthreshold < 0 || relthreshold > 1 )
	return;

    const ElasticModel orgmodl = *this;
    setEmpty();
    const int modelsize = orgmodl.size();

#define mVal(comp,lidx) values[comp*modelsize+lidx]

    TypeSet<float> values( modelsize, mUdf(float) );
    for ( int lidx=0; lidx<modelsize; lidx++ )
    {
	const float pvel = orgmodl[lidx].vel_;
	values[lidx] = pvel;
	if ( !mIsValid(pvel) )
	    return;
    }

    int nrcomp = 1;
    if ( !pvelonly )
    {
	bool dodensity = true;
	TypeSet<float> denvals( modelsize, mUdf(float) );
	for ( int lidx=0; lidx<modelsize; lidx++ )
	{
	    const float den = orgmodl[lidx].den_;
	    denvals += den;
	    if ( !mIsValid(den) )
	    {
		dodensity = false;
		break;
	    }
	}
	
	if ( dodensity )
	{
	    values.append( denvals );
	    nrcomp++;
	}

	bool dosvel = true;
	TypeSet<float> svals( modelsize, mUdf(float) );
	for ( int lidx=0; lidx<modelsize; lidx++ )
	{
	    const float svel = orgmodl[lidx].svel_;
	    svals[lidx] = svel;
	    if ( !mIsValid(svel) )
	    {
		dosvel = false;
		break;
	    }
	}
	if ( dosvel )
	{
	    values.append( svals );
	    nrcomp++;
	}
    }

#define mValRatio(comp,lidx) valuesratio[comp*modelsize+lidx]

    TypeSet<float> valuesratio = values;
    for ( int icomp=0; icomp<nrcomp; icomp++ )
    {
	mValRatio( icomp, 0 ) = 0;
	const float* compvals = &values[icomp*modelsize];
	float prevval = *compvals;
	for ( int lidx=1; lidx<modelsize; lidx++ )
	{
	    const float curval = compvals[lidx];
	    mValRatio( icomp, lidx) = curval < prevval
				    ? prevval / curval - 1.f
				    : curval / prevval - 1.f;
	    prevval = curval;
	}
    }

    TypeSet<Interval<int> > investigationqueue;
    investigationqueue += Interval<int>( 0, modelsize-1 );
    TypeSet<Interval<int> > blocks;
    
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
		firstval[icomp] = mVal( icomp, curblock.start );
		Interval<float> valrange(  firstval[icomp],  firstval[icomp] );
		valranges += valrange;
	    }

	    for ( int lidx=curblock.start+1; lidx<=lastblock; lidx++ )
	    {
		for ( int icomp=0; icomp<nrcomp; icomp++ )
		{
		    const float curval = mVal( icomp, lidx );
		    valranges[icomp].include( curval );

		    const float valratio = mValRatio( icomp, lidx );
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
		pFreeFnErrMsg("Should never happen", "ElasticModel::block");
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
    
    for ( int lidx=0; lidx<blocks.size(); lidx++ )
    {
	ElasticModel blockmdl;
	ElasticLayer outlay( mUdf(float),mUdf(float),mUdf(float),mUdf(float) );
	const Interval<int> curblock = blocks[lidx];
	for ( int lidy=curblock.start; lidy<=curblock.stop; lidy++ )
	    blockmdl += orgmodl[lidy];

	if ( blockmdl.getUpscaledBackus(outlay) )
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

	if ( !mIsValid(ldz) || !mIsValid(layinvelp) )
	    continue;

	totthickness += ldz;
	sonp += ldz / layinvelp;
	velpthickness += ldz;

	if ( mIsValid(layinden) )
	{
	    den += layinden * ldz;
	    denthickness += ldz;
	}

	if ( mIsValid(layinsvel) )
	{
	    sson += ldz / layinsvel;
	    svelthickness += ldz;
	}
    }

    if ( totthickness<mDefEpsF || velpthickness<mDefEpsF || sonp<1e-6f )
	return false;

    outlay.thickness_ = totthickness;
    outlay.vel_ = velpthickness / sonp;
    outlay.den_ = !mIsValid(denthickness) ? mUdf(float) : den / denthickness;
    outlay.svel_ = !mIsValid(svelthickness) || !mIsValid(sson)
		 ? mUdf(float): svelthickness / sson;

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
	
	if ( !mIsValid(ldz) || !mIsValid(layinvelp) ||
	     !mIsValid(layinden) || !mIsValid(layinsvel) )
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
    if ( isEmpty() || maxthickness < mDefEpsf )
	return;

    const ElasticModel orgmodl = *this;
    setEmpty();
    const int initialsz = orgmodl.size();
    int nbinsert = mUdf(int);
    for ( int lidx=0; lidx<initialsz; lidx++ )
    {
	const float thickness = orgmodl[lidx].thickness_;
	if ( !mIsValid(thickness) )
	    continue;

	ElasticLayer newlayer = orgmodl[lidx];
	nbinsert = 1;
	if ( thickness > maxthickness-mDefEpsf )
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
	if ( mIsEqual(curlayer.vel_,prevlayer.vel_,mDefEpsf) &&
	     mIsEqual(curlayer.den_,prevlayer.den_,mDefEpsf) &&
	     mIsEqual(curlayer.svel_,prevlayer.svel_,mDefEpsf) )
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
    *this += prevlayer;
}


