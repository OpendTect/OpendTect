/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Dec 2014
________________________________________________________________________

-*/

#include "uisynthseis.h"

#include "stratsynthgenparams.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uisynthgrp.h"


uiMultiSynthSeisSel::uiMultiSynthSeisSel( uiParent* p, const Setup& su )
    : uiMultiSynthSeisSel(p,su,false)
{
}


uiMultiSynthSeisSel::uiMultiSynthSeisSel( uiParent* p, const Setup& su,
					  bool withderived )
    : uiGroup(p)
    , typedef_(*new EnumDef(SynthGenParams::SynthTypeDef()))
    , selectionChanged(this)
    , parsChanged(this)
{
    topgrp_ = new uiGroup( this, "Top group" );
    if ( su.withzeroff_ )
    {
	const uiSynthSeis::Setup sssu( false, su );
	zerooffsynthgrp_ = new uiSynthSeisSel( topgrp_, sssu );
	mAttachCB( zerooffsynthgrp_->parsChanged,
		   uiMultiSynthSeisSel::parsChangedCB );
	synthgrps_.add( zerooffsynthgrp_ );
	topgrp_->setHAlignObj( zerooffsynthgrp_ );
    }
    else
	typedef_.remove( typedef_.getKeyForIndex(SynthGenParams::ZeroOffset) );

    if ( su.withelasticstack_ )
    {
	//TODO
//	topgrp_->setHAlignObj( elasticsynthgrp_ );
    }
/*    else
		*/

    if ( su.withps_ )
    {
	uiSynthSeis::Setup sssu( true, su );
	uiRayTracer1D::Setup rtsu;
	if ( su.rtsu_ )
	    rtsu = *su.rtsu_;

	prestacksynthgrp_ = new uiSynthSeisSel( topgrp_, sssu, rtsu );
	mAttachCB( prestacksynthgrp_->parsChanged,
		   uiMultiSynthSeisSel::parsChangedCB );
	synthgrps_.add( prestacksynthgrp_ );
	topgrp_->setHAlignObj( prestacksynthgrp_ );
    }
    else
	typedef_.remove( typedef_.getKeyForIndex(SynthGenParams::PreStack) );

    if ( !withderived )
    {
	typedef_.remove( typedef_.getKeyForIndex(SynthGenParams::AngleStack) );
	typedef_.remove( typedef_.getKeyForIndex(SynthGenParams::AVOGradient) );
	typedef_.remove( typedef_.getKeyForIndex(SynthGenParams::InstAttrib) );
    }

    typedef_.remove( typedef_.getKeyForIndex(SynthGenParams::StratProp) );

    if ( typedef_.keys().size() > 1 )
    {
	typelblcbx_ = new uiLabeledComboBox( topgrp_, typedef_,
					     tr("Synthetic type") );
	typelblcbx_->box()->setHSzPol( uiObject::Wide );
	mAttachCB( typelblcbx_->box()->selectionChanged,
		   uiMultiSynthSeisSel::selChgCB );
	for ( auto* synthgrp_ : synthgrps_ )
	{
	    synthgrp_->display( false );
	    synthgrp_->attach( alignedBelow, typelblcbx_ );
	}
    }

    setHAlignObj( topgrp_ );

    mAttachCB( postFinalise(), uiMultiSynthSeisSel::initGrpCB );
}


uiMultiSynthSeisSel::~uiMultiSynthSeisSel()
{
    detachAllNotifiers();
    delete &typedef_;
}


void uiMultiSynthSeisSel::initGrpCB( CallBacker* )
{
    NotifyStopper ns( parsChanged );
    initGrp();
}


void uiMultiSynthSeisSel::initGrp()
{
    selChgCB( nullptr );
    previoussynthgrp_ = current();
}


void uiMultiSynthSeisSel::selChgCB( CallBacker* )
{
    selChg( getType() );
    selectionChanged.trigger();
}


