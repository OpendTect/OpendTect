
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
#include "uimsg.h"
#include "uigeninput.h"
#include "uizrangeinput.h"

using namespace Vel;


void uiLinearVelTransform::initClass()
{
    uiZAxisTransform::factory().addCreator( create, "Linear velocity" );
}


uiZAxisTransform* uiLinearVelTransform::create( uiParent* p,
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
    : uiZAxisTransform( p )
    , t2d_( t2d )
    , rangefld_( 0 )
    , rangechanged_( false )
{
    BufferString velfldlbl( VelocityDesc::toString(VelocityDesc::Interval), " ",
			    VelocityDesc::getVelUnit(true) );

    velfld_ = new uiGenInput( this, velfldlbl.buf(),
			     FloatInpSpec(SI().zInFeet() ? 6000.f : 2000.f ) );
    mAttachCB( velfld_->valuechanging, uiLinearVelTransform::velChangedCB );
    
   
    gradientfld_ = new uiGenInput( this, "Gradient (1/s)", FloatInpSpec(0) );
    gradientfld_->attach( alignedBelow, velfld_ );
    mAttachCB( gradientfld_->valuechanging, uiLinearVelTransform::velChangedCB);
    setHAlignObj( gradientfld_ );
}


void uiLinearVelTransform::enableTargetSampling()
{
    if ( rangefld_ )
	return;
    
    if ( finalised() )
    {
	pErrMsg("You're to late");
	return;
    }
    
    rangefld_ = new uiZRangeInput( this, t2d_, true );
    rangefld_->attach( alignedBelow, gradientfld_ );
    mAttachCB( finaliseDone, uiLinearVelTransform::finalizeDoneCB );
    
    velChangedCB( 0 );
}


void uiLinearVelTransform::finalizeDoneCB(CallBacker*)
{
    mAttachCB( rangefld_->valuechanging, uiLinearVelTransform::rangeChangedCB );
}


void uiLinearVelTransform::rangeChangedCB(CallBacker*)
{
    rangechanged_ = true;
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


bool uiLinearVelTransform::getTargetSampling( StepInterval<float>& res ) const
{
    res = rangefld_->getFZRange();
    return true;
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
	uiMSG().error("Velocity is not set");
	return false;
    }
    
    const float gradient = gradientfld_->getfValue();
    if ( mIsUdf(gradient) )
    {
	uiMSG().error("Gradient is not set");
	return false;
    }
    
    if ( rangefld_ )
    {
	const StepInterval<float> range = rangefld_->getFZRange();
	if ( range.isUdf() )
	{
	    uiMSG().error( "Z-Range is not set" );
	    return false;
	}
    }
    
    return true;
}

