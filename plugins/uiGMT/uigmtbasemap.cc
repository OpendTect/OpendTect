/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		Jube 2008
 RCS:		$Id: uigmtbasemap.cc,v 1.5 2008-08-27 12:35:30 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtbasemap.h"

#include "gmtpar.h"
#include "linear.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"

uiGMTBaseMapGrp::uiGMTBaseMapGrp( uiParent* p )
    : uiDlgGroup(p,"Basemap")
{
    titlefld_ = new uiGenInput( this, "Map title", StringInpSpec("Basemap") );

    xrgfld_ = new uiGenInput( this, "X range",
	    		      IntInpIntervalSpec(false) );
    xrgfld_->valuechanged.notify( mCB(this,uiGMTBaseMapGrp,xyrgChg) );
    xrgfld_->attach( alignedBelow, titlefld_ );

    yrgfld_ = new uiGenInput( this, "Y range",
	    		      IntInpIntervalSpec(false) );
    yrgfld_->valuechanged.notify( mCB(this,uiGMTBaseMapGrp,xyrgChg) );
    yrgfld_->attach( alignedBelow, xrgfld_ );

    resetbut_ = new uiPushButton( this, "&Reset to Survey",
	    			  mCB(this,uiGMTBaseMapGrp,resetCB), false );
    resetbut_->attach( rightTo, yrgfld_ );

    xdimfld_ = new uiGenInput( this, "Map Width (cm)", FloatInpSpec() );
    xdimfld_->valuechanged.notify( mCB(this,uiGMTBaseMapGrp,dimChg) );
    xdimfld_->setElemSzPol( uiObject::Small );
    xdimfld_->attach( alignedBelow, yrgfld_ );

    ydimfld_ = new uiGenInput( this, "Height (cm)", FloatInpSpec() );
    ydimfld_->valuechanged.notify( mCB(this,uiGMTBaseMapGrp,dimChg) );
    ydimfld_->setElemSzPol( uiObject::Small );
    ydimfld_->attach( rightTo, xdimfld_ );

    scalefld_ = new uiGenInput( this, "Scale  1 :", IntInpSpec() );
    scalefld_->valuechanged.notify( mCB(this,uiGMTBaseMapGrp,scaleChg) );
    scalefld_->setElemSzPol( uiObject::Small );
    scalefld_->attach( rightTo, ydimfld_ );

    lebelintvfld_ = new uiGenInput( this, "Label interval (X/Y)",
	    			    IntInpIntervalSpec(false) );
    lebelintvfld_->attach( alignedBelow, xdimfld_ );

    gridlinesfld_ = new uiCheckBox( this, "Draw Gridlines" );
    gridlinesfld_->attach( rightTo, lebelintvfld_ );
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
    float xdim, ydim;
    if ( fld == xdimfld_ )
    {
	xdim = xdimfld_->getfValue();
	if ( xdim < 1 || xdim > 200 )
	{
	    ydim = ydimfld_->getfValue();
	    xdim = ydim * aspectratio_;
	    xdimfld_->setValue( xdim );
	    uiMSG().error( "Map width is beyond permissible limits" );
	    return;
	}

	ydim = xdim / aspectratio_;
	ydimfld_->setValue( ydim );
    }
    else if ( fld == ydimfld_ )
    {
	ydim = ydimfld_->getfValue();
	if ( ydim < 1 || ydim > 200 )
	{
	    xdim = xdimfld_->getfValue();
	    ydim = xdim * aspectratio_;
	    ydimfld_->setValue( ydim );
	    uiMSG().error( "Map height is beyond permissible limits" );
	    return;
	}

	xdim = ydim * aspectratio_;
	xdimfld_->setValue( xdim );
    }

    const Interval<int> xrg = xrgfld_->getIInterval();
    scalefld_->setValue( mNINT( xrg.width() * 100 / xdimfld_->getfValue() ) );
}


void uiGMTBaseMapGrp::resetCB( CallBacker* )
{
    updateFlds( true );
}


void uiGMTBaseMapGrp::scaleChg( CallBacker* cb )
{
    const float factor = (float)scalefld_->getIntValue();
    const Interval<int> xrg = xrgfld_->getIInterval();
    const Interval<int> yrg = yrgfld_->getIInterval();
    xdimfld_->setValue( xrg.width() * 100 / factor );
    ydimfld_->setValue( yrg.width() * 100 / factor );
}


void uiGMTBaseMapGrp::updateFlds( bool fromsurvey )
{
    Interval<float> xrg;
    Interval<float> yrg;
    xrgfld_->valuechanged.disable();
    yrgfld_->valuechanged.disable();
    Interval<int> xintv, yintv;
    if ( fromsurvey )
    {
	const Coord survmin = SI().minCoord( false );
	const Coord survmax = SI().maxCoord( false );
	xrg.start = survmin.x; xrg.stop = survmax.x;
	yrg.start = survmin.y; yrg.stop = survmax.y;
	xintv.setFrom( xrg ); yintv.setFrom( yrg );
	xrgfld_->setValue( xintv );
	yrgfld_->setValue( yintv );
    }
    else
    {
	xintv = xrgfld_->getIInterval();
	yintv = yrgfld_->getIInterval();
	xrg.setFrom( xintv );
	yrg.setFrom( yintv );
    }

    if ( mIsZero(yrg.width(),mDefEps) )
    {
	xrgfld_->valuechanged.enable();
	yrgfld_->valuechanged.enable();
	uiMSG().error( "Y range is beyond permissible limits" );
	return;
    }

    aspectratio_ = xrg.width() / yrg.width();
    if ( aspectratio_ < 0.01 || aspectratio_ > 100 )
    {
	uiMSG().error( "Unreasonable aspect ratio",
		       "Please check the X and Y ranges" );
	xrgfld_->valuechanged.enable();
	yrgfld_->valuechanged.enable();
	return;
    }

    xdimfld_->setValue( aspectratio_ > 1 ? 16 : 16 * aspectratio_ );
    ydimfld_->setValue( aspectratio_ > 1 ? 16 / aspectratio_ : 16 );
    scalefld_->setValue( xrg.width() * 100 / xdimfld_->getfValue() );
    const AxisLayout xaxis( xrg );
    const AxisLayout yaxis( yrg );
    lebelintvfld_->setValue( Interval<float>(xaxis.sd.step,yaxis.sd.step) );
    xrgfld_->valuechanged.enable();
    yrgfld_->valuechanged.enable();
}


bool uiGMTBaseMapGrp::fillPar( IOPar& par ) const
{
    BufferString maptitle = titlefld_->text();
    par.set( ODGMT::sKeyMapTitle, maptitle );

    const Interval<int> xrg = xrgfld_->getIInterval();
    const Interval<int> yrg = yrgfld_->getIInterval();
    if ( xrg.isRev() || yrg.isRev() )
    {
	uiMSG().error( "Invalid X or Y range" );
	return false;
    }

    par.set( ODGMT::sKeyXRange, xrg );
    par.set( ODGMT::sKeyYRange, yrg );

    const float mapwidth = xdimfld_->getfValue();
    const float mapheight = ydimfld_->getfValue();
    par.set( ODGMT::sKeyMapDim, Interval<float>(mapwidth,mapheight) );

    const Interval<float> lblintv = lebelintvfld_->getFInterval();
    par.set( ODGMT::sKeyLabelIntv, lblintv );
    par.setYN( ODGMT::sKeyDrawGridLines, gridlinesfld_->isChecked() );
    return true;
}


bool uiGMTBaseMapGrp::usePar( const IOPar& par )
{
    return true;
}
