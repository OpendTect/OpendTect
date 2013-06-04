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
#include "uiattrsel.h"
#include "uiattribfactory.h"
#include "uibutton.h"
#include "uiprestackanglemute.h"
#include "uiprestacksel.h"
#include "uiprestackprocessorsel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uiveldesc.h"


mInitAttribUI(uiPreStackAttrib,Attrib::PSAttrib,"PreStack",sKeyBasicGrp())


static const char*	statTypeCountStr()	{ return "Fold"; }
static const char*	statTypeAverageStr()	{ return "Stack"; }


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

    BufferStringSet stattypenames; getStatTypeNames(stattypenames);
    stattypefld_ = new uiGenInput( this, "Statistics type",
				   StringListInpSpec(stattypenames) );
    stattypefld_->attach( alignedBelow, calctypefld_ );

    lsqtypefld_ = new uiGenInput( this, "AVO output",
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


void uiPreStackAttrib::getStatTypeNames( BufferStringSet& stattypenames )
{
    stattypenames = Stats::TypeNames();
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
    mIfGetString( Attrib::PSAttrib::velocityIDStr(), mid, 
		  params_.velvolmid_=mid )
    mIfGetInt( Attrib::PSAttrib::angleStartStr(), start,
		 params_.anglerange_.start=start )
    mIfGetInt( Attrib::PSAttrib::angleStopStr(), stop, 
		 params_.anglerange_.stop=stop )
		 
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

    if ( smoothtype == PreStack::AngleComputer::TimeAverage )
    {
	BufferString windowfunction;
	mIfGetString( PreStack::AngleComputer::sKeyWinFunc(), winfunc, 
		      windowfunction=winfunc )
	smpar.set( PreStack::AngleComputer::sKeyWinFunc(), windowfunction );

	float windowparam = mUdf(float); 
	mIfGetFloat( PreStack::AngleComputer::sKeyWinParam(), winpar,
		     windowparam=winpar )
	smpar.set( PreStack::AngleComputer::sKeyWinParam(), windowparam );

	float windowlength = mUdf(float);
	mIfGetFloat( PreStack::AngleComputer::sKeyWinLen(), winlen,
		     windowlength=winlen )
	smpar.set( PreStack::AngleComputer::sKeyWinLen(), windowlength );
    }
    else if ( smoothtype == PreStack::AngleComputer::FFTFilter )
    {
	float f3freq = mUdf(float); 
	mIfGetFloat( PreStack::AngleComputer::sKeyFreqF3(), freq, 
		    f3freq=freq  )
	smpar.set( PreStack::AngleComputer::sKeyFreqF3(), f3freq );

	float f4freq = mUdf(float);
	mIfGetFloat( PreStack::AngleComputer::sKeyFreqF4(), freq, 
		    f4freq=freq )
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
    dopreprocessfld_->setValue( !ppid.isEmpty() && ppid.ID(0)!=0 );
    preprocsel_->setSel( ppid );
    offsrgfld_->setValue( aps->setup().offsrg_ );
    calctypefld_->setValue( (int)aps->setup().calctype_ );
    stattypefld_->setText( getStringfromStatEnum(aps->setup().stattype_) );
    lsqtypefld_->setValue( (int)aps->setup().lsqtype_ );
    offsaxtypefld_->setValue( (int)aps->setup().offsaxis_ );
    valaxtypefld_->setValue( (int)aps->setup().valaxis_ );
    useanglefld_->setChecked( aps->setup().useangle_ );
    if ( aps->setup().useangle_ && !setAngleParameters(desc) )
	return false;

    calcTypSel(0);
    doPreProcSel(0);
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

    if ( smoothtype == PreStack::AngleComputer::TimeAverage )
    {
	BufferString winfunc;
	smpar.get( PreStack::AngleComputer::sKeyWinFunc(), winfunc );
	mSetString( PreStack::AngleComputer::sKeyWinFunc(), winfunc )
	float winparam; 
	smpar.get( PreStack::AngleComputer::sKeyWinParam(), winparam );
	mSetFloat( PreStack::AngleComputer::sKeyWinParam(), winparam )
	float winlength;
	smpar.get( PreStack::AngleComputer::sKeyWinLen(), winlength );
	mSetFloat( PreStack::AngleComputer::sKeyWinLen(), winlength )
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
    const bool useangle = useanglefld_->isChecked();
    mSetBool(Attrib::PSAttrib::useangleStr(),useangle)
    if ( useangle && !getAngleParameters(desc) )
	return false;

    const bool isnorm = calctyp == 0;
    if ( isnorm )
    {
	mSetEnum(Attrib::PSAttrib::stattypeStr(),
		 getStatEnumfromString(stattypefld_->text()))
    }
    else
    {
	mSetEnum(Attrib::PSAttrib::lsqtypeStr(),lsqtypefld_->getIntValue())
	mSetEnum(Attrib::PSAttrib::offsaxisStr(),offsaxtypefld_->getIntValue())
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
    angleTypSel( 0 );
}


void uiPreStackAttrib::angleTypSel( CallBacker* )
{
    const bool useangle = useanglefld_->isChecked();
    anglecompgrp_->display( useangle );
    offsrgfld_->display( !useangle );
    offsrglbl_->display( !useangle );
    offsaxtypefld_->setSensitive( !useangle );
    offsaxtypefld_->setValue( useangle ? PreStack::PropCalc::Sinsq 
				       : PreStack::PropCalc::Sqr );
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
