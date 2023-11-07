/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uirefltrace1d.h"

#include "survinfo.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uimsg.h"

mImplFactory2Param( uiReflCalc1D, uiParent*, const uiReflCalc1D::Setup&,
		    uiReflCalc1D::factory );


// uiReflCalcSel

uiReflCalcSel::uiReflCalcSel( uiParent* p, const uiReflCalc1D::Setup& su )
    : uiGroup( p, "EI Calc Selector" )
    , parsChanged(this)
{
    Factory2Param<uiReflCalc1D,uiParent*,const uiReflCalc1D::Setup&>&
					uireflfact = uiReflCalc1D::factory();
    BufferStringSet typenms = uireflfact.getNames();
    if ( typenms.size() > 1 )
	typenms.remove( AICalc1D::sFactoryKeyword() );

    for ( const auto* typestr : typenms )
	grps_.add( uireflfact.create( typestr->buf(), this, su ) );

    uiLabeledComboBox* reflcalcsel = nullptr;
    if ( grps_.size() > 1 )
    {
	uiStringSet usednms;
	for ( const auto* uireflcalc1d : grps_ )
	    usednms.add( uireflcalc1d->factoryDisplayName() );

	reflcalcsel = new uiLabeledComboBox( this, usednms,
					     tr("EI Calculator") );
	reflcalcselfld_ = reflcalcsel->box();
	reflcalcselfld_->setHSzPol( uiObject::Wide );
	mAttachCB( reflcalcselfld_->selectionChanged,
		   uiReflCalcSel::selReflCalcCB );
    }

    for ( auto* grp : grps_ )
    {
	mAttachCB( grp->parsChanged, uiReflCalcSel::parsChangedCB );
	if ( reflcalcselfld_ )
	    grp->attach( alignedBelow, reflcalcsel );
    }

    if ( !grps_.isEmpty() )
	setHAlignObj( grps_.first() );

    mAttachCB( postFinalize(), uiReflCalcSel::initGrpCB );
}


uiReflCalcSel::~uiReflCalcSel()
{
    detachAllNotifiers();
}


void uiReflCalcSel::initGrpCB( CallBacker* )
{
    setDefault();
    if ( grps_.size() > 1 )
    {
	setCurrentType( uiReflCalc1D::factory().getDefaultName() );
	selReflCalcCB( nullptr );
    }
}


void uiReflCalcSel::setDefault()
{
    FactoryBase& uireflfact = uiReflCalc1D::factory();
    if ( !StringView(uireflfact.getDefaultName()).isEmpty() )
	return;

    const int defidx = uireflfact.getNames().indexOf(
			ReflCalc1D::factory().getDefaultName() );
    if ( defidx >= 0 )
	uireflfact.setDefaultName( defidx );
}


void uiReflCalcSel::selReflCalcCB( CallBacker* )
{
    for ( auto* grp : grps_ )
	grp->display( grp == current() );

    parsChangedCB( nullptr );
}


void uiReflCalcSel::parsChangedCB( CallBacker* )
{
    parsChanged.trigger();
}


uiRetVal uiReflCalcSel::isOK() const
{
    uiRetVal uirv;
    if ( current() )
	uirv = current()->isOK();

    return uirv;
}


void uiReflCalcSel::usePar( const IOPar& par )
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


void uiReflCalcSel::fillPar( IOPar& par ) const
{
    if ( !current() )
	return;

    par.set( sKey::Type(), current()->factoryKeyword() );
    current()->fillPar( par );
}


const uiReflCalc1D* uiReflCalcSel::current() const
{
    const int selidx =
	reflcalcselfld_ ? reflcalcselfld_->currentItem() : 0;
    return grps_.validIdx( selidx ) ? grps_[selidx] : nullptr;
}


bool uiReflCalcSel::setCurrentType( const char* typestr )
{
    if ( !reflcalcselfld_ )
	return false;

    for ( int grpidx=0; grpidx<grps_.size(); grpidx++ )
    {
	if ( StringView(grps_[grpidx]->factoryKeyword()) == typestr )
	{
	    reflcalcselfld_->setCurrentItem( grpidx );
	    selReflCalcCB( nullptr );
	    return true;
	}
    }

    return false;
}


// uiReflCalcAdvancedDlg

class uiReflCalcAdvancedDlg : public uiDialog
{ mODTextTranslationClass(uiReflCalcAdvancedDlg);
public:

			uiReflCalcAdvancedDlg(uiReflCalc1D&);
			~uiReflCalcAdvancedDlg();

    bool		isOK() const		{ return advgrp_; }

    bool		usePar(const IOPar&);
    void		fillPar(IOPar&) const;

    Notifier<uiReflCalcAdvancedDlg> parsChanged;

private:

    void		parsChangedCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    uiReflCalcAdvancedGrp* advgrp_;
};


