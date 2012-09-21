/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2003
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uievaluatedlg.h"
#include "uigeninput.h"
#include "datainpspec.h"
#include "iopar.h"
#include "uislider.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "uiattrdesced.h"

#include "attribparam.h"
#include "attribparamgroup.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribsel.h"
#include "odver.h"
#include "survinfo.h"

using namespace Attrib;


static const char* sKeyInit = "Initial value";
static const char* sKeyIncr = "Increment";


#define mGetValParFromGroup( T, str, desc )\
{\
    mDescGetConstParamGroup(T,str,desc,parstr1_);\
    valpar1 = &(ValParam&)(*str)[pgidx_];\
}
    
AttribParamGroup::AttribParamGroup( uiParent* p, const uiAttrDescEd& ade,
				    const EvalParam& evalparam )
    : uiGroup(p,"")
    , incrfld(0)
    , parstr1_(evalparam.par1_)
    , parstr2_(evalparam.par2_)
    , parlbl_(evalparam.label_)
    , evaloutput_(evalparam.evaloutput_)
    , pgidx_(evalparam.pgidx_)
    , desced_(ade)
{
    if ( evaloutput_ )
    {
	const float val = ade.getOutputValue( ade.curDesc()->selectedOutput() );
	initfld = new uiGenInput( this, sKeyInit, FloatInpSpec(val) );
	setHAlignObj( initfld );
	return;
    }

    if ( !ade.curDesc() )
	return;

    const ValParam* valpar1 = ade.curDesc()->getValParam( parstr1_ );
    const ValParam* valpar2 = ade.curDesc()->getValParam( parstr2_ );

    if ( !valpar1 && !valpar2 && !mIsUdf(pgidx_) )
    {
	mGetValParFromGroup(FloatParam,fpset,(*ade.curDesc()));
	if ( !valpar1 ) mGetValParFromGroup( IntParam, ipset, (*ade.curDesc()));
	if ( !valpar1 ) mGetValParFromGroup(ZGateParam,zgpset,(*ade.curDesc()));
//	if ( !valpar1 ) mGetValParFromGroup(BinIDParam,bpset,(*ade.curDesc()));
    }
    
    DataInpSpec* initspec1 = 0; DataInpSpec* initspec2 = 0;
    DataInpSpec* incrspec1 = 0; DataInpSpec* incrspec2 = 0;
    if ( valpar1 )
	createInputSpecs( valpar1, initspec1, incrspec1 );
    if ( valpar2 )
	createInputSpecs( valpar2, initspec2, incrspec2 );

    if ( initspec1 && initspec2 )
	initfld = new uiGenInput( this, sKeyInit, *initspec1, *initspec2 );
    else
	initfld = new uiGenInput( this, sKeyInit, *initspec1 );

    if ( incrspec1 && incrspec2 )
	incrfld = new uiGenInput( this, sKeyIncr, *incrspec1, *incrspec2 );
    else if ( incrspec1 )
	incrfld = new uiGenInput( this, sKeyIncr, *incrspec1 );

    if ( incrfld )
	incrfld->attach( alignedBelow, initfld );
    setHAlignObj( initfld );

    delete initspec1; delete incrspec1;
    delete initspec2; delete incrspec2;
}


void AttribParamGroup::createInputSpecs( const Attrib::ValParam* param,
					 DataInpSpec*& initspec,
					 DataInpSpec*& incrspec )
{
    mDynamicCastGet(const ZGateParam*,gatepar,param);
    mDynamicCastGet(const BinIDParam*,bidpar,param);
    mDynamicCastGet(const FloatParam*,fpar,param);
    mDynamicCastGet(const IntParam*,ipar,param);

    if ( gatepar )
    {
	initspec = new FloatInpIntervalSpec( gatepar->getValue() );
	const float zfac = SI().zIsTime() ? 1000.f : 1.f;
	const float step = SI().zStep() * zfac;
	incrspec = new FloatInpIntervalSpec( Interval<float>(-step,step) );
    }
    else if ( bidpar )
    {
	initspec = new PositionInpSpec( bidpar->getValue() );
	incrspec = new PositionInpSpec( bidpar->getValue() );
	mDynamicCastGet( PositionInpSpec*,initspc,initspec )
	mDynamicCastGet( PositionInpSpec*,incrspc,incrspec )
	if ( desced_.is2D() )
	{
	    initspc->setup().is2d_ = true;
	    incrspc->setup().is2d_ = true;
	}
    }
    else if ( fpar )
    {
	initspec = new FloatInpSpec( fpar->getfValue() );
	const float step = fpar->limits() ? fpar->limits()->step : 1;
	incrspec = new FloatInpSpec( step );
    }
    else if ( ipar )
    {
	initspec = new IntInpSpec( ipar->getIntValue() );
	const int step = ipar->limits() ? ipar->limits()->step : 1;
	incrspec = new IntInpSpec( step );
    }
}


#define mCreateLabel1(val) \
    evallbl_ = parlbl_; evallbl_ += " ["; evallbl_ += val; evallbl_ += "]";

