/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert & Helene
 Date:          Jan 2008
________________________________________________________________________

-*/


#include "uiprestackattrib.h"
#include "prestackattrib.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "attribstorprovider.h"
#include "ioobjctxt.h"
#include "dbkey.h"
#include "prestackanglecomputer.h"
#include "prestackanglemute.h"
#include "prestackprop.h"
#include "raytrace1d.h"
#include "seispsioprov.h"
#include "windowfunction.h"
#include "uiattrsel.h"
#include "uiattribfactory.h"
#include "uibutton.h"
#include "uiprestackanglemute.h"
#include "uiprestacksel.h"
#include "uiprestackprocessorsel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uiveldesc.h"
#include "od_helpids.h"
#include "uimsg.h"

using namespace Attrib;

mInitAttribUI(uiPreStackAttrib,PSAttrib,uiStrings::sPreStack(),sBasicGrp())


uiPreStackAttrib::uiPreStackAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d, mODHelpKey(mPreStackAttribHelpID) )
    , params_(*new PreStack::AngleCompParams)
    , statsdef_( Stats::TypeDef() )
{
    prestackinpfld_ = new uiPreStackSel( this, is2d );

    gathertypefld_ = new uiGenInput( this, tr("Gather type"),
			     StringListInpSpec(Gather::TypeDef()) );
    gathertypefld_->attach( alignedBelow, prestackinpfld_ );
    gathertypefld_->valuechanged.notify(
				 mCB(this,uiPreStackAttrib,gatherTypSel) );

    xrgfld_ = new uiGenInput( this, tr("Offset range (empty=all) "),
	     FloatInpIntervalSpec(Interval<float>(mUdf(float),mUdf(float))) );
    xrgfld_->attach( alignedBelow, gathertypefld_ );

    dopreprocessfld_ = new uiGenInput( this, tr("Preprocess"),
                                       BoolInpSpec(false) );
    dopreprocessfld_->attach( alignedBelow, xrgfld_ );
    dopreprocessfld_->valuechanged.notify(
	    mCB(this,uiPreStackAttrib,doPreProcSel) );
    preprocsel_ = new PreStack::uiProcSel( this, tr("Preprocessing setup"), 0 );
    preprocsel_->attach( alignedBelow, dopreprocessfld_ );

    calctypefld_ = new uiGenInput( this, tr("Calculation type"),
		   StringListInpSpec(PreStack::PropCalc::CalcTypeDef()) );
    calctypefld_->attach( alignedBelow, preprocsel_ );
    calctypefld_->valuechanged.notify( mCB(this,uiPreStackAttrib,calcTypSel) );

    statsdef_.setUiStringForIndex( statsdef_.indexOf(Stats::Count),
				   tr("Fold") );
    statsdef_.setUiStringForIndex( statsdef_.indexOf(Stats::Average),
				   uiStrings::sStack() );
    stattypefld_ = new uiGenInput( this, tr("Statistics type"),
				   StringListInpSpec(statsdef_) );
    stattypefld_->attach( alignedBelow, calctypefld_ );

    lsqtypefld_ = new uiGenInput( this, tr("AVO output"),
		  StringListInpSpec(PreStack::PropCalc::LSQTypeDef()) );
    lsqtypefld_->attach( alignedBelow, calctypefld_ );

    useanglefld_ = new uiCheckBox( this, tr("Compute Angles") );
    useanglefld_->attach( rightOf, lsqtypefld_ );
    useanglefld_->activated.notify( mCB(this,uiPreStackAttrib,angleTypSel) );
    useanglefld_->setChecked(false);

    const uiString xlabel = SI().xyInFeet()?tr("feet     "):tr("meters    ");
    xrglbl_ = new uiLabel( this, xlabel );
    xrglbl_->attach( rightOf, xrgfld_ );

    xunitfld_ = new uiGenInput( this, uiString::empty(),
				StringListInpSpec(Gather::UnitDef()) );
    xunitfld_->attach( rightOf, gathertypefld_ );
    xunitfld_->valuechanged.notify( mCB(this,uiPreStackAttrib,gatherUnitSel) );

    xaxistypefld_ = new uiGenInput( this, tr("X Axis Transformation:"),
		    StringListInpSpec(PreStack::PropCalc::AxisTypeDef())
				      .setName("X") );
    xaxistypefld_->attach( alignedBelow, lsqtypefld_ );

    valaxtypefld_ = new uiGenInput( this, tr("Amplitude transformations"),
		     StringListInpSpec(PreStack::PropCalc::AxisTypeDef()) );
    valaxtypefld_->attach( alignedBelow, xaxistypefld_ );

    anglecompgrp_ = new PreStack::uiAngleCompGrp( this, params_, false, false );
    anglecompgrp_->attach( alignedBelow, valaxtypefld_ );

    updateCalcType();
    setHAlignObj( prestackinpfld_ );
}


