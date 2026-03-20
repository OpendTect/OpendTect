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
#include "attribsel.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "hiddenparam.h"
#include "stratamp.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

#include "uiattrsel.h"
#include "uibatchjobdispatchersel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"
#include "uimultoutsel.h"
#include "uipossubsel.h"
#include "uistrings.h"
#include "od_helpids.h"

uiStratAmpCalc::Setup::Setup( const char* defmapnm, bool allowattributes )
    : uiDialog::Setup( tr("Stratal Amplitude"), mODHelpKey(mStratAmpCalcHelpID))
    , defmapname_(defmapnm)
    , allowattributes_(allowattributes)
{}


uiStratAmpCalc::Setup::~Setup()
{}


static const char* statstrs[] =
	{ "Min", "Max", "Average", "Median", "RMS", "Sum", "MostFrequent", 0 };

static HiddenParam<uiStratAmpCalc,Attrib::CurrentSel*> hp_sel( nullptr );
static HiddenParam<uiStratAmpCalc,TypeSet<int>*> hp_seloutputs( nullptr );
static HiddenParam<uiStratAmpCalc,BufferStringSet*> hp_seloutnames( nullptr );

uiStratAmpCalc::uiStratAmpCalc( uiParent* p )
    : uiStratAmpCalc( p, Setup() )
{}

uiStratAmpCalc::uiStratAmpCalc( uiParent* p, const Setup& setup )
    : uiDialog( p, uiDialog::Setup(setup.wintitle_,setup.helpkey_))
{
    hp_sel.setParam( this, new Attrib::CurrentSel );
    hp_seloutputs.setParam( this, new TypeSet<int> );
    hp_seloutnames.setParam( this, new BufferStringSet );
    setCtrlStyle( RunAndClose );

    const Attrib::DescSet* ads
		= Attrib::DSHolder().getDescSet(false,!setup.allowattributes_);
    inpfld_ = new uiAttrSel( this, *ads,
			    uiStrings::phrSelect(uiStrings::sAttribute()) );
    mAttachCB( inpfld_->selectionDone, uiStratAmpCalc::inpSel );

    classfld_ = new uiGenInput( this,
		tr("Values are classifications"), BoolInpSpec(false) );
    classfld_->attach( alignedBelow, inpfld_ );

    winoption_= new uiGenInput( this, tr("Window Option"),
		BoolInpSpec(true,tr("Single Horizon"),tr("Double Horizon")) );
    mAttachCB( winoption_->valueChanged, uiStratAmpCalc::choiceSel );
    winoption_->attach( alignedBelow, classfld_ );

    horfld1_ = new uiHorizon3DSel( this, true, uiStrings::sHorizon() );
    mAttachCB( horfld1_->selectionDone, uiStratAmpCalc::inpSel );
    horfld1_->attach( alignedBelow, winoption_ );

    horfld2_ = new uiHorizon3DSel( this, true, uiStrings::sBottomHor() );
    mAttachCB( horfld2_->selectionDone, uiStratAmpCalc::inpSel );
    horfld2_->attach( alignedBelow, horfld1_ );

    const BufferString lbltxt( "Z Offset ",  SI().getZUnitString(), " Top" );
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
			     BoolInpSpec(true,uiStrings::sTopHor(),
					 uiStrings::sBottomHor()) );
    mAttachCB( selfld_->valueChanged, uiStratAmpCalc::setParFileNameCB );
    selfld_->attach( alignedBelow, ampoptionfld_ );

    foldfld_ = new uiGenInput( this, tr("Output fold as an extra attribute"),
			       BoolInpSpec(false) ) ;
    foldfld_->attach( alignedBelow, selfld_ );

    const BufferString defmapname
		= StringView(setup.defmapname_).isEmpty() ? "Stratal Amplitude"
							  : setup.defmapname_;
    attribnamefld_ = new uiGenInput( this, uiStrings::sAttribName(),
				     StringInpSpec(defmapname.buf()) );
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
    hp_sel.removeAndDeleteParam( this );
    hp_seloutputs.removeAndDeleteParam( this );
    hp_seloutnames.removeAndDeleteParam( this );
}


void uiStratAmpCalc::init()
{
    doInit( true );
}


