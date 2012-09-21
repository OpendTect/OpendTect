/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "uiprestackagc.h"

#include "uiprestackprocessor.h"
#include "prestackagc.h"
#include "uigeninput.h"
#include "uimsg.h"

namespace PreStack
{

void uiAGC::initClass()
{
    uiPSPD().addCreator( create, AGC::sFactoryKeyword() );
}


uiDialog* uiAGC::create( uiParent* p, Processor* sgp )
{
    mDynamicCastGet( AGC*, sgagc, sgp );
    if ( !sgagc ) return 0;

    return new uiAGC( p, sgagc );
}


uiAGC::uiAGC( uiParent* p, AGC* sgagc )
    : uiDialog( p, uiDialog::Setup("AGC setup",0,"103.2.1") )
    , processor_( sgagc )
{
    BufferString label = "Window width ";
    BufferString unit;
    processor_->getWindowUnit( unit, true );
    label += unit;
    windowfld_ = new uiGenInput( this, label.buf(),
			     FloatInpSpec(processor_->getWindow().width() ));
    lowenergymute_ = new uiGenInput( this, "Low energy mute (%)",
	    			     FloatInpSpec() );
    lowenergymute_->attach( alignedBelow, windowfld_ );
    const float lowenergymute = processor_->getLowEnergyMute();
    lowenergymute_->setValue(
	    mIsUdf(lowenergymute) ? mUdf(float) : lowenergymute*100 );
}


bool uiAGC::acceptOK( CallBacker* )
{
    if ( !processor_ ) return true;

    const float width = windowfld_->getfValue();
    if ( mIsUdf(width) )
    {
	uiMSG().error("Window width is not set");
	return false;
    }

    processor_->setWindow( Interval<float>( -width/2, width/2 ) );
    const float lowenergymute = lowenergymute_->getfValue();
    if ( mIsUdf(lowenergymute) ) processor_->setLowEnergyMute( mUdf(float) );
    else
    {
	if ( lowenergymute<0 || lowenergymute>99 )
	{
	    uiMSG().error("Low energy mute must be between 0 and 99");
	    return false;
	}

	processor_->setLowEnergyMute( lowenergymute/100 );
    }

    return true;
}

} // namespace PreStack
