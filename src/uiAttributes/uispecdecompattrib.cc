/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uispecdecompattrib.h"
#include "specdecompattrib.h"

#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "trckeyzsampling.h"
#include "hilbertattrib.h"
#include "survinfo.h"
#include "wavelettrans.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uistrings.h"
#include "uitrcpositiondlg.h"

#include "od_helpids.h"

using namespace Attrib;


mInitAttribUI(uiSpecDecompAttrib,SpecDecomp,"Spectral Decomposition",
	      sKeyFreqGrp())


const char* uiSpecDecompAttrib::sKeyBinID() { return "BinID"; }
const char* uiSpecDecompAttrib::sKeyLineName() { return "Line Name"; }
const char* uiSpecDecompAttrib::sKeyTrcNr() { return "Trace Number"; }

uiSpecDecompAttrib::uiSpecDecompAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d, mODHelpKey(mSpecDecompAttribHelpID) )
    , panelview_(*new uiSpecDecompPanel(p))
{
    inpfld_ = createImagInpFld( is2d );
    mAttachCB( inpfld_->selectionDone, uiSpecDecompAttrib::inputSel );

    typefld_ = new uiGenInput( this, tr("Transform type"),
			      BoolInpSpec(true,tr("FFT"),tr("CWT")) );
    typefld_->attach( alignedBelow, inpfld_ );
    mAttachCB( typefld_->valueChanged, uiSpecDecompAttrib::typeSel );

    gatefld_ = new uiGenInput( this, gateLabel(),
			      DoubleInpIntervalSpec().setName("Z start",0)
						     .setName("Z stop",1) );
    gatefld_->attach( alignedBelow, typefld_ );

    uiString tfstr = tr("Display Time/Frequency panel");
    CallBack cbtfpanel = mCB(this,uiSpecDecompAttrib,panelTFPush);
    tfpanelbut_ = new uiPushButton( this, tfstr, cbtfpanel, true );
    tfpanelbut_->attach( alignedBelow, gatefld_ );

    const int nrdec = zIsTime() ? 1 : 2;
    uiString lbl = uiStrings::phrOutput(uiStrings::phrJoinStrings(
	uiStrings::sFrequency().toLower(), toUiString("(%1)")
	.arg(zIsTime() ? tr("Hz") : (SI().zInMeter() ? tr("cycles/km")
						     : tr("cycles/kft")))));
    outpfld_ = new uiLabeledSpinBox( this, lbl, nrdec );
    outpfld_->attach( alignedBelow, tfpanelbut_ );
    outpfld_->box()->doSnap( true );

    stepfld_ = new uiLabeledSpinBox( this, uiStrings::sStep(), nrdec );
    stepfld_->attach( rightTo, outpfld_ );
    mAttachCB( stepfld_->box()->valueChanged, uiSpecDecompAttrib::stepChg );

    waveletfld_ = new uiGenInput( this, uiStrings::sWavelet(),
				 StringListInpSpec(CWT::WaveletTypeNames()) );
    waveletfld_->attach( alignedBelow, typefld_ );

    stepChg(nullptr);
    typeSel(nullptr);
    prevpar_.setEmpty();
    setHAlignObj( inpfld_ );
}


uiSpecDecompAttrib::~uiSpecDecompAttrib()
{
    detachAllNotifiers();
    delete positiondlg_;
    delete &panelview_;
}


void uiSpecDecompAttrib::inputSel( CallBacker* )
{
    if ( !*inpfld_->getInput() )
	return;

    TrcKeyZSampling cs;
    if ( !inpfld_->getRanges(cs) )
	cs.init(true);

    ds_ = cs.zsamp_.step_;
    const int ns = (int)((cs.zsamp_.stop_-cs.zsamp_.start_)/ds_ + .5) + 1;
    int temp = 2;
    while ( temp  < ns )
	temp *= 2;

    nrsamples_ = temp;
    nyqfreq_ = 0.5f / ds_;

    const float freqstep = zIsTime() ? 0.5f : 0.05f;
    const float freqscale = zIsTime() ? 1.f : 1000.f;
    const float scalednyqfreq = nyqfreq_ * freqscale;
    stepfld_->box()->setInterval( freqstep, scalednyqfreq/2 );
    stepfld_->box()->setStep( freqstep, true );
    outpfld_->box()->setMinValue( stepfld_->box()->getFValue() );
    outpfld_->box()->setMaxValue( scalednyqfreq );
}


