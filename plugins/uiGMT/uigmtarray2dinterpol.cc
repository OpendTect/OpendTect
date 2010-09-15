/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uigmtarray2dinterpol.cc,v 1.3 2010-09-15 12:06:09 cvsnageswara Exp $";

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
    uiPushButton* gmtbut = new uiPushButton( this, "", "gmt_logo.png", \
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

    if ( tensionfld_->getfValue()<0 || tensionfld_->getfValue()>1 )
    {
	uiMSG().message( "Tension value should be in between 0 and 1" );
	tensionfld_->setValue( 0.25 );
	return false;
    }

    IOPar iop;
    fillPar( iop );
    GMTSurfaceGrid* res = new GMTSurfaceGrid;
    res->setPar( iop );
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
    , rediusfld_(0) 
{
    if ( hasGMTInst() )
    {
	BufferString lbl( "Search radius " );
	lbl.add( SI().getXYUnitString() );
	rediusfld_ = new uiGenInput( this, lbl, FloatInpSpec(1) );
	const int maxval = (int)mMAX(SI().inlDistance(), SI().crlDistance());
	rediusfld_->setValue( maxval );
	setHAlignObj( rediusfld_ );
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
    if ( rediusfld_ )
	iop.set( "Radius", rediusfld_->getfValue() );
}


bool uiGMTNearNeighborGrid::acceptOK()
{
    if ( !rediusfld_ )
    {
	uiMSG().message( "No GMT instllation found" );
	return false;
    }

    if ( rediusfld_->getfValue() <= 0 )
    {
	uiMSG().message( "Search radius should be greater than 0" );
	rediusfld_->setValue(mMAX(SI().inlDistance(), SI().crlDistance()) );
	return false;
    }

    IOPar iop;
    fillPar( iop );
    GMTNearNeighborGrid* res = new GMTNearNeighborGrid;
    res->setPar( iop );
    result_ = res;

    return true;
}
