/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          March 2010
________________________________________________________________________

-*/

#include "uigmtfaults.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiioobjselgrp.h"
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
    : uiGMTOverlayGrp(p, uiStrings::sFault() )
{
    faultfld_ = new uiIOObjSelGrp( this, mIOObjContext(EMFault3D),
		   uiStrings::sFault(mPlural),
                   uiIOObjSelGrp::Setup(OD::ChooseAtLeastOne) );

    namefld_ = new uiGenInput( this, uiStrings::sName(),
                               StringInpSpec("Faults") );
    namefld_->setElemSzPol( uiObject::Wide );
    namefld_->attach( alignedBelow, faultfld_ );

    optionfld_ = new uiGenInput( this, tr("Intersection with "),
				 BoolInpSpec(true, tr("Z Slice"),
				 uiStrings::sHorizon()) );
    optionfld_->valuechanged.notify( mCB(this,uiGMTFaultsGrp,typeChgCB) );
    optionfld_->attach( alignedBelow, namefld_ );

    const uiString lbl = tr("Z Value %1").arg( SI().getZUnitString() );
    zvaluefld_ = new uiGenInput( this, lbl, IntInpSpec(0) );
    zvaluefld_->attach( alignedBelow, optionfld_ );

    horfld_ = new uiIOObjSel( this, mIOObjContext(EMHorizon3D),
			      uiStrings::sHorizon() );
    horfld_->attach( alignedBelow, optionfld_ );

    linestfld_ = new uiSelLineStyle( this, OD::LineStyle(),
				     uiSelLineStyle::Setup(tr("Line Style") )
						     .color(false) );
    linestfld_->attach( alignedBelow, horfld_ );

    colorfld_ = new uiColorInput( this, uiColorInput::Setup(Color::Black())
				.lbltxt(uiStrings::sColor()) );
    colorfld_->attach( alignedBelow, linestfld_ );

    usecolorbut_ = new uiCheckBox( this, tr("Use fault color"),
				   mCB(this,uiGMTFaultsGrp,useColorCB) );
    usecolorbut_->attach( rightOf, colorfld_ );
    postFinalise().notify( mCB(this,uiGMTFaultsGrp,typeChgCB) );
}


void uiGMTFaultsGrp::typeChgCB( CallBacker* )
{
    const bool onzslice = optionfld_->getBoolValue();
    zvaluefld_->display( onzslice );
    horfld_->display( !onzslice );
}


void uiGMTFaultsGrp::useColorCB( CallBacker* )
{
    colorfld_->setSensitive( !usecolorbut_->isChecked() );
}


bool uiGMTFaultsGrp::fillPar( IOPar& iop ) const
{
    const int nrsel = faultfld_->nrChosen();
    if ( nrsel < 1 )
	{ uiMSG().message( tr("No faults available") ); return false; }
    for ( int idx=0; idx<nrsel; idx++ )
    {
	iop.set( iop.compKey(ODGMT::sKeyFaultID(), idx),
		 faultfld_->chosenID(idx) );
    }

    iop.set( sKey::Name(), namefld_->text() );
    const bool onzslice = optionfld_->getBoolValue();
    iop.setYN( ODGMT::sKeyZIntersectionYN(), onzslice );
    const float zvalue = zvaluefld_->getFValue()/SI().zDomain().userFactor();
    StepInterval<float> zrg = SI().zRange( true );
    if ( onzslice )
    {
	const bool isbetween = zrg.start<=zvalue && zvalue<=zrg.stop;
	if ( !isbetween )
	{
	    uiString msg = tr("Z value is out of survey range(%1, %2)")
	                 .arg( mNINT32(zrg.start*SI().zDomain().userFactor()) )
	                 .arg( mNINT32(zrg.stop*SI().zDomain().userFactor()) );
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

    faultfld_->chooseAll( false );
    TypeSet<MultiID> tosel;
    for ( int idx=0; idx<fltpar->size(); idx++ )
    {
	MultiID mid;
	if (!fltpar->get( toString(idx), mid ) )
	    break;
	IOObj* obj = IOM().get( mid );
	if ( obj )
	    tosel += mid;
	delete obj;
    }
    faultfld_->setChosen( tosel );

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
    OD::LineStyle ls;
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
