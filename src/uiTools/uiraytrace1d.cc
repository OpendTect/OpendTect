/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiraytrace1d.h"

#include "survinfo.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uimsg.h"

mImplFactory2Param( uiRayTracer1D, uiParent*, const uiRayTracer1D::Setup&,
		    uiRayTracer1D::factory );


// uiRayTracerSel

uiRayTracerSel::uiRayTracerSel( uiParent* p, const uiRayTracer1D::Setup& su )
    : uiGroup( p, "Ray Tracer Selector" )
    , parsChanged(this)
{
    Factory2Param<uiRayTracer1D,uiParent*,const uiRayTracer1D::Setup&>&
					uirtfact = uiRayTracer1D::factory();
    BufferStringSet typenms = uirtfact.getNames();
    if ( typenms.size() > 1 )
	typenms.remove( VrmsRayTracer1D::sFactoryKeyword() );

    for ( const auto* typestr : typenms )
	grps_.add( uirtfact.create( typestr->buf(), this, su ) );

    uiLabeledComboBox* raytracersel = nullptr;
    if ( grps_.size() > 1 )
    {
	uiStringSet usednms;
	for ( const auto* uirt1d : grps_ )
	    usednms.add( uirt1d->factoryDisplayName() );

	raytracersel = new uiLabeledComboBox( this, usednms, tr("Ray Tracer") );
	raytracerselfld_ = raytracersel->box();
	raytracerselfld_->setHSzPol( uiObject::Wide );
	mAttachCB( raytracerselfld_->selectionChanged,
		   uiRayTracerSel::selRayTraceCB );
    }

    for ( auto* grp : grps_ )
    {
	mAttachCB( grp->parsChanged, uiRayTracerSel::parsChangedCB );
	if ( raytracerselfld_ )
	    grp->attach( alignedBelow, raytracersel );
    }

    if ( !grps_.isEmpty() )
	setHAlignObj( grps_.first() );

    mAttachCB( postFinalize(), uiRayTracerSel::initGrpCB );
}


uiRayTracerSel::~uiRayTracerSel()
{
    detachAllNotifiers();
}


void uiRayTracerSel::initGrpCB( CallBacker* )
{
    setDefault();
    if ( grps_.size() > 1 )
    {
	setCurrentType( uiRayTracer1D::factory().getDefaultName() );
	selRayTraceCB( nullptr );
    }
}


void uiRayTracerSel::setDefault()
{
    FactoryBase& uirtfact = uiRayTracer1D::factory();
    if ( !StringView(uirtfact.getDefaultName()).isEmpty() )
	return;

    const int defidx = uirtfact.getNames().indexOf(
			RayTracer1D::factory().getDefaultName() );
    if ( defidx >= 0 )
	uirtfact.setDefaultName( defidx );
}


void uiRayTracerSel::selRayTraceCB( CallBacker* )
{
    for ( auto* grp : grps_ )
	grp->display( grp == current() );

    parsChangedCB( nullptr );
}


void uiRayTracerSel::parsChangedCB( CallBacker* )
{
    parsChanged.trigger();
}


uiRetVal uiRayTracerSel::isOK() const
{
    uiRetVal uirv;
    if ( current() )
	uirv = current()->isOK();

    return uirv;
}


void uiRayTracerSel::usePar( const IOPar& par )
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


void uiRayTracerSel::fillPar( IOPar& par ) const
{
    if ( !current() )
	return;

    par.set( sKey::Type(), current()->factoryKeyword() );
    current()->fillPar( par );
}


const uiRayTracer1D* uiRayTracerSel::current() const
{
    const int selidx =
	raytracerselfld_ ? raytracerselfld_->currentItem() : 0;
    return grps_.validIdx( selidx ) ? grps_[selidx] : nullptr;
}


