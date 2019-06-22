/*+
   * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
   * AUTHOR   : Nageswara
   * DATE     : Mar 2008
 -*/


#include "uistratamp.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attriboutput.h"
#include "trckeyzsampling.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "keystrs.h"
#include "stratamp.h"
#include "survinfo.h"
#include "stattype.h"

#include "uiattrsel.h"
#include "uibatchjobdispatchersel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiioobjsel.h"
#include "uipossubsel.h"
#include "uistrings.h"
#include "od_helpids.h"


uiStratAmpCalc::uiStratAmpCalc( uiParent* p )
    : uiDialog( p, Setup(tr("Stratal Amplitude"),mNoDlgTitle,
			 mODHelpKey(mStratAmpCalcHelpID)))
    , isoverwrite_(false)
{
    inpfld_ = new uiAttrSel( this, Attrib::DescSet::global3D(),
			     uiAttrSel::sQuantityToOutput() );
    mAttachCB( inpfld_->selectionChanged, uiStratAmpCalc::inpSel );

    classfld_ = new uiGenInput( this, tr("Values are classifications"),
				  BoolInpSpec(false) );
    classfld_->attach( alignedBelow, inpfld_ );

    winoption_= new uiGenInput( this, tr("Window Option"),
				BoolInpSpec(true, tr("Single Horizon"),
				tr("Double Horizon")) );
    mAttachCB( winoption_->valuechanged, uiStratAmpCalc::choiceSel );
    winoption_->attach( alignedBelow, classfld_ );

    const IOObjContext ctxt1 = mIOObjContext( EMHorizon3D );
    horfld1_ = new uiIOObjSel( this, ctxt1, uiStrings::sHorizon() );
    mAttachCB( horfld1_->selectionDone, uiStratAmpCalc::inpSel );
    horfld1_->attach( alignedBelow, winoption_ );

    const IOObjContext ctxt2 = mIOObjContext( EMHorizon3D );
    horfld2_ = new uiIOObjSel( this, ctxt2, uiStrings::sBottomHor() );
    mAttachCB( horfld2_->selectionDone, uiStratAmpCalc::inpSel );
    horfld2_->attach( alignedBelow, horfld1_ );

    uiString lbltxt = tr("Z Offset Top").withSurvZUnit();
    tophorshiftfld_ = new uiGenInput( this, lbltxt,
				      FloatInpSpec(0).setName("Top") );
    tophorshiftfld_->attach( alignedBelow, horfld2_ );
    tophorshiftfld_->setElemSzPol( uiObject::Small );
    bothorshiftfld_ = new uiGenInput( this, uiStrings::sBottom(),
				      FloatInpSpec(0) );
    bothorshiftfld_->attach( rightTo, tophorshiftfld_ );
    bothorshiftfld_->setElemSzPol( uiObject::Small );

    rangefld_= new uiPosSubSel( this, uiPosSubSel::Setup(false,false) );
    rangefld_->attach( alignedBelow, tophorshiftfld_ );


#define mAddTyp(typ) \
	add( (int)Stats::typ )
    ampstats_.mAddTyp(Min).mAddTyp(Max).mAddTyp(Average)
	     .mAddTyp(Median).mAddTyp(RMS).mAddTyp(Sum).mAddTyp(MostFreq);
    uiStringSet disptyps;
    for ( int idx=0; idx<ampstats_.size(); idx++ )
	disptyps.add( toUiString( (Stats::Type)ampstats_[idx] ) );
    ampoptionfld_ = new uiLabeledComboBox( this, disptyps,
					   tr("Amplitude Option") );
    ampoptionfld_->attach( alignedBelow, rangefld_ );

    selfld_= new uiGenInput( this, tr("Add result as an attribute to"),
			     BoolInpSpec(true,
			     uiStrings::sTopHor(),uiStrings::sBottomHor()) );
    mAttachCB( selfld_->valuechanged, uiStratAmpCalc::setParFileNameCB );
    selfld_->attach( alignedBelow, ampoptionfld_ );

    foldfld_ = new uiGenInput( this, tr("Output fold as an extra attribute"),
			       BoolInpSpec(false) ) ;
    foldfld_->attach( alignedBelow, selfld_ );

    attribnamefld_ = new uiGenInput( this, uiStrings::sAttribName(),
			             StringInpSpec("Stratal Amplitude") );
    mAttachCB( attribnamefld_->valuechanged, uiStratAmpCalc::setParFileNameCB );
    attribnamefld_->attach( alignedBelow, foldfld_ );

    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::NonODBase );
    batchfld_->attach( alignedBelow, attribnamefld_ );
    batchfld_->jobSpec().prognm_ = "od_stratamp";
    setParFileName();

    mAttachCB( postFinalise(), uiStratAmpCalc::choiceSel );
}


