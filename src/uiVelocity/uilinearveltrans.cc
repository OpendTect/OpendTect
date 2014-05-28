
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Aug 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uilinearveltrans.h"
#include "timedepthconv.h"
#include "zdomain.h"
#include "survinfo.h"
#include "binidvalue.h"
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
    BufferString velfldlbl( VelocityDesc::toString(VelocityDesc::Interval), " ",
			    VelocityDesc::getVelUnit(true) );

    velfld_ = new uiGenInput( this, velfldlbl.buf(),
			     FloatInpSpec(SI().zInFeet() ? 6000.f : 2000.f ) );
    mAttachCB( velfld_->valuechanging, uiLinearVelTransform::velChangedCB );


    gradientfld_ = new uiGenInput( this, tr("Gradient (1/s)"), FloatInpSpec(0));
    gradientfld_->attach( alignedBelow, velfld_ );
    mAttachCB( gradientfld_->valuechanging, uiLinearVelTransform::velChangedCB);
    setHAlignObj( gradientfld_ );
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
	    range = SI().zRange( true );
	    const int nrsamples = range.nrSteps()>0 ? range.nrSteps() : 1;
	    range.start = trans->transform( BinIDValue(0,0,range.start) );
	    range.stop = trans->transform( BinIDValue(0,0,range.stop) );
	    if ( range.isUdf() )
		range = StepInterval<float>::udf();
	    else
		range.step = range.width()/nrsamples;
	}

	NotifyStopper stopper( rangefld_->valuechanging );
	rangefld_->setZRange( range );
    }
}


FixedString uiLinearVelTransform::toDomain() const
{
    return t2d_ ? ZDomain::sKeyDepth() : ZDomain::sKeyTime();
}


FixedString uiLinearVelTransform::fromDomain() const
{
    return t2d_ ? ZDomain::sKeyTime() : ZDomain::sKeyDepth();
}


ZAxisTransform*	uiLinearVelTransform::getSelection()
{
    const float vel = velfld_->getfValue();
    if ( mIsUdf(vel) )
	return 0;

    const float gradient = gradientfld_->getfValue();
    if ( mIsUdf(gradient) )
	return 0;

    if ( t2d_ )
	return new LinearT2DTransform( vel, gradient );

    return new LinearD2TTransform( vel, gradient );
}


bool uiLinearVelTransform::acceptOK()
{
    const float vel = velfld_->getfValue();
    if ( mIsUdf(vel) )
    {
	uiMSG().error(tr("Velocity is not set"));
	return false;
    }

    const float gradient = gradientfld_->getfValue();
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