void uiMultiSynthSeisSel::selChg( const char* typ )
{
    const SynthGenParams::SynthType synthtype =
			      SynthGenParams::parseEnumSynthType( typ );
    if ( zerooffsynthgrp_ )
    {
	const bool dodisp = synthtype == SynthGenParams::ZeroOffset;
	if ( zerooffsynthgrp_->isDisplayed() && !dodisp )
	    previoussynthgrp_ = zerooffsynthgrp_;
	zerooffsynthgrp_->display( dodisp );
    }
//    if ( elasticsynthgrp_ )
    if ( prestacksynthgrp_ )
    {
	const bool dodisp = synthtype == SynthGenParams::PreStack;
	if ( prestacksynthgrp_->isDisplayed() && !dodisp )
	    previoussynthgrp_ = prestacksynthgrp_;
	prestacksynthgrp_->display( dodisp );
    }

    if ( !previoussynthgrp_ )
	return;

    IOPar par;
    const SynthGenParams::SynthType prevtype =
		previoussynthgrp_ == zerooffsynthgrp_
		? SynthGenParams::ZeroOffset
		: SynthGenParams::PreStack;
    par.set( SynthGenParams::sKeySynthType(),
	     SynthGenParams::toString(prevtype) );
    previoussynthgrp_->fillPar( par );
    doParsChanged( &par );
}


void uiMultiSynthSeisSel::parsChangedCB( CallBacker* )
{
    if ( !parsChanged.isEnabled() )
	return;

    doParsChanged();
    parsChanged.trigger();
}


void uiMultiSynthSeisSel::doParsChanged( IOPar* par )
{
    if ( par )
	uiMultiSynthSeisSel::usePar( *par );
}


const uiSynthSeisSel* uiMultiSynthSeisSel::current() const
{
    return mSelf().current();
}


uiSynthSeisSel* uiMultiSynthSeisSel::current()
{
    const SynthGenParams::SynthType synthtype =
			      SynthGenParams::parseEnumSynthType( getType() );
    if ( synthtype == SynthGenParams::ZeroOffset )
	return zerooffsynthgrp_;
//    if ( synthtype == SynthGenParams::
    if ( synthtype == SynthGenParams::PreStack )
	return prestacksynthgrp_;
    return nullptr;
}


uiRetVal uiMultiSynthSeisSel::isOK() const
{
    if ( current() )
	return current()->isOK();

    return tr("Empty synthetic generation interface");
}


const char* uiMultiSynthSeisSel::getType() const
{
    if ( typelblcbx_ )
	return typelblcbx_->box()->text();

    SynthGenParams::SynthType typ;
    if ( zerooffsynthgrp_ )
	typ = SynthGenParams::ZeroOffset;
//    else if ( elasticsynthgrp_ )
    else if ( prestacksynthgrp_ )
	typ = SynthGenParams::PreStack;
    else
	return nullptr;

    return SynthGenParams::toString( typ );
}


void uiMultiSynthSeisSel::setType( const char* typestr )
{
    if ( typelblcbx_ )
    {
	const BufferString curtype( getType() );
	if ( curtype == typestr )
	    return;

	typelblcbx_->box()->setCurrentItem( typestr );
	selChgCB( nullptr );
    }
}


void uiMultiSynthSeisSel::setWavelet( const MultiID& dbky )
{
    for ( auto* synthsel : synthgrps_ )
	synthsel->setWavelet( dbky );
}


void uiMultiSynthSeisSel::setWavelet( const char* wvltnm )
{
    for ( auto* synthsel : synthgrps_ )
	synthsel->setWavelet( wvltnm );
}


bool uiMultiSynthSeisSel::usePar( const IOPar& iop )
{
    IOPar par( iop );
    par.removeSubSelection( RayTracer1D::sKeyRayPar() );
    // Everything else applies to all types

    SynthGenParams::SynthType synthtype;
    const bool hastype = SynthGenParams::parseEnum( iop,
			 SynthGenParams::sKeySynthType(), synthtype );

    if ( zerooffsynthgrp_ )
	zerooffsynthgrp_->usePar(
	    hastype && synthtype == SynthGenParams::ZeroOffset ? iop : par );
//    if ( elasticsynthgrp_ )
    if ( prestacksynthgrp_ )
	prestacksynthgrp_->usePar(
	    hastype && synthtype == SynthGenParams::PreStack ? iop : par );

    return true;
}


bool uiMultiSynthSeisSel::useSynthSeisPar( const IOPar& iop )
{
    for ( auto* synthsel : synthgrps_ )
	synthsel->useSynthSeisPar( iop );

    return true;
}


