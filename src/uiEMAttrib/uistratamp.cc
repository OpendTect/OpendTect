/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratamp.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "attriboutput.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "stratamp.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

#include "uiattrsel.h"
#include "uibatchjobdispatchersel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiioobjsel.h"
#include "uilineedit.h"
#include "uipossubsel.h"
#include "uistrings.h"
#include "od_helpids.h"


static const char* statstrs[] =
	{ "Min", "Max", "Average", "Median", "RMS", "Sum", "MostFrequent", 0 };

uiStratAmpCalc::uiStratAmpCalc( uiParent* p )
    : uiDialog( p, Setup(tr("Stratal Amplitude"),mNoDlgTitle,
			 mODHelpKey(mStratAmpCalcHelpID)))
{
    setCtrlStyle( RunAndClose );

    const Attrib::DescSet* ads = Attrib::DSHolder().getDescSet(false,false);
    inpfld_ = new uiAttrSel( this, *ads, "Select Attribute" );
    mAttachCB( inpfld_->selectionDone, uiStratAmpCalc::inpSel );

    classfld_ = new uiGenInput( this,
		tr("Values are classifications"), BoolInpSpec(false) );
    classfld_->attach( alignedBelow, inpfld_ );

    winoption_= new uiGenInput( this, tr("Window Option"),
		BoolInpSpec(true,tr("Single Horizon"),tr("Double Horizon")) );
    mAttachCB( winoption_->valueChanged, uiStratAmpCalc::choiceSel );
    winoption_->attach( alignedBelow, classfld_ );

    const IOObjContext ctxt = mIOObjContext( EMHorizon3D );
    horfld1_ = new uiIOObjSel( this, ctxt, uiStrings::sHorizon() );
    mAttachCB( horfld1_->selectionDone, uiStratAmpCalc::inpSel );
    horfld1_->attach( alignedBelow, winoption_ );

    horfld2_ = new uiIOObjSel( this, ctxt, uiStrings::sBottomHor() );
    mAttachCB( horfld2_->selectionDone, uiStratAmpCalc::inpSel );
    horfld2_->attach( alignedBelow, horfld1_ );

    BufferString lbltxt = "Z Offset ";
    lbltxt += SI().getZUnitString(); lbltxt += " Top";
    tophorshiftfld_ = new uiGenInput( this, toUiString(lbltxt),
				      FloatInpSpec(0).setName("Top") );
    tophorshiftfld_->attach( alignedBelow, horfld2_ );
    tophorshiftfld_->setElemSzPol( uiObject::Small );
    bothorshiftfld_ = new uiGenInput( this, uiStrings::sBottom(),
				      FloatInpSpec(0) );
    bothorshiftfld_->attach( rightTo, tophorshiftfld_ );
    bothorshiftfld_->setElemSzPol( uiObject::Small );

    rangefld_= new uiPosSubSel( this, uiPosSubSel::Setup(false,false) );
    rangefld_->attach( alignedBelow, tophorshiftfld_ );

    ampoptionfld_ = new uiLabeledComboBox( this, statstrs,
					   tr("Amplitude Option") );
    ampoptionfld_->attach( alignedBelow, rangefld_ );

    selfld_= new uiGenInput( this, tr("Add result as HorizonData to"),
	BoolInpSpec(true,uiStrings::sTopHor(),uiStrings::sBottomHor()) );
    mAttachCB( selfld_->valueChanged, uiStratAmpCalc::setParFileNameCB );
    selfld_->attach( alignedBelow, ampoptionfld_ );

    foldfld_ = new uiGenInput( this, tr("Output fold as an extra attribute"),
			       BoolInpSpec(false) ) ;
    foldfld_->attach( alignedBelow, selfld_ );

    attribnamefld_ = new uiGenInput( this, uiStrings::sAttribName(),
				     StringInpSpec("Stratal Amplitude") );
    mAttachCB( attribnamefld_->valueChanged, uiStratAmpCalc::setParFileNameCB );
    attribnamefld_->attach( alignedBelow, foldfld_ );
    attribnamefld_->setDefaultTextValidator();

    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::NonODBase );
    batchfld_->attach( alignedBelow, attribnamefld_ );
    batchfld_->jobSpec().prognm_ = "od_stratamp";
    setParFileName();

    mAttachCB( postFinalize(), uiStratAmpCalc::choiceSel );
}


uiStratAmpCalc::~uiStratAmpCalc()
{
    detachAllNotifiers();
}


void uiStratAmpCalc::init()
{
    const Attrib::DescSet* ads = Attrib::DSHolder().getDescSet(false,false);
    inpfld_->setDescSet( ads );
}


void uiStratAmpCalc::choiceSel( CallBacker* )
{
    usesingle_ = winoption_->getBoolValue();
    horfld1_->setLabelText( usesingle_ ? uiStrings::sHorizon()
				       : uiStrings::sTopHor() );
    horfld2_->display( !usesingle_ );
    selfld_->display( !usesingle_ );
}


void uiStratAmpCalc::inpSel( CallBacker* )
{
    TrcKeySampling hs;
    getAvailableRange( hs );
    TrcKeyZSampling incs( rangefld_->envelope() );
    incs.hsamp_ = hs;
    rangefld_->setInput( incs );
    setParFileName();
}


void uiStratAmpCalc::setParFileName()
{
    BufferString jobnm;
    const bool addtotop = usesingle_ || selfld_->getBoolValue();
    addtotop ? jobnm.add( horfld1_->getInput() )
	     : jobnm.add( horfld2_->getInput() );
    jobnm.add( "_").add( attribnamefld_->text() );
    batchfld_->setJobName( jobnm );
}


