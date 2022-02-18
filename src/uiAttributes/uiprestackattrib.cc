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
#include "ctxtioobj.h"
#include "multiid.h"
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


mInitAttribUI(uiPreStackAttrib,Attrib::PSAttrib,"Prestack",sKeyBasicGrp())


static const char*	statTypeCountStr()	{ return "Fold"; }
static const char*	statTypeAverageStr()	{ return "Stack"; }


uiPreStackAttrib::uiPreStackAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mPreStackAttribHelpID) )
	, params_(*new PreStack::AngleCompParams)
{
    prestackinpfld_ = new uiPreStackSel( this, is2d );

    gathertypefld_ = new uiGenInput( this, tr("Gather type"),
			StringListInpSpec(PSAttrib::GatherTypeNames()) );
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
		   StringListInpSpec(PreStack::PropCalc::CalcTypeNames()) );
    calctypefld_->attach( alignedBelow, preprocsel_ );
    calctypefld_->valuechanged.notify( mCB(this,uiPreStackAttrib,calcTypSel) );

    BufferStringSet stattypenames; getStatTypeNames(stattypenames);
    stattypefld_ = new uiGenInput( this, tr("Statistics type"),
				   StringListInpSpec(stattypenames) );
    stattypefld_->attach( alignedBelow, calctypefld_ );

    lsqtypefld_ = new uiGenInput( this, tr("AVO output"),
		  StringListInpSpec(PreStack::PropCalc::LSQTypeNames()) );
    lsqtypefld_->attach( alignedBelow, calctypefld_ );

    useanglefld_ = new uiCheckBox( this, tr("Compute Angles") );
    useanglefld_->attach( rightOf, gathertypefld_ );
    useanglefld_->activated.notify( mCB(this,uiPreStackAttrib,angleTypSel) );

    const uiString xlabel = SI().xyInFeet()?tr("feet     "):tr("meters    ");
    xrglbl_ = new uiLabel( this, xlabel );
    xrglbl_->attach( rightOf, xrgfld_ );

    xunitfld_ = new uiGenInput( this, uiString::emptyString(),
				StringListInpSpec(PSAttrib::XaxisUnitNames()) );
    xunitfld_->attach( rightOf, gathertypefld_ );
    xunitfld_->valuechanged.notify( mCB(this,uiPreStackAttrib,gatherUnitSel) );

    xaxistypefld_ = new uiGenInput( this, tr("X Axis Transformation:"),
		    StringListInpSpec(PreStack::PropCalc::AxisTypeNames())
				      .setName("X") );
    xaxistypefld_->attach( alignedBelow, lsqtypefld_ );

    valaxtypefld_ = new uiGenInput( this, tr("Amplitude transformations"),
		     StringListInpSpec(PreStack::PropCalc::AxisTypeNames()) );
    valaxtypefld_->attach( alignedBelow, xaxistypefld_ );

    anglecompgrp_ = new PreStack::uiAngleCompGrp( this, params_, false, false );
    anglecompgrp_->attach( alignedBelow, valaxtypefld_ );

    updateCalcType();
    setHAlignObj( prestackinpfld_ );
}


uiPreStackAttrib::~uiPreStackAttrib()
{
}


void uiPreStackAttrib::getStatTypeNames( BufferStringSet& stattypenames )
{
    stattypenames.setEmpty();
    stattypenames.add( Stats::TypeNames() );
    const char* countstr = Stats::toString( Stats::Count );
    const int countidx = stattypenames.indexOf( countstr );
    if ( countidx > -1 )
	*stattypenames[countidx] = statTypeCountStr();

    const char* averagestr = Stats::toString( Stats::Average );
    const int averageidx = stattypenames.indexOf( averagestr );
    if ( averageidx > -1 )
	*stattypenames[averageidx] = statTypeAverageStr();
}