bool uiMultiSynthSeisSel::useReflPars( const IOPar& iop )
{
    for ( auto* synthsel : synthgrps_ )
	synthsel->useReflPars( iop );

    return true;
}


MultiID uiMultiSynthSeisSel::getWaveletID() const
{
    MultiID dbky;
    if ( current() )
	dbky = current()->getWaveletID();

    return dbky;
}


const char* uiMultiSynthSeisSel::getWaveletName() const
{
    return current() ? current()->getWaveletName() : nullptr;
}


void uiMultiSynthSeisSel::fillPar( IOPar& iop ) const
{
    if ( current() )
	current()->fillPar( iop );
}


void uiMultiSynthSeisSel::fillSynthSeisPar( IOPar& iop ) const
{
    if ( current() )
	current()->fillSynthSeisPar( iop );
}


void uiMultiSynthSeisSel::fillReflPars( IOPar& iop ) const
{
    if ( current() )
	current()->fillReflPars( iop );
}



uiFullSynthSeisSel::uiFullSynthSeisSel( uiParent* p, const Setup& su )
    : uiMultiSynthSeisSel(p,su,true)
    , nameChanged(this)
{
    uiGroup* topgrp = topGrp();
    psselfld_ = new uiLabeledComboBox( topgrp, tr("Input Prestack") );
    psselfld_->box()->setHSzPol( uiObject::Wide );
    psselfld_->attach( alignedBelow, typeCBFld() );
    mAttachCB( psselfld_->box()->selectionChanged,
	       uiFullSynthSeisSel::inputChangedCB );

    inpselfld_ = new uiLabeledComboBox( topgrp, uiStrings::sInput() );
    inpselfld_->box()->setHSzPol( uiObject::Wide );
    inpselfld_->attach( alignedBelow, typeCBFld() );
    mAttachCB( inpselfld_->box()->selectionChanged,
	       uiFullSynthSeisSel::inputChangedCB );

    const SynthGenParams angsgp( SynthGenParams::AngleStack );
    FloatInpIntervalSpec finpspec(false);
    finpspec.setLimits( Interval<float>(0,89) );
    finpspec.setDefaultValue( angsgp.anglerg_ );
    angleinpfld_ = new uiGenInput( topgrp, tr("Angle range"), finpspec );
    angleinpfld_->setValue( angsgp.anglerg_ );
    angleinpfld_->attach( alignedBelow, psselfld_ );
    mAttachCB( angleinpfld_->valuechanged, uiFullSynthSeisSel::parsChangedCB );

    EnumDef attribs = Attrib::Instantaneous::OutTypeDef();
    attribs.remove( attribs.getKeyForIndex(Attrib::Instantaneous::RotatePhase));
    instattribfld_ = new uiLabeledListBox( topgrp, uiStrings::sAttribute(),
					   OD::ChooseAtLeastOne );

    instattribfld_->box()->addItems( attribs.strings() );
    instattribfld_->attach( alignedBelow, inpselfld_ );
    mAttachCB( instattribfld_->box()->selectionChanged,
	       uiFullSynthSeisSel::parsChangedCB );

    namefld_ = new uiGenInput( this, uiStrings::sName() );
    namefld_->setElemSzPol( uiObject::Wide );
    namefld_->attach( ensureBelow, topgrp );
    namefld_->attach( alignedBelow, topgrp->attachObj() );
    mAttachCB( namefld_->valuechanging, uiFullSynthSeisSel::nameChangedCB );
    mAttachCB( namefld_->valuechanged, uiFullSynthSeisSel::nameChangedCB );
}


uiFullSynthSeisSel::~uiFullSynthSeisSel()
{
    detachAllNotifiers();
}


void uiFullSynthSeisSel::selChg( const char* typ )
{
    uiMultiSynthSeisSel::selChg( typ );
    const SynthGenParams::SynthType synthtype =
			      SynthGenParams::parseEnumSynthType( typ );
    const bool psbased = synthtype == SynthGenParams::AngleStack ||
			 synthtype == SynthGenParams::AVOGradient;
    const bool attrib = synthtype == SynthGenParams::InstAttrib;
    psselfld_->display( psbased );
    angleinpfld_->display( psbased );
    inpselfld_->display( attrib );
    instattribfld_->display( attrib );
}


