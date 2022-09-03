/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigmtbasemap.h"

#include "gmtpar.h"
#include "axislayout.h"
#include "separstr.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitextedit.h"


static const int cTitleBoxHeight = 4;
static const int cTitleBoxWidth = 36;

uiGMTBaseMapGrp::uiGMTBaseMapGrp( uiParent* p )
    : uiDlgGroup(p,tr("Basemap"))
{
    titlefld_ = new uiGenInput( this, tr("Map title"),
                                StringInpSpec("Basemap") );

    xrgfld_ = new uiGenInput( this, tr("X range"),
			      IntInpIntervalSpec(false) );
    xrgfld_->setElemSzPol( uiObject::Medium );
    xrgfld_->valuechanged.notify( mCB(this,uiGMTBaseMapGrp,xyrgChg) );
    xrgfld_->attach( alignedBelow, titlefld_ );

    yrgfld_ = new uiGenInput( this, tr("Y range"),
			      IntInpIntervalSpec(false) );
    yrgfld_->setElemSzPol( uiObject::Medium );
    yrgfld_->valuechanged.notify( mCB(this,uiGMTBaseMapGrp,xyrgChg) );
    yrgfld_->attach( alignedBelow, xrgfld_ );

    resetbut_ = new uiPushButton( this, tr("Reset to Survey"),
				  mCB(this,uiGMTBaseMapGrp,resetCB), true );
    resetbut_->attach( rightTo, yrgfld_ );

    xdimfld_ = new uiGenInput( this, tr("Map Width (cm)"), FloatInpSpec() );
    xdimfld_->valuechanged.notify( mCB(this,uiGMTBaseMapGrp,dimChg) );
    xdimfld_->setElemSzPol( uiObject::Small );
    xdimfld_->attach( alignedBelow, yrgfld_ );

    ydimfld_ = new uiGenInput( this, tr("Height (cm)"), FloatInpSpec() );
    ydimfld_->valuechanged.notify( mCB(this,uiGMTBaseMapGrp,dimChg) );
    ydimfld_->setElemSzPol( uiObject::Small );
    ydimfld_->attach( rightTo, xdimfld_ );

    scalefld_ = new uiGenInput( this, tr("Scale  1 :"), IntInpSpec() );
    scalefld_->valuechanged.notify( mCB(this,uiGMTBaseMapGrp,scaleChg) );
    scalefld_->setElemSzPol( uiObject::Medium );
    scalefld_->attach( rightTo, ydimfld_ );

    lebelintvfld_ = new uiGenInput( this, tr("Label interval (X/Y)"),
				    IntInpIntervalSpec(false) );
    lebelintvfld_->attach( alignedBelow, xdimfld_ );

    gridlinesfld_ = new uiCheckBox( this, tr("Draw Gridlines") );
    gridlinesfld_->attach( rightTo, lebelintvfld_ );


    remarkfld_ = new uiTextEdit( this, "Remarks" );
    remarkfld_->setPrefHeightInChar( cTitleBoxHeight );
    remarkfld_->setPrefWidthInChar( cTitleBoxWidth );
    remarkfld_->setStretch( 0, 0 );
    remarkfld_->attach( alignedBelow, lebelintvfld_ );
    new uiLabel( this, tr("Remarks (4 lines max)"), remarkfld_ );

    updateFlds( true );
}


void uiGMTBaseMapGrp::xyrgChg( CallBacker* cb )
{
    if ( !xrgfld_ || !yrgfld_ || !lebelintvfld_ )
	return;

    updateFlds( false );
}


void uiGMTBaseMapGrp::reset()
{
    titlefld_->setText( "Basemap" );
    updateFlds( true );
    remarkfld_->setText( "" );
}


