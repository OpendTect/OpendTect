/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uievaluatedlg.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "attribsel.h"
#include "datainpspec.h"
#include "iopar.h"
#include "od_helpids.h"
#include "survinfo.h"

#include "uiattrdesced.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uislider.h"
#include "uispinbox.h"


using namespace Attrib;


#define mGetValParFromGroup( T, str, desc )\
{\
    mDescGetConstParamGroup(T,str,desc,parstr1_);\
    if ( str && !str->isEmpty() ) \
	valpar1 = &(ValParam&)(*str)[pgidx_];\
}



AttribParamGroup::AttribParamGroup( uiParent* p, const uiAttrDescEd& ade,
				    const EvalParam& evalparam )
    : uiGroup(p,"")
    , parlbl_(evalparam.label_)
    , parstr1_(evalparam.par1_)
    , parstr2_(evalparam.par2_)
    , pgidx_(evalparam.pgidx_)
    , evaloutput_(evalparam.evaloutput_)
    , desced_(ade)
{
    if ( evaloutput_ )
    {
	const Attrib::Desc* desc = ade.curDesc();
	const float val = desc ? ade.getOutputValue( desc->selectedOutput() )
			       : 0;
	initfld_ = new uiGenInput( this, sInit(), FloatInpSpec(val) );
	setHAlignObj( initfld_ );
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
	if ( !valpar1 ) mGetValParFromGroup(DoubleParam,dpset,(*ade.curDesc()));
//	if ( !valpar1 ) mGetValParFromGroup(BinIDParam,bpset,(*ade.curDesc()));
    }

    DataInpSpec* initspec1 = 0; DataInpSpec* initspec2 = 0;
    DataInpSpec* incrspec1 = 0; DataInpSpec* incrspec2 = 0;
    if ( valpar1 )
	createInputSpecs( valpar1, initspec1, incrspec1 );
    if ( valpar2 )
	createInputSpecs( valpar2, initspec2, incrspec2 );

    if ( initspec1 && initspec2 )
	initfld_ = new uiGenInput( this, sInit(), *initspec1, *initspec2 );
    else if ( initspec1 )
	initfld_ = new uiGenInput( this, sInit(), *initspec1 );

    if ( incrspec1 && incrspec2 )
	incrfld_ = new uiGenInput( this, sIncr(), *incrspec1, *incrspec2 );
    else if ( incrspec1 )
	incrfld_ = new uiGenInput( this, sIncr(), *incrspec1 );

    if ( initfld_ )
    {
	if ( incrfld_ )
	    incrfld_->attach( alignedBelow, initfld_ );
	setHAlignObj( initfld_ );
    }

    delete initspec1; delete incrspec1;
    delete initspec2; delete incrspec2;
}


AttribParamGroup::~AttribParamGroup()
{}


uiString AttribParamGroup::sInit()
{ return tr("Initial value"); }


uiString AttribParamGroup::sIncr()
{ return tr("Increment"); }


void AttribParamGroup::createInputSpecs( const Attrib::ValParam* param,
					 DataInpSpec*& initspec,
					 DataInpSpec*& incrspec )
{
    mDynamicCastGet(const ZGateParam*,gatepar,param);
    mDynamicCastGet(const BinIDParam*,bidpar,param);
    mDynamicCastGet(const FloatParam*,fpar,param);
    mDynamicCastGet(const IntParam*,ipar,param);
    mDynamicCastGet(const DoubleParam*,dpar,param);

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
	initspec = new FloatInpSpec( fpar->getFValue() );
	const float step = fpar->limits() ? fpar->limits()->step : 1;
	incrspec = new FloatInpSpec( step );
    }
    else if ( ipar )
    {
	IntInpSpec* ispec = new IntInpSpec( ipar->getIntValue() );
	if ( ipar->limits() )
	    ispec->setLimits( *(ipar->limits()) );
	initspec = ispec;
	const int step = ipar->limits() ? ipar->limits()->step : 1;
	incrspec = new IntInpSpec( step );
    }
    else if ( dpar )
    {
	initspec = new DoubleInpSpec( dpar->getFValue() );
	const double step = dpar->limits() ? dpar->limits()->step : 1;
	incrspec = new DoubleInpSpec( step );
    }
}


#define mCreateLabel1(val) \
    evallbl_ = parlbl_; evallbl_ += " ["; evallbl_ += val; evallbl_ += "]";

#define mCreateLabel2(val1,val2) evallbl_ = parlbl_; \
    evallbl_ += " ["; if ( !mIsUdf(val1) ) evallbl_ += val1; \
    evallbl_ += ","; if ( !mIsUdf(val2) ) evallbl_ += val2; evallbl_ += "]";

