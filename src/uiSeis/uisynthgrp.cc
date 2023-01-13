/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisynthgrp.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistrings.h"

#include "survinfo.h"
#include "synthseis.h"

mImplFactory2Param( uiSynthSeis, uiParent*, const uiSynthSeis::Setup&,
		    uiSynthSeis::factory );


// uiSynthSeisSel

uiSynthSeisSel::uiSynthSeisSel( uiParent* p, const uiSynthSeis::Setup& su )
    : uiGroup( p, "Synth Seis Selector" )
    , parsChanged(this)
{
    Factory2Param<uiSynthSeis,uiParent*,const uiSynthSeis::Setup&>&
				    uisynthseisfact = uiSynthSeis::factory();
    BufferStringSet typenms = uisynthseisfact.getNames();
    if ( typenms.size() > 1 )
	typenms.remove( uiBaseSynthSeis::sFactoryKeyword() );

    uiseisfldsgrp_ = new uiGroup( this, "Synthetic generators group" );

    for ( const auto* typestr : typenms )
	grps_.add( uisynthseisfact.create( typestr->buf(), uiseisfldsgrp_, su));

    uiLabeledComboBox* synthseissel = nullptr;
    if ( grps_.size() > 1 )
    {
	uiStringSet usednms;
	for ( const auto* uiseisgrp : grps_ )
	    usednms.add( uiseisgrp->factoryDisplayName() );

	synthseissel = new uiLabeledComboBox( uiseisfldsgrp_,
					      tr("Synthetic Generator") );
	synthseisselfld_ = synthseissel->box();
	synthseisselfld_->setHSzPol( uiObject::Wide );
	mAttachCB( synthseisselfld_->selectionChanged,
		   uiSynthSeisSel::selSynthSeisCB );
    }

    for ( auto* grp : grps_ )
    {
	mAttachCB( grp->parsChanged, uiSynthSeisSel::parsChangedCB );
	if ( synthseissel )
	    grp->attach( alignedBelow, synthseissel );
    }

    if ( !grps_.isEmpty() )
	setHAlignObj( grps_.first() );

    mAttachCB( postFinalize(), uiSynthSeisSel::initGrpCB );
}


uiSynthSeisSel::uiSynthSeisSel( uiParent* p, const uiSynthSeis::Setup& su,
				const uiReflCalc1D::Setup& reflsu )
    : uiSynthSeisSel(p,su)
{
    reflsel_ = new uiReflCalcSel( this, reflsu );
    mAttachCB( reflsel_->parsChanged, uiSynthSeisSel::parsChangedCB );
    reflsel_->attach( alignedBelow, uiseisfldsgrp_ );
    setHAlignObj( reflsel_ );
}


uiSynthSeisSel::uiSynthSeisSel( uiParent* p, const uiSynthSeis::Setup& su,
				const uiRayTracer1D::Setup& rtsu )
    : uiSynthSeisSel(p,su)
{
    rtsel_ = new uiRayTracerSel( this, rtsu );
    mAttachCB( rtsel_->parsChanged, uiSynthSeisSel::parsChangedCB );
    rtsel_->attach( alignedBelow, uiseisfldsgrp_ );
    setHAlignObj( rtsel_ );
}


uiSynthSeisSel::~uiSynthSeisSel()
{
    detachAllNotifiers();
}


void uiSynthSeisSel::initGrpCB( CallBacker* )
{
    setDefault();
    if ( grps_.size() > 1 )
    {
	setCurrentType( uiSynthSeis::factory().getDefaultName() );
	selSynthSeisCB( nullptr );
    }
}


void uiSynthSeisSel::setDefault()
{
    FactoryBase& uisynthseisfact = uiSynthSeis::factory();
    if ( !StringView(uisynthseisfact.getDefaultName()).isEmpty() )
	return;

    const int defidx = uisynthseisfact.getNames().indexOf(
			uiSynthSeis::factory().getDefaultName() );
    if ( defidx >= 0 )
	uisynthseisfact.setDefaultName( defidx );
}


void uiSynthSeisSel::selSynthSeisCB( CallBacker* )
{
    for ( auto* grp : grps_ )
	grp->display( grp == current() );

    parsChangedCB( nullptr );
}