#define mCreateLabel2(val1,val2) evallbl_ = parlbl_; \
    evallbl_ += " ["; if ( !mIsUdf(val1) ) evallbl_ += val1; \
    evallbl_ += ","; if ( !mIsUdf(val2) ) evallbl_ += val2; evallbl_ += "]";

void AttribParamGroup::updatePars( Attrib::Desc& desc, int idx )
{
    ValParam* valpar1 = desc.getValParam( parstr1_ );
    if ( !valpar1 && !mIsUdf(pgidx_) )
    {
	mGetValParFromGroup( FloatParam, fparamset, desc );
	if ( !valpar1 ) mGetValParFromGroup( IntParam, iparamset, desc );
	if ( !valpar1 ) mGetValParFromGroup( ZGateParam, zgparamset, desc );
//	if ( !valpar1 ) mGetValParFromGroup( BinIDParam, bidparamset, desc );
    }
	
    mDynamicCastGet(ZGateParam*,gatepar,valpar1)
    mDynamicCastGet(BinIDParam*,bidpar,valpar1)
    mDynamicCastGet(FloatParam*,fpar,valpar1)
    mDynamicCastGet(IntParam*,ipar,valpar1)

    if ( gatepar )
    {
	const Interval<float> oldrg( initfld->getFInterval() );
	const Interval<float> incr( incrfld->getFInterval() );
	Interval<float> newrg( oldrg );
	if ( !mIsUdf(oldrg.start) ) newrg.start += idx * incr.start;
	if ( !mIsUdf(oldrg.stop) ) newrg.stop += idx * incr.stop;
	mCreateLabel2(newrg.start,newrg.stop)
	gatepar->setValue( newrg );
    }
    else if ( bidpar )
    {
	BinID bid;
	bid.inl = initfld->getBinID().inl + idx * incrfld->getBinID().inl;
	bid.crl = initfld->getBinID().crl + idx * incrfld->getBinID().crl;
	
	if ( desc.is2D() )
	    { mCreateLabel1(bid.crl) }
	else
	    { mCreateLabel2(bid.inl,bid.crl) }

	bidpar->setValue( bid.inl, 0 );
	bidpar->setValue( bid.crl, 1 );

	ValParam* valpar2 = desc.getValParam( parstr2_ );
	mDynamicCastGet(BinIDParam*,bidpar2,valpar2)
	if ( bidpar2 )
	{
	    BinID bid2;
	    bid2.inl = initfld->getBinID(1).inl + 
					idx * incrfld->getBinID(1).inl;
	    bid2.crl = initfld->getBinID(1).crl + 
					idx * incrfld->getBinID(1).crl;
	    bidpar2->setValue( bid2.inl, 0 );
	    bidpar2->setValue( bid2.crl, 1 );

	    evallbl_ += "[";
	    if ( desc.is2D() )
		evallbl_ += bid2.crl;
	    else
	    {
		evallbl_ += bid2.inl;
		evallbl_ += ",";
		evallbl_ += bid2.crl;
	    }
	    evallbl_ += "]";
	}
    }
    else if ( fpar )
    {
	const float val = initfld->getfValue() + idx * incrfld->getfValue();
	mCreateLabel1(val)
	fpar->setValue( val );
    }
    else if ( ipar )
    {
	const int val = initfld->getIntValue() + idx * incrfld->getIntValue();
	mCreateLabel1(val)
	ipar->setValue( val );
    }
    else
	return;
}


void AttribParamGroup::updateDesc( Attrib::Desc& desc, int idx )
{
    if ( !evaloutput_ ) return;

    float step = desced_.getOutputValue( 0 );
    if ( mIsZero(step,mDefEps) ) step = desced_.getOutputValue( 1 );

    const float val = initfld->getfValue() + idx*step;
    desc.selectOutput( desced_.getOutputIdx(val) );
    mCreateLabel1( val );
}



static const StepInterval<int> cSliceIntv(2,30,1);

