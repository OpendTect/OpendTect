/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:		$Id: uiattrvolout.cc,v 1.4 2005-08-15 16:17:29 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattrvolout.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "uiattrsel.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseisfmtscale.h"
#include "uiseistransf.h"
#include "uiseisioobjinfo.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "seistrctr.h"
#include "seistrcsel.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "attribsel.h"
#include "errh.h"
#include "nlamodel.h"
#include "nladesign.h"
#include "survinfo.h"
#include "ptrman.h"
#include "binidselimpl.h"
#include "scaler.h"
#include "cubesampling.h"

using namespace Attrib;

static void setTypeAttr( CtxtIOObj& ctio, bool yn )
{
    if ( yn )
	ctio.ctxt.parconstraints.set( "Type", "Attribute" );
    else
	ctio.ctxt.parconstraints.removeWithKey( "Type" );
}


uiAttrVolOut::uiAttrVolOut( uiParent* p, const DescSet& ad,
				const NLAModel* n, MultiID id )
	: uiFullBatchDialog(p,"Process")
	, ctio(mkCtxtIOObj())
    	, subselpar(*new IOPar)
    	, sel(*new CurrentSel)
	, ads(const_cast<DescSet&>(ad))
	, nlamodel(n)
	, nlaid(id)
{
    setHelpID( "101.2.0" );
    setTitleText( "Create seismic output" );

    uiAttrSelData attrdata( &ad );
    attrdata.nlamodel = nlamodel;
    todofld = new uiAttrSel( uppgrp, "Quantity to output", attrdata );
    todofld->selectiondone.notify( mCB(this,uiAttrVolOut,attrSel) );

    transffld = new uiSeisTransfer( uppgrp, uiSeisTransfer::Setup()
	    	.fornewentry(false).withstep(false).multi2dlines(true) );
    transffld->attach( alignedBelow, todofld );
    transffld->selfld->notifySing2DLineSel(
	    		mCB(this,uiAttrVolOut,singLineSel) );

    ctio.ctxt.forread = false;
    setTypeAttr( ctio, true );
    ctio.ctxt.includeconstraints = true;
    ctio.ctxt.allowcnstrsabsent = false;
    objfld = new uiSeisSel( uppgrp, ctio, SeisSelSetup().selattr(false) );
    objfld->attach( alignedBelow, transffld );

    uppgrp->setHAlignObj( transffld );

    addStdFields();
}


uiAttrVolOut::~uiAttrVolOut()
{
    delete ctio.ioobj;
    delete &ctio;
    delete &sel;
    delete &subselpar;
}


CtxtIOObj& uiAttrVolOut::mkCtxtIOObj()
{
    return *mMkCtxtIOObj(SeisTrc);
}


void uiAttrVolOut::singLineSel( CallBacker* )
{
    const bool is2d = todofld->is2D();
    if ( !is2d ) return;
    singmachfld->setValue( transffld->selfld->isSing2DLine() );
    singTogg( 0 );
    singmachfld->display( !is2d );
}


void uiAttrVolOut::attrSel( CallBacker* )
{
/*
    CubeSampling cs;
    const bool is2d = todofld->is2D();
    transffld->set2D( is2d );
    if ( todofld->getRanges(cs) )
	transffld->selfld->setInput( cs );
    Pol2D p2d = is2d ? Only2D : No2D;
    objfld->set2DPol( p2d );
    setTypeAttr( ctio, !is2d );
    if ( is2d )
    {
	MultiID key;
	if ( ads.getFirstStored(p2d,key) )
	{
	    PtrMan<IOObj> ioobj = IOM().get( key );
	    if ( ioobj )
	    {
		objfld->setInput( ioobj->key() );
		transffld->setInput( *ioobj );
	    }
	}
    }

    singLineSel(0);
*/
}


bool uiAttrVolOut::prepareProcessing()
{
    if ( !objfld->commitInput(true) )
    {
	uiMSG().error( "Please enter an output Seismic data set name" );
	return false;
    }
    else if ( !todofld->checkOutput(*ctio.ioobj) )
	return false;

    sel.ioobjkey = ctio.ioobj->key();
    sel.attrid = todofld->attribID();
    sel.outputnr = todofld->outputNr();
    if ( sel.outputnr < 0 && sel.attrid < 0 )
    {
	uiMSG().error( "Please select the output quantity" );
	return false;
    }

    uiSeisIOObjInfo ioobjinfo( *ctio.ioobj, true );
    SeisIOObjInfo::SpaceInfo spi( transffld->spaceInfo() );
    subselpar.clear();
    transffld->selfld->fillPar( subselpar );
    subselpar.set( "Estimated MBs", ioobjinfo.expectedMBs(spi) );
    return ioobjinfo.checkSpaceLeft(spi);
}


