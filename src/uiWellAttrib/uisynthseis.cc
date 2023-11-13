/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisynthseis.h"

#include "stratsynthgenparams.h"
#include "uicombobox.h"
#include "uifreqfilter.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uispinbox.h"
#include "uisynthgrp.h"


// uiMultiSynthSeisSel::Setup

uiMultiSynthSeisSel::Setup::Setup( const char* wvltseltxt )
    : uiSeisWaveletSel::Setup(wvltseltxt)
    , withzeroff_(true)
    , withps_(true)
    , withelasticstack_(canDoElastic())
    , withelasticgather_(canDoElastic())
{
}


uiMultiSynthSeisSel::Setup::~Setup()
{
}


uiMultiSynthSeisSel::Setup&
uiMultiSynthSeisSel::Setup::withelasticstack( bool yn )
{
    withelasticstack_ = yn && canDoElastic();
    return *this;
}


uiMultiSynthSeisSel::Setup&
uiMultiSynthSeisSel::Setup::withelasticgather( bool yn )
{
    withelasticgather_ = yn && canDoElastic();
    return *this;
}


bool uiMultiSynthSeisSel::Setup::withElasticStack() const
{
    return withelasticstack_;
}


bool uiMultiSynthSeisSel::Setup::withElasticGather() const
{
    return withelasticgather_;
}


bool uiMultiSynthSeisSel::Setup::canDoElastic()
{
    const bool noelastic = ReflCalc1D::factory().size() < 2 &&
		ReflCalc1D::factory().hasName( AICalc1D::sFactoryKeyword() );
    return !noelastic;
}


// uiMultiSynthSeisSel

uiMultiSynthSeisSel::uiMultiSynthSeisSel( uiParent* p, const Setup& su )
    : uiMultiSynthSeisSel(p,su,false)
{
}


