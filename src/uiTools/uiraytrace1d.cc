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
    , raytracerselfld_(0)
    , offsetChanged(this)
{
    const uiStringSet& usernms = uiRayTracer1D::factory().getUserNames();
    const BufferStringSet& facnms = uiRayTracer1D::factory().getNames();

    if ( facnms.size() > 1 )
    {
	raytracerselfld_ = new uiLabeledComboBox( this, tr("Ray-Tracer") );
	raytracerselfld_->box()->setHSzPol( uiObject::Wide );
	raytracerselfld_->box()->selectionChanged.notify(
				mCB( this, uiRayTracerSel, selRayTraceCB) );

    }

    for ( int idx=0; idx<facnms.size(); idx++ )
    {
	const OD::String& facnm( facnms.get(idx) );
	const uiString usernm( usernms.validIdx(idx) ? usernms[idx] : 
			       mToUiStringTodo(facnm) );

	uiRayTracer1D* grp = uiRayTracer1D::factory().create(facnm,this,s,true);
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
	if ( grps_[grpidx]->name() == typestr )
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


uiRayTracer1D::uiRayTracer1D( uiParent* p, const Setup& s )
    : uiGroup( p )
    , doreflectivity_(s.doreflectivity_)
    , downwavefld_( 0 )
    , upwavefld_( 0 )
    , offsetfld_( 0 )
    , offsetstepfld_( 0 )
    , iszerooffsetfld_( 0 )
    , lastfld_( 0 )
    , offsetChanged( this )
{
    if ( s.dooffsets_ )
    {
	uiString olb = tr( "Offset range (start/stop) %1" )
			.arg( SI().getXYUnitString(true) );;
	offsetfld_ = new uiGenInput( this, olb, IntInpIntervalSpec() );
	offsetfld_->setValue(
			Interval<float>(s.offsetrg_.start,s.offsetrg_.stop));
	offsetfld_->setElemSzPol( uiObject::Small );
	offsetfld_->valuechanged.notify(
		mCB(this,uiRayTracer1D,offsetChangedCB) );

	offsetstepfld_ = new uiGenInput( this, uiStrings::sStep() );
	offsetstepfld_->attach( rightOf, offsetfld_ );
	offsetstepfld_->setElemSzPol( uiObject::Small );
	offsetstepfld_->setValue( s.offsetrg_.step );
	if ( s.showzerooffsetfld_ )
	{
	    iszerooffsetfld_ =
		new uiCheckBox( this, tr("Zero Offset"),
				mCB(this,uiRayTracer1D,zeroOffsetChecked) );
	    iszerooffsetfld_->attach( rightTo, offsetstepfld_ );
	}



	lastfld_ = offsetfld_;
    }

    if ( s.convertedwaves_ )
    {
	BoolInpSpec inpspec( true, tr("P"), tr("S") );
	downwavefld_ = new uiGenInput( this, tr("Downward wave-type"), inpspec);
	downwavefld_->attach( alignedBelow, lastfld_ );
	lastfld_ = downwavefld_;

	upwavefld_ = new uiGenInput( this, tr("Upward wave-type"), inpspec );
	upwavefld_->attach( alignedBelow, lastfld_ );
	lastfld_ = upwavefld_;
    }

    IOPar par; RayTracer1D::Setup defaultsetup; defaultsetup.fillPar( par );
    usePar( par );

    if ( lastfld_ ) setHAlignObj( lastfld_ );
}


void uiRayTracer1D::offsetChangedCB( CallBacker* )
{
    offsetChanged.trigger();
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
    if ( downwavefld_ ) downwavefld_->display( !isZeroOffset() );
    if ( upwavefld_ ) upwavefld_->display( !isZeroOffset() );
    offsetChanged.trigger();
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

    if ( downwavefld_ )
    {
	downwavefld_->setValue( tmpsetup.pdown_ );
	upwavefld_->setValue( tmpsetup.pup_ );
    }

    TypeSet<float> offsets; par.get( RayTracer1D::sKeyOffset(), offsets );
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
}



bool uiRayTracer1D::isOffsetFldsDisplayed() const
{
    return offsetfld_ && offsetstepfld_ &&
	   offsetfld_->attachObj()->isDisplayed() &&
	   offsetstepfld_->attachObj()->isDisplayed();
}


void uiRayTracer1D::displayOffsetFlds( bool yn )
{
    if ( !offsetfld_ ) return;

    offsetfld_->display( yn );
    offsetstepfld_->display( yn );
    offsetChanged.trigger();
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
