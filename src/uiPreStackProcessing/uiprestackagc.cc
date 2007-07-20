/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uiprestackagc.cc,v 1.2 2007-07-20 19:36:04 cvskris Exp $";

#include "uiprestackagc.h"

#include "uiprestackprocessor.h"
#include "prestackagc.h"
#include "uigeninput.h"
#include "uimsg.h"

namespace PreStack
{

void uiAGC::initClass()
{
    uiPSPD().addCreator( create, AGC::sName() );
}


uiDialog* uiAGC::create( uiParent* p, Processor* sgp )
{
    mDynamicCastGet( AGC*, sgagc, sgp );
    if ( !sgagc ) return 0;

    return new uiAGC( p, sgagc );
}


uiAGC::uiAGC( uiParent* p,AGC* sgagc )
    : uiDialog( p, uiDialog::Setup("AGC setup",0,"dgb:104.2.0") )
    , processor_( sgagc )
{
    windowfld_ = new uiGenInput( this, "Window width",
			     FloatInpSpec(processor_->getWindow().width() ));
    lowenergymute_ = new uiGenInput( this, "Low energy mute", FloatInpSpec() );
    lowenergymute_->attach( alignedBelow, windowfld_ );
    lowenergymute_->setValue( processor_->getLowEnergyMute() );
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
    processor_->setLowEnergyMute( lowenergymute_->getfValue() );
    return true;
}



}; //namespace
