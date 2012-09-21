/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2004
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uispecdecompattrib.h"
#include "specdecompattrib.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "cubesampling.h"
#include "hilbertattrib.h"
#include "position.h"
#include "survinfo.h"
#include "wavelettrans.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uispinbox.h"
#include "uitrcpositiondlg.h"


using namespace Attrib;


mInitAttribUI(uiSpecDecompAttrib,SpecDecomp,"Spectral Decomp",sKeyFreqGrp())


const char* uiSpecDecompAttrib::sKeyBinID() { return "BinID"; }
const char* uiSpecDecompAttrib::sKeyLineName() { return "Line Name"; }
const char* uiSpecDecompAttrib::sKeyTrcNr() { return "Trace Number"; }

uiSpecDecompAttrib::uiSpecDecompAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d,"101.0.15")
    , nyqfreq_(0)
    , nrsamples_(0)
    , ds_(0)
    , panelview_( new uiSpecDecompPanel(p) )
    , positiondlg_( 0 )
{
    inpfld_ = createImagInpFld( is2d );
    inpfld_->selectionDone.notify( mCB(this,uiSpecDecompAttrib,inputSel) );

    typefld_ = new uiGenInput( this, "Transform type",
	    		      BoolInpSpec(true,"FFT","CWT") );
    typefld_->attach( alignedBelow, inpfld_ );
    typefld_->valuechanged.notify( mCB(this,uiSpecDecompAttrib,typeSel) );

    gatefld_ = new uiGenInput( this, gateLabel(),
	    		      DoubleInpIntervalSpec().setName("Z start",0)
						     .setName("Z stop",1) );
    gatefld_->attach( alignedBelow, typefld_ );

    BufferString tfstr = "&Display Time/Frequency panel";
    CallBack cbtfpanel = mCB(this,uiSpecDecompAttrib,panelTFPush);
    tfpanelbut_ = new uiPushButton( this, tfstr, cbtfpanel, true );
    tfpanelbut_->attach( alignedBelow, gatefld_ );

    BufferString lbl( "Output frequency (" );
    lbl += zIsTime() ? "Hz" :
	(SI().zInMeter() ? "cycles/km" : "cycles/kft"); lbl += ")";
    outpfld_ = new uiLabeledSpinBox( this, lbl, 1 );
    outpfld_->attach( alignedBelow, tfpanelbut_ );
    outpfld_->box()->doSnap( true );

    stepfld_ = new uiLabeledSpinBox( this, "step", 1 );
    stepfld_->attach( rightTo, outpfld_ );
    stepfld_->box()->valueChanged.notify( 
	    			mCB(this,uiSpecDecompAttrib,stepChg) );

    waveletfld_ = new uiGenInput( this, "Wavelet", 
	    			 StringListInpSpec(CWT::WaveletTypeNames()) );
    waveletfld_->attach( alignedBelow, typefld_ );

    stepChg(0);
    typeSel(0);
    prevpar_.setEmpty();
    setHAlignObj( inpfld_ );
}


void uiSpecDecompAttrib::inputSel( CallBacker* )
{
    if ( !*inpfld_->getInput() ) return;

    CubeSampling cs;
    if ( !inpfld_->getRanges(cs) )
	cs.init(true);

    ds_ = cs.zrg.step;
    int ns = (int)((cs.zrg.stop-cs.zrg.start)/ds_ + .5) + 1;
    int temp = 2;
    while ( temp  < ns ) temp *= 2;
    nrsamples_ = temp;
    nyqfreq_ = 0.5f / ds_;

    const float freqscale = zIsTime() ? 1.f : 1000.f;
    const float scalednyqfreq = nyqfreq_ * freqscale;
    stepfld_->box()->setInterval( (float)0.5, scalednyqfreq/2 );
    stepfld_->box()->setStep( (float)0.5, true );
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
    if ( strcmp(desc.attribName(),SpecDecomp::attribName()) )
	return false;

    mIfGetFloatInterval( SpecDecomp::gateStr(),gate, gatefld_->setValue(gate) );
    mIfGetEnum( SpecDecomp::transformTypeStr(), transformtype,
		typefld_->setValue(transformtype==0) );
    mIfGetEnum( SpecDecomp::cwtwaveletStr(), cwtwavelet,
	        waveletfld_->setValue(cwtwavelet) );

    const float freqscale = zIsTime() ? 1.f : 1000.f;
    mIfGetFloat( SpecDecomp::deltafreqStr(), deltafreq,
		 stepfld_->box()->setValue(deltafreq*freqscale) );

    stepChg(0);
    typeSel(0);
    return true;
}


