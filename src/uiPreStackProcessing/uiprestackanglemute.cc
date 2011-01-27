/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uiprestackanglemute.cc,v 1.2 2011-01-27 15:25:57 cvsyuancheng Exp $";

#include "uiprestackanglemute.h"

#include "uiprestackprocessor.h"
#include "prestackanglemute.h"
#include "raytrace1d.h"
#include "survinfo.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiveldesc.h"


namespace PreStack
{

void uiAngleMute::initClass()
{
    uiPSPD().addCreator( create, AngleMute::sFactoryKeyword() );
}


uiDialog* uiAngleMute::create( uiParent* p, Processor* sgp )
{
    mDynamicCastGet( AngleMute*, sgmute, sgp );
    if ( !sgmute ) return 0;

    return new uiAngleMute( p, sgmute );
}


uiAngleMute::uiAngleMute( uiParent* p, AngleMute* rt )
    : uiDialog( p, uiDialog::Setup("AngleMute setup",0,"103.2.2") )
    , processor_( rt )		      
{
   IOObjContext ctxt = uiVelSel::ioContext(); 
   velfuncsel_ = new uiVelSel( this, ctxt, uiSeisSel::Setup(Seis::Vol) );
   velfuncsel_->setLabelText( "Velocity input volume" );
   if ( !rt->velocityVolumeID().isEmpty() )
       velfuncsel_->setInput( rt->velocityVolumeID() ); 
    
    BufferString lb = "Source/Receiver depths";
    lb += SI().getZUnitString( true );
    srdepthfld_ = new uiGenInput(this, lb.buf(), FloatInpIntervalSpec(false));
    srdepthfld_->attach( alignedBelow, velfuncsel_ );
    srdepthfld_->setValue( Interval<float>(rt->angleTracer()->sourceDepth(),
		rt->angleTracer()->receiverDepth()) );
    
    topfld_ = new uiGenInput( this, "Mute type",
	    BoolInpSpec(!processor_->isTailMute(),"Outer","Inner") );
    topfld_->attach( alignedBelow, srdepthfld_ );
    cutofffld_ = new uiGenInput( this, "Mute cutoff angle (in degree)", 
	    FloatInpSpec(false) );
    cutofffld_->attach( alignedBelow, topfld_ );
    cutofffld_->setValue( rt->muteCutoff() );
    taperlenfld_ = new uiGenInput( this, "Taper length (in samples)",
	    FloatInpSpec(processor_->taperLength()) );
    taperlenfld_->attach( alignedBelow, cutofffld_ );
    
}


bool uiAngleMute::acceptOK(CallBacker*)
{
    processor_->angleTracer()->setSourceDepth( srdepthfld_->getfValue(0) );
    processor_->angleTracer()->setReceiverDepth( srdepthfld_->getfValue(1));
    processor_->setTaperLength( taperlenfld_->getfValue() );
    processor_->setTailMute( !topfld_->getBoolValue() );
    processor_->setMuteCutoff( cutofffld_->getfValue() );
    processor_->setVelocityMid( velfuncsel_->key(true) );
    return true;
}


}; //namespace