void uiGMTBaseMapGrp::dimChg( CallBacker* cb )
{
    if ( !cb || !xdimfld_ || !ydimfld_ )
	return;

    mDynamicCastGet(uiGenInput*,fld,cb);
    float xdim, ydim;
    if ( fld == xdimfld_ )
    {
	xdim = xdimfld_->getFValue();
	if ( xdim < 1 || xdim > 200 )
	{
	    ydim = ydimfld_->getFValue();
	    xdim = ydim * aspectratio_;
	    xdimfld_->setValue( xdim );
	    uiMSG().error( tr("Map width is beyond permissible limits") );
	    return;
	}

	ydim = xdim / aspectratio_;
	ydimfld_->setValue( ydim );
    }
    else if ( fld == ydimfld_ )
    {
	ydim = ydimfld_->getFValue();
	if ( ydim < 1 || ydim > 200 )
	{
	    xdim = xdimfld_->getFValue();
	    ydim = xdim * aspectratio_;
	    ydimfld_->setValue( ydim );
	    uiMSG().error( tr("Map height is beyond permissible limits") );
	    return;
	}

	xdim = ydim * aspectratio_;
	xdimfld_->setValue( xdim );
    }

    const Interval<int> xrg = xrgfld_->getIInterval();
    scalefld_->setValue( mNINT32( xrg.width() * 100 / xdimfld_->getFValue() ) );
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
	xrg.start = (float) survmin.x; xrg.stop = (float) survmax.x;
	yrg.start = (float) survmin.y; yrg.stop = (float) survmax.y;
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
	uiMSG().error( tr("Y range is beyond permissible limits") );
	return;
    }

    aspectratio_ = (float) (xrg.width() / yrg.width());
    if ( aspectratio_ < 0.01 || aspectratio_ > 100 )
    {
	uiMSG().error( tr("Unreasonable aspect ratio",
		          "Please check the X and Y ranges") );
	xrgfld_->valuechanged.enable();
	yrgfld_->valuechanged.enable();
	return;
    }

    const AxisLayout<float> xaxis( xrg );
    const AxisLayout<float> yaxis( yrg );
    lebelintvfld_->setValue( Interval<float>(xaxis.sd_.step,yaxis.sd_.step) );
    const float scaleval = (aspectratio_>1?xaxis.sd_.step:yaxis.sd_.step) * 20;
    xdimfld_->setValue( xrg.width() * 100 / scaleval );
    ydimfld_->setValue( yrg.width() * 100 / scaleval );
    scalefld_->setValue( scaleval );
    xrgfld_->valuechanged.enable();
    yrgfld_->valuechanged.enable();
}


bool uiGMTBaseMapGrp::fillPar( IOPar& par ) const
{
    BufferString maptitle = titlefld_->text();
    par.set( ODGMT::sKeyMapTitle(), maptitle );

    const Interval<int> xrg = xrgfld_->getIInterval();
    const Interval<int> yrg = yrgfld_->getIInterval();
    if ( xrg.isRev() || yrg.isRev() )
    {
	uiMSG().error( tr("Invalid X or Y range") );
	return false;
    }

    par.set( ODGMT::sKeyXRange(), xrg );
    par.set( ODGMT::sKeyYRange(), yrg );

    const float mapwidth = xdimfld_->getFValue();
    const float mapheight = ydimfld_->getFValue();
    par.set( ODGMT::sKeyMapDim(), Interval<float>(mapwidth,mapheight) );

    par.set( ODGMT::sKeyMapScale(), scalefld_->getIntValue() );
    const Interval<float> lblintv = lebelintvfld_->getFInterval();
    par.set( ODGMT::sKeyLabelIntv(), lblintv );
    par.setYN( ODGMT::sKeyDrawGridLines(), gridlinesfld_->isChecked() );
    const char* remarks = remarkfld_->text();
    if ( !remarks || !*remarks )
	return true;

    SeparString sepstr( remarks, '\n' );
    BufferStringSet remset;
    int idx = 0;
    while ( idx < sepstr.size() && remset.size() < cTitleBoxHeight )
    {
	BufferString str = sepstr[idx++];
	const int nrstr = str.size() / cTitleBoxWidth;
	for ( int sdx=0; sdx<=nrstr && remset.size()<cTitleBoxHeight; sdx++ )
	{
	    BufferString linestr( str.buf() + cTitleBoxWidth * sdx );
	    if ( sdx < nrstr )
		*( linestr.getCStr() + cTitleBoxWidth ) = '\0';

	    remset.add( linestr );
	}
    }

    par.set( ODGMT::sKeyRemarks(), remset );

    return true;
}


bool uiGMTBaseMapGrp::usePar( const IOPar& par )
{
    const BufferString maptitle = par.find( ODGMT::sKeyMapTitle() );
    titlefld_->setText( maptitle );

    Interval<int> xrg, yrg;
    if ( par.get(ODGMT::sKeyXRange(),xrg) )
	xrgfld_->setValue( xrg );
    if ( par.get(ODGMT::sKeyYRange(),yrg) )
	yrgfld_->setValue( yrg );

    Interval<float> mapdim, lblintv;
    if ( par.get(ODGMT::sKeyMapDim(),mapdim) )
    {
	xdimfld_->setValue( mapdim.start );
	ydimfld_->setValue( mapdim.stop );
    }

    int scaleval = 1;
    if ( par.get(ODGMT::sKeyMapScale(),scaleval) )
	scalefld_->setValue( scaleval );
    if ( par.get(ODGMT::sKeyLabelIntv(),lblintv) )
	lebelintvfld_->setValue( lblintv );

    bool dogrdlines = false;
    par.getYN( ODGMT::sKeyDrawGridLines(), dogrdlines );
    gridlinesfld_->setChecked( dogrdlines );
    BufferStringSet remset;
    par.get( ODGMT::sKeyRemarks(), remset );
    BufferString remarks;
    for ( int idx=0; idx<remset.size(); idx++ )
    {
	remarks += remset.get( idx );
	remarks += "\n";
    }

    remarkfld_->setText( remarks.buf() );

    return true;
}