bool uiRayTracerSel::setCurrentType( const char* typestr )
{
    if ( !raytracerselfld_ )
	return false;

    for ( int grpidx=0; grpidx<grps_.size(); grpidx++ )
    {
	if ( StringView(grps_[grpidx]->factoryKeyword()) == typestr )
	{
	    raytracerselfld_->setCurrentItem( grpidx );
	    selRayTraceCB( nullptr );
	    return true;
	}
    }

    return false;
}


// uiRayTracerAdvancedDlg

class uiRayTracerAdvancedDlg : public uiDialog
{ mODTextTranslationClass(uiRayTracerAdvancedDlg);
public:

			uiRayTracerAdvancedDlg(uiRayTracer1D&);
			~uiRayTracerAdvancedDlg();

    bool		isOK() const		{ return advgrp_; }

    bool		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    Notifier<uiRayTracerAdvancedDlg> parsChanged;

private:

    void		parsChangedCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    uiRayTracerAdvancedGrp* advgrp_;
};


uiRayTracerAdvancedDlg::uiRayTracerAdvancedDlg( uiRayTracer1D& uirt )
    : uiDialog( &uirt, Setup(tr("Ray tracing advanced options"),
		tr("Specify advanced options"),
		mODHelpKey(mRayTracerAdvancedDlgHelpID)) )
    , parsChanged(this)
{
    advgrp_ = uirt.getAvancedGrp( this );
    if ( !advgrp_ )
	return;

    mAttachCB( advgrp_->parsChanged, uiRayTracerAdvancedDlg::parsChangedCB );
}


uiRayTracerAdvancedDlg::~uiRayTracerAdvancedDlg()
{
    detachAllNotifiers();
}


void uiRayTracerAdvancedDlg::parsChangedCB( CallBacker* )
{
    parsChanged.trigger();
}


bool uiRayTracerAdvancedDlg::usePar( const IOPar& iop )
{
    return advgrp_ ? advgrp_->usePar( iop ) : false;
}


void uiRayTracerAdvancedDlg::fillPar( IOPar& iop ) const
{
    if ( advgrp_ )
	advgrp_->fillPar( iop );
}


#define mErrRet(s,act) \
{ uiMsgMainWinSetter mws(mainwin()); if (!s.isEmpty()) uiMSG().error(s); act; }

bool uiRayTracerAdvancedDlg::acceptOK( CallBacker* )
{
    if ( !advgrp_ )
	return true;

    const uiRetVal uirv = advgrp_->isOK();
    if ( !uirv.isOK() )
	mErrRet(uirv,return false)

    return true;
}


// uiRayTracer1D::Setup

uiRayTracer1D::Setup::Setup()
    : dooffsets_(true)
    , doreflectivity_(true)
    , convertedwaves_(true)
{
    withadvanced_ = dooffsets_ && convertedwaves_;
}


uiRayTracer1D::Setup::~Setup()
{
}


// uiRayTracer1D

uiRayTracer1D::uiRayTracer1D( uiParent* p, const Setup& su )
    : uiGroup( p )
    , convertedwaves_(su.convertedwaves_)
    , doreflectivity_(su.doreflectivity_)
    , parsChanged( this )
{
    if ( su.dooffsets_ )
    {
	const StepInterval<float> offsetrg = RayTracer1D::sDefOffsetRange();
	uiString olb = tr( "Offset range (start/stop) %1" )
			.arg( SI().getXYUnitString(true) );;
	offsetfld_ = new uiGenInput( this, olb, IntInpIntervalSpec() );
	offsetfld_->setElemSzPol( uiObject::Small );
	offsetfld_->setValue(
			Interval<float>( offsetrg.start, offsetrg.stop ) );
	mAttachCB( offsetfld_->valuechanged, uiRayTracer1D::parsChangedCB );

	offsetstepfld_ = new uiGenInput( this, uiStrings::sStep() );
	offsetstepfld_->attach( rightOf, offsetfld_ );
	offsetstepfld_->setElemSzPol( uiObject::Small );
	offsetstepfld_->setValue( offsetrg.step );
	mAttachCB( offsetstepfld_->valuechanged, uiRayTracer1D::parsChangedCB );
    }

    if ( su.convertedwaves_ )
    {
	if ( su.withadvanced_ )
	    ensureHasAdvancedButton();
	else
	    setAdvancedGroup( getAvancedGrp(this) );
    }

    if ( offsetfld_ )
	setHAlignObj( offsetfld_ );

    mAttachCB( postFinalize(), uiRayTracer1D::initGrpCB );
}