void uiSpecDecompAttrib::typeSel( CallBacker* )
{
    bool usefft = typefld_->getBoolValue();
    gatefld_->display( usefft );
    waveletfld_->display( !usefft );
}


void uiSpecDecompAttrib::stepChg( CallBacker* )
{
    if ( mIsZero(stepfld_->box()->getFValue(),mDefEps) )
    {
	stepfld_->box()->setValue( 1 );
	outpfld_->box()->setStep( 1, true );
    }
    else
    {
	outpfld_->box()->setMinValue( stepfld_->box()->getFValue() );
	outpfld_->box()->setStep( stepfld_->box()->getFValue(), true );
    }
}


int uiSpecDecompAttrib::getOutputIdx( float outval ) const
{
    const float step = stepfld_->box()->getFValue();
    return mNINT32(outval/step)-1;
}


float uiSpecDecompAttrib::getOutputValue( int idx ) const
{
    const float step = stepfld_->box()->getFValue();
    return float((idx+1)*step);
}


bool uiSpecDecompAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName()!=SpecDecomp::attribName() )
	return false;

    mIfGetFloatInterval( SpecDecomp::gateStr(),gate, gatefld_->setValue(gate) );
    mIfGetEnum( SpecDecomp::transformTypeStr(), transformtype,
		typefld_->setValue(transformtype==0) );
    mIfGetEnum( SpecDecomp::cwtwaveletStr(), cwtwavelet,
		waveletfld_->setValue(cwtwavelet) );

    const float freqscale = zIsTime() ? 1.f : 1000.f;
    mIfGetFloat( SpecDecomp::deltafreqStr(), deltafreq,
		 stepfld_->box()->setValue(deltafreq*freqscale) );

    stepChg(nullptr);
    typeSel(nullptr);
    return true;
}


bool uiSpecDecompAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    inputSel(nullptr);
    return true;
}


bool uiSpecDecompAttrib::setOutput( const Desc& desc )
{
    const float freq = getOutputValue( desc.selectedOutput() );
    outpfld_->box()->setValue( freq );
    return true;
}


bool uiSpecDecompAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName()!=SpecDecomp::attribName() )
	return false;

    mSetEnum( SpecDecomp::transformTypeStr(),typefld_->getBoolValue() ? 0 : 2 );
    mSetFloatInterval( SpecDecomp::gateStr(), gatefld_->getFInterval() );
    mSetEnum( SpecDecomp::cwtwaveletStr(), waveletfld_->getIntValue() );

    const float freqscale = zIsTime() ? 1.f : 1000.f;
    mSetFloat( SpecDecomp::deltafreqStr(),
	       stepfld_->box()->getFValue()/freqscale );

    return true;
}


bool uiSpecDecompAttrib::getInput( Desc& desc )
{
    fillInp( inpfld_, desc, 0 );
    return true;
}


bool uiSpecDecompAttrib::getOutput( Desc& desc )
{
    checkOutValSnapped();
    const int freqidx = getOutputIdx( outpfld_->box()->getFValue() );
    fillOutput( desc, freqidx );
    return true;
}


void uiSpecDecompAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    EvalParam ep( "Frequency" ); ep.evaloutput_ = true;
    params += ep;
    if ( typefld_->getBoolValue() )
	params += EvalParam( zGateLabel(), SpecDecomp::gateStr() );
}


void uiSpecDecompAttrib::checkOutValSnapped() const
{
    const float oldfreq = outpfld_->box()->getFValue();
    const int freqidx = getOutputIdx( oldfreq );
    const float freq = getOutputValue( freqidx );
    if ( oldfreq>0.5 && oldfreq!=freq )
    {
	uiString wmsg = tr("Chosen output frequency \n"
			   "does not fit with frequency step \n"
			   "and will be snapped to nearest suitable frequency");
	uiMSG().warning( wmsg );
    }
}