uiPreStackAttrib::~uiPreStackAttrib()
{
}


bool uiPreStackAttrib::setAngleParameters( const Desc& desc )
{
    mIfGetString( PSAttrib::velocityIDStr(), idstr,
		  params_.velvolmid_=DBKey(idstr) )

    Interval<int> anglerange, normalanglevalrange( 0, 90 );
    mIfGetInt( PSAttrib::angleStartStr(), start, anglerange.start=start )
    mIfGetInt( PSAttrib::angleStopStr(), stop, anglerange.stop=stop )
    if ( normalanglevalrange.includes(anglerange,false) )
	params_.anglerange_ = anglerange;

    BufferString raytracerparam;
    mIfGetString( PSAttrib::rayTracerParamStr(), param,
		  raytracerparam=param )
    params_.raypar_.getParsFrom( raytracerparam );

    IOPar& smpar = params_.smoothingpar_;
    int smoothtype = 0;
    mIfGetEnum( Attrib::PSAttrib::angleSmoothType(), smtype, smoothtype=smtype )
    smpar.set( PreStack::AngleComputer::sKeySmoothType(), smoothtype );

    if ( smoothtype == PreStack::AngleComputer::MovingAverage )
    {
	float windowlength = mUdf(float);
	mIfGetFloat( Attrib::PSAttrib::angleFiltLength(), winlen,
		     windowlength=winlen )
	if ( !mIsUdf(windowlength) )
	    smpar.set( PreStack::AngleComputer::sKeyWinLen(), windowlength );

	BufferString windowfunction;
	mIfGetString( Attrib::PSAttrib::angleFiltFunction(), winfunc,
		      windowfunction=winfunc )
	smpar.set( PreStack::AngleComputer::sKeyWinFunc(), windowfunction );

	if ( windowfunction == CosTaperWindow::sName() )
	{
	    float windowparam = mUdf(float);
	    mIfGetFloat( Attrib::PSAttrib::angleFiltValue(), winpar,
			 windowparam=winpar )
	    if ( !mIsUdf(windowparam) && windowparam >=0 && windowparam <= 1 )
		smpar.set( PreStack::AngleComputer::sKeyWinParam(),windowparam);
	}
    }
    else if ( smoothtype == PreStack::AngleComputer::FFTFilter )
    {
	float f3freq = mUdf(float);
	mIfGetFloat( Attrib::PSAttrib::angleFFTF3Freq(), freq, f3freq=freq  )
	if ( !mIsUdf(f3freq) )
	    smpar.set( PreStack::AngleComputer::sKeyFreqF3(), f3freq );

	float f4freq = mUdf(float);
	mIfGetFloat( Attrib::PSAttrib::angleFFTF4Freq(), freq, f4freq=freq )
	if ( !mIsUdf(f4freq) )
	    smpar.set( PreStack::AngleComputer::sKeyFreqF4(), f4freq );
    }

    anglecompgrp_->updateFromParams();

    return true;

}


