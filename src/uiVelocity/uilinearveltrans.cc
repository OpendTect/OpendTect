/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uilinearveltrans.h"

#include "timedepthconv.h"
#include "zdomain.h"
#include "survinfo.h"
#include "binidvalue.h"

#include "uiconstvel.h"
#include "uimsg.h"
#include "uigeninput.h"
#include "uizrangeinput.h"

namespace Vel
{

void uiLinearVelTransform::initClass()
{
    uiZAxisTransform::factory().addCreator( createInstance, "Linear velocity" );
}


uiZAxisTransform* uiLinearVelTransform::createInstance( uiParent* p,
					        const char* fromdomain,
						const char* todomain )
{
    if ( !fromdomain || !todomain )
	return 0;

    if ( fromdomain==ZDomain::sKeyTime() && todomain==ZDomain::sKeyDepth() )
	return new uiLinearVelTransform( p, true );
    else if ( fromdomain==ZDomain::sKeyDepth() && todomain==ZDomain::sKeyTime())
	return new uiLinearVelTransform( p, false );

    return 0;
}


uiLinearVelTransform::uiLinearVelTransform( uiParent* p, bool t2d )
    : uiTime2DepthZTransformBase( p, t2d )
{
    const uiString velfldlbl( VelocityDesc::toUiString(VelocityDesc::Interval));
    velfld_ = new uiConstantVel( this, Vel::getGUIDefaultVelocity(), velfldlbl);
    mAttachCB( velfld_->valueChanging, uiLinearVelTransform::velChangedCB );

    gradientfld_ = new uiGenInput( this, tr("Gradient (1/s)"), FloatInpSpec(0));
    gradientfld_->attach( rightTo, velfld_ );
    mAttachCB( gradientfld_->valueChanging, uiLinearVelTransform::velChangedCB);

    setHAlignObj( velfld_ );
    postFinalize().notify( mCB(this,uiLinearVelTransform,velChangedCB) );
}


uiLinearVelTransform::~uiLinearVelTransform()
{
    detachAllNotifiers();
}


void uiLinearVelTransform::velChangedCB( CallBacker* )
{
    if ( !rangefld_ )
	return;

    if ( !rangechanged_ )
    {
	StepInterval<float> range( StepInterval<float>::udf() );
	RefMan<ZAxisTransform> trans = getSelection();
	if ( trans )
	{
	    range = trans->getZInterval( false );
	    range.step = trans->getGoodZStep();
	    if ( range.isUdf() ) range.setUdf();
	}

	NotifyStopper stopper( rangefld_->valueChanging );
	rangefld_->setZRange( range );
    }
}


StringView uiLinearVelTransform::toDomain() const
{
    return t2d_ ? ZDomain::sKeyDepth() : ZDomain::sKeyTime();
}


StringView uiLinearVelTransform::fromDomain() const
{
    return t2d_ ? ZDomain::sKeyTime() : ZDomain::sKeyDepth();
}


ZAxisTransform*	uiLinearVelTransform::getSelection()
{
    const float vel = velfld_->getFValue();
    if ( mIsUdf(vel) )
	return 0;

    const float gradient = gradientfld_->getFValue();
    if ( mIsUdf(gradient) )
	return 0;

    if ( t2d_ )
	return new LinearT2DTransform( vel, gradient );

    return new LinearD2TTransform( vel, gradient );
}


bool uiLinearVelTransform::acceptOK()
{
    const float vel = velfld_->getFValue();
    if ( mIsUdf(vel) )
    {
	uiMSG().error(tr("Velocity is not set"));
	return false;
    }

    const float gradient = gradientfld_->getFValue();
    if ( mIsUdf(gradient) )
    {
	uiMSG().error(tr("Gradient is not set"));
	return false;
    }

    if ( rangefld_ )
    {
	const StepInterval<float> range = rangefld_->getFZRange();
	if ( range.isUdf() )
	{
	    uiMSG().error( tr("Z-Range is not set") );
	    return false;
	}
    }

    return true;
}

} // namespace Vel
