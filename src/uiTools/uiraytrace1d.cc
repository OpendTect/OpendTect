/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "uiraytrace1d.h"

#include "keystrs.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiseparator.h"
#include "uistrings.h"

static const float cDefaultOffsetStep = 100.f;

mImplClassFactory( uiRayTracer1D, factory );


uiRayTracerSel::uiRayTracerSel( uiParent* p, const uiRayTracer1D::Setup& s )
    : uiGroup( p, "Ray Tracer Selector" )
    , raytracerselfld_(0)
    , offsetChanged(this)
{
    const uiStringSet& usernms = uiRayTracer1D::factory().getUserNames();
    const BufferStringSet& fackys = uiRayTracer1D::factory().getKeys();

    if ( fackys.size() > 1 )
    {
	raytracerselfld_ = new uiLabeledComboBox( this, tr("Ray-Tracer") );
	raytracerselfld_->box()->setHSzPol( uiObject::Wide );
	raytracerselfld_->box()->selectionChanged.notify(
				mCB(this,uiRayTracerSel,selRayTraceCB) );
    }

    for ( int idx=0; idx<fackys.size(); idx++ )
    {
	const OD::String& facnm( fackys.get(idx) );
	const uiString usernm( usernms.validIdx(idx) ? usernms[idx] :
			       toUiString(facnm) );

	uiRayTracer1D* grp = uiRayTracer1D::factory().create( facnm, this, s );
	if ( grp )
	{
	    grps_ += grp;
	    if ( grp->doOffsets() )
		grp->offsetChanged.notify(
			mCB(this,uiRayTracerSel,offsChangedCB) );
	    if ( raytracerselfld_ )
	    {
		raytracerselfld_->box()->addItem( usernm );
		grp->attach( alignedBelow, raytracerselfld_ );
		grp->setName( facnm );
	    }
	}
    }

    if ( !grps_.isEmpty() )
	setHAlignObj( grps_[0] );

    setCurrent( grps_.size()-1 );
    postFinalise().notify( mCB(this,uiRayTracerSel,selRayTraceCB) );
}


void uiRayTracerSel::offsChangedCB( CallBacker* )
{
    offsetChanged.trigger();
}


void uiRayTracerSel::selRayTraceCB( CallBacker* )
{
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->display( grps_[idx] == current() );
}


void uiRayTracerSel::usePar( const IOPar& par )
{
    BufferString type; par.get( sKey::Type(), type );
    for ( int igrp=0; igrp<grps_.size(); igrp++ )
	grps_[igrp]->usePar( par );
    setCurrentType( type );
    selRayTraceCB( 0 );
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
	raytracerselfld_ ? raytracerselfld_->box()->currentItem() : 0;
    return grps_.validIdx( selidx ) ? grps_[selidx] : 0;
}


bool uiRayTracerSel::setCurrentType( const char* typestr )
{
    if ( !raytracerselfld_ )
	return false;

    for ( int grpidx=0; grpidx<grps_.size(); grpidx++ )
    {
	if ( grps_[grpidx]->hasName(typestr) )
	{
	    raytracerselfld_->box()->setCurrentItem( grpidx );
	    return true;
	}
    }

    return false;
}


bool uiRayTracerSel::setCurrent( int selidx )
{
    if ( !grps_.validIdx(selidx) || !raytracerselfld_ )
	return false;

    raytracerselfld_->box()->setCurrentItem( selidx );
    return true;
}


template <class T>
static StepInterval<float> getDispStepIntv( const StepInterval<T>& rg )
{
    return StepInterval<float>( mCast(float,mNINT32(rg.start)),
				mCast(float,mNINT32(rg.stop)),
			        mCast(float,rg.step) );
}