bool uiPreStackAttrib::setParameters( const Desc& desc )
{
    RefMan<Desc> tmpdesc = new Desc( desc );
    RefMan<PSAttrib> aps = new PSAttrib( *tmpdesc );

    prestackinpfld_->setInput( aps->psID() );

    const DBKey ppid = aps->preProcID();
    if ( ppid.isValid() && ppid.dirID().getI() != 0 )
    {
	dopreprocessfld_->setValue( true );
	preprocsel_->setSel( ppid );
    }

    calctypefld_->setValue( (int)aps->setup().calctype_ );
    if ( aps->setup().calctype_ == PreStack::PropCalc::Stats )
    {
	stattypefld_->setValue( statsdef_.indexOf(aps->setup().stattype_));
    }
    else
    {
	lsqtypefld_->setValue( (int)aps->setup().lsqtype_ );
	xaxistypefld_->setValue( (int)aps->setup().offsaxis_ );
    }

    valaxtypefld_->setValue( (int)aps->setup().valaxis_ );
    const bool useangle = !aps->setup().anglerg_.isUdf();
    useanglefld_->setChecked( useangle );
    if ( useangle && !setAngleParameters(desc) )
	return false;

    if ( !useangle )
    {
	mIfGetEnum(PSAttrib::xaxisunitStr(),xut,xunitfld_->setValue(xut));
	const bool isoffset = gathertypefld_->getIntValue() == 0;
	Interval<float> xrg = isoffset ? aps->setup().offsrg_
			    : aps->setup().anglerg_;
	if ( isoffset && SI().xyInFeet() && !xrg.isUdf() )
	    xrg.scale( mToFeetFactorF );
	xrgfld_->setValue( xrg );
    }

    doPreProcSel(0);
    calcTypSel(0);
    return true;
}


bool uiPreStackAttrib::getAngleParameters( Desc& desc )
{
    if ( !anglecompgrp_->acceptOK() )
	return false;

    mSetString(PSAttrib::velocityIDStr(), params_.velvolmid_.toString() );
    Interval<int>& anglerg = params_.anglerange_;
    if ( mIsUdf(anglerg.start) ) anglerg.start = 0;
    mSetInt(PSAttrib::angleStartStr(),anglerg.start)
    mSetInt(PSAttrib::angleStopStr(),anglerg.stop)

    BufferString rayparstr;
    params_.raypar_.putParsTo( rayparstr );
    mSetString( PSAttrib::rayTracerParamStr(), rayparstr );

    int smoothtype;
    const IOPar& smpar = params_.smoothingpar_;
    smpar.get( PreStack::AngleComputer::sKeySmoothType(), smoothtype );
    mSetEnum( Attrib::PSAttrib::angleSmoothType(), smoothtype )

    if ( smoothtype == PreStack::AngleComputer::MovingAverage )
    {
	float winlength;
	smpar.get( PreStack::AngleComputer::sKeyWinLen(), winlength );
	mSetFloat( Attrib::PSAttrib::angleFiltLength(), winlength )
	BufferString winfunc;
	smpar.get( PreStack::AngleComputer::sKeyWinFunc(), winfunc );
	mSetString( Attrib::PSAttrib::angleFiltFunction(), winfunc )
	if ( winfunc == CosTaperWindow::sName() )
	{
	    float winparam;
	    smpar.get( PreStack::AngleComputer::sKeyWinParam(), winparam );
	    if ( winparam>=0 && winparam <= 1 )
		mSetFloat( Attrib::PSAttrib::angleFiltValue(), winparam )
	}
    }
    else if ( smoothtype == PreStack::AngleComputer::FFTFilter )
    {
	float f3freq;
	smpar.get( PreStack::AngleComputer::sKeyFreqF3(), f3freq );
	mSetFloat( Attrib::PSAttrib::angleFFTF3Freq(), f3freq );
	float f4freq;
	smpar.get( PreStack::AngleComputer::sKeyFreqF4(), f4freq );
	mSetFloat( Attrib::PSAttrib::angleFFTF4Freq(), f4freq );
    }

    return true;
}