bool uiAttrVolOut::fillPar( IOPar& iopar )
{
    return false;
/*
    int nlamodelid = -1;
    if ( nlamodel && todofld->outputNr() >= 0 )
    {
	if ( !nlaid || !(*nlaid) )
	{ 
	    uiMSG().message( "NN needs to be stored before creating volume" ); 
	    return false; 
	}
	addNLA( nlamodelid );
    }
    const int targetid = nlamodelid < 0 ? todofld->attribID() : nlamodelid;

    IOPar attrpar( "Attribute Descriptions" );
    ads.fillPar( attrpar );
    for ( int idx=0; idx<attrpar.size(); idx++ )
    {
        const char* nm = attrpar.getKey(idx);
        BufferString name(CubeOutput::attribkey);
        name += "."; name += nm;
        iopar.add( name, attrpar.getValue(idx) );
    }

    BufferString key;
    BufferString keybase = AttribParseTools::outputstr; keybase += ".1.";
    key = keybase; key += AttribOutput::typekey;
    iopar.set( key, "Cube" );

    key = keybase; key += CubeOutput::attribkey;
    key += "."; key += AttribParseTools::maxkeystr;
    iopar.set( key, 1 );

    key = keybase; key += CubeOutput::attribkey; key += ".0";
    iopar.set( key, targetid );

    key = keybase; key += CubeOutput::seisidkey;
    iopar.set( key, ctio.ioobj->key() );
    transffld->scfmtfld->updateIOObj( ctio.ioobj );

    transffld->selfld->fillPar( subselpar );
    CubeSampling cs; cs.usePar( subselpar );
    key = keybase; key += CubeOutput::inlrangekey;
    if ( !cs.hrg.isEmpty() )
	iopar.set( key, cs.hrg.start.inl, cs.hrg.stop.inl );
    else
    {
	CubeSampling curcs;
	todofld->getRanges( curcs );
	iopar.set( key, curcs.hrg.start.inl, curcs.hrg.stop.inl );
    }

    key = keybase; key += CubeOutput::crlrangekey;
    if ( !cs.hrg.isEmpty() )
	iopar.set( key, cs.hrg.start.crl, cs.hrg.stop.crl );
    else
    {
	CubeSampling curcs;
	todofld->getRanges( curcs );
	iopar.set( key, curcs.hrg.start.crl, curcs.hrg.stop.crl );
    }

    key = keybase; key += CubeOutput::depthrangekey;
    iopar.set( key, cs.zrg.start*SI().zFactor(), cs.zrg.stop*SI().zFactor() );
    CubeSampling::removeInfo( subselpar );
    iopar.mergeComp( subselpar, keybase );

    Scaler* sc = transffld->scfmtfld->getScaler();
    if ( sc )
    {
	key = keybase; key += AttribOutput::scalekey;
	iopar.set( key, sc->toString() );
    }
    delete sc;

    ads.removeAttrib( ads.descNr(nlamodelid) );
    iopar.set( "Target value", todofld->getAttrName() );
    if ( ads.is2D() )
    {
	MultiID ky;
	if ( ads.getFirstStored(Only2D,ky) )
	    iopar.set( "Input Line Set", ky );
    }

    DescSet* rangeds = nlamodelid < 0 ? ads.optimizeClone(targetid)
				      : ads.clone();
    rangeds->setRanges( iopar, true );
    delete rangeds;
    return true;
*/
}


void uiAttrVolOut::addNLA( DescID& id )
{
    /*
    Desc* ad = new CalcAttribDesc( ads );
    BufferString defstr("NN specification=");
    defstr += nlaid;
    ad->setDefStr( defstr, false );
    ad->setHidden( true );
    const NLADesign& nlades = nlamodel->design();
    ad->setUserRef( *nlades.outputs[todofld->outputNr()] );
    ad->selectAttrib( todofld->outputNr() );

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
                    Desc* newdesc = new StorageAttribDesc( ads );
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
    */
}
