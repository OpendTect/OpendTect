/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "uiraytrace1d.h"

#include "survinfo.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiseparator.h"
#include "uistrings.h"

mImplFactory2Param( uiRayTracer1D, uiParent*, const uiRayTracer1D::Setup&,
			uiRayTracer1D::factory );


uiRayTracerSel::uiRayTracerSel( uiParent* p, const uiRayTracer1D::Setup& s )
    : uiGroup( p, "Ray Tracer Selector" )
    , parsChanged(this)
{
    const uiStringSet& usernms = uiRayTracer1D::factory().getUserNames();
    const BufferStringSet& facnms = uiRayTracer1D::factory().getNames();

    uiLabeledComboBox* raytracersel = nullptr;
    if ( facnms.size() > 1 )
    {
	raytracersel = new uiLabeledComboBox( this, tr("Ray-Tracer") );
	raytracerselfld_ = raytracersel->box();
	raytracerselfld_->setHSzPol( uiObject::Wide );
	mAttachCB( raytracerselfld_->selectionChanged,
		   uiRayTracerSel::selRayTraceCB );
    }

    for ( int idx=0; idx<facnms.size(); idx++ )
    {
	const OD::String& facnm = facnms.get( idx );
	const uiString usernm( usernms.validIdx(idx) ? usernms[idx] :
			       mToUiStringTodo(facnm) );

	uiRayTracer1D* grp = uiRayTracer1D::factory().create(facnm,this,s,true);
	if ( !grp )
	    continue;

	grps_ += grp;
	mAttachCB( grp->parsChanged, uiRayTracerSel::parsChangedCB );
	if ( raytracerselfld_ )
	{
	    raytracerselfld_->addItem( usernm );
	    grp->attach( alignedBelow, raytracersel );
	    grp->setName( facnm );
	}
    }

    if ( !grps_.isEmpty() )
	setHAlignObj( grps_.first() );

    mAttachCB( postFinalise(), uiRayTracerSel::initGrp );
}


uiRayTracerSel::~uiRayTracerSel()
{
    detachAllNotifiers();
}


void uiRayTracerSel::initGrp( CallBacker* )
{
    setCurrent( grps_.size()-1 );
    selRayTraceCB( nullptr );
}


void uiRayTracerSel::selRayTraceCB( CallBacker* )
{
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->display( grps_[idx] == current() );

    parsChangedCB( nullptr );
}


void uiRayTracerSel::parsChangedCB( CallBacker* )
{
    parsChanged.trigger();
}


void uiRayTracerSel::usePar( const IOPar& par )
{
    BufferString type;
    par.get( sKey::Type(), type );
    for ( auto* grp : grps_ )
	grp->usePar( par );

    setCurrentType( type );
}


void uiRayTracerSel::fillPar( IOPar& par ) const
{
    if ( !current() )
	return;

    current()->fillPar( par );
    par.set( sKey::Type(), current()->name() );
}


const uiRayTracer1D* uiRayTracerSel::current() const
{
    return const_cast<uiRayTracerSel*>(this)->current();
}


uiRayTracer1D* uiRayTracerSel::current()
{
    const int selidx =
	raytracerselfld_ ? raytracerselfld_->currentItem() : 0;
    return grps_.validIdx( selidx ) ? grps_[selidx] : nullptr;
}


bool uiRayTracerSel::setCurrent( int selidx )
{
    if ( !grps_.validIdx(selidx) || !raytracerselfld_ )
	return false;

    raytracerselfld_->setCurrentItem( selidx );
    selRayTraceCB( nullptr );

    return true;
}


bool uiRayTracerSel::setCurrentType( const char* typestr )
{
    if ( !raytracerselfld_ )
	return false;

    for ( int grpidx=0; grpidx<grps_.size(); grpidx++ )
    {
	if ( grps_[grpidx]->name() == typestr )
	{
	    raytracerselfld_->setCurrentItem( grpidx );
	    selRayTraceCB( nullptr );
	    return true;
	}
    }

    return false;
}



uiRayTracer1D::uiRayTracer1D( uiParent* p, const Setup& s )
    : uiGroup( p )
    , doreflectivity_(s.doreflectivity_)
    , parsChanged( this )
{
    if ( s.dooffsets_ )
    {
	uiString olb = tr( "Offset range (start/stop) %1" )
			.arg( SI().getXYUnitString(true) );;
	offsetfld_ = new uiGenInput( this, olb, IntInpIntervalSpec() );
	offsetfld_->setElemSzPol( uiObject::Small );
	offsetfld_->setValue(
			Interval<float>(s.offsetrg_.start,s.offsetrg_.stop));
	mAttachCB( offsetfld_->valuechanged, uiRayTracer1D::offsetChangedCB );

	offsetstepfld_ = new uiGenInput( this, uiStrings::sStep() );
	offsetstepfld_->attach( rightOf, offsetfld_ );
	offsetstepfld_->setElemSzPol( uiObject::Small );
	offsetstepfld_->setValue( s.offsetrg_.step );
	mAttachCB( offsetstepfld_->valuechanged,
		   uiRayTracer1D::offsetChangedCB );
	if ( s.showzerooffsetfld_ )
	{
	    iszerooffsetfld_ = new uiCheckBox( this, tr("Zero Offset"),
				mCB(this,uiRayTracer1D,zeroOffsetChecked) );
	    iszerooffsetfld_->attach( rightTo, offsetstepfld_ );
	}

	lastfld_ = offsetfld_;
    }

    if ( s.convertedwaves_ )
    {
	const BoolInpSpec inpspec( true, tr("P"), tr("S") );
	downwavefld_ = new uiGenInput( this, tr("Downward wave-type"), inpspec);
	downwavefld_->attach( alignedBelow, lastfld_ );
	mAttachCB( downwavefld_->valuechanged, uiRayTracer1D::parsChangedCB );

	upwavefld_ = new uiGenInput( this, tr("Upward wave-type"), inpspec );
	upwavefld_->attach( alignedBelow, downwavefld_ );
	mAttachCB( upwavefld_->valuechanged, uiRayTracer1D::parsChangedCB );
	lastfld_ = upwavefld_;
    }

    if ( lastfld_ )
	setHAlignObj( lastfld_ );

    mAttachCB( postFinalise(), uiRayTracer1D::initGrp );
}