bool uiSpecDecompAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    inputSel(0);
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
    if ( strcmp(desc.attribName(),SpecDecomp::attribName()) )
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
	params += EvalParam( timegatestr(), SpecDecomp::gateStr() );
}


void uiSpecDecompAttrib::checkOutValSnapped() const
{
    const float oldfreq = outpfld_->box()->getFValue();
    const int freqidx = getOutputIdx( oldfreq );
    const float freq = getOutputValue( freqidx );
    if ( oldfreq>0.5 && oldfreq!=freq )
    {
	BufferString wmsg = "Chosen output frequency \n";
	wmsg += "does not fit with frequency step \n";
	wmsg += "and will be snapped to nearest suitable frequency";
	uiMSG().warning( wmsg );
    }
}


void uiSpecDecompAttrib::panelTFPush( CallBacker* cb )
{
    if ( inpfld_->attribID() == DescID::undef() )
    {
	uiMSG().error( "Please, first, fill in the Input Data field" );
	return;
    }

    MultiID mid;
    getInputMID( mid );

    CubeSampling cs;
    inpfld_->getRanges( cs );
    if ( positiondlg_ ) delete positiondlg_;

    positiondlg_ = new uiTrcPositionDlg( this, cs, ads_->is2D(), mid );
    setPrevSel();
    positiondlg_->show();
    positiondlg_->windowClosed.notify(
	    			mCB(this,uiSpecDecompAttrib,viewPanalCB) );
}


void uiSpecDecompAttrib::viewPanalCB( CallBacker* )
{
    const int res = positiondlg_->uiResult();
    if ( !res )
	return;

    getPrevSel();
    DescSet* dset = new DescSet( *ads_ ); 
    DescID specdecompid = createSpecDecompDesc( dset ); 

    LineKey lk;
    if ( dset->is2D() )
	lk = LineKey( positiondlg_->getLineKey() );

    const CubeSampling cs( positiondlg_->getCubeSampling() );
    panelview_->compAndDispAttrib( dset, specdecompid, cs, lk );
}


void uiSpecDecompAttrib::getPrevSel()
{
    prevpar_.setEmpty();
    if ( !positiondlg_ )
	return;

    if ( ads_->is2D() )
    {
	const char* sellnm = positiondlg_->linesfld_->box()->text();
	prevpar_.set( sKeyLineName(), sellnm );
	prevpar_.set( sKeyTrcNr(), positiondlg_->trcnrfld_->box()->getValue() );
	return;
    }

    BinID bid;
    bid.inl = positiondlg_->inlfld_->box()->getValue();
    bid.crl = positiondlg_->crlfld_->getValue();
    prevpar_.set( sKeyBinID(), bid );
}


void uiSpecDecompAttrib::setPrevSel()
{
    if ( !positiondlg_ )
    {
	prevpar_.setEmpty();
	return;
    }

    if ( prevpar_.isEmpty() )
	return;

    if ( ads_->is2D() )
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
    positiondlg_->inlfld_->box()->setValue( bid.inl );
    positiondlg_->crlfld_->setValue( bid.crl );
}


