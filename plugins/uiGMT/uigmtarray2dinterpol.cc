/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uigmtarray2dinterpol.h"

#include "uiarray2dinterpol.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uigmtinfodlg.h"
#include "uilabel.h"
#include "uimsg.h"

#include "commondefs.h"
#include "envvars.h"
#include "gmtarray2dinterpol.h"
#include "iopar.h"
#include "pixmap.h"
#include "survinfo.h"

static bool hasGMTInst()
{ return GetEnvVar("GMT_SHAREDIR"); }


//uiGMTSurfaceGrid
const char* uiGMTSurfaceGrid::sName()
{ return "Continuous curvature(GMT)"; }


void uiGMTSurfaceGrid::initClass()
{
    uiArray2DInterpolSel::factory().addCreator( create, sName() );
}


uiArray2DInterpol* uiGMTSurfaceGrid::create( uiParent* p )
{
    return new uiGMTSurfaceGrid( p );
}


#define mCreateUI( classname, function ) \
{ \
    BufferString msg( "To use this GMT algorithm you need to install GMT.", \
	    	      "\nClick on GMT-button for more information" ); \
    uiLabel* lbl = new uiLabel( this, msg ); \
    uiPushButton* gmtbut = new uiPushButton( this, "", "gmt_logo", \
	    				mCB(this,classname,function), true ); \
    gmtbut->setToolTip( "GMT info" ); \
    gmtbut->attach( alignedBelow, lbl ); \
    setHAlignObj( lbl ); \
}


uiGMTSurfaceGrid::uiGMTSurfaceGrid( uiParent* p )
    : uiArray2DInterpol( p, "GMT grid" )
    , tensionfld_(0)
{
    if ( hasGMTInst() )
    {
	tensionfld_ = new uiGenInput( this, "Tension", FloatInpSpec(0.25) );
	setHAlignObj( tensionfld_ );
    }
    else
	mCreateUI(uiGMTSurfaceGrid,gmtPushCB);
}



void uiGMTSurfaceGrid::fillPar( IOPar& iop ) const
{
    if ( tensionfld_ )
	iop.set( "Tension", tensionfld_->getfValue() );
}


bool uiGMTSurfaceGrid::acceptOK()
{
    if ( !tensionfld_ )
    {
	uiMSG().message( "No GMT instllation found" );
	return false;
    }

    if ( tensionfld_->getfValue()<=0 || tensionfld_->getfValue()>=1 )
    {
	uiMSG().message( "Tension value should be in between 0 and 1" );
	tensionfld_->setValue( 0.25 );
	return false;
    }

    Array2DInterpol* intrp = Array2DInterpol::factory().create( sName() );
    mDynamicCastGet( GMTSurfaceGrid*, res, intrp );
    if ( !res )
	return false;

    res->setTension( tensionfld_->getfValue() );
    result_ = res;

    return true;
}


void uiGMTSurfaceGrid::gmtPushCB( CallBacker* )
{
    uiGMTInfoDlg dlg( this );
    dlg.go();
}


//uiGMTNearNeighborGrid
const char* uiGMTNearNeighborGrid::sName()
{ return "Nearest neighbor(GMT)"; }


void uiGMTNearNeighborGrid::initClass()
{
    uiArray2DInterpolSel::factory().addCreator( create, sName() );
}


uiArray2DInterpol* uiGMTNearNeighborGrid::create( uiParent* p )
{
    return new uiGMTNearNeighborGrid( p );
}


uiGMTNearNeighborGrid::uiGMTNearNeighborGrid( uiParent* p )
    : uiArray2DInterpol( p, "GMT grid" )
    , radiusfld_(0) 
{
    if ( hasGMTInst() )
    {
	BufferString lbl( "Search radius " );
	lbl.add( SI().getXYUnitString() );
	radiusfld_ = new uiGenInput( this, lbl, FloatInpSpec(1) );
	const int maxval = (int)mMAX(SI().inlDistance(), SI().crlDistance());
	radiusfld_->setValue( maxval );
	setHAlignObj( radiusfld_ );
    }
    else
	mCreateUI(uiGMTNearNeighborGrid,gmtPushCB);
}


void uiGMTNearNeighborGrid::gmtPushCB( CallBacker* )
{
    uiGMTInfoDlg dlg( this );
    dlg.go();
}


void uiGMTNearNeighborGrid::fillPar( IOPar& iop ) const
{
    if ( radiusfld_ )
	iop.set( "Radius", radiusfld_->getfValue() );
}


bool uiGMTNearNeighborGrid::acceptOK()
{
    if ( !radiusfld_ )
    {
	uiMSG().message( "No GMT instllation found" );
	return false;
    }

    if ( radiusfld_->getfValue() <= 0 )
    {
	uiMSG().message( "Search radius should be greater than 0" );
	radiusfld_->setValue(mMAX(SI().inlDistance(), SI().crlDistance()) );
	return false;
    }

    Array2DInterpol* intrp = Array2DInterpol::factory().create( sName() );
    mDynamicCastGet( GMTNearNeighborGrid*, res, intrp );
    if ( ! res )
	return false;

    res->setRadius( radiusfld_->getfValue() );
    result_ = res;

    return true;
}
