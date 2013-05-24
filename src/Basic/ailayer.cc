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


void ElasticModel::upscale( float maxthickness, ElasticModel& oumdl )
{
    ElasticModel inmdl = oumdl;
    oumdl.setEmpty();

    inmdl.setMaxThickness( maxthickness );
    float totthickness = 0.f;
    ElasticModel curmodel;
    ElasticLayer newlayer( mUdf(float), mUdf(float), mUdf(float), mUdf(float) );
    for ( int idx=0; idx<inmdl.size(); idx++ )
    {
	ElasticLayer curlay = inmdl[idx];
	float thickness = curlay.thickness_;
	const float pvel = curlay.vel_;
	if ( !mIsValid(thickness) || !mIsValid(pvel) )
	    continue;

	const bool lastlay = totthickness+thickness > maxthickness-mDefEpsf;
	const float thicknesstoadd = lastlay ? maxthickness - totthickness
	    				     : thickness;
	totthickness += thicknesstoadd;
	if ( lastlay )
	{
	    thickness -= thicknesstoadd;
	    curlay.thickness_ = thicknesstoadd;
	}

	curmodel += curlay;
	if ( lastlay )
	{
	    if ( curmodel.upscaleBackus(newlayer) )
		oumdl += newlayer;

	    totthickness = thickness;
	    curmodel.setEmpty();
	    if ( thickness>mDefEpsf )
	    {
		curlay.thickness_ = thickness;
		curmodel += curlay;
	    }
	}
    }
    if ( totthickness>mDefEpsf && !curmodel.isEmpty() )
	if ( curmodel.upscaleBackus(newlayer) )
	    oumdl += newlayer;
}


void ElasticModel::upscaleByN( int nblock, ElasticModel& oumdl )
{
    ElasticModel inmdl = oumdl;
    oumdl.setEmpty();

    ElasticModel curmdl;
    ElasticLayer newlayer( mUdf(float), mUdf(float), mUdf(float), mUdf(float) );
    for ( int idx=0; idx<inmdl.size(); idx++ )
    {
	curmdl += inmdl[idx];
	if ( (idx+1) % nblock == 0 )
	{
	    if ( curmdl.upscaleBackus(newlayer) )
		oumdl += newlayer;

	    curmdl.setEmpty();
	}
    }

    if ( !curmdl.isEmpty() )
	if ( curmdl.upscaleBackus(newlayer) )
	    oumdl += newlayer;
}


#define mAddBlock( block ) \
    if ( !blocks.isEmpty() && block.start<blocks[0].start ) \
	blocks.insert( 0, block ); \
    else \
	blocks += block


