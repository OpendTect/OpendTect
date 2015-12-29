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


mInitAttribUI(uiPreStackAttrib,Attrib::PSAttrib,"Prestack",sKeyBasicGrp())


uiPreStackAttrib::uiPreStackAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d, mODHelpKey(mPreStackAttribHelpID) )
    , params_(*new PreStack::AngleCompParams)
    , statsdef_( Stats::TypeDef() )
{
    prestackinpfld_ = new uiPreStackSel( this, is2d );

    dopreprocessfld_ = new uiGenInput( this, tr("Preprocess"),
                                       BoolInpSpec(false) );
    dopreprocessfld_->attach( alignedBelow, prestackinpfld_ );
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
				   tr("Stack") );
    stattypefld_ = new uiGenInput( this, tr("Statistics type"),
				   StringListInpSpec(statsdef_) );
    stattypefld_->attach( alignedBelow, calctypefld_ );

    lsqtypefld_ = new uiGenInput( this, tr("AVO output"),
		  StringListInpSpec(PreStack::PropCalc::LSQTypeDef()) );
    lsqtypefld_->attach( alignedBelow, calctypefld_ );

    useanglefld_ = new uiCheckBox( this, tr("Use Angles") );
    useanglefld_->attach( rightOf, lsqtypefld_ );
    useanglefld_->activated.notify( mCB(this,uiPreStackAttrib,angleTypSel) );

    gathertypefld_ = new uiGenInput( this, tr("Gather type"),
			     StringListInpSpec(PSAttrib::GatherTypeDef()) );
    gathertypefld_->attach( alignedBelow, stattypefld_ );
    gathertypefld_->valuechanged.notify(
				 mCB(this,uiPreStackAttrib,gatherTypSel) );

    xrgfld_ = new uiGenInput( this, tr("Offset range (empty=all) "),
	     FloatInpIntervalSpec(Interval<float>(mUdf(float),mUdf(float))) );
    xrgfld_->attach( alignedBelow, gathertypefld_ );

    const uiString xlabel = SI().xyInFeet()?tr("feet     "):tr("meters    ");
    xrglbl_ = new uiLabel( this, xlabel );
    xrglbl_->attach( rightOf, xrgfld_ );

    xunitfld_ = new uiGenInput( this, uiString::emptyString(),
				StringListInpSpec(PSAttrib::XaxisUnitDef()) );
    xunitfld_->attach( rightOf, gathertypefld_ );
    xunitfld_->valuechanged.notify( mCB(this,uiPreStackAttrib,gatherUnitSel) );

    xaxistypefld_ = new uiGenInput( this, tr("X Axis Transformation:"),
		    StringListInpSpec(PreStack::PropCalc::AxisTypeDef())
				      .setName("X") );
    xaxistypefld_->attach( alignedBelow, xrgfld_ );

    valaxtypefld_ = new uiGenInput( this, tr("Amplitude transformations"),
		     StringListInpSpec(PreStack::PropCalc::AxisTypeDef()) );
    valaxtypefld_->attach( alignedBelow, xaxistypefld_ );

    anglecompgrp_ = new PreStack::uiAngleCompGrp( this, params_, false, false );
    anglecompgrp_->attach( alignedBelow, valaxtypefld_ );

    calcTypSel(0);
    setHAlignObj( prestackinpfld_ );
}


uiPreStackAttrib::~uiPreStackAttrib()
{
}


