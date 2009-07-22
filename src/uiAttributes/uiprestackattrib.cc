/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert & Helene
 Date:          Jan 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiprestackattrib.cc,v 1.20 2009-07-22 16:01:37 cvsbert Exp $";


#include "uiprestackattrib.h"
#include "prestackattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "attribstorprovider.h"
#include "ctxtioobj.h"
#include "multiid.h"
#include "prestackprop.h"
#include "seispsioprov.h"
#include "uiattribfactory.h"
#include "uiseissel.h"
#include "uiprestackprocessorsel.h"
#include "uigeninput.h"
#include "uilabel.h"


mInitAttribUI(uiPreStackAttrib,Attrib::PSAttrib,"PreStack",sKeyBasicGrp())


uiPreStackAttrib::uiPreStackAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.17")
	, ctio_(*uiSeisSel::mkCtxtIOObj(is2d?Seis::LinePS:Seis::VolPS,true))
{
    inpfld_ = new uiSeisSel( this, ctio_, uiSeisSel::Setup(is2d,true) );
    dopreprocessfld_ = new uiGenInput( this, "Preprocess",
	    BoolInpSpec(false) );
    dopreprocessfld_->attach( alignedBelow, inpfld_ );
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

    calcTypSel(0);
    setHAlignObj( inpfld_ );
}


uiPreStackAttrib::~uiPreStackAttrib()
{
    delete ctio_.ioobj; delete &ctio_;
}


bool uiPreStackAttrib::setParameters( const Attrib::Desc& desc )
{
    RefMan<Attrib::Desc> tmpdesc = new Attrib::Desc( desc );
    RefMan<Attrib::PSAttrib> aps = new Attrib::PSAttrib( *tmpdesc );

    inpfld_->setInput( aps->psID() );
    dopreprocessfld_->setValue( !aps->preProcID().isEmpty() );
    preprocsel_->setSel( aps->preProcID() );
    offsrgfld_->setValue( aps->setup().offsrg_ );
    calctypefld_->setValue( (int)aps->setup().calctype_ );
    stattypefld_->setValue( (int)aps->setup().stattype_ );
    lsqtypefld_->setValue( (int)aps->setup().lsqtype_ );
    offsaxtypefld_->setValue( (int)aps->setup().offsaxis_ );
    valaxtypefld_->setValue( (int)aps->setup().valaxis_ );
    useazimfld_->setValue( aps->setup().useazim_ );

    calcTypSel(0);
    doPreProcSel(0);
    return true;
}


bool uiPreStackAttrib::getParameters( Desc& desc )
{
    inpfld_->commitInput();
    if ( !ctio_.ioobj )
	{ errmsg_ = "Please select the input data store"; return false; }

    mSetString("id",ctio_.ioobj->key())

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
