/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert & Helene
 Date:          Jan 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiprestackattrib.h"
#include "prestackattrib.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "attribstorprovider.h"
#include "ctxtioobj.h"
#include "multiid.h"
#include "prestackprop.h"
#include "seispsioprov.h"
#include "uiattrsel.h"
#include "uiattribfactory.h"
#include "uibutton.h"
#include "uiprestacksel.h"
#include "uiprestackprocessorsel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uiveldesc.h"

#include "raytrace1d.h"
#include "prestackanglemute.h"
#include "prestackanglecomputer.h"
#include "uiprestackanglemute.h"


mInitAttribUI(uiPreStackAttrib,Attrib::PSAttrib,"PreStack",sKeyBasicGrp())


static const char*	sKeyRayTracer()	    { return "FullRayTracer"; }


uiPreStackAttrib::uiPreStackAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.17")
	, params_(*new PreStack::AngleCompParams)
{
    prestackinpfld_ = new uiPreStackSel( this, is2d );

    dopreprocessfld_ = new uiGenInput( this, "Preprocess", BoolInpSpec(false) );
    dopreprocessfld_->attach( alignedBelow, prestackinpfld_ );
    dopreprocessfld_->valuechanged.notify(
	    mCB(this,uiPreStackAttrib,doPreProcSel) );
    preprocsel_ = new PreStack::uiProcSel( this, "Preprocessing setup", 0 );
    preprocsel_->attach( alignedBelow, dopreprocessfld_ );
    
    calctypefld_ = new uiGenInput( this, "Calculation type",
		   StringListInpSpec(PreStack::PropCalc::CalcTypeNames()) );
    calctypefld_->attach( alignedBelow, preprocsel_ );
    calctypefld_->valuechanged.notify( mCB(this,uiPreStackAttrib,calcTypSel) );

    stattypefld_ = new uiGenInput( this, "Statistics type",
				   StringListInpSpec(Stats::TypeNames()) );
    stattypefld_->attach( alignedBelow, calctypefld_ );

    lsqtypefld_ = new uiGenInput( this, "LSQ output",
		  StringListInpSpec(PreStack::PropCalc::LSQTypeNames()) );
    lsqtypefld_->attach( alignedBelow, calctypefld_ );

    useanglefld_ = new uiCheckBox( this, "Use Angles" );
    useanglefld_->attach( rightOf, lsqtypefld_ );
    useanglefld_->activated.notify( mCB(this,uiPreStackAttrib,angleTypSel) );

    offsrgfld_ = new uiGenInput( this, "Offset range (empty=all)",
	     FloatInpIntervalSpec(Interval<float>(mUdf(float),mUdf(float))) );
    offsrgfld_->attach( alignedBelow, stattypefld_ );

    const char* offslabel = SI().xyInFeet() ? "feet" : "meters";
    offsrglbl_ = new uiLabel( this, offslabel );
    offsrglbl_->attach( rightOf, offsrgfld_ );

    offsaxtypefld_ = new uiGenInput( this, "X Axis Transformation:",
		    StringListInpSpec(PreStack::PropCalc::AxisTypeNames())
	   			      .setName("X") );
    offsaxtypefld_->attach( alignedBelow, offsrgfld_ );

    valaxtypefld_ = new uiGenInput( this, "Amplitude transformations",
		     StringListInpSpec(PreStack::PropCalc::AxisTypeNames()) );
    valaxtypefld_->attach( alignedBelow, offsaxtypefld_ );

    anglecompgrp_ = new PreStack::uiAngleCompGrp( this, params_, false, false );
    anglecompgrp_->attach( alignedBelow, valaxtypefld_ );

    calcTypSel(0);
    setHAlignObj( prestackinpfld_ );
}


uiPreStackAttrib::~uiPreStackAttrib()
{
}


bool uiPreStackAttrib::setParameters( const Attrib::Desc& desc )
{
    RefMan<Attrib::Desc> tmpdesc = new Attrib::Desc( desc );
    RefMan<Attrib::PSAttrib> aps = new Attrib::PSAttrib( *tmpdesc );

    prestackinpfld_->setInput( aps->psID() );

    const MultiID ppid = aps->preProcID();
    dopreprocessfld_->setValue( !ppid.isEmpty() && ppid.ID(0)!=0 );
    preprocsel_->setSel( ppid );
    offsrgfld_->setValue( aps->setup().offsrg_ );
    calctypefld_->setValue( (int)aps->setup().calctype_ );
    stattypefld_->setValue( (int)aps->setup().stattype_ );
    lsqtypefld_->setValue( (int)aps->setup().lsqtype_ );
    offsaxtypefld_->setValue( (int)aps->setup().offsaxis_ );
    valaxtypefld_->setValue( (int)aps->setup().valaxis_ );
    useanglefld_->setChecked( aps->setup().useangle_ );
    anglecompgrp_->setVelocityInput( aps->velocityID() );
    anglecompgrp_->setAngleRange( aps->setup().anglerg_ );

    calcTypSel(0);
    doPreProcSel(0);
    return true;
}


