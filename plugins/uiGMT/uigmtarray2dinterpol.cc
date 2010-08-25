/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uigmtarray2dinterpol.cc,v 1.2 2010-08-25 07:11:11 cvsnageswara Exp $";

#include "uigmtarray2dinterpol.h"

#include "uiarray2dinterpol.h"
#include "uigeninput.h"
#include "uimsg.h"

#include "commondefs.h"
#include "gmtarray2dinterpol.h"
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


uiGMTSurfaceGrid::uiGMTSurfaceGrid( uiParent* p )
    : uiArray2DInterpol( p, "GMT grid" )
{
    tensionfld_ = new uiGenInput( this, "Tension", FloatInpSpec(0.25) );
    setHAlignObj( tensionfld_ );
}


void uiGMTSurfaceGrid::fillPar( IOPar& iop ) const
{
    iop.set( "Tension", tensionfld_->getfValue() );
}


bool uiGMTSurfaceGrid::acceptOK()
{
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
{
    BufferString lbl( "Search radius " );
    lbl.add( SI().getXYUnitString() );
    rediusfld_ = new uiGenInput( this, lbl, FloatInpSpec(1) );
    rediusfld_->setValue((int)mMAX(SI().inlDistance(), SI().crlDistance()) );
    setHAlignObj( rediusfld_ );
}


void uiGMTNearNeighborGrid::fillPar( IOPar& iop ) const
{
    iop.set( "Radius", rediusfld_->getfValue() );
}


bool uiGMTNearNeighborGrid::acceptOK()
{
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
