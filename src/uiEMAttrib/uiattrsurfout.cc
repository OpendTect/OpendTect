/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2004
 RCS:           $Id: uiattrsurfout.cc,v 1.5 2005-07-28 10:53:50 cvshelene Exp $
________________________________________________________________________

-*/


#include "uiattrsurfout.h"
#include "attribdesc.h"
#include "attribdescset.h"
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


uiAttrSurfaceOut::uiAttrSurfaceOut( uiParent* p, const Attrib::DescSet& ad,
				    const NLAModel* n, const MultiID& mid )
    : uiFullBatchDialog(p,"Create surface output","process_attrib_em")
    , ctio(*mMkCtxtIOObj(EMHorizon))
    , ads(const_cast<Attrib::DescSet&>(ad))
    , nlamodel(n)
    , nlaid(mid)
{
    setHelpID( "101.2.5" );
    setTitleText( "" );

    attrfld = new uiAttrSel( uppgrp, &ads, "Quantity to output" );
    attrfld->setNLAModel( nlamodel );
    attrfld->selectiondone.notify( mCB(this,uiAttrSurfaceOut,attribSel) );

    attrnmfld = new uiGenInput( uppgrp, "Attribute name", StringInpSpec() );
    attrnmfld->attach( alignedBelow, attrfld );

    ctio.ctxt.forread = true;
    objfld = new uiIOObjSel( uppgrp, ctio, "Calculate on surface" );
    objfld->attach( alignedBelow, attrnmfld );

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
    int nlamodelid = -1;
    if ( nlamodel && attrfld->outputNr() >= 0 )
    {
	if ( !nlaid || !(*nlaid) )
	{ 
	    uiMSG().message( "NN needs to be stored before creating volume" ); 
	    return false; 
	}
	if ( !addNLA( nlamodelid ) )	return false;
    }

    IOPar attrpar( "Attribute Descriptions" );
    ads.fillPar( attrpar );
    for ( int idx=0; idx<attrpar.size(); idx++ )
    {
        const char* nm = attrpar.getKey(idx);
        BufferString name(Attrib::CubeOutput::attribkey);
        name += "."; name += nm;
        iopar.add( name, attrpar.getValue(idx) );
    }

    BufferString key;
    BufferString keybase = Attrib::Output::outputstr; keybase += ".1.";
    key = keybase; key += Attrib::Output::typekey;
    iopar.set( key, Attrib::Output::surfkey );

    key = keybase; key += Attrib::CubeOutput::attribkey;
    key += "."; key += Attrib::DescSet::highestIDStr();
    iopar.set( key, 1 );

    key = keybase; key += Attrib::CubeOutput::attribkey; key += ".0";
    iopar.set( key, nlamodelid < 0 ? attrfld->attribID() : nlamodelid );

    key = keybase; key += Attrib::LocationOutput::surfidkey;
    iopar.set( key, ctio.ioobj->key() );

    BufferString attrnm = attrnmfld->text();
    if ( attrnm == "" ) attrnm = attrfld->getInput();
    iopar.set( "Target value", attrnm );
    ads.removeDesc( ads.getID(nlamodelid) );

//    ads.setRanges( iopar );////////TODO
    return true;
}


#define mHandleParseErr( str ) \
{ \
    uiMSG().message( str );\
    return false;\
}


bool uiAttrSurfaceOut::addNLA( int& id )
{
    BufferString defstr("NN specification=");
    defstr += nlaid;
    BufferString attribname;
    if ( !Attrib::Desc::getAttribName( defstr, attribname ) )
	mHandleParseErr("Cannot find attribute name");
    RefMan<Attrib::Desc> ad;
    ad = Attrib::PF().createDescCopy(attribname);
    if ( !ad )
    {
	BufferString err = "Cannot find factory-entry for "; err += attribname;
	mHandleParseErr(err);
    }
    if ( !ad->parseDefStr(defstr) )
    {
	BufferString err = "Cannot parse: "; err += defstr;
	mHandleParseErr(err);
    }

    ad->setHidden( true );
    const NLADesign& nlades = nlamodel->design();
    ad->setUserRef( *nlades.outputs[attrfld->outputNr()] );
    ad->selectOutput( attrfld->outputNr() );

    const int nrinputs = nlades.inputs.size();
    for ( int idx=0; idx<nrinputs; idx++ )
    {
        const char* inpname = nlades.inputs[idx]->buf();
        int dscnr = ads.getID( inpname, true );
        if ( dscnr < 0 && IOObj::isKey(inpname) )
        {
            dscnr = ads.getID( inpname, false );
            if ( dscnr < 0 )
            {
                // It could be 'storage', but it's not yet in the old set ...
                PtrMan<IOObj> ioobj = IOM().get( MultiID(inpname) );
                if ( ioobj )
                {
		    BufferString defstr("Storage id="); defstr += inpname;
		    BufferString attribname;
		    if ( !Attrib::Desc::getAttribName( defstr, attribname ) )
			mHandleParseErr("Cannot find attribute name");
		    RefMan<Attrib::Desc> newdesc;
		    newdesc = Attrib::PF().createDescCopy(attribname);
		    if ( !newdesc )
		    {
			BufferString err = "Cannot find factory-entry for "; 
			err += attribname;
			mHandleParseErr(err);
		    }
		    if ( !newdesc->parseDefStr(defstr) )
		    {
			BufferString err = "Cannot parse: "; err += defstr;
			mHandleParseErr(err);
		    }
                    newdesc->setUserRef( ioobj->name() );
                    dscnr = ads.addDesc( newdesc );
                }
            }
	}

        ad->setInput( idx, ads.getDesc(dscnr) );
    }

    id = ads.getID( ads.addDesc( ad ) );
    if ( id == -1 )
    {
        uiMSG().error( ads.errMsg() );
        ad->unRef(); return false;
    }
    return true;
}
