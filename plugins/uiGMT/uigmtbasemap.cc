/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		Jube 2008
 RCS:		$Id: uigmtbasemap.cc,v 1.1 2008-08-01 08:31:21 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtbasemap.h"

#include "gmtpar.h"
#include "linear.h"
#include "survinfo.h"
#include "uigeninput.h"

uiGMTBaseMapGrp::uiGMTBaseMapGrp( uiParent* p )
    : uiDlgGroup(p,"Basemap")
{
    titlefld_ = new uiGenInput( this, "Map title", StringInpSpec("Basemap") );

    xrgfld_ = new uiGenInput( this, "X range",
	    		      FloatInpIntervalSpec(false) );
    xrgfld_->valuechanged.notify( mCB(this,uiGMTBaseMapGrp,xyrgChg) );
    xrgfld_->attach( alignedBelow, titlefld_ );

    yrgfld_ = new uiGenInput( this, "Y range",
	    		      FloatInpIntervalSpec(false) );
    yrgfld_->valuechanged.notify( mCB(this,uiGMTBaseMapGrp,xyrgChg) );
    yrgfld_->attach( alignedBelow, xrgfld_ );

    xdimfld_ = new uiGenInput( this, "Map width (cm)", FloatInpSpec() );
    xdimfld_->valuechanged.notify( mCB(this,uiGMTBaseMapGrp,dimChg) );
    xdimfld_->setElemSzPol( uiObject::Small );
    xdimfld_->attach( alignedBelow, yrgfld_ );

    ydimfld_ = new uiGenInput( this, "Map height (cm)", FloatInpSpec() );
    ydimfld_->valuechanged.notify( mCB(this,uiGMTBaseMapGrp,dimChg) );
    ydimfld_->setElemSzPol( uiObject::Small );
    ydimfld_->attach( rightTo, xdimfld_ );

    lebelintvfld_ = new uiGenInput( this, "Label interval (X/Y)",
	    			    IntInpIntervalSpec(false) );
    lebelintvfld_ ->attach( alignedBelow, xdimfld_ );

    updateFlds( true );
}


void uiGMTBaseMapGrp::xyrgChg( CallBacker* cb )
{
    if ( !xrgfld_ || !yrgfld_ || !lebelintvfld_ )
	return;

    updateFlds( false );
}


void uiGMTBaseMapGrp::dimChg( CallBacker* cb )
{
    if ( !cb || !xdimfld_ || !ydimfld_ )
	return;

    mDynamicCastGet(uiGenInput*,fld,cb);
    if ( fld == xdimfld_ )
    {
	const float xdim = xdimfld_->getfValue();
	const float ydim = xdim / aspectratio_;
	ydimfld_->setValue( ydim );
    }
    else if ( fld == ydimfld_ )
    {
	const float ydim = ydimfld_->getfValue();
	const float xdim = ydim * aspectratio_;
	xdimfld_->setValue( xdim );
    }
}


void uiGMTBaseMapGrp::updateFlds( bool fromsurvey )
{
    Interval<float> xrg;
    Interval<float> yrg;
    if ( fromsurvey )
    {
	const Coord survmin = SI().minCoord( false );
	const Coord survmax = SI().maxCoord( false );
	xrg.start = survmin.x; xrg.stop = survmax.x;
	yrg.start = survmin.y; yrg.stop = survmax.y;
	xrgfld_->setValue( xrg );
	yrgfld_->setValue( yrg );
    }
    else
    {
	xrg = xrgfld_->getFInterval();
	yrg = yrgfld_->getFInterval();
    }

    aspectratio_ = xrg.width() / yrg.width();
    xdimfld_->setValue( aspectratio_ > 1 ? 16 : 16 * aspectratio_ );
    ydimfld_->setValue( aspectratio_ > 1 ? 16 / aspectratio_ : 16 );
    const AxisLayout xaxis( xrg );
    const AxisLayout yaxis( yrg );
    lebelintvfld_->setValue( Interval<float>(xaxis.sd.step,yaxis.sd.step) );
}


bool uiGMTBaseMapGrp::fillPar( IOPar& par ) const
{
    BufferString maptitle = titlefld_->text();
    par.set( ODGMT::sKeyMapTitle, maptitle );

    const Interval<float> xrg = xrgfld_->getFInterval();
    const Interval<float> yrg = yrgfld_->getFInterval();
    par.set( ODGMT::sKeyXRange, xrg );
    par.set( ODGMT::sKeyYRange, yrg );

    const float mapwidth = xdimfld_->getfValue();
    const float mapheight = ydimfld_->getfValue();
    par.set( ODGMT::sKeyMapDim, Interval<float>(mapwidth,mapheight) );

    const Interval<float> lblintv = lebelintvfld_->getFInterval();
    par.set( ODGMT::sKeyLabelIntv, lblintv );
    return true;
}


bool uiGMTBaseMapGrp::usePar( const IOPar& par )
{
    return true;
}