void uiStratAmpCalc::doInit( bool allowattributes )
{
    const Attrib::DescSet* ads
	= Attrib::DSHolder().getDescSet( false, !allowattributes );
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
    auto* sel = hp_sel.getParam( this );
    auto* seloutputs = hp_seloutputs.getParam( this );
    auto* seloutnms = hp_seloutnames.getParam( this );
    seloutnms->erase();
    seloutputs->erase();
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

    if ( inpfld_ )
    {
	sel->attrid_ = inpfld_->attribID();
	sel->outputnr_ = inpfld_->outputNr();
	if ( sel->outputnr_ < 0 && !sel->attrid_.isValid() )
	{
	    uiMSG().error( tr("Please select attribute to calculate") );
	    return false;
	}
    }

    Attrib::DescSet* ads = Attrib::eDSHolder().getDescSet(false,false);
    RefMan<Attrib::Desc> seldesc = ads->getDesc( inpfld_->attribID() );
    if ( seldesc && seldesc->isStored() )
    {
	const int nroutputs = seldesc->nrOutputs();
	if ( nroutputs > 1 )
	{
	    uiMultOutSel multoutdlg( this, *seldesc, false );
	    if ( multoutdlg.doDisp() )
	    {
		if ( multoutdlg.go() )
		{
		    multoutdlg.getSelectedOutputs( *seloutputs );
		    multoutdlg.getSelectedOutNames( *seloutnms );
		}
		else
		    return false;
	    }

	    BufferStringSet availcomps;
	    uiMultOutSel::fillInAvailOutNames( *seldesc, availcomps );
	}
    }

    return true;
}


Attrib::DescSet* uiStratAmpCalc::getFromInpFld(
		TypeSet<Attrib::DescID>& outdescids, int& nrseloutputs )
{
    auto* seloutputs = hp_seloutputs.getParam( this );
    auto* seloutnms = hp_seloutnames.getParam( this );
    Attrib::DescID targetid = inpfld_->attribID();
    Attrib::DescSet* ads = Attrib::eDSHolder().getDescSet(false,false);
    RefMan<Attrib::Desc> seldesc = ads->getDesc( targetid );
    if ( seldesc )
    {
	const bool is2d = inpfld_->is2D();
	Attrib::DescID multoiid = seldesc->getMultiOutputInputID();
	if ( multoiid != Attrib::DescID::undef() )
	{
	    uiAttrSelData attrdata( ads );
	    Attrib::SelInfo attrinf( &attrdata.attrSet(), attrdata.nlamodel_,
				is2d, Attrib::DescID::undef(), false, false );
	    TypeSet<Attrib::SelSpec> targetspecs;
	    if ( !uiMultOutSel::handleMultiCompChain( targetid, multoiid,
				is2d, attrinf, ads, this, targetspecs ) )
		return nullptr;

	    for ( int idx=0; idx<targetspecs.size(); idx++ )
		outdescids += targetspecs[idx].id();
	}
    }

    const int outdescidsz = outdescids.size();
    Attrib::DescSet* ret = outdescidsz ? ads->optimizeClone( outdescids )
				       : ads->optimizeClone( targetid );
    if ( !ret )
	return nullptr;

    nrseloutputs = seloutputs->size() ? seloutputs->size()
				      : outdescidsz ? outdescidsz : 1;
    if ( !seloutputs->isEmpty() )
	ret->createAndAddMultOutDescs( targetid, *seloutputs,
				       *seloutnms, outdescids );
    else if ( outdescids.isEmpty() )
	outdescids += targetid;

    return ret;
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

    PtrMan<Attrib::DescSet> clonedset;
    TypeSet<Attrib::DescID> outdescids;
    int nrseloutputs = 1;
    clonedset = getFromInpFld( outdescids, nrseloutputs );
    if ( !clonedset )
	return false;

    IOPar attrpar( "Attribute Descriptions" );
    if ( !clonedset )
	return false;

    clonedset->fillPar( attrpar );
    iop.mergeComp( attrpar, Attrib::SeisTrcStorOutput::attribkey() );
    const BufferString keybase = IOPar::compKey(Attrib::Output::outputstr(),0);
    const BufferString attribkey =
	IOPar::compKey( keybase, Attrib::SeisTrcStorOutput::attribkey() );
    iop.set( IOPar::compKey(attribkey,Attrib::DescSet::highestIDStr()),
			    nrseloutputs );
    if ( nrseloutputs != outdescids.size() )
	return false;

    for ( int idx=0; idx<nrseloutputs; idx++ )
	iop.set( IOPar::compKey(attribkey,idx), outdescids[idx].asInt() );

    iop.set( IOPar::compKey(sKey::Output(), sKey::Type()), sKey::Cube() );
    return true;
}


bool uiStratAmpCalc::acceptOK( CallBacker* )
{
    prepareProcessing() && fillPar() && batchfld_->start();
    return false;
}