void uiSynthSeisSel::parsChangedCB( CallBacker* )
{
    parsChanged.trigger();
}


uiRetVal uiSynthSeisSel::isOK() const
{
    uiRetVal uirv;
    if ( current() )
	uirv = current()->isOK();

    return uirv;
}


bool uiSynthSeisSel::withRefl() const
{
    return reflsel_ || rtsel_;
}


void uiSynthSeisSel::setWavelet( const MultiID& dbky )
{
    for ( auto* grp : grps_ )
	grp->setWavelet( dbky );
}


void uiSynthSeisSel::setWavelet( const char* wvltnm )
{
    for ( auto* grp : grps_ )
	grp->setWavelet( wvltnm );
}


void uiSynthSeisSel::ensureHasWavelet( const MultiID& wvltid )
{
    for ( auto* grp : grps_ )
	grp->ensureHasWavelet( wvltid );
}


void uiSynthSeisSel::usePar( const IOPar& par )
{
    if ( reflsel_ )
    {
	ConstPtrMan<IOPar> reflpar = par.subselect( ReflCalc1D::sKeyReflPar() );
	useReflPars( reflpar ? *reflpar.ptr() : par );
    }
    else if ( rtsel_ )
    {
	ConstPtrMan<IOPar> reflpar = par.subselect( RayTracer1D::sKeyRayPar() );
	useReflPars( reflpar ? *reflpar.ptr() : par );
    }

    ConstPtrMan<IOPar> synthseisiop =
		    par.subselect( Seis::RaySynthGenerator::sKeySynthPar() );
    useSynthSeisPar( synthseisiop ? *synthseisiop.ptr() : par );
}


void uiSynthSeisSel::useSynthSeisPar( const IOPar& par )
{
    IOPar iop( par );
    BufferString type;
    const bool hastype = par.get( sKey::Type(), type ) && !type.isEmpty();
    if ( hastype )
	iop.removeWithKey( sKey::Type() );

    if ( !iop.isEmpty() )
    {
	for ( auto* grp : grps_ )
	    grp->usePar( iop );
    }

    if ( hastype )
	setCurrentType( type );
}


void uiSynthSeisSel::useReflPars( const IOPar& par )
{
    if ( reflsel_ )
	reflsel_->usePar( par );
    else if ( rtsel_ )
	rtsel_->usePar( par );
}


MultiID uiSynthSeisSel::getWaveletID() const
{
    MultiID dbky;
    if ( current() )
	dbky = current()->getWaveletID();

    return dbky;
}


const char* uiSynthSeisSel::getWaveletName() const
{
    return current() ? current()->getWaveletName() : nullptr;
}


void uiSynthSeisSel::fillPar( IOPar& par ) const
{
    if ( reflsel_ )
    {
	IOPar reflpar;
	fillReflPars( reflpar );
	par.mergeComp( reflpar, ReflCalc1D::sKeyReflPar() );
    }
    else if ( rtsel_ )
    {
	IOPar reflpar;
	fillReflPars( reflpar );
	par.mergeComp( reflpar, RayTracer1D::sKeyRayPar() );
    }
    else
    {
	IOPar reflpar;
	reflpar.set( sKey::Type(), AICalc1D::sFactoryKeyword() );
	ReflCalc1D::setIOParsToSingleAngle( reflpar );
	par.mergeComp( reflpar, ReflCalc1D::sKeyReflPar() );
    }

    IOPar synthseisiop;
    fillSynthSeisPar( synthseisiop );
    par.mergeComp( synthseisiop, Seis::RaySynthGenerator::sKeySynthPar() );
}


void uiSynthSeisSel::fillSynthSeisPar( IOPar& par ) const
{
    if ( !current() )
	return;

    par.set( sKey::Type(), current()->factoryKeyword() );
    current()->fillPar( par );
}


void uiSynthSeisSel::fillReflPars( IOPar& par ) const
{
    if ( reflsel_ )
	reflsel_->fillPar( par );
    else if ( rtsel_ )
	rtsel_->fillPar( par );
}


const uiSynthSeis* uiSynthSeisSel::current() const
{
    return mSelf().current();
}


