/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uiprestackanglemute.cc,v 1.15 2012/04/27 07:56:49 cvsbruno Exp $";

#include "uiprestackanglemute.h"

#include "prestackanglemute.h"
#include "raytrace1d.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uiprestackprocessor.h"
#include "uiraytrace1d.h"
#include "uiseparator.h"
#include "uiveldesc.h"


namespace PreStack
{

uiAngleMuteGrp::uiAngleMuteGrp( uiParent* p, 
			AngleMuteBase::Params& pars, bool dooffset )
    : uiGroup(p,"Angle Mute Group")
    , params_(pars)  
{
    IOObjContext ctxt = uiVelSel::ioContext(); 
    velfuncsel_ = new uiVelSel( this, ctxt, uiSeisSel::Setup(Seis::Vol) );
    velfuncsel_->setLabelText( "Velocity input volume" );
    if ( !params_.velvolmid_.isUdf() )
       velfuncsel_->setInput( params_.velvolmid_ ); 

    uiSeparator* sep = new uiSeparator( this, "sep" );
    sep->attach( stretchedBelow, velfuncsel_ );
 
    uiRayTracer1D::Setup rsu; 
    rsu.dooffsets_ = dooffset; 
    rsu.doreflectivity_ = false;
    raytracerfld_ = new uiRayTracerSel( this, rsu );
    raytracerfld_->usePar( pars.raypar_ );
    raytracerfld_->attach( alignedBelow, velfuncsel_ );
    raytracerfld_->attach( ensureBelow, sep );

    cutofffld_ = new uiGenInput( this, "Mute cutoff angle (degree)", 
	    FloatInpSpec(false) );
    cutofffld_->attach( alignedBelow, raytracerfld_ );
    cutofffld_->setValue( params_.mutecutoff_ );

    blockfld_ = new uiCheckBox( this, "Block (bend points)" );
    blockfld_->attach( alignedBelow, cutofffld_ );
    blockfld_->setChecked( params_.dovelblock_ );

    setHAlignObj( cutofffld_ );
}


bool uiAngleMuteGrp::acceptOK()
{ 
    raytracerfld_->fillPar( params_.raypar_ );
    params_.mutecutoff_ = cutofffld_->getfValue();
    params_.dovelblock_ = blockfld_->isChecked(); 
    params_.velvolmid_ = velfuncsel_->key(true);

    return true;
}



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
    : uiDialog( p, uiDialog::Setup("AngleMute setup",mNoDlgTitle,"103.2.20") )
    , processor_( rt )		      
{
    anglemutegrp_ = new uiAngleMuteGrp( this, processor_->params() );

    uiSeparator* sep = new uiSeparator( this, "Sep" );
    sep->attach( stretchedBelow, anglemutegrp_ );

    topfld_ = new uiGenInput( this, "Mute type",
	    BoolInpSpec(!processor_->params().tail_,"Outer","Inner") );
    topfld_->attach( ensureBelow, sep );
    topfld_->attach( centeredBelow, anglemutegrp_ );

    taperlenfld_ = new uiGenInput( this, "Taper length (samples)",
	    FloatInpSpec(processor_->params().taperlen_) );
    taperlenfld_->attach( alignedBelow, topfld_ );
}


bool uiAngleMute::acceptOK(CallBacker*)
{
    if ( !anglemutegrp_->acceptOK() )
	return false;

    processor_->params().taperlen_ = taperlenfld_->getfValue();
    processor_->params().tail_ = !topfld_->getBoolValue();

    return true;
}


}; //namespace