void uiSpecDecompAttrib::panelTFPush( CallBacker* cb )
{
    if ( inpfld_->attribID() == DescID::undef() )
    {
	uiMSG().error( tr("Please, first, fill in the Input Data field") );
	return;
    }

    if ( positiondlg_ )
    {
	mDetachCB( positiondlg_->windowClosed, uiSpecDecompAttrib::viewPanalCB);
	delete positiondlg_;
    }

    if ( dpfids_.isEmpty() )
    {
	MultiID mid;
	getInputMID( mid );
	TrcKeyZSampling cs;
	inpfld_->getRanges( cs );
	positiondlg_ = new uiTrcPositionDlg( this, cs, is2D(), mid );
    }
    else
    {
	DataPack::FullID dpfid;
	getInputDPID( inpfld_, dpfid );
	positiondlg_ = new uiTrcPositionDlg( this, dpfid );
    }

    setPrevSel();
    positiondlg_->show();
    mAttachCB( positiondlg_->windowClosed, uiSpecDecompAttrib::viewPanalCB );
}


void uiSpecDecompAttrib::viewPanalCB( CallBacker* )
{
    const int res = positiondlg_->uiResult();
    if ( !res )
	return;

    getPrevSel();
    DescSet* dset = ads_ ? new DescSet( *ads_ ) : new DescSet( is2D() );
    DescID specdecompid = createSpecDecompDesc( dset );
    const TrcKeyZSampling cs( positiondlg_->getTrcKeyZSampling() );
    panelview_.compAndDispAttrib( dset, specdecompid, cs );
}


void uiSpecDecompAttrib::getPrevSel()
{
    if ( dpfids_.size() ) return; //TODO implement for dps

    prevpar_.setEmpty();
    if ( !positiondlg_ )
	return;

    if ( is2D() )
    {
	const char* sellnm = positiondlg_->linesfld_->box()->text();
	prevpar_.set( sKeyLineName(), sellnm );
	prevpar_.set( sKeyTrcNr(),
		      positiondlg_->trcnrfld_->box()->getIntValue() );
	return;
    }

    BinID bid;
    bid.inl() = positiondlg_->inlfld_->box()->getIntValue();
    bid.crl() = positiondlg_->crlfld_->getIntValue();
    prevpar_.set( sKeyBinID(), bid );
}


void uiSpecDecompAttrib::setPrevSel()
{
    if ( dpfids_.size() ) return; //TODO implement for dps
    if ( !positiondlg_ )
    {
	prevpar_.setEmpty();
	return;
    }

    if ( prevpar_.isEmpty() )
	return;

    if ( is2D() )
    {
	BufferString lnm;
	prevpar_.get( sKeyLineName(), lnm );
	positiondlg_->linesfld_->box()->setText( lnm );
	positiondlg_->linesfld_->box()->selectionChanged.trigger();
	int trcnr;
	prevpar_.get( sKeyTrcNr(), trcnr );
	positiondlg_->trcnrfld_->box()->setValue( trcnr );
	return;
    }

    BinID bid;
    prevpar_.get( sKeyBinID(), bid );
    positiondlg_->inlfld_->box()->setValue( bid.inl() );
    positiondlg_->crlfld_->setValue( bid.crl() );
}


void uiSpecDecompAttrib::getInputMID( MultiID& mid ) const
{
    if ( !is2D() ) return;

    RefMan<Desc> tmpdesc;
    if ( ads_ )
	tmpdesc = ads_->getDesc( inpfld_->attribID() );

    if ( !tmpdesc )
	return;

    mid = tmpdesc->getStoredID();
}


RefMan<Desc> uiSpecDecompAttrib::createNewDescFromDP( Attrib::DescSet* dset,
					       const char* attrnm,
					       const char* userefstr ) const
{
    RefMan<Desc> newdesc = PF().createDescCopy( attrnm );
    newdesc->selectOutput( 0 );
    RefMan<Desc> inpdesc = getInputDescFromDP( inpfld_ );
    inpdesc->setDescSet( dset );
    dset->addDesc( inpdesc.ptr() );
    newdesc->setInput( 0, inpdesc.ptr() );
    newdesc->selectOutput( 0 );
    newdesc->setHidden( true );
    BufferString usrref = "_"; usrref += inpdesc->userRef();
    if ( userefstr )
	usrref += userefstr;

    newdesc->setUserRef( usrref );
    return newdesc;
}