Stats::Type uiPreStackAttrib::getStatEnumfromString( const char* stattypename )
{
    FixedString typname( stattypename );
    if ( typname==statTypeCountStr() )
	return Stats::Count;
    else if ( typname==statTypeAverageStr() )
	return Stats::Average;

    Stats::Type enm;
    if ( Stats::parseEnum(stattypename,enm) )
	return enm;

    return Stats::Average;
}


const char* uiPreStackAttrib::getStringfromStatEnum( Stats::Type enm )
{
    FixedString typname = Stats::toString( enm );
    if ( !typname )
	return Stats::toString(Stats::Average);

    if ( typname == Stats::toString(Stats::Count) )
	return statTypeCountStr();
    else if ( typname == Stats::toString(Stats::Average) )
	return statTypeAverageStr();

    return typname;
}


bool uiPreStackAttrib::setAngleParameters( const Attrib::Desc& desc )
{
    mIfGetMultiID( Attrib::PSAttrib::velocityIDStr(), mid,
		   params_.velvolmid_=mid )

    Interval<int> anglerange, normalanglevalrange( 0, 90 );
    mIfGetInt( Attrib::PSAttrib::angleStartStr(), start, anglerange.start=start)
    mIfGetInt( Attrib::PSAttrib::angleStopStr(), stop, anglerange.stop=stop )
    if ( normalanglevalrange.includes(anglerange,false) )
	params_.anglerange_ = anglerange;

    BufferString raytracerparam;
    mIfGetString( Attrib::PSAttrib::rayTracerParamStr(), param,
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


bool uiPreStackAttrib::setParameters( const Attrib::Desc& desc )
{
    RefMan<Attrib::Desc> tmpdesc = new Attrib::Desc( desc );
    RefMan<Attrib::PSAttrib> aps = new Attrib::PSAttrib( *tmpdesc );

    prestackinpfld_->setInput( aps->psID() );

    const MultiID ppid = aps->preProcID();
    const bool dopreproc = !ppid.isUdf();
    dopreprocessfld_->setValue( dopreproc );
    if ( dopreproc )
	preprocsel_->setSel( ppid );

    calctypefld_->setValue( (int)aps->setup().calctype_ );
    if ( aps->setup().calctype_ == PreStack::PropCalc::Stats )
    {
	stattypefld_->setText( getStringfromStatEnum(aps->setup().stattype_) );
    }
    else
    {
	lsqtypefld_->setValue( (int)aps->setup().lsqtype_ );
	xaxistypefld_->setValue( (int)aps->setup().offsaxis_ );
    }

    valaxtypefld_->setValue( (int)aps->setup().valaxis_ );
    useanglefld_->setChecked( aps->setup().useangle_ );
    mIfGetEnum(PSAttrib::gathertypeStr(),gtp,gathertypefld_->setValue(gtp));
    if ( aps->setup().useangle_ && !setAngleParameters(desc) )
	return false;

    if ( !aps->setup().useangle_ )
    {
	mIfGetEnum(PSAttrib::xaxisunitStr(),xut,xunitfld_->setValue(xut));
	const bool isoffset = gathertypefld_->getIntValue() == 0;
	Interval<float> xrg;
	if ( isoffset )
	    xrg =  aps->setup().offsrg_;
	else
	    xrg.set( aps->setup().anglerg_.start, aps->setup().anglerg_.stop );
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

    mSetMultiID( Attrib::PSAttrib::velocityIDStr(), params_.velvolmid_ );
    Interval<int>& anglerg = params_.anglerange_;
    if ( mIsUdf(anglerg.start) ) anglerg.start = 0;
    mSetInt(Attrib::PSAttrib::angleStartStr(),anglerg.start)
    mSetInt(Attrib::PSAttrib::angleStopStr(),anglerg.stop)

    BufferString rayparstr;
    params_.raypar_.putParsTo( rayparstr );
    mSetString( Attrib::PSAttrib::rayTracerParamStr(), rayparstr );

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
    if ( !prestackinpfld_->commitInput() )
	return false;
    StepInterval<float> xrg = xrgfld_->getFStepInterval();
    const bool isoffset = gathertypefld_->getIntValue() == 0;
    if ( xrg.start > xrg.stop )
    {
	errmsg_ = tr("Start value of the %1 range field is greater than stop "
		     "value.")
		     .arg( isoffset ? uiStrings::sOffset() : tr("Angle") );
	uiMSG().error( errmsg_ );
	return false;
    }
    mSetMultiID(Attrib::StorageProvider::keyStr(),prestackinpfld_->getMultiID())

    const bool dopreproc = dopreprocessfld_->getBoolValue();
    MultiID preprocmid = MultiID::udf();
    if ( dopreproc && !preprocsel_->getSel(preprocmid) )
    {
	errmsg_ = tr( "Please select preprocessing setup" );
	return false;
    }

    mSetMultiID( Attrib::PSAttrib::preProcessStr(), preprocmid );

    const int calctyp = calctypefld_->getIntValue();
    mSetEnum(Attrib::PSAttrib::calctypeStr(),calctyp)
    if ( calctyp == 0 )
    {
	mSetEnum( Attrib::PSAttrib::stattypeStr(),
		  getStatEnumfromString(stattypefld_->text()) )
    }
    else
    {
	mSetEnum(Attrib::PSAttrib::lsqtypeStr(),lsqtypefld_->getIntValue())
	mSetEnum(Attrib::PSAttrib::offsaxisStr(),xaxistypefld_->getIntValue())
    }

    mSetEnum(Attrib::PSAttrib::valaxisStr(),valaxtypefld_->getIntValue())

    const bool useangle = useanglefld_->isChecked();
    mSetBool(Attrib::PSAttrib::useangleStr(),useangle)
    const int gathertype = gathertypefld_->getIntValue();
    mSetEnum(Attrib::PSAttrib::gathertypeStr(),gathertype)

    if ( useangle && !getAngleParameters(desc) )
	return false;

    if ( !useangle )
    {
	mSetEnum(Attrib::PSAttrib::xaxisunitStr(),xunitfld_->getIntValue());
	Interval<float> offsrg = xrgfld_->getFInterval();
	if ( mIsUdf(offsrg.start) )
	    offsrg.start = 0;

	mSetFloat(Attrib::PSAttrib::offStartStr(),offsrg.start)
	mSetFloat(Attrib::PSAttrib::offStopStr(),offsrg.stop)
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
    gatherTypSel( 0 );
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
    uiString xlbl = isoffset ? tr("Offset range (empty=all)") :
						tr("Angle range (empty=all)");

    xrgfld_->setTitleText(xlbl);

    if ( isoffset )
	xrglbl_->setText( SI().getUiXYUnitString(false,false) );
    else
    {
	useanglefld_->setChecked( false );
	xaxistypefld_->setValue( PreStack::PropCalc::Sinsq );
	gatherUnitSel( 0 );
    }

    useanglefld_->display( isoffset );
    gathertypefld_->display( true );
    xunitfld_->display( !isoffset );

    if ( cb )  //helps to populate non-default values
    {
	if ( finalised() )
	    xrgfld_->setEmpty();

	if ( !isoffset  )
	    xaxistypefld_->setValue( PreStack::PropCalc::Sinsq );
	else
	    xaxistypefld_->setValue( PreStack::PropCalc::Norm );
    }

    angleTypSel( 0 );
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
	if ( finalised() )
	    xrgfld_->setEmpty();

	if ( iscomputeangle )
	    xaxistypefld_->setValue( PreStack::PropCalc::Sinsq );
	else
	    xaxistypefld_->setValue( PreStack::PropCalc::Norm );
    }

    xaxistypefld_->setSensitive( isoffset && !iscomputeangle );
}


void uiPreStackAttrib::gatherUnitSel( CallBacker* )
{
    const bool isdegrees = xunitfld_->getIntValue() == 0;
    if ( xunitfld_->isDisplayed() )
	xrglbl_->setText( isdegrees ? tr("degrees") : tr("radians") );
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