uiRayTracer1D::~uiRayTracer1D()
{
    detachAllNotifiers();
}


void uiRayTracer1D::initGrp( CallBacker* )
{
    IOPar par;
    RayTracer1D::Setup defaultsetup;
    defaultsetup.fillPar( par );
    usePar( par );
}


void uiRayTracer1D::offsetChangedCB( CallBacker* cb )
{
    doOffsetChanged();
    parsChangedCB( nullptr );
}


void uiRayTracer1D::parsChangedCB( CallBacker* )
{
    parsChanged.trigger();
}


bool uiRayTracer1D::isZeroOffset() const
{
    if ( iszerooffsetfld_ )
	return iszerooffsetfld_->isChecked();
    return !isOffsetFldsDisplayed();
}


void uiRayTracer1D::zeroOffsetChecked( CallBacker* )
{
    displayOffsetFlds( !iszerooffsetfld_->isChecked() );
    if ( downwavefld_ )
	downwavefld_->display( !isZeroOffset() );

    if ( upwavefld_ )
	upwavefld_->display( !isZeroOffset() );

    parsChangedCB( nullptr );
}

#define mIsZeroOffset( offsets ) \
    (offsets.isEmpty() || (offsets.size()==1 && mIsZero(offsets[0],mDefEps)))
bool isZeroOffset( TypeSet<float> offsets )
{
    if ( offsets.isEmpty() )
	return true;
    const bool iszero = mIsZero(offsets[0],mDefEps);
    return offsets.size()==1 && iszero;
}


bool uiRayTracer1D::usePar( const IOPar& par )
{
    RayTracer1D::Setup tmpsetup;
    tmpsetup.usePar( par );
    NotifyStopper ns( parsChanged );

    if ( downwavefld_ )
    {
	downwavefld_->setValue( tmpsetup.pdown_ );
	upwavefld_->setValue( tmpsetup.pup_ );
    }

    TypeSet<float> offsets;
    par.get( RayTracer1D::sKeyOffset(), offsets );
    const bool isps = !mIsZeroOffset(offsets);
    displayOffsetFlds( isps );
    if ( isps )
    {
	const float convfactor = SI().xyInFeet() ? mToFeetFactorF : 1;
	Interval<float> offsetrg( offsets[0], offsets[offsets.size()-1] );
	if ( offsetfld_ && offsetstepfld_  )
	{
	    offsetfld_->setValue( offsetrg );
	    const float step =
		offsets.size() > 1 ? offsets[1]-offsets[0]
				   : RayTracer1D::sDefOffsetRange().step;
	    offsetstepfld_->setValue( step * convfactor );
	}
    }

    if ( iszerooffsetfld_ )
	iszerooffsetfld_->setChecked( !isps );

    ns.enableNotification();
    parsChangedCB( nullptr );

    return true;
}


void uiRayTracer1D::fillPar( IOPar& par ) const
{
    RayTracer1D::Setup tmpsetup;

    if ( downwavefld_ )
    {
	tmpsetup.pdown_ = downwavefld_->getBoolValue();
	tmpsetup.pup_ = upwavefld_->getBoolValue();
    }
    tmpsetup.fillPar( par );

    StepInterval<float> offsetrg;
    if ( !isZeroOffset() )
    {
	offsetrg.start = mCast( float, offsetfld_->getIInterval().start );
	offsetrg.stop = mCast( float, offsetfld_->getIInterval().stop );
	offsetrg.step = offsetstepfld_->getFValue();
    }

    TypeSet<float> offsets;
    for ( int idx=0; idx<offsetrg.nrSteps()+1; idx++ )
	offsets += offsetrg.atIndex( idx );

    par.set( RayTracer1D::sKeyOffset(), offsets );
    par.set( RayTracer1D::sKeyOffsetInFeet(), SI().xyInFeet() );
    par.setYN( RayTracer1D::sKeyReflectivity(), doreflectivity_);
}


void uiRayTracer1D::setOffsetRange( StepInterval<float> rg )
{
    offsetfld_->setValue( rg );
    offsetstepfld_->setValue( rg.step );
    offsetChangedCB( nullptr );
}


bool uiRayTracer1D::isOffsetFldsDisplayed() const
{
    return offsetfld_ && offsetstepfld_ &&
	   offsetfld_->attachObj()->isDisplayed() &&
	   offsetstepfld_->attachObj()->isDisplayed();
}


void uiRayTracer1D::displayOffsetFlds( bool yn )
{
    if ( !offsetfld_ )
	return;

    offsetfld_->display( yn );
    offsetstepfld_->display( yn );
    offsetChangedCB( nullptr );
}



uiVrmsRayTracer1D::uiVrmsRayTracer1D(uiParent* p,const uiRayTracer1D::Setup& s)
    : uiRayTracer1D( p, s )
{}


void uiVrmsRayTracer1D::initClass()
{
    uiRayTracer1D::factory().addCreator(create,
				VrmsRayTracer1D::sFactoryKeyword(),
				VrmsRayTracer1D::sFactoryDisplayName() );
}
