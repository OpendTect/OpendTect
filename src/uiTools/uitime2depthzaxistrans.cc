/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K Tingdahl
 Date:		Apr 2014
________________________________________________________________________

-*/

#include "uitime2depthzaxistrans.h"

#include "uizrangeinput.h"



uiTime2DepthZTransformBase::uiTime2DepthZTransformBase( uiParent* p, bool t2d )
    : uiZAxisTransform( p )
    , t2d_( t2d )
    , rangefld_( 0 )
    , rangechanged_( false )
{
    mAttachCB( postFinalize(), uiTime2DepthZTransformBase::finalizeDoneCB );
}


uiTime2DepthZTransformBase::~uiTime2DepthZTransformBase()
{
    detachAllNotifiers();
}


FixedString uiTime2DepthZTransformBase::toDomain() const
{
    return t2d_ ? ZDomain::sKeyDepth() : ZDomain::sKeyTime();
}


FixedString uiTime2DepthZTransformBase::fromDomain() const
{
    return t2d_ ? ZDomain::sKeyTime() : ZDomain::sKeyDepth();
}


void uiTime2DepthZTransformBase::enableTargetSampling()
{
    if ( rangefld_ )
	return;

    if ( !finalized() )
    {
	rangefld_ = new uiZRangeInput(this,t2d_,true);
	rangefld_->attach( alignedBelow, hAlignObj() );
    }
}


bool
uiTime2DepthZTransformBase::getTargetSampling(StepInterval<float>& res) const
{
    if ( !rangefld_ )
	return false;

    res = rangefld_->getFZRange();
    return true;
}


void uiTime2DepthZTransformBase::finalizeDoneCB(CallBacker*)
{
    if ( rangefld_ )
	mAttachCB( rangefld_->valuechanging,
		   uiTime2DepthZTransformBase::rangeChangedCB );
}