void uiFullSynthSeisSel::doParsChanged( IOPar* par )
{
    uiMultiSynthSeisSel::doParsChanged( par );
    IOPar iop;
    fillPar( iop );
    const SynthGenParams::SynthType synthtype =
			      SynthGenParams::parseEnumSynthType( getType() );
    BufferString nm;
    SynthGenParams genparams( synthtype );
    genparams.usePar( iop );
    genparams.createName( nm );
    if ( nm == getOutputName() )
	return;

    setOutputName( nm );
    nameChangedCB( nullptr );
}


void uiFullSynthSeisSel::inputChangedCB( CallBacker* cb )
{
    mDynamicCastGet(uiComboBox*,uicbfld,cb);
    if ( uicbfld && uicbfld->size() > 1 &&
	 uicbfld->isPresent(SynthGenParams::sKeyInvalidInputPS()) )
	doMan( uicbfld, SynthGenParams::sKeyInvalidInputPS(), false );

    parsChangedCB( cb );
}


void uiFullSynthSeisSel::nameChangedCB( CallBacker* )
{
    const BufferString newnm( getOutputName() );
    nameChanged.trigger( newnm );
}


void uiFullSynthSeisSel::setOutputName( const char* nm )
{
    namefld_->setText( nm );
}


const char* uiFullSynthSeisSel::getOutputName() const
{
    return namefld_->text();
}


void uiFullSynthSeisSel::manPSSynth( const char* nm, bool isnew )
{
    doMan( psselfld_->box(), nm, isnew );
}


void uiFullSynthSeisSel::manInpSynth( const char* nm, bool isnew )
{
    doMan( inpselfld_->box(), nm, isnew );
}


void uiFullSynthSeisSel::doMan( uiComboBox* cbfld, const char* nm, bool isnew )
{
    NotifyStopper ns( cbfld->selectionChanged );
    const FileMultiString newnames( nm );
    if ( newnames.isEmpty() )
	return;

    BufferStringSet nms;
    const BufferString curnm( cbfld->text() );

    cbfld->getItems( nms );
    if ( isnew )
    {
	bool added = false;
	for ( int idx=0; idx<newnames.size(); idx++ )
	    added = nms.addIfNew( newnames[idx] ) || added;
	if ( !added )
	    return;
    }
    else if ( !isnew )
    {
	for ( int idx=0; idx<newnames.size(); idx++ )
	    nms.remove( newnames[idx] );
    }

    cbfld->setEmpty();
    cbfld->addItems( nms );
    if ( isnew )
    {
	if ( curnm != newnames[0] )
	    cbfld->setCurrentItem( curnm );

	if ( !cbfld->sensitive() )
	{
	    for ( const auto* dsnm : nms )
	    {
		if ( *dsnm != SynthGenParams::sKeyInvalidInputPS() )
		{
		    cbfld->setSensitive( true );
		    break;
		}
	    }
	}
    }
}


bool uiFullSynthSeisSel::usePar( const IOPar& par )
{
    NotifyStopper nssel( selectionChanged );
    NotifyStopper nspars( parsChanged );

    SynthGenParams::SynthType synthtype;
    if ( !SynthGenParams::parseEnum(par,SynthGenParams::sKeySynthType(),
				   synthtype) )
	return false;

    setType( SynthGenParams::toString(synthtype) );
    nssel.enableNotification();

    SynthGenParams genparams( synthtype );

    genparams.usePar( par );
    setOutputName( genparams.name_ );
    if ( genparams.isRawOutput() )
	return uiMultiSynthSeisSel::usePar( par );

    if ( genparams.isPSBased() )
    {
	uiComboBox* psbox = psselfld_->box();
	if ( psbox->isPresent(genparams.inpsynthnm_) )
	{
	    psbox->setCurrentItem( genparams.inpsynthnm_ );
	    psbox->setSensitive( genparams.inpsynthnm_ !=
				 SynthGenParams::sKeyInvalidInputPS() ||
				 psbox->size() > 1 );
	}
	else if ( genparams.inpsynthnm_.isEmpty() )
	    psbox->setSensitive( true );
	else
	{
	    psbox->addItem( genparams.inpsynthnm_ );
	    psbox->setSensitive( false );
	}

	NotifyStopper angparschgstopper( angleinpfld_->valuechanged );
	angleinpfld_->setValue( genparams.anglerg_ );
    }
    else if ( genparams.isAttribute() )
    {
	NotifyStopper ns_inpsel( inpselfld_->box()->selectionChanged );
	NotifyStopper ns_instattrib( instattribfld_->box()->selectionChanged );
	uiComboBox* inpbox = inpselfld_->box();
	if ( inpbox->isPresent(genparams.inpsynthnm_) )
	{
	    inpbox->setCurrentItem( genparams.inpsynthnm_ );
	    inpbox->setSensitive( genparams.inpsynthnm_ !=
				  SynthGenParams::sKeyInvalidInputPS() ||
				  inpbox->size() > 1 );
	}
	else if ( genparams.inpsynthnm_.isEmpty() )
	    inpbox->setSensitive( true );
	else
	{
	    inpbox->addItem( genparams.inpsynthnm_ );
	    inpbox->setSensitive( false );
	}

	instattribfld_->box()->chooseAll( false );
	instattribfld_->box()->setChosen( genparams.attribtype_ );
    }
    else
	return false;

    return true;
}