void uiSpecDecompAttrib::getInputMID( MultiID& mid ) const
{                                                                               
    if ( !ads_->is2D() ) return;

    Desc* tmpdesc = ads_->getDesc( inpfld_->attribID() );
    if ( !tmpdesc ) return;

    mid = MultiID( tmpdesc->getStoredID().buf() );
}


DescID uiSpecDecompAttrib::createSpecDecompDesc( DescSet* dset ) const
{
    inpfld_->processInput();
    DescID inpid = inpfld_->attribID();
    
    Desc* newdesc = createNewDesc( dset, inpid,
				   SpecDecomp::attribName(), 0, 0, "" );
    if ( !newdesc )
	return DescID::undef();

    DescID hilbid;
    createHilbertDesc( dset, hilbid );
    if ( !newdesc->setInput( 1, dset->getDesc(hilbid)) )
	return DescID::undef();
    
    fillInSDDescParams( newdesc );
    newdesc->updateParams();
    newdesc->setUserRef( "spectral decomposition" );
    return dset->addDesc( newdesc );
}


Desc* uiSpecDecompAttrib::createNewDesc( DescSet* descset, DescID inpid,
					 const char* attribnm, int seloutidx,
					 int inpidx, BufferString specref) const
{
    Desc* inpdesc = descset->getDesc( inpid );
    Desc* newdesc = PF().createDescCopy( attribnm );
    if ( !newdesc || !inpdesc )
	return 0;

    newdesc->selectOutput( seloutidx );
    newdesc->setInput( inpidx, inpdesc );
    newdesc->setHidden( true );
    BufferString usrref = "_"; usrref += inpdesc->userRef(); usrref += specref; 
    newdesc->setUserRef( usrref );
    return newdesc;
}


#define mSetParam( type, nm, str, fn )\
{\
    mDynamicCastGet(type##Param*, nm, newdesc->getValParam(str))\
    nm->setValue( fn );\
}


void uiSpecDecompAttrib::fillInSDDescParams( Desc* newdesc ) const
{
    mSetParam(Enum,type,SpecDecomp::transformTypeStr(),
	      typefld_->getBoolValue() ? 0 : 2)
    mSetParam(Enum,cwt,SpecDecomp::cwtwaveletStr(),waveletfld_->getIntValue())
    mSetParam(ZGate,gate,SpecDecomp::gateStr(), gatefld_->getFInterval())

    const float freqscale = zIsTime() ? 1.f : 1000.f;
    mSetParam(Float,dfreq,SpecDecomp::deltafreqStr(),
	      stepfld_->box()->getFValue()/freqscale)
}


void uiSpecDecompAttrib::createHilbertDesc( DescSet* descset,
					    DescID& inputid ) const
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
	const Desc* dsc = descset->getDesc( attribids[idx] );
	if ( !passStdCheck( dsc, Hilbert::attribName(), 0 , 0 , inputid ) )
	    continue;

	inputid = attribids[idx];
	return;
    }

    Desc* newdesc = createNewDesc( descset, inputid, Hilbert::attribName(), 0,
				   0, "_imag" );
    inputid = newdesc ? descset->addDesc( newdesc ) : DescID::undef();
}


bool uiSpecDecompAttrib::passStdCheck( const Desc* dsc, const char* attribnm,
				       int seloutidx, int inpidx,
				       DescID inpid ) const
{
    if ( strcmp(dsc->attribName(),attribnm) )
	return false;

    if ( dsc->selectedOutput() != seloutidx )
	return false;

    const Desc* inputdesc = dsc->getInput( inpidx );
    if ( !inputdesc || inputdesc->id() != inpid )
	return false;

    return true;
}

//______________________________________________________________________

const char* uiSpecDecompPanel::getProcName()
{ return "Compute all frequencies for a single trace"; }

const char* uiSpecDecompPanel::getPackName()
{ return "Spectral Decomposition time/frequency spectrum"; }

const char* uiSpecDecompPanel::getPanelName()
{ return "Time Frequency spectrum"; }
