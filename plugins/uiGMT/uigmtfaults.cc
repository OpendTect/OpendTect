/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          March 2010
 RCS:           $Id: uigmtfaults.cc,v 1.5 2011-04-21 13:09:13 cvsbert Exp $
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
#include "emmanager.h"
#include "emsurfacetr.h"
#include "fixedstring.h"
#include "gmtpar.h"
#include "iopar.h"
#include "multiid.h"
#include "survinfo.h"

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
	      : uiGMTOverlayGrp(p,"Fault")
{
    faultfld_ = new uiIOObjSel( this, mIOObjContext(EMFault3D), "Fault" );
    faultfld_->selectionDone.notify( mCB(this,uiGMTFaultsGrp,faultSel) );

    namefld_ = new uiGenInput( this, "Name" );
    namefld_->attach( alignedBelow, faultfld_ );

    optionfld_ = new uiGenInput( this, "Intersection with ",
	    			 BoolInpSpec("true", "Z Slice", "Horizon") );
    optionfld_->valuechanged.notify( mCB(this,uiGMTFaultsGrp,typeChgCB) );
    optionfld_->attach( alignedBelow, namefld_ );

    BufferString lbl( "Z Value ", SI().getZUnitString() );
    zvaluefld_ = new uiGenInput( this, lbl, IntInpSpec(0) );
    zvaluefld_->attach( alignedBelow, optionfld_ );

    horfld_ = new uiIOObjSel( this, mIOObjContext(EMHorizon3D), "Horizon" );
    horfld_->attach( alignedBelow, optionfld_ );

    linestfld_ = new uiSelLineStyle( this, LineStyle(), "Line Style" );
    linestfld_->attach( alignedBelow, horfld_ );

    finaliseDone.notify( mCB(this,uiGMTFaultsGrp,typeChgCB) );
}


void uiGMTFaultsGrp::typeChgCB( CallBacker* )
{
    const bool onzslice = optionfld_->getBoolValue();
    zvaluefld_->display( onzslice );
    horfld_->display( !onzslice );
}


void uiGMTFaultsGrp::faultSel( CallBacker* )
{
    if ( faultfld_->ioobj(true) )
	namefld_->setText( faultfld_->ioobj()->name() );
}


bool uiGMTFaultsGrp::fillPar( IOPar& iop ) const
{
    if ( !faultfld_->ioobj(true) )
	return false;

    iop.set( ODGMT::sKeyFaultID, faultfld_->key() );
    iop.set( sKey::Name, namefld_->text() );
    const bool onzslice = optionfld_->getBoolValue();
    iop.setYN( ODGMT::sKeyZIntersectionYN, onzslice );
    const float zvalue = zvaluefld_->getfValue()/SI().zFactor();
    StepInterval<float> zrg = SI().zRange( true );
    const bool isbetween = zrg.start<=zvalue && zvalue<=zrg.stop;
    if ( !isbetween )
    {
	BufferString msg( "Z value is out of survey range(" );
	msg.add( mNINT(zrg.start*SI().zFactor()) ).add( " , " )
	   .add( mNINT(zrg.stop*SI().zFactor()) ).add( ")" );
	uiMSG().message( msg );
	return false;
    }

    if ( onzslice )
	iop.set( ODGMT::sKeyZVals, zvalue );
    else
    {
	if ( !horfld_->ioobj() )
	    return false;

	iop.set( ODGMT::sKeyHorizonID, horfld_->key() );
    }

    BufferString lskey;
    linestfld_->getStyle().toString( lskey );
    iop.set( ODGMT::sKeyLineStyle, lskey );

    return true;
}


bool uiGMTFaultsGrp::usePar( const IOPar& iop )
{
    FixedString fault = iop.find( ODGMT::sKeyFaultID );
    MultiID faultid( fault.str() );
    faultfld_->setInput( faultid );

    BufferString nm;
    iop.get( sKey::Name, nm );
    namefld_->setText( nm );

    bool onzslice = false;
    iop.getYN( ODGMT::sKeyZIntersectionYN, onzslice );
    optionfld_->setValue( onzslice );
    if ( !onzslice )
    {
	MultiID horid;
	iop.get( ODGMT::sKeyHorizonID, horid );
	horfld_->setInput( horid );
    }

    FixedString lskey = iop.find( ODGMT::sKeyLineStyle );
    LineStyle ls;
    ls.fromString( lskey.str() );
    linestfld_->setStyle( ls );

    typeChgCB( 0 );
    return true;
}


void uiGMTFaultsGrp::reset()
{
    faultfld_->clear();
    namefld_->setText( "" );
    optionfld_->setValue( true, 0 );
    zvaluefld_->setValue( 0 );
    horfld_->clear();
    typeChgCB( 0 );
}
