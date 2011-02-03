/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uiprestackanglemute.cc,v 1.7 2011-02-03 21:49:03 cvsyuancheng Exp $";

#include "uiprestackanglemute.h"

#include "uiprestackprocessor.h"
#include "prestackanglemute.h"
#include "raytrace1d.h"
#include "survinfo.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uiraytrace1d.h"
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
   
    raytracerfld_ = new uiRayTracer1D( this, true, false, &rt->getSetup() );
    raytracerfld_->attach( alignedBelow, velfuncsel_ );

    topfld_ = new uiGenInput( this, "Mute type",
	    BoolInpSpec(!processor_->isTailMute(),"Outer","Inner") );
    topfld_->attach( alignedBelow, raytracerfld_ );
    cutofffld_ = new uiGenInput( this, "Mute cutoff angle (degree)", 
	    FloatInpSpec(false) );
    cutofffld_->attach( alignedBelow, topfld_ );
    cutofffld_->setValue( rt->muteCutoff() );
    taperlenfld_ = new uiGenInput( this, "Taper length (samples)",
	    FloatInpSpec(processor_->taperLength()) );
    taperlenfld_->attach( alignedBelow, cutofffld_ );
    
}


bool uiAngleMute::acceptOK(CallBacker*)
{
    RayTracer1D::Setup rsetup;
    if ( !raytracerfld_->fill( rsetup ) )
	return false;

    processor_->setSetup( rsetup );

    processor_->setTaperLength( taperlenfld_->getfValue() );
    processor_->setTailMute( !topfld_->getBoolValue() );
    processor_->setMuteCutoff( cutofffld_->getfValue() );
    processor_->setVelocityMid( velfuncsel_->key(true) );
    return true;
}


}; //namespace