uiRayTracer1D::uiRayTracer1D( uiParent* p, const Setup& setup )
    : uiGroup( p )
    , doreflectivity_(setup.doreflectivity_)
    , downwavefld_( 0 )
    , upwavefld_( 0 )
    , offsetfld_( 0 )
    , iszerooffsetfld_( 0 )
    , lastfld_( 0 )
    , offsetChanged( this )
{
    if ( setup.dooffsets_ )
    {
	const uiString olb = uiStrings::sOffsetRange().withSurvXYUnit();
	offsetfld_ = new uiGenInput( this, olb, IntInpIntervalSpec(true) );
	offsetfld_->setValue( getDispStepIntv(setup.offsetrg_) );
	offsetfld_->setElemSzPol( uiObject::Small );
	offsetfld_->valuechanged.notify(
		mCB(this,uiRayTracer1D,offsetChangedCB) );

	if ( setup.showzerooffsetfld_ )
	{
	    iszerooffsetfld_ =
		new uiCheckBox( this, tr("Zero Offset"),
				mCB(this,uiRayTracer1D,zeroOffsetChecked) );
	    iszerooffsetfld_->attach( rightTo, offsetfld_ );
	}
	lastfld_ = offsetfld_;
    }

    if ( setup.convertedwaves_ )
    {
	BoolInpSpec inpspec( true, tr("P","wave type"), tr("S","wave type") );
	downwavefld_ = new uiGenInput( this, tr("Downward wave-type"), inpspec);
	if ( lastfld_ )
	    downwavefld_->attach( alignedBelow, lastfld_ );
	lastfld_ = downwavefld_;

	upwavefld_ = new uiGenInput( this, tr("Upward wave-type"), inpspec );
	upwavefld_->attach( alignedBelow, lastfld_ );
	lastfld_ = upwavefld_;
    }

    IOPar par; RayTracer1D::Setup defaultsetup; defaultsetup.fillPar( par );
    usePar( par );

    if ( lastfld_ )
	setHAlignObj( lastfld_ );
}


void uiRayTracer1D::offsetChangedCB( CallBacker* )
{
    offsetChanged.trigger();
}


bool uiRayTracer1D::isZeroOffset() const
{
    return iszerooffsetfld_ ? iszerooffsetfld_->isChecked() : false;
}


void uiRayTracer1D::zeroOffsetChecked( CallBacker* )
{
    const bool iszeroffs = isZeroOffset();
    if ( offsetfld_ )
	offsetfld_->display( !iszeroffs );
    if ( downwavefld_ )
	downwavefld_->display( !iszeroffs );
    if ( upwavefld_ )
	upwavefld_->display( !iszeroffs );
    offsetChanged.trigger();
}


bool uiRayTracer1D::usePar( const IOPar& par )
{
    RayTracer1D::Setup tmpsetup;
    tmpsetup.usePar( par );

    if ( downwavefld_ )
    {
	downwavefld_->setValue( tmpsetup.pdown_ );
	upwavefld_->setValue( tmpsetup.pup_ );
    }

    TypeSet<float> offsets; par.get( RayTracer1D::sKeyOffset(), offsets );
    bool iszerooffs = par.isTrue( sKey::IsPS() );
    if ( !iszerooffs )
    {
	// support legacy where sKey::IsPS() is not set
	const auto nroffs = offsets.size();
	iszerooffs = nroffs < 1 || (nroffs==1 && mIsZero(offsets[0],0.001f));
    }
    if ( iszerooffsetfld_ )
	iszerooffsetfld_->setChecked( iszerooffs );

    if ( offsetfld_ )
    {
	if ( iszerooffs && !iszerooffsetfld_ )
	    offsetfld_->setValue( Interval<int>(0,0) );
	else
	{
	    const float step = offsets.size() > 1 ? offsets[1]-offsets[0]
						  : cDefaultOffsetStep;
	    StepInterval<float> offsetrg( offsets[0], offsets.last(), step );
	    if ( SI().xyInFeet() )
		offsetrg.scale( mToFeetFactorF );
	    offsetfld_->setValue( getDispStepIntv(offsetrg) );
	}
    }

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

    const bool iszeroofss = isZeroOffset();
    par.setYN( sKey::IsPS(), iszeroofss );
    StepInterval<float> offsetrg;
    if ( !iszeroofss )
    {
	offsetrg = offsetfld_->getFStepInterval();
	if ( SI().xyInFeet() )
	    offsetrg.scale( mFromFeetFactorF );
	offsetrg = getDispStepIntv( offsetrg );
    }

    TypeSet<float> offsets;
    const int nroffs = offsetrg.nrSteps() + 1;
    for ( int idx=0; idx<nroffs; idx++ )
	offsets += offsetrg.atIndex( idx );

    par.set( RayTracer1D::sKeyOffset(), offsets );
    par.setYN( RayTracer1D::sKeyReflectivity(), doreflectivity_);
}


void uiRayTracer1D::setOffsetRange( const StepInterval<float>& rg )
{
    offsetfld_->setValue( getDispStepIntv(rg) );
}


uiVrmsRayTracer1D::uiVrmsRayTracer1D(uiParent* p,const uiRayTracer1D::Setup& s)
    : uiRayTracer1D( p, s )
{
}


void uiVrmsRayTracer1D::initClass()
{
    uiRayTracer1D::factory().addCreator(create,
				VrmsRayTracer1D::sFactoryKeyword(),
				VrmsRayTracer1D::sFactoryDisplayName() );
}
