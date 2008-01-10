/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2004
 RCS:           $Id: uiattrsurfout.cc,v 1.22 2008-01-10 08:41:18 cvshelene Exp $
________________________________________________________________________

-*/


#include "uiattrsurfout.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "attribfactory.h"
#include "emsurfacetr.h"
#include "uiattrsel.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "uimsg.h"
#include "ptrman.h"
#include "multiid.h"
#include "keystrs.h"


using namespace Attrib;

uiAttrSurfaceOut::uiAttrSurfaceOut( uiParent* p, const DescSet& ad,
				    const NLAModel* n, const MultiID& mid )
    : uiAttrEMOut( p, ad, n, mid, "Create surface output" )
    , ctio_(*mGetCtxtIOObj(EMHorizon3D,Surf))
{
    setHelpID( "104.4.0" );

    attrnmfld_ = new uiGenInput( uppgrp_, "Attribute name", StringInpSpec() );
    attrnmfld_->attach( alignedBelow, attrfld_ );

    ctio_.ctxt.forread = true;
    objfld_ = new uiIOObjSel( uppgrp_, ctio_, "Calculate on surface" );
    objfld_->attach( alignedBelow, attrnmfld_ );
    objfld_->selectiondone.notify( mCB(this,uiAttrSurfaceOut,objSel) );

    uppgrp_->setHAlignObj( attrfld_ );
    addStdFields( false, true );
}


uiAttrSurfaceOut::~uiAttrSurfaceOut()
{
    delete ctio_.ioobj;
    delete &ctio_;
}


void uiAttrSurfaceOut::attribSel( CallBacker* )
{
    attrnmfld_->setText( attrfld_->getInput() );
    objSel(0);
}


void uiAttrSurfaceOut::objSel( CallBacker* )
{
    if ( !objfld_->ctxtIOObj().ioobj ) return;
    BufferString parnm( objfld_->ctxtIOObj().ioobj->name() );
    parnm += " "; parnm += attrnmfld_->text();
    setParFileNmDef( parnm );
}


bool uiAttrSurfaceOut::prepareProcessing()
{
    if ( !objfld_->commitInput(false) )
    {
	uiMSG().error( "Please select surface" );
	return false;
    }

    return uiAttrEMOut::prepareProcessing();
}


bool uiAttrSurfaceOut::fillPar( IOPar& iopar )
{
    uiAttrEMOut::fillPar( iopar );
    fillOutPar( iopar, Output::surfkey, LocationOutput::surfidkey,
	    	ctio_.ioobj->key() );

    BufferString attrnm = attrnmfld_->text();
    if ( attrnm.isEmpty() ) attrnm = attrfld_->getInput();
    iopar.set( "Target value", attrnm );
    return true;
}