void AttribParamGroup::updatePars( Attrib::Desc& desc, int idx )
{
    if ( !initfld_ || !incrfld_ )
	return;

    ValParam* valpar1 = desc.getValParam( parstr1_ );
    if ( !valpar1 && !mIsUdf(pgidx_) )
    {
	mGetValParFromGroup( FloatParam, fparamset, desc );
	if ( !valpar1 ) mGetValParFromGroup( IntParam, iparamset, desc );
	if ( !valpar1 ) mGetValParFromGroup( ZGateParam, zgparamset, desc );
	if ( !valpar1 ) mGetValParFromGroup( DoubleParam, dpset, desc );
//	if ( !valpar1 ) mGetValParFromGroup( BinIDParam, bidparamset, desc );
    }

    mDynamicCastGet(ZGateParam*,gatepar,valpar1)
    mDynamicCastGet(BinIDParam*,bidpar,valpar1)
    mDynamicCastGet(FloatParam*,fpar,valpar1)
    mDynamicCastGet(IntParam*,ipar,valpar1)
    mDynamicCastGet(DoubleParam*,dpar,valpar1)

    if ( gatepar )
    {
	const Interval<float> oldrg( initfld_->getFInterval() );
	const Interval<float> incr( incrfld_->getFInterval() );
	Interval<float> newrg( oldrg );
	if ( !mIsUdf(oldrg.start) ) newrg.start += idx * incr.start;
	if ( !mIsUdf(oldrg.stop) ) newrg.stop += idx * incr.stop;
	mCreateLabel2(newrg.start,newrg.stop)
	gatepar->setValue( newrg );
    }
    else if ( bidpar )
    {
	BinID bid;
	bid.inl() = initfld_->getBinID().inl() + idx * incrfld_->getBinID().inl();
	bid.crl() = initfld_->getBinID().crl() + idx * incrfld_->getBinID().crl();

	if ( desc.is2D() )
	    { mCreateLabel1(bid.crl()) }
	else
	    { mCreateLabel2(bid.inl(),bid.crl()) }

	bidpar->setValue( bid.inl(), 0 );
	bidpar->setValue( bid.crl(), 1 );

	ValParam* valpar2 = desc.getValParam( parstr2_ );
	mDynamicCastGet(BinIDParam*,bidpar2,valpar2)
	if ( bidpar2 )
	{
	    BinID bid2;
	    bid2.inl() = initfld_->getBinID(1).inl() +
					idx * incrfld_->getBinID(1).inl();
	    bid2.crl() = initfld_->getBinID(1).crl() +
					idx * incrfld_->getBinID(1).crl();
	    bidpar2->setValue( bid2.inl(), 0 );
	    bidpar2->setValue( bid2.crl(), 1 );

	    evallbl_ += "[";
	    if ( desc.is2D() )
		evallbl_ += bid2.crl();
	    else
	    {
		evallbl_ += bid2.inl();
		evallbl_ += ",";
		evallbl_ += bid2.crl();
	    }
	    evallbl_ += "]";
	}
    }
    else if ( fpar )
    {
	const float val = initfld_->getFValue() + idx * incrfld_->getFValue();
	mCreateLabel1(val)
	fpar->setValue( val );
    }
    else if ( ipar )
    {
	const int val = initfld_->getIntValue() + idx * incrfld_->getIntValue();
	mCreateLabel1(val)
	ipar->setValue( val );
    }
    else if ( dpar )
    {
	const double val = initfld_->getDValue() + idx * incrfld_->getDValue();
	mCreateLabel1(val)
	dpar->setValue( val );
    }
    else
	return;
}


void AttribParamGroup::updateDesc( Attrib::Desc& desc, int idx )
{
    if ( !evaloutput_ )
	return;

    double step = mCast( double, desced_.getOutputValue(0) );
    if ( mIsZero(step,mDefEps) )
	step = mCast( double, desced_.getOutputValue(1) );

    const double val = initfld_->getDValue() + idx*step;
    desc.selectOutput( desced_.getOutputIdx(mCast(float,val)) );
    mCreateLabel1( val );
}



static const StepInterval<int> cSliceIntv(2,30,1);