bool uiPreStackAttrib::getParameters( Desc& desc )
{
    const DBKey dbky = prestackinpfld_->getDBKey();
    if ( dbky.isInvalid() && !dbky.hasAuxKey() )
	return false;

    StepInterval<float> xrgfldint = xrgfld_->getFStepInterval();
    bool isoffset = gathertypefld_->getIntValue() == 0;
    if ( xrgfldint.start > xrgfldint.stop )
    {
	uiMSG().error(
	    tr("Start value of the %1 range field is greater than stop value.")
		 .arg(isoffset ? uiStrings::sOffset() : uiStrings::sAngle()) );
	return false;
    }
    mSetString(Attrib::StorageProvider::keyStr(), dbky.toString())

    if ( dopreprocessfld_->getBoolValue() )
    {
	DBKey mid;
	if ( !preprocsel_->getSel(mid))
	{
	    uiMSG().error( uiStrings::phrSelect(tr("preprocessing setup") ) );
	    return false;
	}
	mSetString(PSAttrib::preProcessStr(), mid.toString() );
    }

    const int calctyp = calctypefld_->getIntValue();
    mSetEnum(PSAttrib::calctypeStr(),calctyp)
    if ( calctyp == 0 )
    {
	mSetEnum( PSAttrib::stattypeStr(),
		  statsdef_.getEnumValForIndex(stattypefld_->getIntValue()));
    }
    else
    {
	mSetEnum(PSAttrib::lsqtypeStr(),lsqtypefld_->getIntValue())
	mSetEnum(PSAttrib::offsaxisStr(),xaxistypefld_->getIntValue())
    }

    mSetEnum(PSAttrib::valaxisStr(),valaxtypefld_->getIntValue())

    const bool useangle = useanglefld_->isChecked();
    mSetBool(PSAttrib::useangleStr(),useangle)
    if ( useangle && !getAngleParameters(desc) )
	return false;

    if ( !useangle )
    {
	const int gathertype = gathertypefld_->getIntValue();
	mSetEnum(PSAttrib::gathertypeStr(),gathertype)
	mSetEnum(PSAttrib::xaxisunitStr(),xunitfld_->getIntValue());
	Interval<float> offsrg = xrgfld_->getFInterval();
	if ( SI().xyInFeet() && !offsrg.isUdf() && gathertype == 0 )
	    offsrg.scale( mFromFeetFactorF );

	if ( mIsUdf(offsrg.start) ) offsrg.start = 0;
	mSetFloat(PSAttrib::offStartStr(),offsrg.start)
	mSetFloat(PSAttrib::offStopStr(),offsrg.stop)
    }

    return true;
}


void uiPreStackAttrib::doPreProcSel( CallBacker* )
{
    preprocsel_->display( dopreprocessfld_->getBoolValue() );
}


void uiPreStackAttrib::calcTypSel( CallBacker* cb )
{
    updateCalcType();
    gatherTypSel( cb );
}


void uiPreStackAttrib::updateCalcType()
{
    const bool isnorm = calctypefld_->getIntValue() == 0;
    stattypefld_->display( isnorm );
    lsqtypefld_->display( !isnorm );
    xaxistypefld_->display( !isnorm );
}


void uiPreStackAttrib::gatherTypSel( CallBacker* cb )
{
    const bool isoffset = gathertypefld_->getIntValue() == 0;
    uiString xlbl = tr("%1 range (empty=all)")
				      .arg(toUiString(gathertypefld_->text()));
    xrgfld_->setTitleText( xlbl );
    if ( isoffset )
	xrglbl_->setText( SI().xyUnitString(false) );
    else
    {
	xaxistypefld_->setValue( PreStack::PropCalc::Sinsq );
	gatherUnitSel( 0 );
    }

    useanglefld_->display( isoffset );
    gathertypefld_->display( true );
    xunitfld_->display( !isoffset );

    angleTypSel( cb );
}


void uiPreStackAttrib::angleTypSel( CallBacker* cb)
{
    if ( is2D() )
    {
	useanglefld_->setChecked( false );
	useanglefld_->display( false );
    }

    const bool isoffset = gathertypefld_->getIntValue() == 0;
    const bool iscomputeangle = useanglefld_->isChecked();

    xrgfld_->setSensitive( !iscomputeangle || !isoffset );
    xrglbl_->setSensitive( !iscomputeangle || !isoffset );
    anglecompgrp_->display( isoffset && iscomputeangle );
    if ( cb )  //helps to populate non-default values
    {
	if ( !isoffset || iscomputeangle )
	    xaxistypefld_->setValue( PreStack::PropCalc::Sinsq );
	if ( isoffset || !iscomputeangle )
	    xaxistypefld_->setValue( PreStack::PropCalc::Norm );
	if ( finalised() )
	    xrgfld_->setEmpty();
    }

    xaxistypefld_->setSensitive( isoffset && !iscomputeangle );


}


void uiPreStackAttrib::gatherUnitSel( CallBacker* )
{
    const bool isdegrees = xunitfld_->getIntValue() == 0;
    if ( xunitfld_->rightObj()->isDisplayed() )
	xrglbl_->setText( toUiString(isdegrees ? "deg" : "rad") );
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