uiSynthSeis* uiSynthSeisSel::current()
{
    const int selidx =
	synthseisselfld_ ? synthseisselfld_->currentItem() : 0;
    return grps_.validIdx( selidx ) ? grps_[selidx] : nullptr;
}


bool uiSynthSeisSel::setCurrentType( const char* typestr )
{
    if ( !synthseisselfld_ )
	return false;

    for ( int grpidx=0; grpidx<grps_.size(); grpidx++ )
    {
	if ( StringView(grps_[grpidx]->factoryKeyword()) == typestr )
	{
	    synthseisselfld_->setCurrentItem( grpidx );
	    selSynthSeisCB( nullptr );
	    return true;
	}
    }

    return false;
}


// uiSynthSeisAdvancedDlg

class uiSynthSeisAdvancedDlg : public uiDialog
{ mODTextTranslationClass(uiSynthSeisAdvancedDlg);
public:
			uiSynthSeisAdvancedDlg(uiSynthSeis&);
			~uiSynthSeisAdvancedDlg();

    bool		isOK() const		{ return advgrp_; }

    bool		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    Notifier<uiSynthSeisAdvancedDlg> parsChanged;

private:

    void		parsChangedCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    uiSynthSeisAdvancedGrp* advgrp_;
};


uiSynthSeisAdvancedDlg::uiSynthSeisAdvancedDlg( uiSynthSeis& uisg )
    : uiDialog( &uisg, Setup(tr("Synthetic generation advanced options"),
		tr("Specify advanced options"),
		mODHelpKey(mSynthGenAdvancedDlgHelpID)) )
    , parsChanged(this)
{
    advgrp_ = uisg.getAvancedGrp( this );
    if ( !advgrp_ )
	return;

    mAttachCB( advgrp_->parsChanged, uiSynthSeisAdvancedDlg::parsChangedCB );
}


uiSynthSeisAdvancedDlg::~uiSynthSeisAdvancedDlg()
{
    detachAllNotifiers();
}


void uiSynthSeisAdvancedDlg::parsChangedCB( CallBacker* )
{
    parsChanged.trigger();
}


bool uiSynthSeisAdvancedDlg::usePar( const IOPar& iop )
{
    return advgrp_ ? advgrp_->usePar( iop ) : false;
}


void uiSynthSeisAdvancedDlg::fillPar( IOPar& iop ) const
{
    if ( advgrp_ )
	advgrp_->fillPar( iop );
}


#define mErrRet(s,act) \
{ uiMsgMainWinSetter mws(mainwin()); if (!s.isEmpty()) uiMSG().error(s); act; }

bool uiSynthSeisAdvancedDlg::acceptOK( CallBacker* )
{
    if ( !advgrp_ )
	return true;

    const uiRetVal uirv = advgrp_->isOK();
    if ( !uirv.isOK() )
	mErrRet(uirv,return false)

    return true;
}


// uiSynthSeis::Setup

uiSynthSeis::Setup::Setup( bool withnmo, const uiSeisWaveletSel::Setup& wvltsu )
    : wvltsu_(wvltsu)
    , withnmo_(withnmo)
    , withadvanced_(true)
{
}


uiSynthSeis::Setup::~Setup()
{
}


// uiSynthSeis

uiSynthSeis::uiSynthSeis( uiParent* p, const Setup& su )
    : uiGroup( p )
    , withnmo_(su.withnmo_)
    , parsChanged( this )
{
    wvltfld_ = new uiSeisWaveletSel( this, su.wvltsu_ );
    mAttachCB( wvltfld_->newSelection, uiSynthSeis::parsChangedCB );
    wvltfld_->setFrame( false );

    if ( su.withadvanced_ )
	ensureHasAdvancedButton();
    else
	setAdvancedGroup( getAvancedGrp(this) );

    setHAlignObj( wvltfld_ );

    mAttachCB( postFinalize(), uiSynthSeis::initGrpCB );
}


uiSynthSeis::~uiSynthSeis()
{
    detachAllNotifiers();
    delete lastiop_;
}


void uiSynthSeis::initGrpCB( CallBacker* )
{
    initGrp();
}


void uiSynthSeis::initGrp()
{
    setName( factoryKeyword() );
}


uiObject* uiSynthSeis::lastFld() const
{
    return wvltfld_->attachObj();
}


