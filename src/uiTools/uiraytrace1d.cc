/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uiraytrace1d.cc,v 1.3 2011-02-01 21:32:39 cvsyuancheng Exp $";

#include "uiraytrace1d.h"

#include "survinfo.h"
#include "uigeninput.h"


uiRayTracer1D::uiRayTracer1D( uiParent* p, bool dosourcereceiverdepth,
	bool convertedwaves, 
	const RayTracer1D::Setup* setup )
    : uiGroup( p )
    , srcdepthfld_( 0 )
    , downwavefld_( 0 )
    , upwavefld_( 0 )
{
    if ( !dosourcereceiverdepth && !convertedwaves )
    {
	pErrMsg("Nothing to do");
	return;
    }

    if ( dosourcereceiverdepth )
    {
	BufferString lb = "Source/Receiver depths";
	lb += SI().zIsTime() ? "(ft)" : SI().getZUnitString( true );
	srcdepthfld_ =new uiGenInput(this,lb.buf(),FloatInpIntervalSpec(false));
	if ( setup )
	{
	    Interval<float> depths(setup->sourcedepth_, setup->receiverdepth_);
	    srcdepthfld_->setValue( depths );
	}
    }

    if ( convertedwaves )
    {
	BoolInpSpec inpspec( true, "P", "S" );
	downwavefld_ = new uiGenInput( this, "Downward wave-type",
		inpspec );
	if ( srcdepthfld_ )
	    downwavefld_->attach( alignedBelow, srcdepthfld_ );

	upwavefld_ = new uiGenInput( this, "Upward wave-type",
		inpspec );
	upwavefld_->attach( alignedBelow, downwavefld_ );

	if ( setup )
	{
	    downwavefld_->setValue( setup->pdown_ );
	    upwavefld_->setValue( setup->pup_ );
	}
    }

    if ( dosourcereceiverdepth )
	setHAlignObj( srcdepthfld_ );
    else
	setHAlignObj( downwavefld_ );
}


bool uiRayTracer1D::fill( RayTracer1D::Setup& setup ) const
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
