/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackagc.h"

#include "uiprestackprocessor.h"
#include "prestackagc.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "od_helpids.h"

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
    : uiDialog( p, uiDialog::Setup(tr("AGC setup"),mNoDlgTitle,
                                    mODHelpKey(mPreStackAGCHelpID) ) )
    , processor_( sgagc )
{
    BufferString unit;
    processor_->getWindowUnit( unit, true );
    uiString label = tr("Window width %1").arg(mToUiStringTodo(unit));
    windowfld_ = new uiGenInput( this, label,
			     FloatInpSpec(processor_->getWindow().width() ));
    lowenergymute_ = new uiGenInput( this, tr("Low energy mute (%)"),
	    			     FloatInpSpec() );
    lowenergymute_->attach( alignedBelow, windowfld_ );
    const float lowenergymute = processor_->getLowEnergyMute();
    lowenergymute_->setValue(
	    mIsUdf(lowenergymute) ? mUdf(float) : lowenergymute*100 );
}


uiAGC::~uiAGC()
{}


bool uiAGC::acceptOK( CallBacker* )
{
    if ( !processor_ ) return true;

    const float width = windowfld_->getFValue();
    if ( mIsUdf(width) )
    {
	uiMSG().error(tr("Window width is not set"));
	return false;
    }

    processor_->setWindow( Interval<float>( -width/2, width/2 ) );
    const float lowenergymute = lowenergymute_->getFValue();
    if ( mIsUdf(lowenergymute) ) processor_->setLowEnergyMute( mUdf(float) );
    else
    {
	if ( lowenergymute<0 || lowenergymute>99 )
	{
	    uiMSG().error(tr("Low energy mute must be between 0 and 99"));
	    return false;
	}

	processor_->setLowEnergyMute( lowenergymute/100 );
    }

    return true;
}

} // namespace PreStack