void ElasticModel::block( float relthreshold, bool pvelonly,
			  ElasticModel& oumdl )
{
    if ( oumdl.isEmpty() )
	return;

    ElasticModel inmdl = oumdl;
    oumdl.setEmpty();
    const int modelsize = inmdl.size();

#define mVal(comp,idx) values[comp*modelsize+idx]

    TypeSet<float> values( modelsize, mUdf(float) );
    for ( int idx=0; idx<modelsize; idx++ )
    {
	const float pvel = inmdl[idx].vel_;
	values[idx] = pvel;
	if ( !mIsValid(pvel) )
	    return;
    }

    int nrcomp = 1;
    if ( !pvelonly )
    {
	for ( int idx=0; idx<modelsize; idx++ )
	{
	    const float den = inmdl[idx].den_;
	    values += den;
	    if ( !mIsValid(den) )
		return;
	}

	nrcomp++;

	bool dosvel = true;
	TypeSet<float> svals( modelsize, mUdf(float) );
	for ( int idx=0; idx<modelsize; idx++ )
	{
	    const float svel = inmdl[idx].svel_;
	    svals[idx] = svel;
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

#define mValRatio(comp,idx) valuesratio[comp*modelsize+idx]

    TypeSet<float> valuesratio = values;
    for ( int icomp=0; icomp<nrcomp; icomp++ )
    {
	mValRatio( icomp, 0 ) = 0;
	const float* compvals = &values[icomp*modelsize];
	float prevval = *compvals;
	for ( int idx=1; idx<modelsize; idx++ )
	{
	    const float curval = compvals[idx];
	    mValRatio( icomp, idx) = curval < prevval
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

	    for ( int idx=curblock.start+1; idx<=lastblock; idx++ )
	    {
		for ( int icomp=0; icomp<nrcomp; icomp++ )
		{
		    const float curval = mVal( icomp, idx );
		    valranges[icomp].include( curval );

		    const float valratio = mValRatio( icomp, idx );
		    if ( valratio >= maxvalratio )
		    {
			if ( !mIsEqual(valratio,maxvalratio,1e-5f) )
			    bendpoints.erase();

			bendpoints += idx;
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
		pFreeFnErrMsg("Should never happen", "BlockElasticModel");
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
    
    for ( int idx=0; idx<blocks.size(); idx++ )
    {
	ElasticModel blockmdl;
	ElasticLayer outlay( mUdf(float),mUdf(float),mUdf(float),mUdf(float) );
	const Interval<int> curblock = blocks[idx];
	for ( int idy=curblock.start; idy<=curblock.stop; idy++ )
	    blockmdl += inmdl[idy];

	if ( blockmdl.upscaleBackus(outlay) )
	    oumdl += outlay;
    }
}


bool ElasticModel::upscaleByThicknessAvg( ElasticModel& inmdl,
					  ElasticLayer& outlay )
{
    if ( inmdl.isEmpty() )
	return false;

    outlay.thickness_ = mUdf(float);
    outlay.vel_ = mUdf(float);
    outlay.den_ = mUdf(float);
    outlay.svel_ = mUdf(float);

    float totthickness=0.f, sonp=0.f, den=0.f, sson=0.f;
    float velpthickness=0.f, denthickness=0.f, svelthickness=0.f;
    for ( int idx=0; idx<inmdl.size(); idx++ )
    {
	const ElasticLayer& layerin = inmdl[idx];
	const float ldz = layerin.thickness_;
	const float layinvelp = layerin.vel_;
	const float layinden = layerin.den_;
	const float layinsvel = layerin.svel_;

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


bool ElasticModel::upscaleBackus( ElasticModel& inmdl, ElasticLayer& outlay,
       				  float theta )
{
    if ( inmdl.isEmpty() )
	return false;

    outlay.thickness_ = mUdf(float);
    outlay.vel_ = mUdf(float);
    outlay.den_ = mUdf(float);
    outlay.svel_ = mUdf(float);

    float totthickness=0.f, den=0.f;
    float x=0.f, y=0.f, z=0.f, u=0.f, v=0.f, w=0.f;
    for ( int idx=0; idx<inmdl.size(); idx++ )
    {
	const ElasticLayer& layerin = inmdl[idx];
	const float ldz = layerin.thickness_;
	const float layinvelp = layerin.vel_;
	const float layinden = layerin.den_;
	const float layinsvel = layerin.svel_;
	
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
	return inmdl.upscaleByThicknessAvg( outlay );

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
	return inmdl.upscaleByThicknessAvg( outlay );

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


void ElasticModel::setMaxThickness( float maxthickness, ElasticModel& oumdl )
{
    if ( oumdl.isEmpty() )
	return;

    ElasticModel inmdl = oumdl;
    oumdl.setEmpty();
    const int initialsz = inmdl.size();
    int nbinsert = mUdf(int);
    for ( int lidx=0; lidx<initialsz; lidx++ )
    {
	const float thickness = inmdl[lidx].thickness_;
	ElasticLayer newlayer = inmdl[lidx];
	nbinsert = 1;
	if ( thickness > maxthickness )
	{
	    nbinsert = mCast( int, thickness/maxthickness );
	    newlayer.thickness_ /= (float)nbinsert;
	}
	for ( int nlidx=0; nlidx<nbinsert; nlidx++ )
	    oumdl += newlayer;
    }
}


