/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiattrsurfout.cc,v 1.32 2011-04-11 04:52:59 cvsnageswara Exp $";


#include "uiattrsurfout.h"

#include "array2dinterpol.h"
#include "array2dinterpolimpl.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribfactory.h"
#include "attriboutput.h"
#include "bufstring.h"
#include "ctxtioobj.h"
#include "emsurfacetr.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "multiid.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "ptrman.h"
#include "survinfo.h"

#include "uiarray2dinterpol.h"
#include "uiattrsel.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uidlggroup.h"
#include "uimsg.h"

using namespace Attrib;

uiAttrSurfaceOut::uiAttrSurfaceOut( uiParent* p, const DescSet& ad,
				    const NLAModel* n, const MultiID& mid )
    : uiAttrEMOut( p, ad, n, mid, "Create surface output" )
    , ctio_(*mMkCtxtIOObj(EMHorizon3D))
    , interpol_(0)
{
    setHelpID( "104.4.0" );

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

    ctio_.ctxt.forread = true;

    objfld_ = new uiIOObjSel( uppgrp_, ctio_, "Calculate on surface" );
    objfld_->attach( alignedBelow, filludffld_ );
    objfld_->selectionDone.notify( mCB(this,uiAttrSurfaceOut,objSelCB) );

    uppgrp_->setHAlignObj( attrfld_ );
    addStdFields( false, true );
}


uiAttrSurfaceOut::~uiAttrSurfaceOut()
{
    delete ctio_.ioobj;
    delete &ctio_;
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
    iop.get( sKey::Name, methodname_ );

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
	uiMSG().error( "Please select surface" );
	return false;
    }

    return uiAttrEMOut::prepareProcessing();
}


bool uiAttrSurfaceOut::fillPar( IOPar& iopar )
{
    uiAttrEMOut::fillPar( iopar );
    BufferString outid;
    outid += ctio_.ioobj->key();
    if ( settingsbut_->isDisplayed() )
	fillGridPar( iopar );

    fillOutPar( iopar, Output::surfkey(), LocationOutput::surfidkey(), outid );

    BufferString attrnm = attrnmfld_->text();
    if ( attrnm.isEmpty() )
	attrnm = attrfld_->getInput();

    iopar.set( sKey::Target, attrnm );
    return true;
}


void uiAttrSurfaceOut::fillGridPar( IOPar& par ) const
{
    IOPar gridpar, iopar;
    if ( interpol_ )
    {
	gridpar.set( sKey::Name, methodname_ );
	interpol_->fillPar( gridpar );
    }

    iopar.mergeComp( gridpar, "Grid" );
    par.merge( iopar );
}