uiStratAmpCalc::~uiStratAmpCalc()
{
    detachAllNotifiers();
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

    const IOObj* hor1ioobj = horfld1_->ioobj( true );
    if ( hor1ioobj )
    {
	EM::IOObjInfo eminfo( hor1ioobj->key() );
	TrcKeySampling emhs;
	emhs.set( eminfo.getInlRange(), eminfo.getCrlRange() );
	hs.limitTo( emhs );
    }

    const IOObj* hor2ioobj = horfld2_->ioobj( true );
    if ( !usesingle_ && hor2ioobj )
    {
	EM::IOObjInfo eminfo( hor2ioobj->key() );
	TrcKeySampling emhs;
	emhs.set( eminfo.getInlRange(), eminfo.getCrlRange() );
	hs.limitTo( emhs );
    }
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiStratAmpCalc::checkInpFlds()
{
    if ( inpfld_->haveSelection() )
	mErrRet( tr("Missing Input\nPlease select the input data"));

    if ( usesingle_ && !horfld1_->ioobj() )
	return false;

    if ( !usesingle_ )
    {
	const IOObj* hor1ioobj = horfld1_->ioobj();
	const IOObj* hor2ioobj = horfld2_->ioobj();
	if ( !hor1ioobj || !hor2ioobj )
	    return false;

	if ( hor1ioobj->key() == hor2ioobj->key() )
	    mErrRet( tr("Select Two Different Horizons") );
    }

    return true;
}


bool uiStratAmpCalc::prepareProcessing()
{
    if ( !checkInpFlds() ) return false;

    const bool addtotop = usesingle_ || selfld_->getBoolValue();
    const EM::IOObjInfo eminfo( addtotop ? horfld1_->key() : horfld2_->key() );
    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );
    const char* attribnm = attribnamefld_->text();
    isoverwrite_ = false;
    if ( attrnms.isPresent( attribnm ) )
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
    iop.setYN( StratAmpCalc::sKeySingleHorizonYN(), usesingle_ );
    iop.set( StratAmpCalc::sKeyTopHorizonID(), horfld1_->key() );
    if ( !usesingle_ )
	iop.set( StratAmpCalc::sKeyBottomHorizonID(), horfld2_->key() );

    const bool addtotop = usesingle_ || selfld_->getBoolValue();
    iop.setYN( StratAmpCalc::sKeyAddToTopYN(), addtotop );
    Stats::Type stattyp = (Stats::Type)ampoptionfld_->box()->currentItem();
    iop.set( StratAmpCalc::sKeyAmplitudeOption(), Stats::toString(stattyp) );
    iop.setYN( StratAmpCalc::sKeyOutputFoldYN(), foldfld_->getBoolValue() );
    iop.set( StratAmpCalc::sKeyTopShift(),
	     tophorshiftfld_->getFValue() / SI().zDomain().userFactor() );
    iop.set( StratAmpCalc::sKeyBottomShift(),
	     bothorshiftfld_->getFValue() / SI().zDomain().userFactor() );
    iop.set( StratAmpCalc::sKeyAttribName(), attribnamefld_->text() );
    iop.setYN( StratAmpCalc::sKeyIsClassification(), classfld_->getBoolValue());
    iop.setYN( StratAmpCalc::sKeyIsOverwriteYN(), isoverwrite_ );

    TrcKeySampling hs;
    getAvailableRange( hs );
    TrcKeySampling inhs = rangefld_->envelope().hsamp_;
    hs.limitTo( inhs );
    IOPar subselpar;
    hs.fillPar( subselpar );
    subselpar.set( sKey::ZRange(), SI().zRange() );
    iop.mergeComp( subselpar, IOPar::compKey(sKey::Output(),sKey::Subsel()) );

    const Attrib::DescID targetid = inpfld_->attribID();
    Attrib::DescSet* procattrset = Attrib::DescSet::global(inpfld_->is2D())
					.optimizeClone( targetid );
    IOPar attrpar( "Attribute Descriptions" );
    if ( !procattrset )
	return false;

    procattrset->fillPar( attrpar );
    for ( int idx=0; idx<attrpar.size(); idx++ )
    {
	const char* nm = attrpar.getKey( idx );
	iop.add( IOPar::compKey(Attrib::SeisTrcStorOutput::attribkey(),nm),
		   attrpar .getValue(idx) );
    }

    Attrib::Desc* desc = procattrset->getDesc( targetid );
    DBKey storedid = desc ? desc->getStoredID() : DBKey::getInvalid();
    if ( storedid.isValid() )
	iop.set( "Input Line Set", storedid );

    const BufferString keybase = IOPar::compKey( Attrib::Output::outputstr(),0);
    const BufferString attribkey =
	    IOPar::compKey( keybase, Attrib::SeisTrcStorOutput::attribkey() );
    iop.set( IOPar::compKey(attribkey,Attrib::DescSet::highestIDStr()), 1 );
    iop.set( IOPar::compKey(attribkey,0), targetid );
    iop.set( IOPar::compKey(sKey::Output(), sKey::Type()), sKey::Cube() );
    return true;
}


bool uiStratAmpCalc::acceptOK()
{
    prepareProcessing() && fillPar() && batchfld_->start();
    return false;
}
