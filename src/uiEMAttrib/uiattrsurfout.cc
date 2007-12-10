/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2004
 RCS:           $Id: uiattrsurfout.cc,v 1.21 2007-12-10 12:59:52 cvsbert Exp $
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
    : uiFullBatchDialog(p,Setup("Create surface output")
	    		  .procprognm("process_attrib_em"))
    , ctio(*mGetCtxtIOObj(EMHorizon3D,Surf))
    , ads(const_cast<DescSet&>(ad))
    , nlamodel(n)
    , nlaid(mid)
{
    setHelpID( "104.4.0" );
    setTitleText( "" );

    attrfld = new uiAttrSel( uppgrp_, &ads, ads.is2D(), "Quantity to output" );
    attrfld->setNLAModel( nlamodel );
    attrfld->selectiondone.notify( mCB(this,uiAttrSurfaceOut,attribSel) );

    attrnmfld = new uiGenInput( uppgrp_, "Attribute name", StringInpSpec() );
    attrnmfld->attach( alignedBelow, attrfld );

    ctio.ctxt.forread = true;
    objfld = new uiIOObjSel( uppgrp_, ctio, "Calculate on surface" );
    objfld->attach( alignedBelow, attrnmfld );
    objfld->selectiondone.notify( mCB(this,uiAttrSurfaceOut,objSel) );

    uppgrp_->setHAlignObj( attrfld );
    addStdFields( false, true );
}


uiAttrSurfaceOut::~uiAttrSurfaceOut()
{
    delete ctio.ioobj;
    delete &ctio;
}


void uiAttrSurfaceOut::attribSel( CallBacker* )
{
    attrnmfld->setText( attrfld->getInput() );
    objSel(0);
}


void uiAttrSurfaceOut::objSel( CallBacker* )
{
    if ( !objfld->ctxtIOObj().ioobj ) return;
    BufferString parnm( objfld->ctxtIOObj().ioobj->name() );
    parnm += " "; parnm += attrnmfld->text();
    setParFileNmDef( parnm );
}


bool uiAttrSurfaceOut::prepareProcessing()
{
    if ( !objfld->commitInput(false) )
    {
	uiMSG().error( "Please select surface" );
	return false;
    }

    attrfld->processInput();
    if ( attrfld->attribID() < 0 && attrfld->outputNr() < 0 )
    {
	uiMSG().error( "Please select the output quantity" );
	return false;
    }

    return true;
}


bool uiAttrSurfaceOut::fillPar( IOPar& iopar )
{
    DescID nladescid( -1, true );
    if ( nlamodel && attrfld->outputNr() >= 0 )
    {
	if ( !nlaid || !(*nlaid) )
	{ 
	    uiMSG().message( "NN needs to be stored before creating volume" ); 
	    return false; 
	}
	if ( !addNLA( nladescid ) )	return false;
    }

    const DescID targetid = nladescid < 0 ? attrfld->attribID() : nladescid;
    DescSet* clonedset = ads.optimizeClone( targetid );
    if ( !clonedset )
	return false;

    IOPar attrpar( "Attribute Descriptions" );
    clonedset->fillPar( attrpar );
	    
    for ( int idx=0; idx<attrpar.size(); idx++ )
    {
        const char* nm = attrpar.getKey( idx );
        iopar.add( IOPar::compKey(SeisTrcStorOutput::attribkey,nm),
		   attrpar.getValue(idx) );
    }

    BufferString key;
    BufferString keybase = Output::outputstr; keybase += ".1.";
    key = keybase; key += sKey::Type;
    iopar.set( key, Output::surfkey );

    key = keybase; key += SeisTrcStorOutput::attribkey;
    key += "."; key += DescSet::highestIDStr();
    iopar.set( key, 1 );

    key = keybase; key += SeisTrcStorOutput::attribkey; key += ".0";
    iopar.set( key, nladescid < 0 ? attrfld->attribID().asInt() 
	    			  : nladescid.asInt() );

    key = keybase; key += LocationOutput::surfidkey;
    iopar.set( key, ctio.ioobj->key() );

    BufferString attrnm = attrnmfld->text();
    if ( attrnm.isEmpty() ) attrnm = attrfld->getInput();
    iopar.set( "Target value", attrnm );
    ads.removeDesc( nladescid );

//    ads.setRanges( iopar );////////TODO
    delete clonedset;
    return true;
}


#define mErrRet(str) { uiMSG().message( str ); return false; }

bool uiAttrSurfaceOut::addNLA( DescID& id )
{
    BufferString defstr("NN specification=");
    defstr += nlaid;

    const int outputnr = attrfld->outputNr();
    BufferString errmsg;
    EngineMan::addNLADesc( defstr, id, ads, outputnr, nlamodel, errmsg );
    if ( errmsg.size() )
	mErrRet( errmsg );

    return true;
}
