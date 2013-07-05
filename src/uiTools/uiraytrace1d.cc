/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiraytrace1d.h"

#include "survinfo.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiseparator.h"


mImplFactory2Param( uiRayTracer1D, uiParent*, const uiRayTracer1D::Setup&, 
			uiRayTracer1D::factory );


uiRayTracerSel::uiRayTracerSel( uiParent* p, const uiRayTracer1D::Setup& s ) 
    : uiGroup( p, "Ray Tracer Selector" )
    , raytracerselfld_(0)
    , offsetChanged(this)
{
    const BufferStringSet& usernms = uiRayTracer1D::factory().getNames( true );
    const BufferStringSet& facnms = uiRayTracer1D::factory().getNames( false );

    if ( facnms.size() > 1 )
    {
	raytracerselfld_ = new uiLabeledComboBox( this, "Ray-Tracer" );
	raytracerselfld_->box()->setHSzPol( uiObject::Wide );
	raytracerselfld_->box()->selectionChanged.notify( 
				mCB( this, uiRayTracerSel, selRayTraceCB) );

    }

    for ( int idx=0; idx<facnms.size(); idx++ )
    {
	const BufferString& facnm( facnms.get(idx) );
	const BufferString& usernm( usernms.validIdx(idx) ?
				usernms.get(idx ) : facnm );

	uiRayTracer1D* grp = uiRayTracer1D::factory().create(facnm,this,s,true);
	if ( grp )
	{
	    grps_ += grp;
	    if ( grp->doOffsets() )
    		grp->offsetChanged().notify(
			mCB(this,uiRayTracerSel,offsChangedCB) );
	    if ( raytracerselfld_ ) 
	    {
		raytracerselfld_->box()->addItem( usernm );
		raytracerselfld_->box()->setCurrentItem( usernm );
		grp->attach( alignedBelow, raytracerselfld_ );
		grp->setName( facnm );
	    }
	}
    }
    
    if ( !grps_.isEmpty() )
	setHAlignObj( grps_[0] );

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
    int igrp = 0;
    for ( ; igrp<grps_.size(); igrp++ )
    {
	if ( grps_[igrp]->name() == type )
	{
	    grps_[igrp]->usePar( par );
	    break;
	}
    }
    if ( raytracerselfld_ )
    {
	raytracerselfld_->box()->setCurrentItem( igrp );
	selRayTraceCB( 0 );
    }
}


void uiRayTracerSel::fillPar( IOPar& par ) const
{
    if ( !current() ) return;
    current()->fillPar( par );
    par.set( sKey::Type(), current()->name() );
}


const uiRayTracer1D* uiRayTracerSel::current() const
{
    int selidx = raytracerselfld_ ? raytracerselfld_->box()->currentItem() : 0;
    return grps_.validIdx( selidx ) ? grps_[selidx] : 0;
}


uiRayTracer1D* uiRayTracerSel::current()
{
    int selidx = raytracerselfld_ ? raytracerselfld_->box()->currentItem() : 0;
    return grps_.validIdx( selidx ) ? grps_[selidx] : 0;
}


bool uiRayTracerSel::setCurrent( int selidx )
{
    if ( !grps_.validIdx(selidx) ) return false;
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
    , lastfld_( 0 )
{
    BufferString xylbl( SI().getXYUnitString(true) );

    if ( s.dooffsets_ )
    {
	BufferString olb = "offset range "; olb += xylbl; olb +="(start/stop)";
	offsetfld_ = new uiGenInput( this, olb, IntInpIntervalSpec() );
	offsetfld_->setValue(
			Interval<float>(s.offsetrg_.start,s.offsetrg_.stop));
	offsetfld_->setElemSzPol( uiObject::Small );

	offsetstepfld_ = new uiGenInput( this, "step" );
	offsetstepfld_->attach( rightOf, offsetfld_ );
	offsetstepfld_->setElemSzPol( uiObject::Small );
	offsetstepfld_->setValue( s.offsetrg_.step );
	lastfld_ = offsetfld_; 
    }

    if ( s.convertedwaves_ )
    {
	BoolInpSpec inpspec( true, "P", "S" );
	downwavefld_ = new uiGenInput( this, "Downward wave-type", inpspec );
	downwavefld_->attach( alignedBelow, lastfld_ );
	lastfld_ = downwavefld_;

	upwavefld_ = new uiGenInput( this, "Upward wave-type", inpspec );
	upwavefld_->attach( alignedBelow, lastfld_ );
	lastfld_ = upwavefld_; 
    }

    IOPar par; RayTracer1D::Setup defaultsetup; defaultsetup.fillPar( par ); 
    usePar( par ); 

    if ( lastfld_ ) setHAlignObj( lastfld_ );
}


Notifier<uiGenInput>& uiRayTracer1D::offsetChanged()
{ return offsetfld_->valuechanged; }


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
    if ( offsets.size()>1 )
    {
	Interval<float> offsetrg( offsets[0], offsets[offsets.size()-1] );
	if ( offsetfld_ && offsetstepfld_  )
	{
	    offsetfld_->setValue( offsetrg );
	    const float step = offsets.size() > 1 ? offsets[1]-offsets[0] : 0;
	    offsetstepfld_->setValue( step );
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

    StepInterval<float> offsetrg;
    if ( offsetfld_ && offsetstepfld_ && offsetfld_->attachObj()->isDisplayed())
    {
	offsetrg.start = mCast( float, offsetfld_->getIInterval().start );
	offsetrg.stop = mCast( float, offsetfld_->getIInterval().stop );
	offsetrg.step = mCast( float, (int)offsetstepfld_->getfValue() );
    }
    TypeSet<float> offsets; 
    for ( int idx=0; idx<offsetrg.nrSteps()+1; idx++ )
	offsets += offsetrg.atIndex( idx );

    par.set( RayTracer1D::sKeyOffset(), offsets );
    par.setYN( RayTracer1D::sKeyReflectivity(), doreflectivity_);
}


void uiRayTracer1D::setOffsetRange( StepInterval<float> rg )
{
    offsetfld_->setValue( rg );
    offsetstepfld_->setValue( rg.step );
}



void uiRayTracer1D::displayOffsetFlds( bool yn )
{
    offsetfld_->display( yn );
    offsetstepfld_->display( yn );
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


