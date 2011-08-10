/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uiraytrace1d.cc,v 1.7 2011-08-10 15:24:09 cvsbruno Exp $";

#include "uiraytrace1d.h"

#include "survinfo.h"
#include "uigeninput.h"


uiRayTracer1D::uiRayTracer1D( uiParent* p, const Setup& s)
    : uiGroup( p )
    , srcdepthfld_( 0 )
    , downwavefld_( 0 )
    , upwavefld_( 0 )
    , offsetfld_( 0 ) 
    , offsetstepfld_( 0 )
{
    if ( !s.dosourcereceiverdepth_ && !s.convertedwaves_ )
    {
	pErrMsg("Nothing to do"); return;
    }

    BufferString dptlbl( SI().zIsTime() ? "(ft)" : SI().getZUnitString(true) );

    RayTracer1D::Setup rsu = s.raysetup_ ? *s.raysetup_ : RayTracer1D::Setup();
    if ( s.dosourcereceiverdepth_ )
    {
	BufferString lb = "Source/Receiver depths"; lb += dptlbl;
	srcdepthfld_ =new uiGenInput(this,lb.buf(),FloatInpIntervalSpec(false));
	Interval<float> depths;
	depths.set( rsu.sourcedepth_, rsu.receiverdepth_);
	srcdepthfld_->setValue( depths );
    }

    if ( s.dooffsets_ )
    {
	BufferString olb = "offset range "; olb += dptlbl; olb +="(start/stop)";
	offsetfld_ = new uiGenInput( this, olb, IntInpIntervalSpec() );
	offsetfld_->setValue(Interval<float>(s.offsetrg_.start,s.offsetrg_.stop));
	offsetfld_->setElemSzPol( uiObject::Small );
	if ( srcdepthfld_ )
	    offsetfld_->attach( alignedBelow, srcdepthfld_ );

	offsetstepfld_ = new uiGenInput( this, "step" );
	offsetstepfld_->attach( rightOf, offsetfld_ );
	offsetstepfld_->setElemSzPol( uiObject::Small );
	offsetstepfld_->setValue( s.offsetrg_.step );
    }

    if ( s.convertedwaves_ )
    {
	BoolInpSpec inpspec( true, "P", "S" );
	downwavefld_ = new uiGenInput( this, "Downward wave-type", inpspec );
	if ( srcdepthfld_ )
	    downwavefld_->attach( alignedBelow, srcdepthfld_ );
	else if ( offsetfld_ )
	    offsetstepfld_->attach( alignedBelow, offsetfld_ );

	upwavefld_ = new uiGenInput( this, "Upward wave-type", inpspec );
	upwavefld_->attach( alignedBelow, downwavefld_ );

	downwavefld_->setValue( rsu.pdown_ );
	upwavefld_->setValue( rsu.pup_ );
    }

    if ( s.dosourcereceiverdepth_ )
	setHAlignObj( srcdepthfld_ );
    else
	setHAlignObj( downwavefld_ );

}


bool uiRayTracer1D::fill( RayTracer1D::Setup& setup )
{
    if ( srcdepthfld_ )
    {
	setup.sourcedepth_ = srcdepthfld_->getfValue(0);
	setup.receiverdepth_ = srcdepthfld_->getfValue(1);
    }

    if ( downwavefld_ )
    {
	setup.pdown_ = downwavefld_->getBoolValue();
	setup.pup_ = upwavefld_->getBoolValue();
    }

    return true;
}


void uiRayTracer1D::getOffsets( StepInterval<float>& offsetrg ) const
{
    if ( offsetfld_ && offsetstepfld_  )
    {
	offsetrg.start = offsetfld_->getIInterval().start;
	offsetrg.stop = offsetfld_->getIInterval().stop;
	offsetrg.step = (int)offsetstepfld_->getfValue();
    }
}


void uiRayTracer1D::displayOffsetFlds( bool yn ) 
{
    if ( offsetfld_ && offsetstepfld_ )
    {
	offsetfld_->display( yn );
	offsetstepfld_->display( yn );
    }
}