uiRayTracer1D::~uiRayTracer1D()
{
    detachAllNotifiers();
    delete lastiop_;
}


void uiRayTracer1D::initGrpCB( CallBacker* )
{
    initGrp();
}


void uiRayTracer1D::initGrp()
{
    setName( factoryKeyword() );
}


void uiRayTracer1D::ensureHasAdvancedButton()
{
    if ( advbut_ )
	return;

    CallBack cbadv = mCB(this,uiRayTracer1D,getAdvancedPush);
    advbut_ = new uiPushButton( this, tr("RayTracing parameters"), cbadv,false);
    if ( lastFld() )
	advbut_->attach( alignedBelow, lastFld() );
    else
	{ pErrMsg("Incorrect layout"); }
}


void uiRayTracer1D::setAdvancedGroup( uiRayTracerAdvancedGrp* grp )
{
    if ( !grp )
	return;

    if ( advgrp_ )
	mDetachCB( advgrp_->parsChanged, uiRayTracer1D::parsChangedCB );

    delete advgrp_;
    advgrp_ = grp;
    mAttachCB( advgrp_->parsChanged, uiRayTracer1D::parsChangedCB );

    if ( lastFld() )
	advgrp_->attach( alignedBelow, lastFld() );
    else
	setHAlignObj( advgrp_ );
}


uiRayTracerAdvancedGrp* uiRayTracer1D::getAvancedGrp( uiParent* p )
{
    return doConvertedWaves() ?
		new uiRayTracerAdvancedGrp( p, doConvertedWaves(),
					    doReflectivity() ) : nullptr;
}


