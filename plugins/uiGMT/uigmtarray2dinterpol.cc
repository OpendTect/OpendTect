/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/


#include "uigmtarray2dinterpol.h"

#include "uiarray2dinterpol.h"
#include "uigeninput.h"
#include "uigmtinfodlg.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitoolbutton.h"

#include "commondefs.h"
#include "envvars.h"
#include "gmtarray2dinterpol.h"
#include "initgmtplugin.h"
#include "iopar.h"
#include "survinfo.h"


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


static void createUi( uiArray2DInterpol* p, const CallBack& cb )
{
    uiString msg = od_static_tr( "create_GMT_UI",
			"To use this GMT algorithm you need to install GMT."
		       "\nClick on GMT-button for more information" );
    uiLabel* lbl = new uiLabel( p, msg );
    uiButton* gmtbut = new uiToolButton( p, "gmt_logo",
					 od_static_tr("create_GMT_UI",
					              "GMT info"),cb);

    gmtbut->attach( alignedBelow, lbl );
    p->setHAlignObj( lbl );
}


uiGMTSurfaceGrid::uiGMTSurfaceGrid( uiParent* p )
    : uiArray2DInterpol( p, tr("GMT grid") )
    , tensionfld_(0)
{
    if ( GMT::hasGMT() )
    {
	tensionfld_ = new uiGenInput( this, uiStrings::sTension(),
				      FloatInpSpec(0.25) );
	setHAlignObj( tensionfld_ );
    }
    else
	createUi( this, mCB(this,uiGMTSurfaceGrid,gmtPushCB));
}



void uiGMTSurfaceGrid::fillPar( IOPar& iop ) const
{
    if ( tensionfld_ )
	iop.set( "Tension", tensionfld_->getFValue() );
}


void uiGMTSurfaceGrid::setValuesFrom( const Array2DInterpol& arr )
{
    mDynamicCastGet(const GMTSurfaceGrid*, gmtsurf, &arr );
    if ( !gmtsurf )
	return;

   tensionfld_->setValue( gmtsurf->getTension() );
}


bool uiGMTSurfaceGrid::acceptOK()
{
    if ( !tensionfld_ )
    {
	uiMSG().message( tr("No GMT installation found") );
	return false;
    }

    if ( tensionfld_->getFValue()<=0 || tensionfld_->getFValue()>=1 )
    {
	uiMSG().message( tr("Tension value should be in between 0 and 1") );
	tensionfld_->setValue( 0.25 );
	return false;
    }

    Array2DInterpol* intrp = Array2DInterpol::factory().create( sName() );
    mDynamicCastGet( GMTSurfaceGrid*, res, intrp );
    if ( !res )
	return false;

    res->setTension( tensionfld_->getFValue() );
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
    : uiArray2DInterpol( p, tr("GMT grid") )
    , radiusfld_(0)
{
    if ( GMT::hasGMT() )
    {
	const uiString lbl = tr( "Search radius %1" )
				.arg( SI().getXYUnitString() );
	radiusfld_ = new uiGenInput( this, lbl, FloatInpSpec(1) );
	const int maxval = (int)mMAX(SI().inlDistance(), SI().crlDistance());
	radiusfld_->setValue( maxval );
	setHAlignObj( radiusfld_ );
    }
    else
	createUi( this, mCB(this,uiGMTNearNeighborGrid,gmtPushCB));
}


void uiGMTNearNeighborGrid::setValuesFrom( const Array2DInterpol& arr )
{
    mDynamicCastGet(const GMTNearNeighborGrid*, gmtneighbor, &arr );
    if ( !gmtneighbor )
      return;

   radiusfld_->setValue( gmtneighbor->getRadius() );
}


void uiGMTNearNeighborGrid::gmtPushCB( CallBacker* )
{
    uiGMTInfoDlg dlg( this );
    dlg.go();
}


void uiGMTNearNeighborGrid::fillPar( IOPar& iop ) const
{
    if ( radiusfld_ )
	iop.set( "Radius", radiusfld_->getFValue() );
}


bool uiGMTNearNeighborGrid::acceptOK()
{
    if ( !radiusfld_ )
    {
	uiMSG().message( tr("No GMT installation found") );
	return false;
    }

    if ( radiusfld_->getFValue() <= 0 )
    {
	uiMSG().message( tr("Search radius should be greater than 0") );
	radiusfld_->setValue(mMAX(SI().inlDistance(), SI().crlDistance()) );
	return false;
    }

    Array2DInterpol* intrp = Array2DInterpol::factory().create( sName() );
    mDynamicCastGet( GMTNearNeighborGrid*, res, intrp );
    if ( ! res )
	return false;

    res->setRadius( radiusfld_->getFValue() );
    result_ = res;

    return true;
}