bool uiPreStackAttrib::getParameters( Desc& desc )
{
    if ( !prestackinpfld_->commitInput() )
	return false;

    mSetString(Attrib::StorageProvider::keyStr(),prestackinpfld_->getMultiID())

    if ( dopreprocessfld_->getBoolValue() )
    {
	MultiID mid;
	if ( !preprocsel_->getSel(mid))
	    { errmsg_ = "Please select preprocessing setup"; return false; }
	mSetString(Attrib::PSAttrib::preProcessStr(), mid );
    }
    else
    {
	mSetString(Attrib::PSAttrib::preProcessStr(), ((const char*) 0) );
    }

    Interval<float> offsrg = offsrgfld_->getFInterval();
    if ( mIsUdf(offsrg.start) ) offsrg.start = 0;
    mSetFloat(Attrib::PSAttrib::offStartStr(),offsrg.start)
    mSetFloat(Attrib::PSAttrib::offStopStr(),offsrg.stop)
    const int calctyp = calctypefld_->getIntValue();
    mSetEnum(Attrib::PSAttrib::calctypeStr(),calctyp)
    const bool isnorm = calctyp == 0;
    if ( isnorm )
    {
	mSetEnum(Attrib::PSAttrib::stattypeStr(),stattypefld_->getIntValue())
    }
    else
    {
	mSetEnum(Attrib::PSAttrib::lsqtypeStr(),lsqtypefld_->getIntValue())
	mSetEnum(Attrib::PSAttrib::offsaxisStr(),offsaxtypefld_->getIntValue())
	const bool isangleparam = useanglefld_->isChecked();
	mSetBool(Attrib::PSAttrib::useangleStr(),isangleparam )

	if ( isangleparam )
	{
	    if ( !anglecompgrp_->acceptOK() )
		return false;

	    mSetString(Attrib::PSAttrib::velocityIDStr(), params_.velvolmid_ );
	    Interval<float>& anglerg = params_.anglerange_;
	    if ( mIsUdf(anglerg.start) ) anglerg.start = 0;
	    mSetFloat(Attrib::PSAttrib::angleStartStr(),anglerg.start)
	    mSetFloat(Attrib::PSAttrib::angleStopStr(),anglerg.stop)

	    IOPar& raypar = params_.raypar_;
	    BufferString raytracertype;
	    raypar.get( sKey::Type(), raytracertype );
	    if ( raytracertype == sKeyRayTracer() )
	    {
		float thresholdparam;
		raypar.get( RayTracer1D::sKeyBlockRatio(), thresholdparam );
		mSetFloat( RayTracer1D::sKeyBlockRatio(), thresholdparam );
		mSetBool( Attrib::PSAttrib::rayTracerStr(), true );
	    }
	    
	    int smoothtype;
	    IOPar smpar ( params_.smoothingpar_ );
	    smpar.get( PreStack::AngleComputer::sKeySmoothType(), smoothtype );
	    mSetInt( PreStack::AngleComputer::sKeySmoothType(), smoothtype )
	    if ( smoothtype == PreStack::AngleComputer::TimeAverage )
	    {
		BufferString winfunc; float winparam, winlength;
		smpar.get( PreStack::AngleComputer::sKeyWinFunc(), winfunc );
		smpar.get( PreStack::AngleComputer::sKeyWinParam(), winparam );
		smpar.get( PreStack::AngleComputer::sKeyWinLen(), winlength );

		mSetString( PreStack::AngleComputer::sKeyWinFunc(), winfunc )
		mSetFloat( PreStack::AngleComputer::sKeyWinParam(), winparam )
		mSetFloat( PreStack::AngleComputer::sKeyWinLen(), winlength )
	    }
	    else if ( smoothtype == PreStack::AngleComputer::FFTFilter )
	    {
		float f3freq, f4freq;
		smpar.get( PreStack::AngleComputer::sKeyFreqF3(), f3freq );
		smpar.get( PreStack::AngleComputer::sKeyFreqF4(), f4freq );

		mSetFloat( PreStack::AngleComputer::sKeyFreqF3(), f3freq );
		mSetFloat( PreStack::AngleComputer::sKeyFreqF3(), f4freq );
	    }
	}
	
    }
    mSetEnum(Attrib::PSAttrib::valaxisStr(),valaxtypefld_->getIntValue())
    return true;
}


void uiPreStackAttrib::calcTypSel( CallBacker* )
{
    const bool isnorm = calctypefld_->getIntValue() == 0;
    stattypefld_->display( isnorm );
    lsqtypefld_->display( !isnorm );
    offsaxtypefld_->display( !isnorm );
    useanglefld_->display( !isnorm );
    if ( !isnorm )
	angleTypSel( 0 );
    else
	anglecompgrp_->display( false );  
}


void uiPreStackAttrib::angleTypSel( CallBacker* )
{
    bool isangleparam = useanglefld_->isChecked();
    anglecompgrp_->display( isangleparam );
    offsaxtypefld_->setSensitive( !isangleparam );
    offsaxtypefld_->setValue( isangleparam ? PreStack::PropCalc::Sinsq
					   : PreStack::PropCalc::Norm );
}


void uiPreStackAttrib:: doPreProcSel(CallBacker*)
{
    preprocsel_->display( dopreprocessfld_->getBoolValue() );
}


void uiPreStackAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( "Offset start", PSAttrib::offStartStr() );
    params += EvalParam( "Offset stop", PSAttrib::offStopStr() );
}


void uiPreStackAttrib::setDataPackInp( const TypeSet<DataPack::FullID>& ids ) 
{
    uiAttrDescEd::setDataPackInp( ids );
    prestackinpfld_->setDataPackInp( ids );
}


bool uiPreStackAttrib::setInput( const Desc& desc )
{
    return true;
}