void uiSynthSeis::ensureHasAdvancedButton()
{
    if ( advbut_ )
	return;

    CallBack cbadv = mCB(this,uiSynthSeis,getAdvancedPush);
    advbut_ = new uiPushButton( this, tr("Synthetic generation parameters"),
				cbadv, false );
    if ( lastFld() )
	advbut_->attach( alignedBelow, lastFld() );
    else
	{ pErrMsg("Incorrect layout"); }
}


void uiSynthSeis::setAdvancedGroup( uiSynthSeisAdvancedGrp* grp )
{
    if ( !grp )
	return;

    if ( advgrp_ )
	mDetachCB( advgrp_->parsChanged, uiSynthSeis::parsChangedCB );

    delete advgrp_;
    advgrp_ = grp;
    mAttachCB( advgrp_->parsChanged, uiSynthSeis::parsChangedCB );

    if ( lastFld() )
	advgrp_->attach( alignedBelow, lastFld() );
    else
	setHAlignObj( advgrp_ );
}


uiSynthSeisAdvancedGrp* uiSynthSeis::getAvancedGrp( uiParent* p )
{
    return new uiSynthSeisAdvancedGrp( p, withNMO() );
}


void uiSynthSeis::getAdvancedPush( CallBacker* )
{
    if ( !advdlg_ )
    {
	advdlg_ = new uiSynthSeisAdvancedDlg( *this );
	if ( !advdlg_ || !advdlg_->isOK() )
	{
	    deleteAndNullPtr( advdlg_ );
	    return;
	}

	if ( lastiop_ )
	{
	    advdlg_->usePar( *lastiop_ );
	    deleteAndNullPtr( lastiop_ );
	}

	mAttachCB( advdlg_->parsChanged,uiSynthSeis::parsChangedCB );
    }

    IOPar iop, newiop;
    advdlg_->fillPar( iop );
    NotifyStopper ns( advdlg_->parsChanged );
    if ( advdlg_->go() != uiDialog::Accepted )
    {
	advdlg_->usePar( iop );
	return;
    }

    advdlg_->fillPar( newiop );
    if ( newiop != iop )
    {
	ns.enableNotification();
	parsChangedCB( nullptr );
    }
}


void uiSynthSeis::parsChangedCB( CallBacker* )
{
    parsChanged.trigger();
}


void uiSynthSeis::setWavelet( const MultiID& dbky )
{
    wvltfld_->setInput( dbky );
}


void uiSynthSeis::setWavelet( const char* wvltnm )
{
    wvltfld_->setInput( wvltnm );
}


void uiSynthSeis::ensureHasWavelet( const MultiID& /* dbky */ )
{
    const MultiID curwvltid = wvltfld_->getID();
    NotifyStopper ns( wvltfld_->newSelection );
    wvltfld_->rebuildList();
    wvltfld_->setInput( curwvltid );
}


bool uiSynthSeis::usePar( const IOPar& par )
{
    NotifyStopper ns( parsChanged );

    MultiID waveletid;
    if ( par.get(sKey::WaveletID(),waveletid) )
	setWavelet( waveletid );

    if ( advdlg_ )
	advdlg_->usePar( par );
    else if ( advgrp_ )
	advgrp_->usePar( par );
    else
    {
	delete lastiop_;
	lastiop_ = new IOPar( par );
    }

    ns.enableNotification();
    parsChangedCB( nullptr );

    return true;
}


uiRetVal uiSynthSeis::isOK() const
{
    uiRetVal uirv;
    if ( !wvltfld_->getWavelet() )
	uirv.add( uiStrings::phrSelect(tr("a valid wavelet")) );

    if ( advgrp_ )
	uirv = advgrp_->isOK();

    return uirv;
}


MultiID uiSynthSeis::getWaveletID() const
{
    return wvltfld_->getID();
}


const char* uiSynthSeis::getWaveletName() const
{
    return wvltfld_->getWaveletName();
}


void uiSynthSeis::fillPar( IOPar& par ) const
{
    par.set( sKey::WaveletID(), getWaveletID() );
    if ( advdlg_ )
	advdlg_->fillPar( par );
    else if ( advgrp_ )
	advgrp_->fillPar( par );
}


// uiBaseSynthSeis