uiEvaluateDlg::uiEvaluateDlg( uiParent* p, uiAttrDescEd& ade, bool store )
    : uiDialog(p,uiDialog::Setup(tr("Evaluate attribute"),tr("Set parameters")
				,mODHelpKey(mEvaluateDlgHelpID) )
		.modal(false)
	        .oktext(tr("Accept"))
		.canceltext(uiString::emptyString()))
    , calccb(this)
    , showslicecb(this)
    , seldesc_(0)
    , desced_(ade)
    , srcid_(-1,true)
    , initpar_(*new IOPar)
    , enabstore_(store)
    , haspars_(false)
{
    srcid_ = ade.curDesc()->id();
    attrset_ = ade.curDesc()->descSet()->optimizeClone( srcid_ );
    if ( attrset_ )
	attrset_->fillPar( initpar_ );

    TypeSet<EvalParam> params;
    desced_.getEvalParams( params );
    if ( params.isEmpty() )
	return;

    haspars_ = true;
    BufferStringSet strs;
    for ( int idx=0; idx<params.size(); idx++ )
	strs.add( params[idx].label_ );

    evalfld_ = new uiGenInput( this, tr("Evaluate"),
				StringListInpSpec(strs) );
    evalfld_->valueChanged.notify( mCB(this,uiEvaluateDlg,variableSel) );

    uiGroup* pargrp = new uiGroup( this, "" );
    pargrp->setStretch( 1, 1 );
    pargrp->attach( alignedBelow, evalfld_ );
    for ( int idx=0; idx<params.size(); idx++ )
	grps_ += new AttribParamGroup( pargrp, ade, params[idx] );

    pargrp->setHAlignObj( grps_[0] );

    nrstepsfld_ = new uiLabeledSpinBox( this, tr("Nr of steps") );
    nrstepsfld_->box()->setInterval( cSliceIntv );
    nrstepsfld_->attach( alignedBelow, pargrp );

    calcbut_ = new uiPushButton( this, uiStrings::sCalculate(), true );
    calcbut_->activated.notify( mCB(this,uiEvaluateDlg,calcPush) );
    calcbut_->attach( rightTo, nrstepsfld_ );

    sliderfld_ = new uiSlider( this, tr("Slice"), "Slice slider" );
    sliderfld_->attach( alignedBelow, nrstepsfld_ );
    sliderfld_->valueChanged.notify( mCB(this,uiEvaluateDlg,sliderMove) );
    sliderfld_->setTickMarks( uiSlider::Below );
    sliderfld_->setSensitive( false );

    storefld_ = new uiCheckBox( this, tr("Store slices on 'Accept'") );
    storefld_->attach( alignedBelow, sliderfld_ );
    storefld_->setChecked( false );
    storefld_->setSensitive( false );

    displaylbl_ = new uiLabel( this, uiString::emptyString() );
    displaylbl_->attach( widthSameAs, sliderfld_ );
    displaylbl_->attach( alignedBelow, storefld_ );

    postFinalize().notify( mCB(this,uiEvaluateDlg,doFinalize) );
}


void uiEvaluateDlg::doFinalize( CallBacker* )
{
    variableSel(0);
}


uiEvaluateDlg::~uiEvaluateDlg()
{
    delete &initpar_;
}


void uiEvaluateDlg::variableSel( CallBacker* )
{
    if ( !evalfld_ )
	return;

    const int sel = evalfld_->getIntValue();
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->display( idx==sel );
}


void uiEvaluateDlg::calcPush( CallBacker* )
{
    if ( !attrset_ )
	return;

    attrset_->usePar( initpar_ );
    sliderfld_->setValue(0);
    lbls_.erase();
    specs_.erase();

    const int sel = evalfld_->getIntValue();
    if ( sel >= grps_.size() ) return;
    AttribParamGroup* pargrp = grps_[sel];

    Attrib::Desc* desc = attrset_->getDesc( srcid_ );
    if ( !desc )
	return;

    const BufferString userchosenref = desc->userRef();
    const int nrsteps = nrstepsfld_->box()->getIntValue();
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	Desc* newad = idx ? new Desc(*desc) : desc;
	if ( !newad ) return;
	pargrp->updatePars( *newad, idx );
	pargrp->updateDesc( *newad, idx );

	BufferString defstr; newad->getDefStr( defstr ); //Only for debugging

	const char* lbl = pargrp->getLabel();
	const BufferString usrref( userchosenref.buf(), " - ", lbl );
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

    if ( enabstore_ ) storefld_->setSensitive( true );
    sliderfld_->setSensitive( true );
    sliderfld_->setMaxValue( mCast(float,nrsteps-1) );
    sliderfld_->setTickStep( 1 );
    sliderMove(0);
}


void uiEvaluateDlg::sliderMove( CallBacker* )
{
    const int sliceidx = sliderfld_->getIntValue();
    if ( sliceidx >= lbls_.size() )
	return;

    displaylbl_->setText( toUiString(lbls_[sliceidx]->buf()) );
    showslicecb.trigger( sliceidx );
}


bool uiEvaluateDlg::acceptOK( CallBacker* )
{
    if ( !sliderfld_ || !attrset_ )
	return false;

    const int sliceidx = sliderfld_->getIntValue();
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
    return enabstore_ && storefld_ ? storefld_->isChecked() : false;
}
