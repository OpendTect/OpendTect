/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uilinearveltrans.h"

#include "separstr.h"
#include "timedepthconv.h"
#include "unitofmeasure.h"
#include "zdomain.h"

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
						const char* domainstr,
						const char* /**/)
{
    const FileMultiString fms( domainstr );
    if ( fms.isEmpty() || fms.size() < 2 )
	return nullptr;

    const BufferString fromdomain = fms[0];
    const BufferString todomain = fms[1];

    if ( fromdomain == ZDomain::sKeyTime() &&
					todomain == ZDomain::sKeyDepth() )
	return new uiLinearVelTransform( p, true );
    else if ( fromdomain == ZDomain::sKeyDepth() &&
					    todomain == ZDomain::sKeyTime() )
	return new uiLinearVelTransform( p, false );

    return nullptr;
}


uiLinearVelTransform::uiLinearVelTransform( uiParent* p, bool t2d )
    : uiTime2DepthZTransformBase( p, t2d )
{
    const uiString velfldlbl( OD::toUiString(OD::VelocityType::Interval) );
    velfld_ = new uiConstantVel( this, getGUIDefaultVelocity(), velfldlbl );
    mAttachCB( velfld_->valueChanging, uiLinearVelTransform::velChangedCB );

    // gradient is always Vel/depth, thus always in m/s/m == 1/s
    const uiString zunitlbl = UnitOfMeasure::surveyDefZUnitAnnot(true,false);
    gradientfld_ = new uiGenInput( this, tr("Gradient (1/s)"), FloatInpSpec(0));
    gradientfld_->attach( rightTo, velfld_ );
    mAttachCB( gradientfld_->valueChanging, uiLinearVelTransform::velChangedCB);

    setHAlignObj( velfld_ );
    mAttachCB( postFinalize(), uiLinearVelTransform::initGrpCB );
}


uiLinearVelTransform::~uiLinearVelTransform()
{
    detachAllNotifiers();
}


void uiLinearVelTransform::initGrpCB( CallBacker* )
{
    velChangedCB( nullptr );
    uiTime2DepthZTransformBase::finalizeDoneCB( nullptr );
}


void uiLinearVelTransform::velChangedCB( CallBacker* )
{
    if ( !rangefld_ )
	return;

    if ( !rangechanged_ )
    {
	ZSampling range = ZSampling::udf();
	ConstRefMan<ZAxisTransform> trans = getSelection();
	if ( trans )
	{
	    range.setFrom( trans->getZInterval( false ) );
	    range.step = trans->getGoodZStep();
	}

	NotifyStopper stopper( rangefld_->valuechanging );
	rangefld_->setZRange( range );
    }
}


StringView uiLinearVelTransform::toDomain() const
{
    return isTimeToDepth() ? ZDomain::sKeyDepth() : ZDomain::sKeyTime();
}


StringView uiLinearVelTransform::fromDomain() const
{
    return isTimeToDepth() ? ZDomain::sKeyTime() : ZDomain::sKeyDepth();
}


ZAxisTransform*	uiLinearVelTransform::getSelection()
{
    const double vel = velfld_->getDValue();
    if ( mIsUdf(vel) )
	return nullptr;

    const double gradient = gradientfld_->getDValue();
    if ( mIsUdf(gradient) )
	return nullptr;

    if ( isTimeToDepth() )
	return new LinearT2DTransformNew( vel, gradient );
    else
	return new LinearD2TTransformNew( vel, gradient );
}


bool uiLinearVelTransform::acceptOK()
{
    const double vel = velfld_->getDValue();
    if ( mIsUdf(vel) )
    {
	uiMSG().error(tr("Velocity is not set"));
	return false;
    }

    const double gradient = gradientfld_->getDValue();
    if ( mIsUdf(gradient) )
    {
	uiMSG().error(tr("Gradient is not set"));
	return false;
    }

    if ( rangefld_ )
    {
	const ZSampling range = rangefld_->getFZRange();
	if ( range.isUdf() )
	{
	    uiMSG().error( tr("Z-Range is not set") );
	    return false;
	}
    }

    return true;
}

} // namespace Vel