uiBaseSynthSeis::uiBaseSynthSeis( uiParent* p, const uiSynthSeis::Setup& su )
    : uiSynthSeis(p,su)
{
}


uiBaseSynthSeis::~uiBaseSynthSeis()
{
}


void uiBaseSynthSeis::initClass()
{
    uiSynthSeis::factory().addCreator( create, sFactoryKeyword(),
				       sFactoryDisplayName() );
}


// uiSynthSeisAdvancedGrp

mDefineEnumUtils(uiSynthSeisAdvancedGrp,ConvDomain,"Convolution Domain")
{
    "Frequency",
    "Time",
    nullptr
};


uiSynthSeisAdvancedGrp::uiSynthSeisAdvancedGrp( uiParent* p, bool withnmo )
    : uiGroup( p, "Synthetic generation advanced parameters" )
    , parsChanged(this)
{
    const bool dofreq = Seis::SynthGenBase::cDefIsFrequency();
    const ConvDomain domain = dofreq ? Freq : TWT;
    convdomainfld_ = new uiGenInput( this, tr("Convolution Domain"),
				     StringListInpSpec(ConvDomainNames()) );
    convdomainfld_->setText( toString(domain) );
    mAttachCB( convdomainfld_->valuechanged,
	       uiSynthSeisAdvancedGrp::parsChangedCB );

    if ( withnmo )
	createNMOGrp();

    setHAlignObj( convdomainfld_ );

    mAttachCB( postFinalize(), uiSynthSeisAdvancedGrp::initGrpCB );
}


void uiSynthSeisAdvancedGrp::createNMOGrp()
{
    nmofld_ = new uiGenInput( this, tr("Apply NMO corrections"),
			      BoolInpSpec(true) );
    nmofld_->setValue( true );
    nmofld_->attach( alignedBelow, convdomainfld_ );
    mAttachCB( nmofld_->valuechanged, uiSynthSeisAdvancedGrp::nmoSelCB );

    const double stretchlimpc =
			mToPercent(Seis::SynthGenBase::cStdStretchLimit());
    DoubleInpSpec inpspec( stretchlimpc );
    inpspec.setLimits( Interval<double>(1.,500.) );
    stretchmutelimitfld_ = new uiGenInput( this, tr("Stretch mute (%)"),
					   inpspec );
    stretchmutelimitfld_->attach( alignedBelow, nmofld_ );
    mAttachCB( stretchmutelimitfld_->valuechanged,
	       uiSynthSeisAdvancedGrp::parsChangedCB );

    const double mutelengthms = Seis::SynthGenBase::cStdMuteLength() *
				ZDomain::Time().userFactor();
    mutelenfld_ = new uiGenInput( this, tr("Mute taper-length (ms)"),
				  DoubleInpSpec(mutelengthms) );
    mutelenfld_->attach( alignedBelow, stretchmutelimitfld_ );
    mAttachCB( mutelenfld_->valuechanged,
	       uiSynthSeisAdvancedGrp::parsChangedCB );
}


uiSynthSeisAdvancedGrp::~uiSynthSeisAdvancedGrp()
{
    detachAllNotifiers();
}


void uiSynthSeisAdvancedGrp::initGrpCB( CallBacker* )
{
    initGrp();
}


void uiSynthSeisAdvancedGrp::initGrp()
{
    if ( withNMO() )
	nmoSelCB( nullptr );
}


uiObject* uiSynthSeisAdvancedGrp::lastFld() const
{
    return mutelenfld_ ? mutelenfld_->attachObj() : convdomainfld_->attachObj();
}


void uiSynthSeisAdvancedGrp::setStretchMutePerc( double stretchlimpc )
{
    if ( mIsEqual(stretchlimpc,stretchmutelimitfld_->getDValue(),1e-3) )
	return;

    stretchmutelimitfld_->setValue( stretchlimpc );
}


void uiSynthSeisAdvancedGrp::setMuteLengthMs( double mutelengthms )
{
    if ( mIsEqual(mutelengthms,mutelenfld_->getDValue(),1e-3) )
	return;

    mutelenfld_->setValue( mutelengthms );
}


bool uiSynthSeisAdvancedGrp::wantNMOCorr() const
{
    return withNMO() && nmofld_->getBoolValue();
}


