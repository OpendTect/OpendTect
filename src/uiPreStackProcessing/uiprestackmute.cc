/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "uiprestackmute.h"

#include "uiprestackprocessor.h"
#include "prestackmute.h"
#include "prestackmutedeftransl.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uimsg.h"

namespace PreStack
{

void uiMute::initClass()
{
    uiPSPD().addCreator( create, Mute::sFactoryKeyword() );
}


uiDialog* uiMute::create( uiParent* p, Processor* sgp )
{
    mDynamicCastGet( Mute*, sgmute, sgp );
    if ( !sgmute ) return 0;

    return new uiMute( p, sgmute );
}


uiMute::uiMute( uiParent* p, Mute* sgmute )
    : uiDialog( p, uiDialog::Setup("Mute setup",0,"103.2.2") )
    , processor_( sgmute )
    , ctio_( *mMkCtxtIOObj(MuteDef) )
{
    mutedeffld_ = new uiIOObjSel( this, ctio_ );
    topfld_ = new uiGenInput( this, "Mute type",
	    		      BoolInpSpec(true,"Outer","Inner") );
    topfld_->attach( alignedBelow, mutedeffld_ );
    taperlenfld_ = new uiGenInput( this, "Taper length (in samples)",
	    			   FloatInpSpec() );
    taperlenfld_->attach( alignedBelow, topfld_ );

    mutedeffld_->setInput( processor_->muteDefID() );
    topfld_->setValue( !processor_->isTailMute() );
    taperlenfld_->setValue( processor_->taperLength() );
}


bool uiMute::acceptOK(CallBacker*)
{
    if ( !processor_ ) return true;

    if ( mutedeffld_->isEmpty() )
	processor_->setEmptyMute();
    else if ( !mutedeffld_->commitInput() || !ctio_.ioobj )
    {
	uiMSG().error("Cannot find mute");
	return false;
    }
    else
	processor_->setMuteDefID( ctio_.ioobj->key() );

    processor_->setTaperLength( taperlenfld_->getfValue() );
    processor_->setTailMute( !topfld_->getBoolValue() );
    return true;
}


}; //namespace
