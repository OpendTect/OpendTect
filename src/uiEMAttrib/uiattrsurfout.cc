/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2004
 RCS:           $Id: uiattrsurfout.cc,v 1.6 2005-07-29 13:08:11 cvsnanne Exp $
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
    BufferString attribname;
    if ( !Attrib::Desc::getAttribName(defstr,attribname) )
	mErrRet("Cannot find attribute name");
    RefMan<Attrib::Desc> ad = Attrib::PF().createDescCopy( attribname );
    if ( !ad )
    {
	BufferString err = "Cannot find factory-entry for "; err += attribname;
	mErrRet(err);
    }

    if ( !ad->parseDefStr(defstr) )
    {
	BufferString err = "Cannot parse: "; err += defstr;
	mErrRet(err);
    }

    ad->setHidden( true );
    const NLADesign& nlades = nlamodel->design();
    ad->setUserRef( *nlades.outputs[attrfld->outputNr()] );
    ad->selectOutput( attrfld->outputNr() );

    const int nrinputs = nlades.inputs.size();
    for ( int idx=0; idx<nrinputs; idx++ )
    {
        const char* inpname = nlades.inputs[idx]->buf();
	Attrib::DescID descid = ads.getID( inpname, true );
        if ( descid < 0 && IOObj::isKey(inpname) )
        {
            descid = ads.getID( inpname, false );
            if ( descid < 0 )
            {
                // It could be 'storage', but it's not yet in the old set ...
                PtrMan<IOObj> ioobj = IOM().get( MultiID(inpname) );
                if ( ioobj )
                {
		    BufferString defstr("Storage id="); defstr += inpname;
		    BufferString attribname;
		    if ( !Attrib::Desc::getAttribName( defstr, attribname ) )
			mErrRet("Cannot find attribute name")
		    RefMan<Attrib::Desc> newdesc = 
					Attrib::PF().createDescCopy(attribname);
		    if ( !newdesc )
		    {
			BufferString err = "Cannot find factory-entry for "; 
			err += attribname;
			mErrRet(err);
		    }

		    if ( !newdesc->parseDefStr(defstr) )
		    {
			BufferString err = "Cannot parse: "; err += defstr;
			mErrRet(err);
		    }
                    newdesc->setUserRef( ioobj->name() );
                    descid = ads.addDesc( newdesc );
                }
            }
	}

        ad->setInput( idx, ads.getDesc(descid) );
    }

    id = ads.addDesc( ad );
    if ( id < 0 )
    {
        uiMSG().error( ads.errMsg() );
        return false;
    }

    return true;
}