uiReflCalcAdvancedDlg::uiReflCalcAdvancedDlg( uiReflCalc1D& uirt )
    : uiDialog( &uirt, Setup(tr("EI Calculation advanced options"),
		tr("Specify advanced options"),
		mODHelpKey(mEICalcAdvancedDlgHelpID)) )
    , parsChanged(this)
{
    advgrp_ = uirt.getAvancedGrp( this );
    if ( !advgrp_ )
	return;

    mAttachCB( advgrp_->parsChanged, uiReflCalcAdvancedDlg::parsChangedCB );
}


uiReflCalcAdvancedDlg::~uiReflCalcAdvancedDlg()
{
    detachAllNotifiers();
}


void uiReflCalcAdvancedDlg::parsChangedCB( CallBacker* )
{
    parsChanged.trigger();
}


bool uiReflCalcAdvancedDlg::usePar( const IOPar& iop )
{
    return advgrp_ ? advgrp_->usePar( iop ) : false;
}


void uiReflCalcAdvancedDlg::fillPar( IOPar& iop ) const
{
    if ( advgrp_ )
	advgrp_->fillPar( iop );
}


#define mErrRet(s,act) \
{ uiMsgMainWinSetter mws(mainwin()); if (!s.isEmpty()) uiMSG().error(s); act; }

bool uiReflCalcAdvancedDlg::acceptOK( CallBacker* )
{
    if ( !advgrp_ )
	return true;

    const uiRetVal uirv = advgrp_->isOK();
    if ( !uirv.isOK() )
	mErrRet(uirv,return false)

    return true;
}


// uiReflCalc1D::Setup

uiReflCalc1D::Setup::Setup( bool singleangle )
    : singleangle_(singleangle)
    , doangles_(true)
{
    withadvanced_ = doangles_;
}


uiReflCalc1D::Setup::~Setup()
{
}


// uiReflCalc1D

uiReflCalc1D::uiReflCalc1D( uiParent* p, const Setup& su )
    : uiGroup( p )
    , parsChanged( this )
{
    if ( su.doangles_ )
    {
	if ( su.singleangle_ )
	{
	    const float defangle = ReflCalc1D::sDefAngle( Seis::AngleDegrees );
	    anglefld_ = new uiGenInput( this, tr("Chi angle (deg)"),
					IntInpSpec(defangle,-90,90) );
	    anglefld_->setElemSzPol( uiObject::Small );
	    anglefld_->setValue( defangle );
	    mAttachCB( anglefld_->valueChanged, uiReflCalc1D::parsChangedCB );
	}
	else
	{
	    const StepInterval<float> anglerg =
			      ReflCalc1D::sDefAngleRange( Seis::AngleDegrees );
	    const uiString olb = tr( "Chi angle range (start/stop) (deg)" );
	    IntInpIntervalSpec chilimits;
	    chilimits.setLimits( Interval<int>(-90,90) );
	    anglefld_ = new uiGenInput( this, olb, chilimits );
	    anglefld_->setElemSzPol( uiObject::Small );
	    anglefld_->setValue(
			    Interval<float>( anglerg.start, anglerg.stop ) );
	    mAttachCB( anglefld_->valueChanged, uiReflCalc1D::parsChangedCB );

	    anglestepfld_ = new uiGenInput( this, uiStrings::sStep() );
	    anglestepfld_->attach( rightOf, anglefld_ );
	    anglestepfld_->setElemSzPol( uiObject::Small );
	    anglestepfld_->setValue( anglerg.step );
	    mAttachCB( anglestepfld_->valueChanged,uiReflCalc1D::parsChangedCB);
	}
    }

    if ( anglefld_ )
	setHAlignObj( anglefld_ );

    mAttachCB( postFinalize(), uiReflCalc1D::initGrpCB );
}


uiReflCalc1D::~uiReflCalc1D()
{
    detachAllNotifiers();
    delete lastiop_;
}


void uiReflCalc1D::initGrpCB( CallBacker* )
{
    initGrp();
}


void uiReflCalc1D::initGrp()
{
    setName( factoryKeyword() );
}


void uiReflCalc1D::ensureHasAdvancedButton()
{
    if ( advbut_ )
	return;

    CallBack cbadv = mCB(this,uiReflCalc1D,getAdvancedPush);
    advbut_ = new uiPushButton( this, tr("EI Calculation parameters"),
				cbadv, false );
    if ( lastFld() )
	advbut_->attach( alignedBelow, lastFld() );
    else
	{ pErrMsg("Incorrect layout"); }
}


void uiReflCalc1D::setAdvancedGroup( uiReflCalcAdvancedGrp* grp )
{
    if ( !grp )
	return;

    if ( advgrp_ )
	mDetachCB( advgrp_->parsChanged, uiReflCalc1D::parsChangedCB );

    delete advgrp_;
    advgrp_ = grp;
    mAttachCB( advgrp_->parsChanged, uiReflCalc1D::parsChangedCB );

    if ( lastFld() )
	advgrp_->attach( alignedBelow, lastFld() );
    else
	setHAlignObj( advgrp_ );
}


