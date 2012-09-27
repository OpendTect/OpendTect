/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          March 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uigmtfaults.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
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
#include "ioman.h"
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
    faultfld_ = new uiIOObjSelGrp( this, *mMkCtxtIOObj(EMFault3D),
	    			   "Faults", true, false );

    namefld_ = new uiGenInput( this, "Name", StringInpSpec("Faults") );
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

    linestfld_ = new uiSelLineStyle( this, LineStyle(),
	    			     uiSelLineStyle::Setup("Line Style" )
				     		     .color(false) );
    linestfld_->attach( alignedBelow, horfld_ );

    colorfld_ = new uiColorInput( this, uiColorInput::Setup(Color::Black())
						      .lbltxt("Color") );
    colorfld_->attach( alignedBelow, linestfld_ );

    usecolorbut_ = new uiCheckBox( this, "Use fault color",
	    			   mCB(this,uiGMTFaultsGrp,useColorCB) );
    usecolorbut_->attach( rightOf, colorfld_ );
    postFinalise().notify( mCB(this,uiGMTFaultsGrp,typeChgCB) );
}


void uiGMTFaultsGrp::typeChgCB( CallBacker* )
{
    const bool onzslice = optionfld_->getBoolValue();
    zvaluefld_->display( onzslice );
    horfld_->display( !onzslice )
}


void uiGMTFaultsGrp::useColorCB( CallBacker* )
{
    colorfld_->setSensitive( !usecolorbut_->isChecked() );
}


bool uiGMTFaultsGrp::fillPar( IOPar& iop ) const
{
    if ( !faultfld_->nrSel() )
    {
	uiMSG().message( "Please select atleast one fault" );
	return false;
    }

    for ( int idx=0; idx<faultfld_->nrSel(); idx++ )
    {
	iop.set( iop.compKey(ODGMT::sKeyFaultID(), idx),
		 faultfld_->selected(idx) );
    }

    iop.set( sKey::Name(), namefld_->text() );
    const bool onzslice = optionfld_->getBoolValue();
    iop.setYN( ODGMT::sKeyZIntersectionYN(), onzslice );
    const float zvalue = zvaluefld_->getfValue()/SI().zDomain().userFactor();
    StepInterval<float> zrg = SI().zRange( true );
    if ( onzslice )
    {
	const bool isbetween = zrg.start<=zvalue && zvalue<=zrg.stop;
	if ( !isbetween )
	{
	    BufferString msg( "Z value is out of survey range(" );
	    msg.add( mNINT32(zrg.start*SI().zDomain().userFactor()) ).add( " , " )
	       .add( mNINT32(zrg.stop*SI().zDomain().userFactor()) ).add( ")" );
	    uiMSG().message( msg );
	    return false;
	}

	iop.set( ODGMT::sKeyZVals(), zvalue );
    }
    else
    {
	if ( !horfld_->ioobj() )
	    return false;

	iop.set( ODGMT::sKeyHorizonID(), horfld_->key() );
    }

    BufferString lskey;
    linestfld_->getStyle().toString( lskey );
    iop.set( ODGMT::sKeyLineStyle(), lskey );
    iop.setYN( ODGMT::sKeyUseFaultColorYN(), usecolorbut_->isChecked() );
    if ( !usecolorbut_->isChecked() )
	iop.set( ODGMT::sKeyFaultColor(), colorfld_->color() );

    return true;
}


bool uiGMTFaultsGrp::usePar( const IOPar& iop )
{
    IOPar* fltpar = iop.subselect( ODGMT::sKeyFaultID() );
    if ( !fltpar )
	return false;

    faultfld_->getListField()->clearSelection();
    for ( int idx=0; idx<fltpar->size(); idx++ )
    {
	MultiID mid;
	if (!fltpar->get( toString(idx), mid ) )
	    return false;

	IOObj* obj = IOM().get( mid );
	if ( !obj )
	    return false;

	const int selid = faultfld_->getListField()->indexOf( obj->name() );
	faultfld_->getListField()->setSelected( selid, true );
    }

    BufferString nm;
    iop.get( sKey::Name(), nm );
    namefld_->setText( nm );

    bool onzslice = false;
    iop.getYN( ODGMT::sKeyZIntersectionYN(), onzslice );
    optionfld_->setValue( onzslice );
    if ( onzslice )
    {
	float zvalue;
	iop.get( ODGMT::sKeyZVals(), zvalue );
	zvaluefld_->setValue( zvalue*SI().zDomain().userFactor() );
    }
    else
    {
	MultiID horid;
	iop.get( ODGMT::sKeyHorizonID(), horid );
	horfld_->setInput( horid );
    }

    FixedString lskey = iop.find( ODGMT::sKeyLineStyle() );
    LineStyle ls;
    ls.fromString( lskey.str() );
    linestfld_->setStyle( ls );

    bool usecoloryn = false;
    iop.getYN( ODGMT::sKeyUseFaultColorYN(), usecoloryn );
    usecolorbut_->setChecked( usecoloryn );

    if ( !usecoloryn )
    {
	Color clr;
	iop.get( ODGMT::sKeyFaultColor(), clr );
	colorfld_->setColor( clr );
    }

    typeChgCB( 0 );
    return true;
}


void uiGMTFaultsGrp::reset()
{
    namefld_->setText( "" );
    optionfld_->setValue( true, 0 );
    zvaluefld_->setValue( 0 );
    horfld_->clear();
    typeChgCB( 0 );
}
