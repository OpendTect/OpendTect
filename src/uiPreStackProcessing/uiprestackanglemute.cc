/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackanglemute.h"

#include "prestackanglecomputer.h"
#include "prestackanglemute.h"
#include "raytrace1d.h"
#include "survinfo.h"
#include "windowfunction.h"
#include "uibutton.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiprestackprocessor.h"
#include "uiraytrace1d.h"
#include "uiseparator.h"
#include "uiveldesc.h"
#include "od_helpids.h"


namespace PreStack
{

uiAngleCompGrp::uiAngleCompGrp( uiParent* p, PreStack::AngleCompParams& pars,
				OD::GeomSystem gs, bool dooffset,
				bool isformute, bool withadvanced )
    : uiGroup(p,"Angle Mute Group")
    , params_(pars)
    , isformute_(isformute)
    , dooffset_(dooffset)
{
    const bool is2d = ::is2D( gs );
    const bool issynth = ::isSynthetic( gs );
    uiGroup* alignobj = nullptr;
    if ( !issynth )
    {
	velfuncsel_ = new uiVelSel( this, VelocityDesc::getVelVolumeLabel(),
				    is2d );
	alignobj = velfuncsel_;
	if ( !params_.velvolmid_.isUdf() )
	   velfuncsel_->setInput( params_.velvolmid_ );
    }

    if ( isformute_ )
    {
	anglefld_ = new uiGenInput( this, tr("Mute cutoff angle (degree)"),
				     FloatInpSpec(false) );
	if ( velfuncsel_ )
	    anglefld_->attach( alignedBelow, velfuncsel_ );

	anglefld_->setValue( params_.mutecutoff_ );
	alignobj = anglefld_;
    }
    else
    {
	anglefld_ = new uiGenInput( this, tr("Angle range"),
				    IntInpIntervalSpec(params_.anglerange_) );
	if ( velfuncsel_ )
	    anglefld_->attach( alignedBelow, velfuncsel_ );

	anglelbl_ = new uiLabel( this, tr("degrees") );
	anglelbl_->attach( rightOf, anglefld_ );
	alignobj = anglefld_;
    }

    if ( withadvanced )
    {
	const CallBack cbadv = mCB(this,uiAngleCompGrp,advPushButCB);
	advpushbut_ = new uiPushButton( this, tr("Advanced Parameters"),
					cbadv, false );
	if ( velfuncsel_ )
	    advpushbut_->attach( rightAlignedBelow, velfuncsel_ );
	else if ( anglelbl_ )
	    advpushbut_->attach( rightOf, anglelbl_ );
	else if ( anglefld_ )
	    advpushbut_->attach( rightOf, anglefld_ );
    }

    if ( alignobj )
	setHAlignObj( alignobj );
}


uiAngleCompGrp::~uiAngleCompGrp()
{
    detachAllNotifiers();
}


void uiAngleCompGrp::updateFromParams()
{
    if ( velfuncsel_ )
	velfuncsel_->setInput( params_.velvolmid_ );

    if ( isformute_ )
	anglefld_->setValue( params_.mutecutoff_ );
    else
	anglefld_->setValue( params_.anglerange_ );
}


bool uiAngleCompGrp::acceptOK()
{
    if ( velfuncsel_ )
    {
	if ( !velfuncsel_->ioobj() )
	    return false;

	params_.velvolmid_ = velfuncsel_->key( true );
    }

    const Interval<int> normalanglevalrange( 0, 90 );
    if ( isformute_ )
    {
	params_.mutecutoff_ = anglefld_->getFValue();
	if ( !normalanglevalrange.includes(params_.mutecutoff_,false) )
	{
	    uiMSG().error(
		   tr("Please select the mute cutoff between 0 and 90 degree"));
	    return false;
	}
    }
    else
    {
	Interval<int> anglerange = anglefld_->getIInterval();
	anglerange.sort();
	if ( !normalanglevalrange.includes(anglerange,false) )
	{
	    uiMSG().error(tr("Please provide angle range"
			     " between 0 and 90 degree"));
	    return false;
	}

	params_.anglerange_ = anglerange;
    }

    if ( advpushbut_ )
    {
	if ( !advpardlg_ )
	{
	    advpardlg_ = new uiAngleCompAdvParsDlg( this, params_, dooffset_,
						    isformute_ );
	    advpardlg_->finalize();
	}

	advpardlg_->acceptOK( nullptr );
    }

    return true;
}


void uiAngleCompGrp::advPushButCB( CallBacker* )
{
    if ( advpardlg_)
	advpardlg_->updateFromParams();
    else
    {
	advpardlg_ = new uiAngleCompAdvParsDlg( this, params_, dooffset_,
						isformute_ );
    }

    advpardlg_->go();
}


// uiAngleCompAdvParsDlg

uiAngleCompAdvParsDlg::uiAngleCompAdvParsDlg( uiParent* p,
					      PreStack::AngleCompParams& pars,
					      bool dooffsets, bool isformute )
    : uiDialog(p,Setup(tr("Advanced Parameter"),tr("Advanced angle parameters"),
		       mODHelpKey(mAngleCompAdvParsDlgHelpID)))
    , isformute_(isformute)
    , params_(pars)
{
    uiRayTracer1D::Setup rtsu;
    rtsu.dooffsets( dooffsets ).doreflectivity( false )
	.convertedwaves( false ).withadvanced( false );
    raytracerfld_ = new uiRayTracerSel( this, rtsu );

    if ( !isformute_ )
	createAngleCompFields();

    mAttachCB( postFinalize(), uiAngleCompAdvParsDlg::finalizeCB );
}


uiAngleCompAdvParsDlg::~uiAngleCompAdvParsDlg()
{
    detachAllNotifiers();
}


void uiAngleCompAdvParsDlg::createAngleCompFields()
{
    smoothtypefld_ = new uiGenInput( this, tr("Smoothing Type"),
    StringListInpSpec(PreStack::AngleComputer::smoothingTypeNames()) );
    smoothtypefld_->attach( alignedBelow, raytracerfld_ );
    smoothtypefld_->setValue( PreStack::AngleComputer::FFTFilter );
    mAttachCB( smoothtypefld_->valueChanged,
	       uiAngleCompAdvParsDlg::smoothTypeSel );

    const BufferStringSet& windowfunctions = WINFUNCS().getNames();
    smoothwindowfld_ = new uiGenInput( this, tr("Window/Taper"),
				       StringListInpSpec(windowfunctions) );
    smoothwindowfld_->attach( alignedBelow, smoothtypefld_ );
    mAttachCB( smoothwindowfld_->valueChanged,
	       uiAngleCompAdvParsDlg::smoothWindowSel );

    smoothwinparamfld_ = new uiGenInput( this, tr("Taper length"),
					 FloatInpSpec() );
    smoothwinparamfld_->attach( rightOf, smoothwindowfld_ );
    smoothwinparamlbl_ = new uiLabel( this, toUiString("%") );
    smoothwinparamlbl_->attach( rightOf, smoothwinparamfld_ );

    smoothwinlengthfld_ = new uiGenInput(this, tr("Window width"),
								FloatInpSpec());
    smoothwinlengthfld_->attach( alignedBelow, smoothwindowfld_ );
    smoothwinlengthlbl_ = new uiLabel( this, SI().getUiZUnitString(false) );
    smoothwinlengthlbl_->attach( rightOf, smoothwinlengthfld_ );

    freqf3fld_ = new uiGenInput( this, tr("Frequency F3"), FloatInpSpec() );
    freqf3fld_->attach( alignedBelow, smoothtypefld_ );
    freqf3fld_->setToolTip(tr("Frequency where the cosine tapering "
			      "window starts: Amplitude=input"));
    freqf3lbl_ = new uiLabel( this, tr("Hz") );
    freqf3lbl_->attach( rightOf, freqf3fld_ );

    freqf4fld_ = new uiGenInput( this, tr("Frequency F4"), FloatInpSpec() );
    freqf4fld_->attach( alignedBelow, freqf3fld_ );
    freqf4fld_->setToolTip(tr("Frequency where the cosine tapering "
			      "window stops: Amplitude=0"));
    freqf4lbl_ = new uiLabel( this, tr("Hz") );
    freqf4lbl_->attach( rightOf, freqf4fld_ );
}


void uiAngleCompAdvParsDlg::getRayTracerPars()
{
    raytracerfld_->fillPar( params_.raypar_ );
}


void uiAngleCompAdvParsDlg::setRayTracerPars()
{
    raytracerfld_->usePar( params_.raypar_ );
}


bool uiAngleCompAdvParsDlg::acceptOK( CallBacker* )
{
    getRayTracerPars();
    if ( isformute_ )
	return true;

    IOPar& iopar = params_.smoothingpar_;
    iopar.set( PreStack::AngleComputer::sKeySmoothType(),
	       smoothtypefld_->getIntValue() );
    if ( isSmoothTypeMovingAverage() )
    {
	iopar.set( PreStack::AngleComputer::sKeyWinLen(),
		smoothwinlengthfld_->getFValue()/SI().zDomain().userFactor() );
	iopar.set( PreStack::AngleComputer::sKeyWinFunc(),
		   smoothwindowfld_->text() );
	if ( smoothwindowfld_->text() == CosTaperWindow::sName() )
	{
	    const float uservalue = smoothwinparamfld_->getFValue();
	    if ( !mIsUdf(uservalue) )
		iopar.set( PreStack::AngleComputer::sKeyWinParam(),
			   1 - uservalue / 100 );
	}
    }
    else if ( isSmoothTypeFFTFilter() )
    {
	iopar.set( PreStack::AngleComputer::sKeyFreqF3(),
		   freqf3fld_->getFValue() );
	iopar.set( PreStack::AngleComputer::sKeyFreqF4(),
		   freqf4fld_->getFValue() );
    }

    return true;
}


void uiAngleCompAdvParsDlg::updateFromParams()
{
    setRayTracerPars();
    if ( isformute_ )
	return;

    const IOPar& iopar = params_.smoothingpar_;
    int smoothtype = 0;
    const bool hastype = iopar.get( PreStack::AngleComputer::sKeySmoothType(),
				    smoothtype );
    smoothtypefld_->setValue( smoothtype );
    if ( hastype && smoothtype == PreStack::AngleComputer::MovingAverage )
    {
	BufferString windowname;
	if ( iopar.get(PreStack::AngleComputer::sKeyWinFunc(),windowname) )
	    smoothwindowfld_->setText( windowname );
	float windowparam, windowlength;
	if ( iopar.get(PreStack::AngleComputer::sKeyWinParam(),windowparam) )
	    smoothwinparamfld_->setValue( mNINT32((1-windowparam)*100) );
	if ( iopar.get(PreStack::AngleComputer::sKeyWinLen(),windowlength) )
	    smoothwinlengthfld_->setValue( windowlength *
					   SI().zDomain().userFactor() );
    }
    else if ( hastype && smoothtype == PreStack::AngleComputer::FFTFilter )
    {
	float freqf3, freqf4;
	if ( iopar.get(PreStack::AngleComputer::sKeyFreqF3(),freqf3) )
	    freqf3fld_->setValue( freqf3 );
	if ( iopar.get(PreStack::AngleComputer::sKeyFreqF4(),freqf4) )
	    freqf4fld_->setValue( freqf4 );
    }

    smoothTypeSel(0);
}


bool uiAngleCompAdvParsDlg::isSmoothTypeMovingAverage()
{
    const char* smoothtype = smoothtypefld_->text();
    PreStack::AngleComputer::smoothingType smtype;
    PreStack::AngleComputer::parseEnum( smoothtype, smtype );
    return smtype == PreStack::AngleComputer::MovingAverage;
}


bool uiAngleCompAdvParsDlg::isSmoothTypeFFTFilter()
{
    const char* smoothtype = smoothtypefld_->text();
    PreStack::AngleComputer::smoothingType smtype;
    PreStack::AngleComputer::parseEnum( smoothtype, smtype );
    return smtype == PreStack::AngleComputer::FFTFilter;
}


void uiAngleCompAdvParsDlg::smoothTypeSel( CallBacker* )
{
    const bool ismovingavg = isSmoothTypeMovingAverage();
    const bool isfftfilter = isSmoothTypeFFTFilter();
    smoothwindowfld_->display( ismovingavg );
    smoothwinlengthfld_->display( ismovingavg );
    smoothwinlengthlbl_->display( ismovingavg );
    freqf3fld_->display( isfftfilter );
    freqf4fld_->display( isfftfilter );
    freqf3lbl_->display( isfftfilter );
    freqf4lbl_->display( isfftfilter );

    smoothWindowSel(0);
}


void uiAngleCompAdvParsDlg::smoothWindowSel( CallBacker* )
{
    const bool ismovingavg = isSmoothTypeMovingAverage();
    StringView smoothwindow = smoothwindowfld_->text();
    const bool iscostaper = smoothwindow==CosTaperWindow::sName();
    smoothwinparamfld_->display( ismovingavg && iscostaper );
    smoothwinparamlbl_->display( ismovingavg && iscostaper );
}


void uiAngleCompAdvParsDlg::finalizeCB( CallBacker* )
{
    updateFromParams();
    if ( isformute_ )
	return;

    smoothTypeSel(0);
}


void uiAngleMute::initClass()
{
    uiPSPD().addCreator( create, AngleMute::sFactoryKeyword() );
}


void uiAngleMute::removeClass()
{
    uiPSPD().removeCreator( create );
}


uiDialog* uiAngleMute::create( uiParent* p, Processor* sgp )
{
    mDynamicCastGet( AngleMute*, sgmute, sgp );
    if ( !sgmute )
	return nullptr;

    return new uiAngleMute( p, sgmute );
}


uiAngleMute::uiAngleMute( uiParent* p, AngleMute* rt, bool withadvanced )
    : uiDialog(p,Setup(tr("AngleMute setup"),mODHelpKey(mAngleMuteHelpID)))
    , processor_(rt)
{
    anglecompgrp_ = new uiAngleCompGrp( this, processor_->params(),
					processor_->getGeomSystem(),
					false, true, withadvanced );

    auto* sep = new uiSeparator( this, "Sep" );
    sep->attach( stretchedBelow, anglecompgrp_ );

    topfld_ = new uiGenInput( this, tr("Mute type"),
	    BoolInpSpec(!processor_->params().tail_,tr("Outer"),tr("Inner")) );
    topfld_->attach( ensureBelow, sep );
    topfld_->attach( centeredBelow, anglecompgrp_ );

    taperlenfld_ = new uiGenInput( this, tr("Taper length (samples)"),
	    FloatInpSpec(processor_->params().taperlen_) );
    taperlenfld_->attach( alignedBelow, topfld_ );
}


uiAngleMute::~uiAngleMute()
{}


bool uiAngleMute::acceptOK( CallBacker* )
{
    if ( !anglecompgrp_->acceptOK() )
	return false;

    if ( !processor_->params().raypar_.isPresent(sKey::Type()) )
	processor_->params().raypar_.set( sKey::Type(),
				    RayTracer1D::factory().getDefaultName() );

    processor_->params().tail_ = !topfld_->getBoolValue();
    processor_->params().taperlen_ = taperlenfld_->getFValue();

    return true;
}

} // namespace PreStack