uiMultiSynthSeisSel::uiMultiSynthSeisSel( uiParent* p, const Setup& su,
					  bool withderived )
    : uiGroup(p)
    , selectionChanged(this)
    , parsChanged(this)
    , typedef_(*new EnumDef(SynthGenParams::SynthTypeDef()))
{
    const EnumDef synthdef = SynthGenParams::SynthTypeDef();
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
	typedef_.remove( synthdef.getKeyForIndex(SynthGenParams::ZeroOffset) );

    if ( su.withElasticStack() )
    {
	uiSynthSeis::Setup sssu( false, su );
	uiReflCalc1D::Setup reflsu( true );
	if ( su.reflsu_ )
	    reflsu = *su.reflsu_;

	elasticstacksynthgrp_ = new uiSynthSeisSel( topgrp_, sssu, reflsu );
	mAttachCB( elasticstacksynthgrp_->parsChanged,
		   uiMultiSynthSeisSel::parsChangedCB );
	synthgrps_.add( elasticstacksynthgrp_ );
	topgrp_->setHAlignObj( elasticstacksynthgrp_ );
    }
    else
	typedef_.remove( synthdef.getKeyForIndex(SynthGenParams::ElasticStack));

    if ( su.withElasticGather() )
    {
	uiSynthSeis::Setup sssu( false, su );
	uiReflCalc1D::Setup reflsu( false );
	if ( su.reflsu_ )
	    reflsu = *su.reflsu_;

	elasticgathersynthgrp_ = new uiSynthSeisSel( topgrp_, sssu, reflsu );
	mAttachCB( elasticgathersynthgrp_->parsChanged,
		   uiMultiSynthSeisSel::parsChangedCB );
	synthgrps_.add( elasticgathersynthgrp_ );
	topgrp_->setHAlignObj( elasticgathersynthgrp_ );
    }
    else
	typedef_.remove(
			synthdef.getKeyForIndex(SynthGenParams::ElasticGather));

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
	typedef_.remove( synthdef.getKeyForIndex(SynthGenParams::PreStack) );

    if ( !withderived )
    {
	typedef_.remove( synthdef.getKeyForIndex(SynthGenParams::AngleStack) );
	typedef_.remove( synthdef.getKeyForIndex(SynthGenParams::AVOGradient) );
	typedef_.remove( synthdef.getKeyForIndex(SynthGenParams::InstAttrib) );
	typedef_.remove( synthdef.getKeyForIndex(
					SynthGenParams::FilteredSynthetic) );
	typedef_.remove( synthdef.getKeyForIndex(
					SynthGenParams::FilteredStratProp) );
    }

    typedef_.remove( synthdef.getKeyForIndex(SynthGenParams::StratProp) );

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

    mAttachCB( postFinalize(), uiMultiSynthSeisSel::initGrpCB );
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

    if ( elasticstacksynthgrp_ )
    {
	const bool dodisp = synthtype == SynthGenParams::ElasticStack;
	if ( elasticstacksynthgrp_->isDisplayed() && !dodisp )
	    previoussynthgrp_ = elasticstacksynthgrp_;
	elasticstacksynthgrp_->display( dodisp );
    }

    if ( elasticgathersynthgrp_ )
    {
	const bool dodisp = synthtype == SynthGenParams::ElasticGather;
	if ( elasticgathersynthgrp_->isDisplayed() && !dodisp )
	    previoussynthgrp_ = elasticgathersynthgrp_;
	elasticgathersynthgrp_->display( dodisp );
    }

    if ( prestacksynthgrp_ )
    {
	const bool dodisp = synthtype == SynthGenParams::PreStack;
	if ( prestacksynthgrp_->isDisplayed() && !dodisp )
	    previoussynthgrp_ = prestacksynthgrp_;
	prestacksynthgrp_->display( dodisp );
    }

    if ( !previoussynthgrp_ )
	return;

    SynthGenParams::SynthType prevtype;
    if ( previoussynthgrp_ == zerooffsynthgrp_ )
	prevtype = SynthGenParams::ZeroOffset;
    else if ( previoussynthgrp_ == elasticstacksynthgrp_ )
	prevtype = SynthGenParams::ElasticStack;
    else if ( previoussynthgrp_ == elasticgathersynthgrp_ )
	prevtype = SynthGenParams::ElasticGather;
    else if ( previoussynthgrp_ == prestacksynthgrp_ )
	prevtype = SynthGenParams::PreStack;
    else
	{ pErrMsg("Should not be reached"); return; }

    IOPar par;
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
    if ( !par )
	return;

    IOPar iop( *par );
    iop.removeWithKey( SynthGenParams::sKeySynthType() );
    uiMultiSynthSeisSel::usePar( iop );
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
    if ( synthtype == SynthGenParams::ElasticStack )
	return elasticstacksynthgrp_;
    if ( synthtype == SynthGenParams::ElasticGather )
	return elasticgathersynthgrp_;
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
    else if ( elasticstacksynthgrp_ )
	typ = SynthGenParams::ElasticStack;
    else if ( elasticgathersynthgrp_ )
	typ = SynthGenParams::ElasticGather;
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


void uiMultiSynthSeisSel::ensureHasWavelet( const MultiID& wvltid )
{
    for ( auto* synthsel : synthgrps_ )
	synthsel->ensureHasWavelet( wvltid );
}


bool uiMultiSynthSeisSel::setFrom( const SynthGenParams& sgp )
{
    if ( !sgp.isRawOutput() )
	return false;

    IOPar iop;
    sgp.fillPar( iop );
    return uiMultiSynthSeisSel::usePar( iop );
}


bool uiMultiSynthSeisSel::usePar( const IOPar& iop )
{
    NotifyStopper nssel( selectionChanged );
    NotifyStopper nspars( parsChanged );

    SynthGenParams::SynthType synthtype;
    const bool hastype = SynthGenParams::parseEnum( iop,
			 SynthGenParams::sKeySynthType(), synthtype );
    if ( hastype && (synthtype == SynthGenParams::ZeroOffset ||
		     synthtype == SynthGenParams::ElasticStack ||
		     synthtype == SynthGenParams::ElasticGather ||
		     synthtype == SynthGenParams::PreStack) )
    {
	const BufferString typestr = SynthGenParams::toString( synthtype );
	if ( typestr != getType() )
	    setType( SynthGenParams::toString(synthtype) );
    }

    nssel.enableNotification();

    IOPar par( iop );
    par.removeSubSelection( ReflCalc1D::sKeyReflPar() );
    par.removeSubSelection( RayTracer1D::sKeyRayPar() );
    // Everything else applies to all types

    if ( zerooffsynthgrp_ )
	zerooffsynthgrp_->usePar(
	    hastype && synthtype == SynthGenParams::ZeroOffset ? iop : par );
    if ( elasticstacksynthgrp_ )
	elasticstacksynthgrp_->usePar(
	    hastype && synthtype == SynthGenParams::ElasticStack ? iop : par );
    if ( elasticgathersynthgrp_ )
	elasticgathersynthgrp_->usePar(
	    hastype && synthtype == SynthGenParams::ElasticGather ? iop : par );
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


bool uiMultiSynthSeisSel::getGenParams( SynthGenParams& sgp ) const
{
    IOPar iop;
    fillPar( iop );
    const SynthGenParams::SynthType synthtype =
			SynthGenParams::parseEnumSynthType( getType() );
    sgp = SynthGenParams( synthtype );
    sgp.usePar( iop );
    return !iop.isEmpty();
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


// uiFullSynthSeisSel

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
    mAttachCB( angleinpfld_->valueChanged, uiFullSynthSeisSel::parsChangedCB );

    EnumDef attribs = Attrib::Instantaneous::OutTypeDef();
    attribs.remove( attribs.getKeyForIndex(Attrib::Instantaneous::RotatePhase));
    instattribfld_ = new uiLabeledListBox( topgrp, uiStrings::sAttribute(),
					   OD::ChooseAtLeastOne );

    instattribfld_->box()->addItems( attribs.strings() );
    instattribfld_->attach( alignedBelow, inpselfld_ );
    mAttachCB( instattribfld_->box()->selectionChanged,
	       uiFullSynthSeisSel::parsChangedCB );

    filtertypefld_ = new uiGenInput( topgrp, uiStrings::sType(),
	 BoolInpSpec(true,
	     uiStrings::phrJoinStrings(toUiString("FFT"), uiStrings::sFilter()),
		     uiStrings::sAverage()) );
    mAttachCB(filtertypefld_->valueChanged, uiFullSynthSeisSel::filterChgCB);
    mAttachCB(filtertypefld_->valueChanged, uiFullSynthSeisSel::parsChangedCB);
    filtertypefld_->attach( alignedBelow, inpselfld_ );
    freqfld_ = new uiFreqFilterSelFreq( topgrp );
    freqfld_->attach( alignedBelow, filtertypefld_ );
    mAttachCB(freqfld_->parchanged, uiFullSynthSeisSel::parsChangedCB);
    smoothwindowfld_ = new uiLabeledSpinBox( topgrp,
					     tr("Window size (samples)") );
    smoothwindowfld_->box()->setValue( 100 );
    smoothwindowfld_->attach( alignedBelow, filtertypefld_ );
    mAttachCB(smoothwindowfld_->box()->valueChanged,
	      uiFullSynthSeisSel::parsChangedCB);

    namefld_ = new uiGenInput( this, uiStrings::sName() );
    namefld_->setElemSzPol( uiObject::Wide );
    namefld_->attach( ensureBelow, topgrp );
    namefld_->attach( alignedBelow, topgrp->attachObj() );
    mAttachCB( namefld_->valueChanging, uiFullSynthSeisSel::nameChangedCB );
    mAttachCB( namefld_->valueChanged, uiFullSynthSeisSel::nameChangedCB );
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
    const bool filter = synthtype == SynthGenParams::FilteredSynthetic ||
			synthtype == SynthGenParams::FilteredStratProp;
    psselfld_->display( psbased );
    angleinpfld_->display( psbased );
    inpselfld_->display( attrib || filter );
    instattribfld_->display( attrib );
    filtertypefld_->display( filter );
    filterChgCB( nullptr );
    doParsChanged();
}


void uiFullSynthSeisSel::filterChgCB( CallBacker* )
{
    const SynthGenParams::SynthType synthtype =
			      SynthGenParams::parseEnumSynthType( getType() );
    const bool filter = synthtype == SynthGenParams::FilteredSynthetic ||
			synthtype == SynthGenParams::FilteredStratProp;
    const bool dofreqfilter = filtertypefld_->getBoolValue();
    freqfld_->display( filter && dofreqfilter );
    smoothwindowfld_->display( filter && !dofreqfilter );
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


void uiFullSynthSeisSel::manPSSynth( const BufferStringSet& nms )
{
    doMan( psselfld_->box(), nms );
}


void uiFullSynthSeisSel::manInpSynth( const BufferStringSet& nms )
{
    doMan( inpselfld_->box(), nms );
    doParsChanged();
}


void uiFullSynthSeisSel::doMan( uiComboBox* cbfld, const BufferStringSet& nms )
{
    if ( nms.isEmpty() )
	return;

    BufferStringSet oldnms;
    cbfld->getItems( oldnms );
    if ( nms == oldnms )
	return;

    NotifyStopper ns( cbfld->selectionChanged );
    const BufferString curnm( cbfld->text() );
    cbfld->setEmpty();
    if ( nms.isEmpty() )
	return;

    cbfld->addItems( nms );
    if ( nms.isPresent(curnm) )
	cbfld->setCurrentItem( curnm.buf() );

    cbfld->setSensitive( nms.get(0) != SynthGenParams::sKeyInvalidInputPS() ||
			 nms.size() > 1 );
}


bool uiFullSynthSeisSel::setFrom( const SynthGenParams& sgp )
{
    if ( sgp.isRawOutput() && uiMultiSynthSeisSel::setFrom(sgp) )
    {
	setOutputName( sgp.name_ );
	return true;
    }

    IOPar par;
    sgp.fillPar( par );
    return usePar( par );
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
	    psbox->setCurrentItem( genparams.inpsynthnm_.buf() );
	    psbox->setSensitive( genparams.inpsynthnm_ !=
				 SynthGenParams::sKeyInvalidInputPS() ||
				 psbox->size() > 1 );
	}
	else if ( genparams.inpsynthnm_.isEmpty() )
	    psbox->setSensitive( true );
	else
	{
	    psbox->addItem( genparams.inpsynthnm_ );
	    psbox->setCurrentItem( genparams.inpsynthnm_.buf() );
	    psbox->setSensitive( genparams.inpsynthnm_ !=
				 SynthGenParams::sKeyInvalidInputPS() ||
				 psbox->size() > 1 );
	}

	NotifyStopper angparschgstopper( angleinpfld_->valueChanged );
	angleinpfld_->setValue( genparams.anglerg_ );
    }
    else if ( genparams.isAttribute() )
    {
	NotifyStopper ns_inpsel( inpselfld_->box()->selectionChanged );
	NotifyStopper ns_instattrib( instattribfld_->box()->selectionChanged );
	uiComboBox* inpbox = inpselfld_->box();
	if ( inpbox->isPresent(genparams.inpsynthnm_) )
	{
	    inpbox->setCurrentItem( genparams.inpsynthnm_.buf() );
	    inpbox->setSensitive( genparams.inpsynthnm_ !=
				  SynthGenParams::sKeyInvalidInputPS() ||
				  inpbox->size() > 1 );
	}
	else if ( genparams.inpsynthnm_.isEmpty() )
	    inpbox->setSensitive( true );
	else
	{
	    inpbox->addItem( genparams.inpsynthnm_ );
	    inpbox->setCurrentItem( genparams.inpsynthnm_.buf() );
	    inpbox->setSensitive( genparams.inpsynthnm_ !=
				  SynthGenParams::sKeyInvalidInputPS() ||
				  inpbox->size() > 1 );
	}

	instattribfld_->box()->chooseAll( false );
	instattribfld_->box()->setChosen( genparams.attribtype_ );
    }
    else if ( genparams.isFiltered() )
    {
	NotifyStopper ns_inpsel( inpselfld_->box()->selectionChanged );
	NotifyStopper ns_filter( filtertypefld_->valueChanged );
	NotifyStopper ns_freq( freqfld_->parchanged );
	NotifyStopper ns_window( smoothwindowfld_->box()->valueChanged );
	uiComboBox* inpbox = inpselfld_->box();
	if ( inpbox->isPresent(genparams.inpsynthnm_) )
	{
	    inpbox->setCurrentItem( genparams.inpsynthnm_.buf() );
	    inpbox->setSensitive( genparams.inpsynthnm_ !=
				  SynthGenParams::sKeyInvalidInputPS() ||
				  inpbox->size() > 1 );
	}
	else if ( genparams.inpsynthnm_.isEmpty() )
	    inpbox->setSensitive( true );
	else
	{
	    inpbox->addItem( genparams.inpsynthnm_ );
	    inpbox->setCurrentItem( genparams.inpsynthnm_.buf() );
	    inpbox->setSensitive( genparams.inpsynthnm_ !=
				  SynthGenParams::sKeyInvalidInputPS() ||
				  inpbox->size() > 1 );
	}

	const bool doaverage = genparams.filtertype_==sKey::Average();
	filtertypefld_->setValue( !doaverage );
	if ( doaverage )
	    smoothwindowfld_->box()->setValue( genparams.windowsz_ );
	else
	{
	    FFTFilter::Type ftype;
	    FFTFilter::parseEnum( genparams.filtertype_, ftype );
	    freqfld_->setFreqRange( genparams.freqrg_ );
	    freqfld_->setFilterType( ftype );
	}
	filterChgCB( nullptr );
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
    else if ( sgp.isFiltered() )
    {
	if ( inpselfld_->box()->isEmpty() )
	    mErrRet( tr("Cannot filter without input") );
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
    else if ( sgp.isFiltered() )
    {
	const bool dofreqfilter = filtertypefld_->getBoolValue();
	const FFTFilter::Type ftype = freqfld_->filterType();
	iop.set( SynthGenParams::sKeyInput(), inpselfld_->box()->text() );
	if ( dofreqfilter )
	{
	    iop.set( sKey::Filter(), FFTFilter::toString(ftype) );
	    iop.set( SynthGenParams::sKeyFreqRange(), freqfld_->freqRange() );
	}
	else
	{
	    iop.set( sKey::Filter(), sKey::Average() );
	    iop.set( sKey::Size(), smoothwindowfld_->box()->getIntValue() );
	}
    }
    else
	iop.setEmpty();
}