bool uiPreStackAttrib::setAngleParameters( const Attrib::Desc& desc )
{
    mIfGetString( Attrib::PSAttrib::velocityIDStr(), mid,
		  params_.velvolmid_=mid )

    Interval<int> anglerange, normalanglevalrange( 0, 90 );
    mIfGetInt( Attrib::PSAttrib::angleStartStr(), start,
		anglerange.start=start )
    mIfGetInt( Attrib::PSAttrib::angleStopStr(), stop,
		anglerange.stop=stop )
    if ( normalanglevalrange.includes(anglerange,false) )
	params_.anglerange_ = anglerange;

    anglecompgrp_->updateFromParams();

    BufferString raytracerparam;
    mIfGetString( Attrib::PSAttrib::rayTracerParamStr(), param,
		  raytracerparam=param )
    params_.raypar_.getParsFrom( raytracerparam );

    IOPar& smpar = params_.smoothingpar_;
    int smoothtype = 0;
    mIfGetEnum( PreStack::AngleComputer::sKeySmoothType(), smtype,
		smoothtype=smtype )
    smpar.set( PreStack::AngleComputer::sKeySmoothType(), smoothtype );

    if ( smoothtype == PreStack::AngleComputer::MovingAverage )
    {
	float windowlength = mUdf(float);
	mIfGetFloat( PreStack::AngleComputer::sKeyWinLen(), winlen,
		     windowlength=winlen )
	if ( !mIsUdf(windowlength) )
	    smpar.set( PreStack::AngleComputer::sKeyWinLen(), windowlength );

	BufferString windowfunction;
	mIfGetString( PreStack::AngleComputer::sKeyWinFunc(), winfunc,
		      windowfunction=winfunc )
	smpar.set( PreStack::AngleComputer::sKeyWinFunc(), windowfunction );

	if ( windowfunction == CosTaperWindow::sName() )
	{
	    float windowparam = mUdf(float);
	    mIfGetFloat( PreStack::AngleComputer::sKeyWinParam(), winpar,
			 windowparam=winpar )
	    if ( !mIsUdf(windowparam) && windowparam >=0 && windowparam <= 1 )
		smpar.set( PreStack::AngleComputer::sKeyWinParam(),
			   windowparam );
	}
    }
    else if ( smoothtype == PreStack::AngleComputer::FFTFilter )
    {
	float f3freq = mUdf(float);
	mIfGetFloat( PreStack::AngleComputer::sKeyFreqF3(), freq,
		    f3freq=freq  )
	if ( !mIsUdf(f3freq) )
	    smpar.set( PreStack::AngleComputer::sKeyFreqF3(), f3freq );

	float f4freq = mUdf(float);
	mIfGetFloat( PreStack::AngleComputer::sKeyFreqF4(), freq,
		    f4freq=freq )
	if ( !mIsUdf(f4freq) )
	    smpar.set( PreStack::AngleComputer::sKeyFreqF4(), f4freq );
    }

    return true;

}


bool uiPreStackAttrib::setParameters( const Attrib::Desc& desc )
{
    RefMan<Attrib::Desc> tmpdesc = new Attrib::Desc( desc );
    RefMan<Attrib::PSAttrib> aps = new Attrib::PSAttrib( *tmpdesc );

    prestackinpfld_->setInput( aps->psID() );

    const MultiID ppid = aps->preProcID();
    if ( !ppid.isEmpty() && ppid.ID(0)!=0 )
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
    useanglefld_->setChecked( aps->setup().useangle_ );
    if ( aps->setup().useangle_ && !setAngleParameters(desc) )
	return false;

    if ( !aps->setup().useangle_ )
    {
	mIfGetEnum(PSAttrib::gathertypeStr(),gtp,gathertypefld_->setValue(gtp));
	mIfGetEnum(PSAttrib::xaxisunitStr(),xut,xunitfld_->setValue(xut));
	Interval<float> offsrg( aps->setup().offsrg_ );
	if ( SI().xyInFeet() && !offsrg.isUdf() &&
	     gathertypefld_->getIntValue() == 0 )
	    offsrg.scale( mToFeetFactorF );

	xrgfld_->setValue( offsrg );
    }

    doPreProcSel(0);
    calcTypSel(0);
    return true;
}