uiReflCalcAdvancedGrp* uiReflCalc1D::getAvancedGrp( uiParent* p )
{
    return nullptr;
/*    return doConvertedWaves() ?
		new uiReflCalcAdvancedGrp( p, doConvertedWaves(),
					    doReflectivity() ) : nullptr;*/
}


void uiReflCalc1D::getAdvancedPush( CallBacker* )
{
    if ( !advdlg_ )
    {
	advdlg_ = new uiReflCalcAdvancedDlg( *this );
	if ( !advdlg_ || !advdlg_->isOK() )
	{
	    closeAndNullPtr( advdlg_ );
	    return;
	}

	if ( lastiop_ )
	{
	    advdlg_->usePar( *lastiop_ );
	    deleteAndNullPtr( lastiop_ );
	}

	mAttachCB( advdlg_->parsChanged, uiReflCalc1D::parsChangedCB );
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


void uiReflCalc1D::parsChangedCB( CallBacker* )
{
    parsChanged.trigger();
}


bool uiReflCalc1D::usePar( const IOPar& par )
{
    NotifyStopper ns( parsChanged );

    TypeSet<float> angles; bool angleindegrees = true;
    par.get( ReflCalc1D::sKeyAngle(), angles );
    const float convfactor =
	par.getYN(ReflCalc1D::sKeyAngleInDegrees(),angleindegrees) &&
	!angleindegrees ? mRad2DegF : 1.f;
    if ( !angles.isEmpty() && anglefld_ )
    {
	if ( angles.size() == 1 )
	{
	    anglefld_->setValue( angles[0] * convfactor );
	}
	else if ( anglestepfld_ && angles.size() > 1 )
	{
	    Interval<float> anglerg( angles.first(), angles.last() );
	    anglerg.scale( convfactor );
	    anglefld_->setValue( anglerg );
	    const float step = angles.size() > 1
		     ? angles[1]-angles[0]
		     : ReflCalc1D::sDefAngleRange( Seis::AngleDegrees ).step;
	    anglestepfld_->setValue( step * convfactor );
	}
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


void uiReflCalc1D::fillPar( IOPar& par ) const
{
    if ( anglefld_ && anglefld_->dataInpSpec()->nElems() == 1 )
    {
	const float angleval = anglefld_->getFValue();
	par.set( ReflCalc1D::sKeyAngle(), angleval );
    }
    else if ( anglefld_ && anglestepfld_ )
    {
	StepInterval<int> anglerg;
	anglerg.start = anglefld_->getIStepInterval().start;
	anglerg.stop = anglefld_->getIStepInterval().stop;
	anglerg.step = anglestepfld_->getIntValue();
	TypeSet<float> angles;
	for ( int idx=0; idx<anglerg.nrSteps()+1; idx++ )
	    angles += anglerg.atIndex( idx );

	par.set( ReflCalc1D::sKeyAngle(), angles );
    }

    par.setYN( ReflCalc1D::sKeyAngleInDegrees(), true );
    if ( advdlg_ )
	advdlg_->fillPar( par );
    else if ( advgrp_ )
	advgrp_->fillPar( par );
}


uiRetVal uiReflCalc1D::isOK() const
{
    uiRetVal uirv;
    if ( advgrp_ )
	uirv = advgrp_->isOK();

    return uirv;
}


// uiAICalc1D

uiAICalc1D::uiAICalc1D( uiParent* p, const uiReflCalc1D::Setup& su )
    : uiReflCalc1D( p, su )
{
}


uiAICalc1D::~uiAICalc1D()
{
}


void uiAICalc1D::initClass()
{
    uiReflCalc1D::factory().addCreator( create, sFactoryKeyword(),
					sFactoryDisplayName() );
}


void uiAICalc1D::fillPar( IOPar& ) const
{
}


bool uiAICalc1D::usePar( const IOPar& )
{
    return true;
}


// uiReflCalcAdvancedGrp

uiReflCalcAdvancedGrp::uiReflCalcAdvancedGrp( uiParent* p )
    : uiGroup(p,"EI Calculation advanced options")
    , parsChanged(this)
{
    mAttachCB( postFinalize(), uiReflCalcAdvancedGrp::initGrpCB );
}


uiReflCalcAdvancedGrp::~uiReflCalcAdvancedGrp()
{
    detachAllNotifiers();
}


void uiReflCalcAdvancedGrp::initGrpCB( CallBacker* )
{
    initGrp();
}


void uiReflCalcAdvancedGrp::initGrp()
{
}


void uiReflCalcAdvancedGrp::parsChangedCB( CallBacker* )
{
    parsChanged.trigger();
}


bool uiReflCalcAdvancedGrp::usePar( const IOPar& )
{
    return true;
}


void uiReflCalcAdvancedGrp::fillPar( IOPar& ) const
{
}


uiRetVal uiReflCalcAdvancedGrp::isOK() const
{
    return uiRetVal();
}