DescID uiSpecDecompAttrib::createSpecDecompDesc( DescSet* dset ) const
{
    DescID inpid;
    RefMan<Desc> newdesc;
    if ( dpfids_.size() )
	newdesc = createNewDescFromDP( dset, SpecDecomp::attribName(), nullptr);
    else
    {
	inpfld_->processInput();
	inpid = inpfld_->attribID();
	newdesc = createNewDesc( dset, inpid, SpecDecomp::attribName(),
				 0, 0, "" );
    }

    if ( !newdesc )
	return DescID::undef();

    DescID hilbid;
    createHilbertDesc( dset, hilbid );
    if ( !newdesc->setInput(1,dset->getDesc(hilbid).ptr()) )
	return DescID::undef();

    fillInSDDescParams( newdesc.ptr() );
    newdesc->updateParams();
    newdesc->setUserRef( "spectral decomposition" );
    return dset->addDesc( newdesc.ptr() );
}


RefMan<Desc> uiSpecDecompAttrib::createNewDesc( DescSet* descset, DescID inpid,
					 const char* attribnm, int seloutidx,
					 int inpidx, BufferString specref) const
{
    RefMan<Desc> inpdesc = descset->getDesc( inpid );
    RefMan<Desc> newdesc = PF().createDescCopy( attribnm );
    if ( !newdesc || !inpdesc )
	return nullptr;

    newdesc->selectOutput( seloutidx );
    newdesc->setInput( inpidx, inpdesc.ptr() );
    newdesc->setHidden( true );
    BufferString usrref = "_"; usrref += inpdesc->userRef(); usrref += specref;
    newdesc->setUserRef( usrref );
    return newdesc;
}


#define mSetParam( type, nm, str, fn )\
{ \
    mDynamicCastGet(type##Param*, nm, newdesc->getValParam(str))\
    nm->setValue( fn );\
}


void uiSpecDecompAttrib::fillInSDDescParams( Desc* newdesc ) const
{
    mSetParam(Enum,type,SpecDecomp::transformTypeStr(),
	      typefld_->getBoolValue() ? 0 : 2)
    mSetParam(Enum,cwt,SpecDecomp::cwtwaveletStr(),waveletfld_->getIntValue())
    mSetParam(ZGate,gate,SpecDecomp::gateStr(), gatefld_->getFInterval())

    //show Frequencies with a step of 1 in Time and 1e-3 in Depth,
    //independently of what the user can have specified previously
    //in the output/step fields
    //little trick to have correct axes annotation (at least in time)
    mSetParam(Float,dfreq,SpecDecomp::deltafreqStr(), zIsTime() ? 1.f : 0.001f )
}


void uiSpecDecompAttrib::createHilbertDesc( DescSet* descset,
					    DescID& inputid ) const
{
    RefMan<Desc> hilbertdesc;
    if ( dpfids_.size() )
    {
	hilbertdesc = createNewDescFromDP( descset, Hilbert::attribName(),
					   "_imag" );
    }
    else
    {
	if ( inputid == DescID::undef() )
	{
	    inpfld_->processInput();
	    inputid = inpfld_->attribID();
	}

	TypeSet<DescID> attribids;
	descset->getIds( attribids );
	for ( int idx=0; idx<attribids.size(); idx++ )
	{
	    ConstRefMan<Desc> dsc = descset->getDesc( attribids[idx] );
	    if ( !passStdCheck(dsc.ptr(),Hilbert::attribName(),0,0,inputid) )
		continue;

	    inputid = attribids[idx];
	    return;
	}

	hilbertdesc = createNewDesc( descset, inputid, Hilbert::attribName(),
				     0, 0, "_imag" );
    }

    inputid = hilbertdesc ? descset->addDesc( hilbertdesc.ptr() )
			  : DescID::undef();
}


bool uiSpecDecompAttrib::passStdCheck( const Desc* dsc, const char* attribnm,
				       int seloutidx, int inpidx,
				       DescID inpid ) const
{
    if ( dsc->attribName()!=attribnm )
	return false;

    if ( dsc->selectedOutput() != seloutidx )
	return false;

    ConstRefMan<Desc> inputdesc = dsc->getInput( inpidx );
    if ( !inputdesc || inputdesc->id() != inpid )
	return false;

    return true;
}


// uiSpecDecompPanel
uiSpecDecompPanel::uiSpecDecompPanel( uiParent* p )
    : uiAttribPanel( p )
{}


uiSpecDecompPanel::~uiSpecDecompPanel()
{}


const char* uiSpecDecompPanel::getProcName()
{ return "Compute all frequencies for a single trace"; }

const char* uiSpecDecompPanel::getPackName()
{ return "Spectral Decomposition time/frequency spectrum"; }

const char* uiSpecDecompPanel::getPanelName()
{ return "Time Frequency spectrum"; }