double uiSynthSeisAdvancedGrp::getStrechtMutePerc() const
{
    return stretchmutelimitfld_ ? stretchmutelimitfld_->getDValue()
				: mUdf(double);
}


double uiSynthSeisAdvancedGrp::getMuteLengthMs() const
{
    return mutelenfld_ ? mutelenfld_->getDValue() : mUdf(double);
}


void uiSynthSeisAdvancedGrp::nmoSelCB( CallBacker* )
{
    stretchmutelimitfld_->display( wantNMOCorr() );
    mutelenfld_->display( wantNMOCorr() );
    parsChangedCB( nullptr );
}


void uiSynthSeisAdvancedGrp::parsChangedCB( CallBacker* )
{
    parsChanged.trigger();
}


bool uiSynthSeisAdvancedGrp::usePar( const IOPar& iop )
{
    NotifyStopper ns( parsChanged );
    bool changed = false;

    const ConvDomain prevdomain = parseEnumConvDomain( convdomainfld_->text() );
    bool dofreq;
    if ( iop.getYN(Seis::SynthGenBase::sKeyConvDomain(),dofreq) )
    {
	const ConvDomain domain = dofreq ? Freq : TWT;
	if ( domain != prevdomain )
	{
	    convdomainfld_->setText( toString(domain) );
	    changed = true;
	}
    }

    if ( withNMO() )
    {
	bool donmo = true;
	iop.getYN( Seis::SynthGenBase::sKeyNMO(), donmo );
	if ( !donmo && !nmofld_->getBoolValue() )
	    return !changed;
	else if ( donmo )
	{
	    if ( nmofld_->getBoolValue() )
	    {
		double stretchlimpc, mutelengthms;
		if ( iop.get(Seis::SynthGenBase::sKeyStretchLimit(),
			     stretchlimpc) && stretchlimpc > 0. &&
		    !mIsEqual(stretchlimpc,
			mFromPercent(stretchmutelimitfld_->getDValue()),1e-5) )
		{
		    setStretchMutePerc( mToPercent(stretchlimpc) );
		    changed = true;
		}

		if ( iop.get(Seis::SynthGenBase::sKeyMuteLength(),mutelengthms)
			&& mutelengthms > 0. &&
		     !mIsEqual(mutelengthms,mutelenfld_->getDValue()/
			ZDomain::Time().userFactor(),1e-5) )
		{
		    setMuteLengthMs( mutelengthms*ZDomain::Time().userFactor());
		    changed = true;
		}
	    }
	    else
	    {
		changed = true;
		nmofld_->setValue( donmo );
		nmoSelCB( nullptr );
	    }
	}
	else
	{
	    changed = true;
	    nmofld_->setValue( donmo );
	    nmoSelCB( nullptr );
	}
    }

    ns.enableNotification();
    if ( changed )
	parsChangedCB( nullptr );

    return !changed;
}


void uiSynthSeisAdvancedGrp::fillPar( IOPar& iop ) const
{
    const ConvDomain domain = parseEnumConvDomain( convdomainfld_->text() );
    iop.setYN( Seis::SynthGenBase::sKeyConvDomain(), domain == Freq );
    if ( !withNMO() )
	return;

    const bool wantsnmo = wantNMOCorr();
    iop.setYN( Seis::SynthGenBase::sKeyNMO(), wantsnmo );
    if ( wantsnmo )
    {
	iop.set( Seis::SynthGenBase::sKeyStretchLimit(),
		 mFromPercent( getStrechtMutePerc() ) );
	iop.set( Seis::SynthGenBase::sKeyMuteLength(),
		 getMuteLengthMs() / ZDomain::Time().userFactor() );
    }
}


uiRetVal uiSynthSeisAdvancedGrp::isOK() const
{
    uiRetVal uirv;
    if ( withNMO() )
    {
	const double stretchlimpc = getStrechtMutePerc();
	if ( mIsUdf(stretchlimpc) || stretchlimpc < 0. )
	    uirv.add( tr("The stretch mute must be more than 0%") );

	const double mutelengthms =  getMuteLengthMs();
	if ( mIsUdf(mutelengthms) || mutelengthms < 0. )
	    uirv.add( tr("The mute length must be more than zero.") );
    }

    return uirv;
}
