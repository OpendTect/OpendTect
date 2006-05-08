/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2004
 RCS:           $Id: uiattrsurfout.cc,v 1.14 2006-05-08 15:39:34 cvsnanne Exp $
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


uiAttrSurfaceOut::uiAttrSurfaceOut( uiParent* p, const Attrib::DescSet& ad,
				    const NLAModel* n, const MultiID& mid )
    : uiFullBatchDialog(p,Setup("Create surface output")
	    		  .procprognm("process_attrib_em"))
    , ctio(*mMkCtxtIOObj(EMHorizon))
    , ads(const_cast<Attrib::DescSet&>(ad))
    , nlamodel(n)
    , nlaid(mid)
{
    setHelpID( "104.4.0" );
    setTitleText( "" );

    attrfld = new uiAttrSel( uppgrp, &ads, "Quantity to output" );
    attrfld->setNLAModel( nlamodel );
    attrfld->selectiondone.notify( mCB(this,uiAttrSurfaceOut,attribSel) );

    attrnmfld = new uiGenInput( uppgrp, "Attribute name", StringInpSpec() );
    attrnmfld->attach( alignedBelow, attrfld );

    ctio.ctxt.forread = true;
    objfld = new uiIOObjSel( uppgrp, ctio, "Calculate on surface" );
    objfld->attach( alignedBelow, attrnmfld );
    objfld->selectiondone.notify( mCB(this,uiAttrSurfaceOut,objSel) );

    uppgrp->setHAlignObj( attrfld );
    addStdFields();
    singmachfld->display( false );
    singmachfld->setValue( true );
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
    Attrib::DescID nladescid( -1, true );
    if ( nlamodel && attrfld->outputNr() >= 0 )
    {
	if ( !nlaid || !(*nlaid) )
	{ 
	    uiMSG().message( "NN needs to be stored before creating volume" ); 
	    return false; 
	}
	if ( !addNLA( nladescid ) )	return false;
    }

    IOPar attrpar( "Attribute Descriptions" );
    ads.fillPar( attrpar );
    for ( int idx=0; idx<attrpar.size(); idx++ )
    {
        const char* nm = attrpar.getKey(idx);
        BufferString name(Attrib::SeisTrcStorOutput::attribkey);
        name += "."; name += nm;
        iopar.add( name, attrpar.getValue(idx) );
    }

    BufferString key;
    BufferString keybase = Attrib::Output::outputstr; keybase += ".1.";
    key = keybase; key += sKey::Type;
    iopar.set( key, Attrib::Output::surfkey );

    key = keybase; key += Attrib::SeisTrcStorOutput::attribkey;
    key += "."; key += Attrib::DescSet::highestIDStr();
    iopar.set( key, 1 );

    key = keybase; key += Attrib::SeisTrcStorOutput::attribkey; key += ".0";
    iopar.set( key, nladescid < 0 ? attrfld->attribID().asInt() 
	    			  : nladescid.asInt() );

    key = keybase; key += Attrib::LocationOutput::surfidkey;
    iopar.set( key, ctio.ioobj->key() );

    BufferString attrnm = attrnmfld->text();
    if ( attrnm == "" ) attrnm = attrfld->getInput();
    iopar.set( "Target value", attrnm );
    ads.removeDesc( nladescid );

//    ads.setRanges( iopar );////////TODO
    return true;
}


#define mErrRet(str) { uiMSG().message( str ); return false; }

bool uiAttrSurfaceOut::addNLA( Attrib::DescID& id )
{
    BufferString defstr("NN specification=");
    defstr += nlaid;

    const int outputnr = attrfld->outputNr();
    BufferString errmsg;
    Attrib::EngineMan::addNLADesc( defstr, id, ads, outputnr, nlamodel, errmsg);
    if ( errmsg.size() )
	mErrRet( errmsg );

    return true;
}
