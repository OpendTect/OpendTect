/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2004
 RCS:           $Id: uiattrsurfout.cc,v 1.1 2004-10-04 16:09:54 nanne Exp $
________________________________________________________________________

-*/


#include "uiattrsurfout.h"
#include "attribdescimpl.h"
#include "attribdescset.h"
#include "attribparsetools.h"
#include "attriboutputimpl.h"
#include "emsurfacetr.h"
#include "uiattrsel.h"
#include "uiioobjsel.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "uimsg.h"
#include "ptrman.h"
#include "multiid.h"


uiAttrSurfaceOut::uiAttrSurfaceOut( uiParent* p, const AttribDescSet& ad,
				    const NLAModel* n, const MultiID& mid )
    : uiFullBatchDialog(p,"Create surface output","process_attrib_em")
    , ctio(*mMkCtxtIOObj(EMHorizon))
    , ads(const_cast<AttribDescSet&>(ad))
    , nlamodel(n)
    , nlaid(mid)
{
    setHelpID( "101.2.0" );
    setTitleText( "Create surface output" );

    attrfld = new uiAttrSel( uppgrp, &ads, "Quantity to output" );
    attrfld->setNLAModel( nlamodel );

    ctio.ctxt.forread = true;
    objfld = new uiIOObjSel( uppgrp, ctio, "Calculate on surface" );
    objfld->attach( alignedBelow, attrfld );

    uppgrp->setHAlignObj( attrfld );
    addStdFields();
}


uiAttrSurfaceOut::~uiAttrSurfaceOut()
{
    delete ctio.ioobj;
    delete &ctio;
}


bool uiAttrSurfaceOut::prepareProcessing()
{
    if ( !objfld->commitInput(false) )
    {
	uiMSG().error( "Please select surface" );
	return false;
    }

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
	addNLA( nlamodelid );
    }

    IOPar attrpar( "Attribute Descriptions" );
    ads.fillPar( attrpar );
    for ( int idx=0; idx<attrpar.size(); idx++ )
    {
        const char* nm = attrpar.getKey(idx);
        BufferString name(CubeAttribOutput::attribkey);
        name += "."; name += nm;
        iopar.add( name, attrpar.getValue(idx) );
    }

    BufferString key;
    BufferString keybase = AttribParseTools::outputstr; keybase += ".1.";
    key = keybase; key += AttribOutput::typekey;
    iopar.set( key, "Surface" );

    key = keybase; key += CubeAttribOutput::attribkey;
    key += "."; key += AttribParseTools::maxkeystr;
    iopar.set( key, 1 );

    key = keybase; key += CubeAttribOutput::attribkey; key += ".0";
    iopar.set( key, nlamodelid < 0 ? attrfld->attribID() : nlamodelid );

    key = keybase; key += "Surface ID";
    iopar.set( key, ctio.ioobj->key() );

    iopar.set( "Target value", attrfld->getInput() );
    ads.removeAttrib( ads.descNr(nlamodelid) );

    ads.setRanges( iopar );
    return true;
}


void uiAttrSurfaceOut::addNLA( int& id )
{
    AttribDesc* ad = new CalcAttribDesc( ads );
    BufferString defstr("NN specification=");
    defstr += nlaid;
    ad->setDefStr( defstr, false );
    ad->setHidden( true );
    const NLADesign& nlades = nlamodel->design();
    ad->setUserRef( *nlades.outputs[attrfld->outputNr()] );
    ad->selectAttrib( attrfld->outputNr() );

    const int nrinputs = nlades.inputs.size();
    for ( int idx=0; idx<nrinputs; idx++ )
    {
        const char* inpname = nlades.inputs[idx]->buf();
        int dscnr = ads.descNr( inpname, true );
        if ( dscnr < 0 && IOObj::isKey(inpname) )
        {
            dscnr = ads.descNr( inpname, false );
            if ( dscnr < 0 )
            {
                // It could be 'storage', but it's not yet in the set ...
                PtrMan<IOObj> ioobj = IOM().get( MultiID(inpname) );
                if ( ioobj )
                {
                    AttribDesc* newdesc = new StorageAttribDesc( ads );
                    newdesc->setDefStr( inpname, false );
                    newdesc->setUserRef( ioobj->name() );
                    dscnr = ads.addAttrib( newdesc );
                }
            }
	}

        ad->setInput( idx, ads.id(dscnr) );
    }

    id = ads.id( ads.addAttrib( ad ) );
    if ( id == -1 )
    {
        uiMSG().error( ads.errMsg() );
        delete ad; return;
    }
}