uiEvaluateDlg::uiEvaluateDlg( uiParent* p, uiAttrDescEd& ade, bool store )
    : uiDialog(p,uiDialog::Setup("Evaluate attribute","Set parameters"
				,"101.3.1")
	    	.modal(false)
	        .oktext("Accept")
	        .canceltext(""))
    , calccb(this)
    , showslicecb(this)
    , desced_(ade)
    , initpar_(*new IOPar)
    , seldesc_(0)
    , enabstore_(store)
    , srcid_(-1,true)
    , haspars_(false)
{
    srcid_ = ade.curDesc()->id();
    DescSet* newattrset = ade.curDesc()->descSet()->optimizeClone( srcid_ );
    if ( newattrset ) attrset_ = newattrset;
    attrset_->fillPar( initpar_ );

    TypeSet<EvalParam> params;
    desced_.getEvalParams( params );
    if ( params.isEmpty() ) return;

    haspars_ = true;
    BufferStringSet strs;
    for ( int idx=0; idx<params.size(); idx++ )
	strs.add( params[idx].label_ );

    evalfld = new uiGenInput( this, "Evaluate", StringListInpSpec(strs) );
    evalfld->valuechanged.notify( mCB(this,uiEvaluateDlg,variableSel) );

    uiGroup* pargrp = new uiGroup( this, "" );
    pargrp->setStretch( 1, 1 );
    pargrp->attach( alignedBelow, evalfld );
    for ( int idx=0; idx<params.size(); idx++ )
	grps_ += new AttribParamGroup( pargrp, ade, params[idx] );

    pargrp->setHAlignObj( grps_[0] );

    nrstepsfld = new uiLabeledSpinBox( this, "Nr of steps" );
    nrstepsfld->box()->setInterval( cSliceIntv );
    nrstepsfld->attach( alignedBelow, pargrp );

    calcbut = new uiPushButton( this, "Calculate", true );
    calcbut->activated.notify( mCB(this,uiEvaluateDlg,calcPush) );
    calcbut->attach( rightTo, nrstepsfld );

    sliderfld = new uiSliderExtra( this, "Slice", "Slice slider" );
    sliderfld->attach( alignedBelow, nrstepsfld );
    sliderfld->sldr()->valueChanged.notify( 
	    				mCB(this,uiEvaluateDlg,sliderMove) );
    sliderfld->sldr()->setTickMarks( uiSlider::Below );
    sliderfld->setSensitive( false );

    storefld = new uiCheckBox( this, "Store slices on 'Accept'" );
    storefld->attach( alignedBelow, sliderfld );
    storefld->setChecked( false );
    storefld->setSensitive( false );

    displaylbl = new uiLabel( this, "" );
    displaylbl->attach( widthSameAs, sliderfld );
    displaylbl->attach( alignedBelow, storefld );

    postFinalise().notify( mCB(this,uiEvaluateDlg,doFinalise) );
}


void uiEvaluateDlg::doFinalise( CallBacker* )
{
    variableSel(0);
}


uiEvaluateDlg::~uiEvaluateDlg()
{
    deepErase( lbls_ );
}


void uiEvaluateDlg::variableSel( CallBacker* )
{
    const int sel = evalfld->getIntValue();
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->display( idx==sel );
}


void uiEvaluateDlg::calcPush( CallBacker* )
{
    float vsn = mODMajorVersion + 0.1*mODMinorVersion;
    attrset_->usePar( initpar_, vsn );
    sliderfld->sldr()->setValue(0);
    lbls_.erase();
    specs_.erase();

    const int sel = evalfld->getIntValue();
    if ( sel >= grps_.size() ) return;
    AttribParamGroup* pargrp = grps_[sel];

    Desc& srcad = *attrset_->getDesc( srcid_ );
    BufferString userchosenref = srcad.userRef();
    const int nrsteps = nrstepsfld->box()->getValue();
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	Desc* newad = idx ? new Desc(srcad) : &srcad;
	if ( !newad ) return;
	pargrp->updatePars( *newad, idx );
	pargrp->updateDesc( *newad, idx );

	BufferString defstr; newad->getDefStr( defstr ); //Only for debugging

	const char* lbl = pargrp->getLabel();
	BufferString usrref = newad->attribName();
	usrref += " - "; usrref += lbl;
	newad->setUserRef( usrref );
	if ( newad->selectedOutput()>=newad->nrOutputs() )
	{
	    newad->ref(); newad->unRef();
	    continue;
	}

	if ( idx ) 
	    attrset_->addDesc( newad );

	lbls_ += new BufferString( lbl );
	SelSpec as; as.set( *newad );

	// trick : use as -> objectRef to store the userref;
	// possible since objref_ is only used for NN which cannot be evaluated
	as.setObjectRef( userchosenref.buf() );
		
	specs_ += as;
    }

    if ( specs_.isEmpty() )
	return;

    calccb.trigger();

    if ( enabstore_ ) storefld->setSensitive( true );
    sliderfld->setSensitive( true );
    sliderfld->sldr()->setMaxValue( nrsteps-1 );
    sliderfld->sldr()->setTickStep( 1 );
    sliderMove(0);
}


void uiEvaluateDlg::sliderMove( CallBacker* )
{
    const int sliceidx = sliderfld->sldr()->getIntValue();
    if ( sliceidx >= lbls_.size() )
	return;

    displaylbl->setText( lbls_[sliceidx]->buf() );
    showslicecb.trigger( sliceidx );
}


bool uiEvaluateDlg::acceptOK( CallBacker* )
{
    const int sliceidx = sliderfld->sldr()->getIntValue();
    if ( sliceidx < specs_.size() )
	seldesc_ = attrset_->getDesc( specs_[sliceidx].id() );

    return true;
}


void uiEvaluateDlg::getEvalSpecs( TypeSet<Attrib::SelSpec>& specs ) const
{
    specs = specs_;
}


bool uiEvaluateDlg::storeSlices() const
{
    return enabstore_ ? storefld->isChecked() : false;
}
