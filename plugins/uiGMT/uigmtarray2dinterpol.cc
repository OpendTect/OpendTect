/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uigmtarray2dinterpol.cc,v 1.1 2010-08-13 11:03:33 cvsnageswara Exp $";

#include "uigmtarray2dinterpol.h"

#include "uiarray2dinterpol.h"
#include "uigeninput.h"
#include "uimsg.h"

#include "gmtarray2dinterpol.h"
#include "iopar.h"


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
    : uiGMTArray2DInterpol( p )
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
    GMTArray2DInterpol* res = new GMTArray2DInterpol;
    res->setPar( iop );
    result_ = res;

    return true;
}
