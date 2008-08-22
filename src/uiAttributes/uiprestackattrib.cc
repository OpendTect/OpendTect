/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        B.Bril & H.Huck
 Date:          Jan 2008
 RCS:		$Id: uiprestackattrib.cc,v 1.10 2008-08-22 13:30:42 cvsbert Exp $
________________________________________________________________________

-*/


#include "uiprestackattrib.h"
#include "prestackattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "attribstorprovider.h"
#include "ctxtioobj.h"
#include "multiid.h"
#include "seispsioprov.h"
#include "uiattribfactory.h"
#include "uiseissel.h"
#include "uigeninput.h"
#include "uilabel.h"


mInitAttribUI(uiPreStackAttrib,PreStack,"PreStack",sKeyBasicGrp)


uiPreStackAttrib::uiPreStackAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.17")
	, ctio_(is2d?*mMkCtxtIOObj(SeisPS2D):*mMkCtxtIOObj(SeisPS3D))
{
    inpfld_ = new uiSeisSel( this, ctio_, uiSeisSel::Setup(is2d,true) );
    offsrgfld_ = new uiGenInput( this, "Offset range (empty=all)",
	     FloatInpIntervalSpec(Interval<float>(mUdf(float),mUdf(float))) );
    offsrgfld_->attach( alignedBelow, inpfld_ );

    calctypefld_ = new uiGenInput( this, "Calculation type",
		   StringListInpSpec(SeisPSPropCalc::CalcTypeNames) );
    calctypefld_->attach( alignedBelow, offsrgfld_ );
    calctypefld_->valuechanged.notify( mCB(this,uiPreStackAttrib,calcTypSel) );

    stattypefld_ = new uiGenInput( this, "Statistics type",
				   StringListInpSpec(Stats::TypeNames) );
    stattypefld_->attach( alignedBelow, calctypefld_ );

    lsqtypefld_ = new uiGenInput( this, "LSQ output",
		  StringListInpSpec(SeisPSPropCalc::LSQTypeNames) );
    lsqtypefld_->attach( alignedBelow, calctypefld_ );

    valaxtypefld_ = new uiGenInput( this, "Axis transformations",
		     StringListInpSpec(SeisPSPropCalc::AxisTypeNames) );
    valaxtypefld_->attach( alignedBelow, lsqtypefld_ );
    xlbl_ = new uiLabel( this, "X:" );
    xlbl_->attach( rightOf, valaxtypefld_ );
    offsaxtypefld_ = new uiGenInput( this, "",
		    StringListInpSpec(SeisPSPropCalc::AxisTypeNames)
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
    RefMan<Attrib::PreStack> aps = new Attrib::PreStack( *tmpdesc );

    inpfld_->setInput( aps->psID() );
    Interval<float> offsrg = aps->setup().offsrg_;
    if ( offsrg.start > 1e28 ) offsrg.start = mUdf(float);
    if ( offsrg.stop > 1e28 ) offsrg.stop = mUdf(float);
    offsrgfld_->setValue( offsrg );
    calctypefld_->setValue( (int)aps->setup().calctype_ );
    stattypefld_->setValue( (int)aps->setup().stattype_ );
    lsqtypefld_->setValue( (int)aps->setup().lsqtype_ );
    offsaxtypefld_->setValue( (int)aps->setup().offsaxis_ );
    valaxtypefld_->setValue( (int)aps->setup().valaxis_ );
    useazimfld_->setValue( aps->setup().useazim_ );

    calcTypSel(0);
    return true;
}


bool uiPreStackAttrib::getParameters( Desc& desc )
{
    inpfld_->processInput();
    if ( !ctio_.ioobj )
	{ errmsg_ = "Please select the input data store"; return false; }

    mSetString("id",ctio_.ioobj->key())
    Interval<float> offsrg = offsrgfld_->getFInterval();
    if ( mIsUdf(offsrg.start) ) offsrg.start = 0;
    if ( mIsUdf(offsrg.stop) ) offsrg.start = 1e29;
    mSetFloatInterval(Attrib::PreStack::offsRgStr(),offsrg)
    const int calctyp = calctypefld_->getIntValue();
    mSetEnum(Attrib::PreStack::calctypeStr(),calctyp)
    const bool isnorm = calctyp == 0;
    if ( isnorm )
    {
	mSetEnum(Attrib::PreStack::stattypeStr(),stattypefld_->getIntValue())
    }
    else
    {
	mSetEnum(Attrib::PreStack::lsqtypeStr(),lsqtypefld_->getIntValue())
	mSetEnum(Attrib::PreStack::offsaxisStr(),offsaxtypefld_->getIntValue())
	mSetBool(Attrib::PreStack::useazimStr(),useazimfld_->getBoolValue())
    }
    mSetEnum(Attrib::PreStack::valaxisStr(),valaxtypefld_->getIntValue())
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
