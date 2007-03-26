/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uiprestackagc.cc,v 1.1 2007-03-26 21:12:02 cvskris Exp $";

#include "uiprestackagc.h"

#include "uiprestackprocessor.h"
#include "prestackagc.h"
#include "uigeninput.h"

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
    windowfld_ = new uiGenInput( this, "Window", FloatInpIntervalSpec(false) );
    lowenergymute_ = new uiGenInput( this, "Low energy mute", FloatInpSpec() );
    lowenergymute_->attach( alignedBelow, windowfld_ );

    windowfld_->setValue( processor_->getWindow() );
    lowenergymute_->setValue( processor_->getLowEnergyMute() );
}


bool uiAGC::acceptOK( CallBacker* )
{
    if ( !processor_ ) return true;

    processor_->setWindow( windowfld_->getFInterval() );
    processor_->setLowEnergyMute( lowenergymute_->getfValue() );
    return true;
}



}; //namespace