bool uiPreStackAttrib::getAngleParameters( Desc& desc )
{
    if ( !anglecompgrp_->acceptOK() )
	return false;

    mSetString(Attrib::PSAttrib::velocityIDStr(), params_.velvolmid_ );
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
    mSetEnum( PreStack::AngleComputer::sKeySmoothType(), smoothtype )

    if ( smoothtype == PreStack::AngleComputer::MovingAverage )
    {
	float winlength;
	smpar.get( PreStack::AngleComputer::sKeyWinLen(), winlength );
	mSetFloat( PreStack::AngleComputer::sKeyWinLen(), winlength )
	BufferString winfunc;
	smpar.get( PreStack::AngleComputer::sKeyWinFunc(), winfunc );
	mSetString( PreStack::AngleComputer::sKeyWinFunc(), winfunc )
	if ( winfunc == CosTaperWindow::sName() )
	{
	    float winparam;
	    smpar.get( PreStack::AngleComputer::sKeyWinParam(), winparam );
	    if ( winparam>=0 && winparam <= 1 )
		mSetFloat( PreStack::AngleComputer::sKeyWinParam(), winparam )
	}
    }
    else if ( smoothtype == PreStack::AngleComputer::FFTFilter )
    {
	float f3freq;
	smpar.get( PreStack::AngleComputer::sKeyFreqF3(), f3freq );
	mSetFloat( PreStack::AngleComputer::sKeyFreqF3(), f3freq );
	float f4freq;
	smpar.get( PreStack::AngleComputer::sKeyFreqF4(), f4freq );
	mSetFloat( PreStack::AngleComputer::sKeyFreqF4(), f4freq );
    }

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
	    { errmsg_ = uiStrings::phrSelect(tr("preprocessing setup")); 
								return false; }
	mSetString(Attrib::PSAttrib::preProcessStr(), mid );
    }

    const int calctyp = calctypefld_->getIntValue();
    mSetEnum(Attrib::PSAttrib::calctypeStr(),calctyp)
    if ( calctyp == 0 )
    {
	mSetEnum( Attrib::PSAttrib::stattypeStr(),
		  statsdef_.getEnumValForIndex(stattypefld_->getIntValue()));
    }
    else
    {
	mSetEnum(Attrib::PSAttrib::lsqtypeStr(),lsqtypefld_->getIntValue())
	mSetEnum(Attrib::PSAttrib::offsaxisStr(),xaxistypefld_->getIntValue())
    }

    mSetEnum(Attrib::PSAttrib::valaxisStr(),valaxtypefld_->getIntValue())

    const bool useangle = useanglefld_->isChecked();
    mSetBool(Attrib::PSAttrib::useangleStr(),useangle)
    if ( useangle && !getAngleParameters(desc) )
	return false;

    if ( !useangle )
    {
	const int gathertype = gathertypefld_->getIntValue();
	mSetEnum(Attrib::PSAttrib::gathertypeStr(),gathertype)
	mSetEnum(Attrib::PSAttrib::xaxisunitStr(),xunitfld_->getIntValue());
	Interval<float> offsrg = xrgfld_->getFInterval();
	if ( SI().xyInFeet() && !offsrg.isUdf() && gathertype == 0 )
	    offsrg.scale( mFromFeetFactorF );

	if ( mIsUdf(offsrg.start) ) offsrg.start = 0;
	mSetFloat(Attrib::PSAttrib::offStartStr(),offsrg.start)
	mSetFloat(Attrib::PSAttrib::offStopStr(),offsrg.stop)
    }

    return true;
}


void uiPreStackAttrib::doPreProcSel( CallBacker* )
{
    preprocsel_->display( dopreprocessfld_->getBoolValue() );
}


void uiPreStackAttrib::calcTypSel( CallBacker* )
{
    const bool isnorm = calctypefld_->getIntValue() == 0;
    stattypefld_->display( isnorm );
    lsqtypefld_->display( !isnorm );
    xaxistypefld_->display( !isnorm );
    angleTypSel( 0 );
}


void uiPreStackAttrib::angleTypSel( CallBacker* )
{
    if ( is2D() )
    {
	useanglefld_->setChecked( false );
	useanglefld_->display( false );
    }

    const bool useangle = useanglefld_->isChecked();
    anglecompgrp_->display( useangle );
    gathertypefld_->display( !useangle );
    xrgfld_->display( !useangle );
    xrglbl_->display( !useangle );
    xunitfld_->display( !useangle );
    xaxistypefld_->setSensitive( !useangle );
    if ( useangle )
	 xaxistypefld_->setValue( PreStack::PropCalc::Sinsq );
    else
	gatherTypSel( 0 );
}


void uiPreStackAttrib::gatherTypSel( CallBacker* )
{
    const bool isoffset = gathertypefld_->getIntValue() == 0;
    uiString xlbl = tr("%1 range (empty=all)")
				      .arg(toUiString(gathertypefld_->text()));
    xrgfld_->setTitleText( xlbl );
    xunitfld_->display( !isoffset );
    if ( isoffset )
    	xrglbl_->setText( SI().getUiXYUnitString(false,false) );
    else
    {
	xaxistypefld_->setValue( PreStack::PropCalc::Sinsq );
	gatherUnitSel( 0 );
    }

    xaxistypefld_->setSensitive( isoffset );
}


void uiPreStackAttrib::gatherUnitSel( CallBacker* )
{
    const bool isdegrees = xunitfld_->getIntValue() == 0;
    if ( xunitfld_->rightObj()->isDisplayed() )
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