void uiRayTracer1D::getAdvancedPush( CallBacker* )
{
    if ( !advdlg_ )
    {
	advdlg_ = new uiRayTracerAdvancedDlg( *this );
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

	mAttachCB( advdlg_->parsChanged, uiRayTracer1D::parsChangedCB );
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


void uiRayTracer1D::parsChangedCB( CallBacker* )
{
    parsChanged.trigger();
}


bool uiRayTracer1D::usePar( const IOPar& par )
{
    NotifyStopper ns( parsChanged );

    TypeSet<float> offsets;
    par.get( RayTracer1D::sKeyOffset(), offsets );
    const float convfactor = SI().xyInFeet() ? mToFeetFactorF : 1;
    if ( !offsets.isEmpty() && offsetfld_ && offsetstepfld_ )
    {
	Interval<float> offsetrg( offsets[0], offsets[offsets.size()-1] );
	offsetfld_->setValue( offsetrg );
	const float step = offsets.size() > 1
			 ? offsets[1]-offsets[0]
			 : RayTracer1D::sDefOffsetRange().step;
	offsetstepfld_->setValue( step * convfactor );
    }

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


void uiRayTracer1D::fillPar( IOPar& par ) const
{
    if ( offsetfld_ && offsetstepfld_ )
    {
	StepInterval<float> offsetrg;
	offsetrg.start = mCast( float, offsetfld_->getIInterval().start );
	offsetrg.stop = mCast( float, offsetfld_->getIInterval().stop );
	offsetrg.step = offsetstepfld_->getFValue();
	TypeSet<float> offsets;
	for ( int idx=0; idx<offsetrg.nrSteps()+1; idx++ )
	    offsets += offsetrg.atIndex( idx );

	par.set( RayTracer1D::sKeyOffset(), offsets );
	par.setYN( RayTracer1D::sKeyOffsetInFeet(), SI().xyInFeet() );
    }

    par.setYN( RayTracer1D::sKeyReflectivity(), doreflectivity_ );
    if ( advdlg_ )
	advdlg_->fillPar( par );
    else if ( advgrp_ )
	advgrp_->fillPar( par );
}


uiRetVal uiRayTracer1D::isOK() const
{
    uiRetVal uirv;
    if ( advgrp_ )
	uirv = advgrp_->isOK();

    return uirv;
}


// uiVrmsRayTracer1D

uiVrmsRayTracer1D::uiVrmsRayTracer1D(uiParent* p,const uiRayTracer1D::Setup& su)
    : uiRayTracer1D( p, su )
{}


uiVrmsRayTracer1D::~uiVrmsRayTracer1D()
{
}


void uiVrmsRayTracer1D::initClass()
{
    uiRayTracer1D::factory().addCreator( create, sFactoryKeyword(),
					 sFactoryDisplayName() );
}


// uiRayTracerAdvancedGrp

uiRayTracerAdvancedGrp::uiRayTracerAdvancedGrp( uiParent* p,
						bool convertedwaves,
						bool doreflectivity )
    : uiGroup(p,"Ray tracing advanced options")
    , parsChanged(this)
{
    if ( !convertedwaves )
	return;

    const RayTracer1D::Setup defrtsu;
    const BoolInpSpec inpspecdn( defrtsu.pdown_, tr("P"), tr("S") );
    downwavefld_ = new uiGenInput( this, tr("Downward wave-type"), inpspecdn );
    mAttachCB( downwavefld_->valuechanged,
	       uiRayTracerAdvancedGrp::parsChangedCB );

    const BoolInpSpec inpspecup( defrtsu.pup_, tr("P"), tr("S") );
    upwavefld_ = new uiGenInput( this, tr("Upward wave-type"), inpspecup );
    upwavefld_->attach( alignedBelow, downwavefld_ );
    mAttachCB( upwavefld_->valuechanged,
	       uiRayTracerAdvancedGrp::parsChangedCB );

    setHAlignObj( downwavefld_ );

    mAttachCB( postFinalize(), uiRayTracerAdvancedGrp::initGrpCB );
}


uiRayTracerAdvancedGrp::~uiRayTracerAdvancedGrp()
{
    detachAllNotifiers();
}


void uiRayTracerAdvancedGrp::initGrpCB( CallBacker* )
{
    initGrp();
}


void uiRayTracerAdvancedGrp::initGrp()
{
}


void uiRayTracerAdvancedGrp::parsChangedCB( CallBacker* )
{
    parsChanged.trigger();
}


bool uiRayTracerAdvancedGrp::usePar( const IOPar& iop )
{
    if ( !downwavefld_ )
	return false;

    RayTracer1D::Setup tmpsetup; tmpsetup.usePar( iop );

    if ( downwavefld_->getBoolValue() == tmpsetup.pdown_ &&
	 upwavefld_->getBoolValue() == tmpsetup.pup_ )
	return false;

    NotifyStopper ns( parsChanged );
    downwavefld_->setValue( tmpsetup.pdown_ );
    upwavefld_->setValue( tmpsetup.pup_ );
    ns.enableNotification();
    parsChangedCB( nullptr );

    return true;
}


void uiRayTracerAdvancedGrp::fillPar( IOPar& iop ) const
{
    if ( !downwavefld_ )
	return;

    RayTracer1D::Setup tmpsetup; tmpsetup.usePar( iop );
    tmpsetup.pdown_ = downwavefld_->getBoolValue();
    tmpsetup.pup_ = upwavefld_->getBoolValue();
    tmpsetup.fillPar( iop );
}


uiRetVal uiRayTracerAdvancedGrp::isOK() const
{
    return uiRetVal();
}
