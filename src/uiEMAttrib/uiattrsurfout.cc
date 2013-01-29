/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2004
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiattrsurfout.h"

#include "array2dinterpolimpl.h"
#include "attriboutput.h"
#include "ctxtioobj.h"
#include "emsurfacetr.h"
#include "emsurfaceauxdata.h"
#include "emsurfauxdataio.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"

#include "uiarray2dinterpol.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uidlggroup.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"

using namespace Attrib;

uiAttrSurfaceOut::uiAttrSurfaceOut( uiParent* p, const DescSet& ad,
				    const NLAModel* n, const MultiID& mid )
    : uiAttrEMOut( p, ad, n, mid, "Calculate Horizon Data" )
    , interpol_(0)
{
    setHelpID( "104.4.0" );
    setCtrlStyle( DoAndStay );

    attrnmfld_ = new uiGenInput( uppgrp_, "Attribute name", StringInpSpec() );
    attrnmfld_->attach( alignedBelow, attrfld_ );

    filludffld_ = new uiGenInput( uppgrp_, "Fill undefined parts",
	    			  BoolInpSpec(false) );
    filludffld_->valuechanged.notify( mCB(this,uiAttrSurfaceOut,fillUdfSelCB) );
    filludffld_->attach( alignedBelow, attrnmfld_ );

    settingsbut_ = new uiPushButton( uppgrp_, "Settings",
	    			 mCB(this,uiAttrSurfaceOut,settingsCB), false);
    settingsbut_->display( false );
    settingsbut_->attach( rightOf, filludffld_ );

    objfld_ = new uiIOObjSel( uppgrp_, mIOObjContext(EMHorizon3D),
			      "Calculate on Horizon" );
    objfld_->attach( alignedBelow, filludffld_ );
    objfld_->selectionDone.notify( mCB(this,uiAttrSurfaceOut,objSelCB) );

    uppgrp_->setHAlignObj( attrfld_ );
    addStdFields( false, true );
}


uiAttrSurfaceOut::~uiAttrSurfaceOut()
{
}


void uiAttrSurfaceOut::fillUdfSelCB( CallBacker* )
{
    const bool isdisplay = filludffld_->getBoolValue();
    settingsbut_->display( isdisplay );
    if ( settingsbut_->isDisplayed() )
    {
	InverseDistanceArray2DInterpol* tempinterpol =
	    				new InverseDistanceArray2DInterpol;
	const float defradius = 10*(SI().inlDistance()+SI().crlDistance());
	tempinterpol->setSearchRadius( defradius );
	tempinterpol->setFillType( Array2DInterpol::ConvexHull );
	tempinterpol->setStepSize( 1 );
	tempinterpol->setMaxHoleSize( mUdf(float) );
	interpol_ = tempinterpol;
    }
}


void uiAttrSurfaceOut::settingsCB( CallBacker* )
{
    uiSingleGroupDlg dlg( this, uiDialog::Setup("Interpolation",
					     "Interpolation Settings","") );
    uiArray2DInterpolSel* interpolsel =
		new uiArray2DInterpolSel( &dlg, true, true, false, interpol_ );
    dlg.setGroup( interpolsel );
    if ( !dlg.go() )
	return;

    IOPar iop;
    interpolsel->fillPar( iop );
    iop.get( sKey::Name(), methodname_ );

    if ( interpol_ ) delete interpol_;

    interpol_ = interpolsel->getResult();
}


void uiAttrSurfaceOut::attribSel( CallBacker* )
{
    attrnmfld_->setText( attrfld_->getInput() );
    objSelCB(0);
}


void uiAttrSurfaceOut::objSelCB( CallBacker* )
{
    if ( !objfld_->ioobj(true) ) return;

    BufferString parnm( objfld_->ioobj(true)->name() );
    parnm += " "; parnm += attrnmfld_->text();
    setParFileNmDef( parnm );
}


bool uiAttrSurfaceOut::prepareProcessing()
{
    if ( !objfld_->commitInput() )
    {
	uiMSG().error( "Please select Horizon" );
	return false;
    }

    return uiAttrEMOut::prepareProcessing();
}


bool uiAttrSurfaceOut::fillPar( IOPar& iopar )
{
    uiAttrEMOut::fillPar( iopar );
    const IOObj* ioobj = objfld_->ioobj();
    if ( !ioobj ) return false;

    if ( settingsbut_->isDisplayed() )
	fillGridPar( iopar );

    fillOutPar( iopar, Output::surfkey(),
		LocationOutput::surfidkey(), ioobj->key() );

    BufferString attrnm = attrnmfld_->text();
    if ( attrnm.isEmpty() )
	attrnm = attrfld_->getInput();

    BufferString attrfnm =
	EM::SurfaceAuxData::getFileName( *ioobj, attrnm );
    if ( !attrfnm.isEmpty() )
    {
	const int val = uiMSG().askOverwrite("Horizon data with this attribute"
		" name already exists. Do you want to overwrite?");
	if ( val==0 )
	    return false;
    }
    else
    {
	attrfnm = EM::SurfaceAuxData::getFreeFileName( *ioobj );
	const bool res =
	    EM::dgbSurfDataWriter::writeDummyHeader( attrfnm, attrnm );
	if ( !res )
	{
	    uiMSG().error( "Cannot save Horizon data to: ", attrfnm );
	    return false;
	}
    }

    iopar.set( sKey::Target(), attrnm );
    return true;
}


void uiAttrSurfaceOut::fillGridPar( IOPar& par ) const
{
    IOPar gridpar, iopar;
    if ( interpol_ )
    {
	gridpar.set( sKey::Name(), methodname_ );
	interpol_->fillPar( gridpar );
    }

    iopar.mergeComp( gridpar, "Grid" );
    par.merge( iopar );
}

