/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uiprestackmute.cc,v 1.3 2008-06-04 09:12:33 cvsbert Exp $";

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
    uiPSPD().addCreator( create, Mute::sName() );
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
    , ctio_( *mGetCtxtIOObj(MuteDef,Misc) )
{
    mutedeffld_ = new uiIOObjSel( this, ctio_ );
    topfld_ = new uiGenInput( this, "Mute type",
	    		      BoolInpSpec(true,"Top","Tail") );
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
    if ( !mutedeffld_->commitInput(false) || !ctio_.ioobj )
    {
	uiMSG().error( "Please select a Mute Definition" );
	return false;
    }

    processor_->setMuteDefID( ctio_.ioobj->key() );
    processor_->setTaperLength( taperlenfld_->getfValue() );
    processor_->setTailMute( !topfld_->getBoolValue() );
    return true;
}


}; //namespace
