
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "uibouncysettingsdlg.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uiseparator.h"
#include "uicolor.h"
#include "uimsg.h"
#include "randcolor.h"
#include "survinfo.h"
#include "beachballdata.h"

namespace uiBouncy
{
uiBouncySettingsDlg::uiBouncySettingsDlg( uiParent* p, 
	visBeachBall::BallProperties* bp ) 
    : uiGroup( p, "Bouncy setup")
    , propertyChanged( this )
{
    const CallBack chgCB ( mCB(this, uiBouncySettingsDlg, changeCB) );

    radiusfld_ = new uiGenInput( this, "Ball's radius", FloatInpSpec( 1000 ) );
    radiusfld_->valuechanged.notify( chgCB );
    color1sel_ = new uiColorInput( this, 
	    uiColorInput::Setup( getRandStdDrawColor() ).lbltxt( "Color 1" ) );
    color1sel_->attach( alignedBelow, radiusfld_ );
    color1sel_->colorChanged.notify( chgCB );
    color2sel_ = new uiColorInput( this, 
	    uiColorInput::Setup( getRandStdDrawColor() ).lbltxt( "Color 2" ) );
    color2sel_->attach( alignedBelow, color1sel_ );
    color2sel_->colorChanged.notify( chgCB );

    uiGroup* inlcrlgrp = new uiGroup( this, "InlCrl group" );     
    inlcrlgrp->attach( leftAlignedBelow, color2sel_ );
    inlfld_ = new uiGenInput( inlcrlgrp, "In-line", 
	    IntInpSpec().setName("Inl-field") );
    inlfld_->setElemSzPol( uiObject::Small );
    inlfld_->setValue( 500 );
    inlfld_->valuechanged.notify( 
	    mCB(this, uiBouncySettingsDlg, inl_crlChangedCB) );
    crlfld_ = new uiGenInput( inlcrlgrp, "Cross-line",
	    IntInpSpec().setName("Crl-field") );
    crlfld_->setElemSzPol( uiObject::Small );
    crlfld_->setValue( 500 );
    crlfld_->attach( alignedBelow, inlfld_ );
    crlfld_->valuechanged.notify( 
	    mCB(this, uiBouncySettingsDlg, inl_crlChangedCB) );

    uiSeparator* sep = new uiSeparator( this, "VSep", false );
    sep->attach( rightOf, inlcrlgrp );
    sep->attach( heightSameAs, inlcrlgrp );

    uiGroup* xygrp = new uiGroup( this, "XY group" );
    xygrp->attach( rightOf, sep );
    xfld_ = new uiGenInput( xygrp, "X-coordinate", 
	    DoubleInpSpec().setName("X-field") );
    xfld_->setElemSzPol( uiObject::Small );
    xfld_->valuechanged.notify( mCB(this, uiBouncySettingsDlg, x_yChangedCB) );
    yfld_ = new uiGenInput( xygrp, "Y-coordinate", 
	    DoubleInpSpec().setName("Y-field") );
    yfld_->setElemSzPol( uiObject::Small );
    yfld_->attach( alignedBelow, xfld_ );
    yfld_->valuechanged.notify( mCB(this, uiBouncySettingsDlg, x_yChangedCB) );

    if ( bp )
	setBallProperties( *bp );
    inl_crlChangedCB( 0 );
}


uiBouncySettingsDlg::~uiBouncySettingsDlg()
{
}


void uiBouncySettingsDlg::changeCB( CallBacker* )
{
    // update beachball for user to preview the change
    propertyChanged.trigger();
}


void uiBouncySettingsDlg::inl_crlChangedCB( CallBacker* )
{
    if ( ( strcmp( inlfld_->text(), "" ) == 0 ) || 
	 ( strcmp( crlfld_->text(), "" ) == 0 ) )
    {
	xfld_->setText( "" ); yfld_->setText( "" );
    	return;
    }

    BinID binid( inlfld_->getIntValue(), crlfld_->getIntValue() );
    if ( binid == BinID::udf() )
    {
	xfld_->setText( "" ); yfld_->setText( "" );
	return;
    }

    Coord coord( SI().transform( binid ) );
    xfld_->setValue( coord.x );
    yfld_->setValue( coord.y );
    inlfld_->setValue( binid.inl );
    crlfld_->setValue( binid.crl );

    changeCB( 0 );
}


void uiBouncySettingsDlg::x_yChangedCB( CallBacker* )
{
     Coord coord( xfld_->getdValue(), yfld_->getdValue() );
     if ( coord == Coord::udf() )
     {
	 inlfld_->setText( "" ); crlfld_->setText( "" );
	 return;
     }

     BinID binid( SI().transform( coord ) );
     inlfld_->setValue( binid.inl );
     crlfld_->setValue( binid.crl );
     xfld_->setValue( coord.x );
     yfld_->setValue( coord.y );

     changeCB( 0 );
}

#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiBouncySettingsDlg::isOK()
{
    if ( ( radiusfld_->isUndef( 0 ) ) || ( !binIDOK() ) )
    {
	mErrRet( "Error in input. Try again!" );
	return false;
    }
    else 
	return true;
}


bool uiBouncySettingsDlg::binIDOK()
{
    BinID binid( inlfld_->getIntValue(), crlfld_->getIntValue() );
    if ( binid == BinID::udf() )
	return false;
    else return true;
}


visBeachBall::BallProperties uiBouncySettingsDlg::getBallProperties() const
{
    return visBeachBall::BallProperties( "", radiusfld_->getIntValue(),
	   color1sel_->color(), color2sel_->color(), 
	   Coord3( xfld_->getdValue(), yfld_->getdValue(), 0.5 ), 0.5 );
}


void uiBouncySettingsDlg::setBallProperties( 
	const visBeachBall::BallProperties& bp )
{
    radiusfld_->setValue( bp.radius() );
    color1sel_->setColor( bp.color1() );
    color2sel_->setColor( bp.color2() );
    Coord3 pos = bp.pos();
    xfld_->setValue( pos.x );
    yfld_->setValue( pos.y );
    x_yChangedCB( 0 );
}

}