void uiStratAmpCalc::setParFileNameCB( CallBacker* )
{
    setParFileName();
}


void uiStratAmpCalc::getAvailableRange( TrcKeySampling& hs )
{
    TrcKeyZSampling cs;
    if ( inpfld_->getRanges(cs) )
	hs.limitTo( cs.hsamp_ );

    if ( horfld1_->ioobj(false) )
    {
	const EM::IOObjInfo eminfo( horfld1_->key(true) );
	TrcKeySampling emhs;
	emhs.set( eminfo.getInlRange(), eminfo.getCrlRange() );
	hs.limitTo( emhs );
    }

    if ( !usesingle_ && horfld2_->ioobj(false) )
    {
	const EM::IOObjInfo eminfo( horfld2_->key(true) );
	TrcKeySampling emhs;
	emhs.set( eminfo.getInlRange(), eminfo.getCrlRange() );
	hs.limitTo( emhs );
    }
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiStratAmpCalc::checkInpFlds()
{
    if ( inpfld_->isEmpty() )
	mErrRet( tr("Missing Input\nPlease select the input "
		    "attribute / seismics"));

    const IOObj* ioobj1 = horfld1_->ioobj( true );
    const IOObj* ioobj2 = horfld2_->ioobj( true );
    if ( usesingle_ && !ioobj1 )
	mErrRet( tr("Missing Input\nPlease select the input Horizon") );

    if ( !usesingle_ && (!ioobj1 || !ioobj2) )
	mErrRet( tr("Missing Input\nPlease Check Top / Bottom Horizon") );

    if ( !usesingle_ && ioobj1->key() == ioobj2->key() )
	mErrRet( tr("Select Two Different Horizons") );

    return true;
}


bool uiStratAmpCalc::prepareProcessing()
{
    if ( !checkInpFlds() )
	return false;

    const bool addtotop = usesingle_ || selfld_->getBoolValue();
    const EM::IOObjInfo eminfo( addtotop ? horfld1_->key() : horfld2_->key() );
    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );
    const char* attribnm = attribnamefld_->text();
    isoverwrite_ = false;
    if ( attrnms.isPresent(attribnm) )
    {
	uiString errmsg = tr("Attribute name %1 already exists, Overwrite?")
			.arg(attribnm);
	if ( !uiMSG().askOverwrite(errmsg) )
	    return false;
	else
	    isoverwrite_ = true;
    }

    return true;
}


bool uiStratAmpCalc::fillPar()
{
    IOPar& iop = batchfld_->jobSpec().pars_;
    iop.setEmpty();

    iop.setYN( StratAmpCalc::sKeySingleHorizonYN(), usesingle_ );
    iop.set( StratAmpCalc::sKeyTopHorizonID() , horfld1_->key() );
    if ( !usesingle_ )
	iop.set( StratAmpCalc::sKeyBottomHorizonID(), horfld2_->key() );

    const bool addtotop = usesingle_ || selfld_->getBoolValue();
    iop.setYN( StratAmpCalc::sKeyAddToTopYN(), addtotop );
    iop.set( StratAmpCalc::sKeyAmplitudeOption(), ampoptionfld_->box()->text());
    iop.setYN( StratAmpCalc::sKeyOutputFoldYN(), foldfld_->getBoolValue() );
    iop.set( StratAmpCalc::sKeyTopShift(),
	     tophorshiftfld_->getFValue() / SI().zDomain().userFactor() );
    iop.set( StratAmpCalc::sKeyBottomShift(),
	     bothorshiftfld_->getFValue() / SI().zDomain().userFactor() );
    iop.set( StratAmpCalc::sKeyAttribName(), attribnamefld_->text() );

    const bool isclass = classfld_->getBoolValue();
    iop.setYN( StratAmpCalc::sKeyIsClassification(), isclass );
    iop.setYN( StratAmpCalc::sKeyIsOverwriteYN(), isoverwrite_ );

    TrcKeySampling hs;
    getAvailableRange( hs );
    TrcKeySampling inhs = rangefld_->envelope().hsamp_;
    hs.limitTo( inhs );
    IOPar subselpar;
    hs.fillPar( subselpar );
    subselpar.set( sKey::ZRange(), SI().zRange(false) );
    iop.mergeComp( subselpar, IOPar::compKey(sKey::Output(),sKey::Subsel()) );

    const Attrib::DescID targetid = inpfld_->attribID();
    Attrib::DescSet* clonedset = Attrib::DSHolder().getDescSet(
			    inpfld_->is2D(),false)->optimizeClone( targetid );
    IOPar attrpar( "Attribute Descriptions" );
    if ( !clonedset ) return false;

    clonedset->fillPar( attrpar );
    iop.mergeComp( attrpar, Attrib::SeisTrcStorOutput::attribkey() );

    const Attrib::Desc* desc = clonedset->getDesc( targetid );
    if ( desc && desc->is2D() )
	iop.set( "Input Line Set", desc->getStoredID() );

    delete clonedset;

    const BufferString keybase = IOPar::compKey( Attrib::Output::outputstr(),0);
    const BufferString attribkey =
	    IOPar::compKey( keybase, Attrib::SeisTrcStorOutput::attribkey() );
    iop.set( IOPar::compKey(attribkey,Attrib::DescSet::highestIDStr()), 1 );
    iop.set( IOPar::compKey(attribkey,0), targetid.asInt() );
    iop.set( IOPar::compKey(sKey::Output(), sKey::Type()), sKey::Cube() );
    return true;
}


bool uiStratAmpCalc::acceptOK( CallBacker* )
{
    prepareProcessing() && fillPar() && batchfld_->start();
    return false;
}
