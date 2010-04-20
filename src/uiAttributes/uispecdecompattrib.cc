/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uispecdecompattrib.cc,v 1.28 2010-04-20 18:09:13 cvskris Exp $";

#include "uispecdecompattrib.h"
#include "specdecompattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "cubesampling.h"
#include "survinfo.h"
#include "wavelettrans.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uispinbox.h"

using namespace Attrib;

mInitAttribUI(uiSpecDecompAttrib,SpecDecomp,"Spectral Decomp",sKeyFreqGrp())


uiSpecDecompAttrib::uiSpecDecompAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d,"101.0.15")
    , nyqfreq_(0)
    , nrsamples_(0)
    , ds_(0)
    
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

    BufferString lbl( "Output frequency (" );
    lbl += zIsTime() ? "Hz" : "cycles/mm"; lbl += ")";
    outpfld_ = new uiLabeledSpinBox( this, lbl, 1 );
    outpfld_->attach( alignedBelow, gatefld_ );
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
    nyqfreq_ = 0.5 / ds_;

    const float freqscale = zIsTime() ? 1 : 1000;
    const float scalednyqfreq = nyqfreq_ * freqscale;
    outpfld_->box()->setMinValue( stepfld_->box()->getFValue() );
    outpfld_->box()->setMaxValue( scalednyqfreq );
    stepfld_->box()->setInterval( (float)0.5, scalednyqfreq/2 );
    stepfld_->box()->setStep( (float)0.5, true );
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
    return mNINT(outval/step) - 1;
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

    const float freqscale = zIsTime() ? 1 : 1000;
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

    const float freqscale = zIsTime() ? 1 : 1000;
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

