/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          March 2010
 RCS:           $Id: uigmtfaults.cc,v 1.1 2010-03-26 11:59:57 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uigmtfaults.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uisellinest.h"
#include "uitaskrunner.h"

#include "bufstring.h"
#include "ctxtioobj.h"
#include "draw.h"
#include "emsurfacetr.h"
#include "iopar.h"
#include "survinfo.h"
#include "emmanager.h"
#include "gmtpar.h"


int uiGMTFaultsGrp::factoryid_ = -1;

void uiGMTFaultsGrp::initClass()
{
    if ( factoryid_ < 0 )
	factoryid_ = uiGMTOF().add( "Faults",
				    uiGMTFaultsGrp::createInstance );
}


uiGMTOverlayGrp* uiGMTFaultsGrp::createInstance( uiParent* p )
{
    return new uiGMTFaultsGrp( p );
}


uiGMTFaultsGrp::uiGMTFaultsGrp( uiParent* p )
	      : uiGMTOverlayGrp(p,"Faults")
	      , faultctio_(*mMkCtxtIOObj(EMFault3D))
	      , surfacectio_(*mMkCtxtIOObj(EMHorizon3D))
{
    faultfld_ = new uiIOObjSel( this, faultctio_, "Select Fault" );
    faultfld_->selectionDone.notify( mCB(this,uiGMTFaultsGrp,loadFault) );

    namefld_ = new uiGenInput( this, "Name" );
    namefld_->attach( alignedBelow, faultfld_ );

    optionfld_ = new uiGenInput( this, "Intersection with ",
	    			 BoolInpSpec("true", "Z Slice", "Horizon") );
    optionfld_->valuechanged.notify( mCB(this,uiGMTFaultsGrp,typeChgCB) );
    optionfld_->attach( alignedBelow, namefld_ );

    BufferString lbl = "Z Value ";
    lbl.add( SI().getZUnitString() );
    zvaluefld_ = new uiGenInput( this, lbl, IntInpSpec(0) );
    zvaluefld_->attach( alignedBelow, optionfld_ );

    surfacefld_ = new uiIOObjSel( this, surfacectio_, "Select Horizon" );
    surfacefld_->attach( alignedBelow, optionfld_ );
    surfacefld_->selectionDone.notify( mCB(this,uiGMTFaultsGrp,loadSurface) );


    linestfld_ = new uiSelLineStyle( this, LineStyle(), "Line Style" );
    linestfld_->attach( alignedBelow, surfacefld_ );

    finaliseDone.notify( mCB(this,uiGMTFaultsGrp,typeChgCB) );
}


void uiGMTFaultsGrp::typeChgCB( CallBacker* )
{
    const bool iszslice = optionfld_->getBoolValue();
    zvaluefld_->display( iszslice );
    surfacefld_->display( !iszslice );
}


bool uiGMTFaultsGrp::loadFault( CallBacker* )
{
    if ( !faultfld_->commitInput() )
	return false;

    IOObj* faultobj = faultctio_.ioobj;
    if ( !faultobj )
	return false;

    uiTaskRunner dlg( this );
    EM::EMObject* obj = EM::EMM().loadIfNotFullyLoaded( faultobj->key(),
	    						&dlg );
    namefld_->setText( faultctio_.ioobj->name() );

    return true;
}


bool uiGMTFaultsGrp::loadSurface( CallBacker* )
{
    if ( !surfacefld_->commitInput() )
	return false;

    IOObj* surfaceobj = surfacectio_.ioobj;
    if ( !surfaceobj )
	return false;

    uiTaskRunner dlg( this );
    EM::EMObject* obj = EM::EMM().loadIfNotFullyLoaded( surfaceobj->key(),
							&dlg );
    return true;
}


bool uiGMTFaultsGrp::fillPar( IOPar& iop ) const
{
    if ( !faultfld_->commitInput() || !faultctio_.ioobj )
    {
	uiMSG().message( "Please select a fault" );
	return false;
    }

    faultfld_->fillPar( iop );
    iop.set( sKey::Name, namefld_->text() );
    const bool iszslice = optionfld_->getBoolValue();
    iop.setYN( ODGMT::sKeyIntersection, iszslice );
    const int zvalue = mNINT( zvaluefld_->getfValue() );
    if ( iszslice )
	iop.set( ODGMT::sKeyZVals, zvalue );

    if ( iszslice  && zvalue<=0 )
    {
	uiMSG().message( "Z value should be greater than zero" );
	return false;
    }

    if ( (!surfacefld_->commitInput() || !surfacectio_.ioobj) && !iszslice )
    {
	uiMSG().message( "Please select a horizon" );
	return false;
    }

    if ( !iszslice )
    	surfacefld_->fillPar( iop );

    BufferString lskey;
    linestfld_->getStyle().toString( lskey );
    iop.set( ODGMT::sKeyLineStyle, lskey );

    return true;
}


bool uiGMTFaultsGrp::usePar( const IOPar& iop )
{
    faultfld_->usePar( iop );
    surfacefld_->usePar( iop );
    return true;
}


void uiGMTFaultsGrp::reset()
{
    faultfld_->clear();
    namefld_->setText( "" );
    optionfld_->setValue( true, 0 );
    zvaluefld_->setValue( 0 );
    surfacefld_->clear();
    typeChgCB( 0 );
}
