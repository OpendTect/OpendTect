/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Arnaud Huck
Date:          5 March 2013
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "ailayer.h"

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


void upscaleElasticModel( const ElasticModel& inmdl, ElasticModel& oumdl,
			  float maxthickness )
{
    float thickness = 0.f;
    float pvel = 0.f; float pvelthickness = 0.f;
    float den = 0.f; float denthickness = 0.f;
    float vels = 0.f; float velsthickness = 0.f;
    for ( int idx=0; idx<inmdl.size(); idx++ )
    {
	float ldz = inmdl[idx].thickness_;
	if ( mIsUdf(ldz) || mIsZero(ldz,1e-2) )
	    continue;

	thickness += ldz;
	if ( thickness > maxthickness && ldz < maxthickness-1e-2 )
	{
	    ldz -= thickness - maxthickness;
	    thickness = maxthickness;
	}

	if ( !mIsUdf(inmdl[idx].vel_) )
	{
	    pvel += inmdl[idx].vel_ * ldz;
	    pvelthickness += ldz;
	}
	if ( !mIsUdf(inmdl[idx].den_) )
	{
	    den  += inmdl[idx].den_ * ldz;
	    denthickness += ldz;
	}
	if ( !mIsUdf(inmdl[idx].svel_) )
	{
	    vels += inmdl[idx].svel_ * ldz;
	    velsthickness += ldz;
	}

	if ( thickness >= maxthickness-1e-2 )
	{
	    pvel = mIsZero(pvel, 1e-2) ? mUdf(float) : pvel/pvelthickness;
	    den = mIsZero(den, 1e-2) ? mUdf(float) : den/denthickness;
	    vels = mIsZero(vels, 1e-2) ? mUdf(float) : vels/velsthickness;
	    oumdl += ElasticLayer( thickness, pvel, vels, den );

	    ldz = inmdl[idx].thickness_ - ldz;
	    thickness = ldz;
	    pvel = !mIsUdf(inmdl[idx].vel_) ? inmdl[idx].vel_ * ldz : 0.f;
	    pvelthickness = !mIsUdf(inmdl[idx].vel_) ? ldz : 0.f;
	    den = !mIsUdf(inmdl[idx].den_) ? inmdl[idx].den_ * ldz : 0.f;
	    denthickness = !mIsUdf(inmdl[idx].den_) ? ldz : 0.f;
	    vels = !mIsUdf(inmdl[idx].svel_) ? inmdl[idx].svel_ * ldz : 0.f;
	    velsthickness = !mIsUdf(inmdl[idx].svel_) ? ldz : 0.f;
	}
    }
    if ( !mIsZero(thickness,1e-2) )
    {
	pvel = mIsZero(pvel, 1e-2) ? mUdf(float) : pvel/pvelthickness;
	den = mIsZero(den, 1e-2) ? mUdf(float) : den/denthickness;
	vels = mIsZero(vels, 1e-2) ? mUdf(float) : vels/velsthickness;
	oumdl += ElasticLayer( thickness, pvel, vels, den );
    }
}


void upscaleElasticModelByN( const ElasticModel& inmdl, ElasticModel& oumdl,
       			     int nblock )
{
    float thickness = 0.f;
    float pvel = 0.f; float pvelthickness = 0.f;
    float den = 0.f; float denthickness = 0.f;
    float vels = 0.f; float velsthickness = 0.f;
    for ( int idx=0; idx<inmdl.size(); idx++ )
    {
	const float ldz = inmdl[idx].thickness_;
	if ( mIsUdf(ldz) || mIsZero(ldz,1e-2) )
	    continue;

	thickness += ldz;
	if ( !mIsUdf(inmdl[idx].vel_) )
	{
	    pvel += inmdl[idx].vel_ * ldz;
	    pvelthickness += ldz;
	}
	if ( !mIsUdf(inmdl[idx].den_) )
	{
	    den  += inmdl[idx].den_ * ldz;
	    denthickness += ldz;
	}
	if ( !mIsUdf(inmdl[idx].svel_) )
	{
	    vels += inmdl[idx].svel_ * ldz;
	    velsthickness += ldz;
	}
	if ( (idx+1) % nblock == 0 )
	{
	    pvel = mIsZero(pvel, 1e-2) ? mUdf(float) : pvel/pvelthickness;
	    den = mIsZero(den, 1e-2) ? mUdf(float) : den/denthickness;
	    vels = mIsZero(vels, 1e-2) ? mUdf(float) : vels/velsthickness;
	    oumdl += ElasticLayer( thickness, pvel, vels, den );
	    thickness = 0.f; pvel = 0.f; den = 0.f; vels = 0.f;
	    pvelthickness = 0.f; denthickness = 0.f; velsthickness = 0.f;
	}
    }
    if ( !mIsZero(thickness,1e-2) )
    {
	pvel = mIsZero(pvel, 1e-2) ? mUdf(float) : pvel/pvelthickness;
	den = mIsZero(den, 1e-2) ? mUdf(float) : den/denthickness;
	vels = mIsZero(vels, 1e-2) ? mUdf(float) : vels/velsthickness;
	oumdl += ElasticLayer( thickness, pvel, vels, den );
    }
}

