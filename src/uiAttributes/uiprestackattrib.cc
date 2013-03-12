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
#include "uiprestacksel.h"
#include "uiprestackprocessorsel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uiveldesc.h"


mInitAttribUI(uiPreStackAttrib,Attrib::PSAttrib,"PreStack",sKeyBasicGrp())


uiPreStackAttrib::uiPreStackAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.17")
{
    prestackinpfld_ = new uiPreStackSel( this, is2d );

    dopreprocessfld_ = new uiGenInput( this, "Preprocess", BoolInpSpec(false) );
    dopreprocessfld_->attach( alignedBelow, prestackinpfld_ );
    dopreprocessfld_->valuechanged.notify(
	    mCB(this,uiPreStackAttrib,doPreProcSel) );
    preprocsel_ = new PreStack::uiProcSel( this, "Preprocessing setup", 0 );
    preprocsel_->attach( alignedBelow, dopreprocessfld_ );
    offsrgfld_ = new uiGenInput( this, "Offset range (empty=all)",
	     FloatInpIntervalSpec(Interval<float>(mUdf(float),mUdf(float))) );
    offsrgfld_->attach( alignedBelow, preprocsel_ );

    calctypefld_ = new uiGenInput( this, "Calculation type",
		   StringListInpSpec(PreStack::PropCalc::CalcTypeNames()) );
    calctypefld_->attach( alignedBelow, offsrgfld_ );
    calctypefld_->valuechanged.notify( mCB(this,uiPreStackAttrib,calcTypSel) );

    stattypefld_ = new uiGenInput( this, "Statistics type",
				   StringListInpSpec(Stats::TypeNames()) );
    stattypefld_->attach( alignedBelow, calctypefld_ );

    lsqtypefld_ = new uiGenInput( this, "LSQ output",
		  StringListInpSpec(PreStack::PropCalc::LSQTypeNames()) );
    lsqtypefld_->attach( alignedBelow, calctypefld_ );
    lsqtypefld_->valuechanged.notify( mCB(this,uiPreStackAttrib,lsqTypSel) );

    valaxtypefld_ = new uiGenInput( this, "Axis transformations",
		     StringListInpSpec(PreStack::PropCalc::AxisTypeNames()) );
    valaxtypefld_->attach( alignedBelow, lsqtypefld_ );
    xlbl_ = new uiLabel( this, "X:" );
    xlbl_->attach( rightOf, valaxtypefld_ );
    offsaxtypefld_ = new uiGenInput( this, "",
		    StringListInpSpec(PreStack::PropCalc::AxisTypeNames())
	   			      .setName("X") );
    offsaxtypefld_->attach( rightOf, xlbl_ );

    useazimfld_ = new uiGenInput( this, "X = Azimuth", BoolInpSpec(false) );
    useazimfld_->attach( alignedBelow, valaxtypefld_ );

    IOObjContext ctxt = uiVelSel::ioContext();
    ctxt.forread = true;
    uiSeisSel::Setup su( false, false ); su.seltxt( "Velocity Data" );
    velselfld_ = new uiVelSel( this, ctxt, su, false );
    velselfld_->attach( alignedBelow, useazimfld_ );

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
    useazimfld_->setValue( aps->setup().useazim_ );
    velselfld_->setInput( aps->velocityID() );

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
	mSetBool(Attrib::PSAttrib::useazimStr(),useazimfld_->getBoolValue())
	const int lsqtype = lsqtypefld_->getIntValue();
	const bool isangleparam = ( lsqtype==PreStack::PropCalc::AngleA0 || 
	                            lsqtype==PreStack::PropCalc::AngleCoeff );
	if ( isangleparam )
	    mSetString(Attrib::PSAttrib::velocityIDStr(),velselfld_->key())
	
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
    xlbl_->display( !isnorm );
    useazimfld_->display( !isnorm );
    if ( !isnorm )
	lsqTypSel( 0 );
    else
	velselfld_->display( false );
}


void uiPreStackAttrib::lsqTypSel( CallBacker* )
{
    const int lsqtype = lsqtypefld_->getIntValue();
    const bool isangleparam = ( lsqtype==PreStack::PropCalc::AngleA0 || 
	                        lsqtype==PreStack::PropCalc::AngleCoeff );
    velselfld_->display( isangleparam );
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
