/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uiraytrace1d.cc,v 1.4 2011-07-12 10:51:55 cvsbruno Exp $";

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
    , vp2vsfld_( 0 )			 
{
    if ( !s.dosourcereceiverdepth_ && !s.convertedwaves_ )
    {
	pErrMsg("Nothing to do"); return;
    }

    BufferString dptlbl( SI().zIsTime() ? "(ft)" : SI().getZUnitString(true) );

    if ( s.dosourcereceiverdepth_ )
    {
	BufferString lb = "Source/Receiver depths"; lb += dptlbl;
	srcdepthfld_ =new uiGenInput(this,lb.buf(),FloatInpIntervalSpec(false));
	if ( s.raysetup_ )
	{
	    Interval<float> depths( s.raysetup_->sourcedepth_, 
				    s.raysetup_->receiverdepth_);
	    srcdepthfld_->setValue( depths );
	}
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

	if ( s.raysetup_ )
	{
	    downwavefld_->setValue( s.raysetup_->pdown_ );
	    upwavefld_->setValue( s.raysetup_->pup_ );
	}
    }

    if ( s.dopwave2swaveconv_ )
    {
	vp2vsfld_ = new uiGenInput( this, "Vp, Vs factors (a/b)", 
					FloatInpIntervalSpec() );
	vp2vsfld_->attach( alignedBelow, srcdepthfld_ );
	if ( s.raysetup_ )
	{
	    vp2vsfld_->setValue( Interval<float>( s.raysetup_->pvel2svelafac_,
						  s.raysetup_->pvel2svelbfac_));
	}
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

    if ( vp2vsfld_ )
    {
	setup.pvel2svelafac_ = vp2vsfld_->getFInterval().start;
	setup.pvel2svelbfac_ = vp2vsfld_->getFInterval().stop;
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