void uiFullSynthSeisSel::getChosenInstantAttribs( BufferStringSet& nms ) const
{
    instattribfld_->box()->getChosen( nms );
}


#define mErrRet(msg)	{ uirv = msg; return uirv; }

uiRetVal uiFullSynthSeisSel::isOK() const
{
    uiRetVal uirv;
    const BufferString nm( getOutputName() );
    if ( nm.isEmpty() )
	mErrRet(tr("Please specify a valid name"));

    const SynthGenParams::SynthType synthtype =
			SynthGenParams::parseEnumSynthType( getType() );
    const SynthGenParams sgp = SynthGenParams( synthtype );
    if ( sgp.isRawOutput() )
	return uiMultiSynthSeisSel::isOK();

    if ( sgp.isPSBased() )
    {
	if ( psselfld_->box()->isEmpty() )
	    mErrRet( tr("Cannot generate an angle stack synthetics without "
			"any NMO corrected Prestack.") );

	if ( !psselfld_->box()->sensitive() )
	    mErrRet( tr("Cannot change synthetic data as the dependent "
			"prestack synthetic data has already been removed"));

    }
    else if ( sgp.isAttribute() )
    {
	if ( inpselfld_->box()->isEmpty() )
	    mErrRet( tr("Cannot generate attributes without "
			"any post stack synthetics.") );

	if ( !inpselfld_->box()->sensitive() )
	     mErrRet( tr("Cannot change synthetic data as the dependent "
			 "poststack synthetic data has already been removed") );
    }
    else
	mErrRet(tr("Unknown internal error"))

    return uirv;
}


void uiFullSynthSeisSel::fillPar( IOPar& iop ) const
{
    const SynthGenParams::SynthType synthtype =
			      SynthGenParams::parseEnumSynthType( getType() );

    iop.set( SynthGenParams::sKeySynthType(),
	     SynthGenParams::toString(synthtype) );

    SynthGenParams sgp = SynthGenParams( synthtype );
    const BufferString outnm( getOutputName() );
    if ( !outnm.isEmpty() )
    {
	sgp.name_ = outnm;
	iop.set( sKey::Name(), outnm );
    }

    if ( sgp.isRawOutput() )
    {
	uiMultiSynthSeisSel::fillPar( iop );
	iop.set( SynthGenParams::sKeyWaveLetName(), getWaveletName() );
    }
    else if ( sgp.isPSBased() )
    {
	iop.set( SynthGenParams::sKeyInput(), psselfld_->box()->text() );
	iop.set( SynthGenParams::sKeyAngleRange(),angleinpfld_->getFInterval());
    }
    else if ( sgp.isAttribute() )
    {
	const Attrib::Instantaneous::OutType attribtype =
	  (Attrib::Instantaneous::OutType) instattribfld_->box()->firstChosen();
	iop.set( SynthGenParams::sKeyInput(), inpselfld_->box()->text() );
	iop.set( sKey::Attribute(),Attrib::Instantaneous::toString(attribtype));
    }
    else
	iop.setEmpty();
}
