/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:		$Id: uiattrvolout.cc,v 1.38 2008-02-19 15:14:51 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattrvolout.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attriboutput.h"
#include "attribengman.h"
#include "uiattrsel.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseisfmtscale.h"
#include "uiseistransf.h"
#include "uiseisioobjinfo.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "seistrctr.h"
#include "seisselection.h"
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
#include "scaler.h"
#include "cubesampling.h"
#include "keystrs.h"

using namespace Attrib;

const char* uiAttrVolOut::sKeyMaxCrlRg = "Maximum Crossline Range";
const char* uiAttrVolOut::sKeyMaxInlRg = "Maximum Inline Range";


uiAttrVolOut::uiAttrVolOut( uiParent* p, const DescSet& ad,
			    const NLAModel* n, MultiID id )
	: uiFullBatchDialog(p,Setup("Process"))
	, ctio(mkCtxtIOObj())
    	, subselpar(*new IOPar)
    	, sel(*new CurrentSel)
	, ads(const_cast<DescSet&>(ad))
	, nlamodel(n)
	, nlaid(id)
{
    setHelpID( "101.2.0" );

    bool is2d = ad.is2D();
    setTitleText( is2d ? "Create 2D seismic output":"Create 3D seismic output");

    uiAttrSelData attrdata( &ad );
    attrdata.nlamodel = nlamodel;
    todofld = new uiAttrSel( uppgrp_, "Quantity to output", attrdata, is2d);
    todofld->selectiondone.notify( mCB(this,uiAttrVolOut,attrSel) );

    transffld = new uiSeisTransfer( uppgrp_, uiSeisTransfer::Setup(is2d,false)
	    	.fornewentry(true).withstep(false).multiline(true) );
    transffld->attach( alignedBelow, todofld );
    if ( transffld->selFld2D() )
	transffld->selFld2D()->singLineSel.notify(
				mCB(this,uiAttrVolOut,singLineSel) );

    ctio.ctxt.forread = false;
    ctio.ctxt.parconstraints.set( sKey::Type, sKey::Steering );
    ctio.ctxt.includeconstraints = false;
    ctio.ctxt.allowcnstrsabsent = true;
    objfld = new uiSeisSel( uppgrp_, ctio,
	    		    uiSeisSel::Setup(is2d,false).selattr(false));
    objfld->attach( alignedBelow, transffld );

    uppgrp_->setHAlignObj( transffld );

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
    if ( !transffld->selFld2D() ) return;

    if ( singmachfld_ ) singmachfld_->setValue( transffld->selFld2D()->isSingLine() );
    singTogg( 0 );
    if ( singmachfld_ ) singmachfld_->display( false );
}


void uiAttrVolOut::attrSel( CallBacker* )
{
    CubeSampling cs;
    const bool is2d = todofld->is2D();
    if ( todofld->getRanges(cs) )
	transffld->selfld->setInput( cs );
    if ( is2d )
    {
	MultiID key;
	const Desc* desc = ads.getFirstStored();
	if ( desc && desc->getMultiID(key) )
	{
	    PtrMan<IOObj> ioobj = IOM().get( key );
	    if ( ioobj )
	    {
		objfld->setInput( ioobj->key() );
		transffld->setInput( *ioobj );
	    }
	}
    }

    setParFileNmDef( todofld->getInput() );
    singLineSel(0);
}


bool uiAttrVolOut::prepareProcessing()
{
    if ( !objfld->commitInput(true) || !ctio.ioobj )
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

    if ( todofld->is3D() )
    {
	ctio.ioobj->pars().set( sKey::Type, sKey::Attribute );
	IOM().commitChanges( *ctio.ioobj );
    }

    uiSeisIOObjInfo ioobjinfo( *ctio.ioobj, true );
    SeisIOObjInfo::SpaceInfo spi( transffld->spaceInfo() );
    subselpar.clear();
    transffld->selfld->fillPar( subselpar );
    subselpar.set( "Estimated MBs", ioobjinfo.expectedMBs(spi) );
    return ioobjinfo.checkSpaceLeft(spi);
}


bool uiAttrVolOut::fillPar( IOPar& iop )
{
    DescID nlamodelid(-1, true);
    if ( nlamodel && todofld->outputNr() >= 0 )
    {
	if ( !nlaid || !(*nlaid) )
	{ 
	    uiMSG().message( "NN needs to be stored before creating volume" ); 
	    return false; 
	}
	addNLA( nlamodelid );
    }
    const DescID targetid = nlamodelid < 0 ? todofld->attribID() : nlamodelid;

    IOPar attrpar( "Attribute Descriptions" );
    DescSet* clonedset = ads.optimizeClone( targetid );
    if ( !clonedset )
	return false; 
    clonedset->fillPar( attrpar );

    for ( int idx=0; idx<attrpar.size(); idx++ )
    {
        const char* nm = attrpar.getKey(idx);
        BufferString key(SeisTrcStorOutput::attribkey);
        key += "."; key += nm;
        iop.add( key, attrpar.getValue(idx) );
    }

    BufferString key;
    BufferString keybase = Output::outputstr; keybase += ".1.";
    key = keybase; key += sKey::Type;
    iop.set( key, "Cube" );

    key = keybase; key += SeisTrcStorOutput::attribkey;
    key += "."; key += DescSet::highestIDStr();
    iop.set( key, 1 );

    key = keybase; key += SeisTrcStorOutput::attribkey; key += ".0";
    iop.set( key, targetid.asInt() );

    key = keybase; key += SeisTrcStorOutput::seisidkey;
    iop.set( key, ctio.ioobj->key() );
    transffld->scfmtfld->updateIOObj( ctio.ioobj );

    transffld->selfld->fillPar( subselpar );
    CubeSampling cs; cs.usePar( subselpar );
    IOPar tmpiop;
    if ( !cs.hrg.isEmpty() )
	cs.fillPar( tmpiop );
    else
    {
	CubeSampling curcs; todofld->getRanges( curcs );
	curcs.fillPar( tmpiop );
    }
    iop.mergeComp( tmpiop, keybase );

    CubeSampling::removeInfo( subselpar );
    iop.mergeComp( subselpar, keybase );

    Scaler* sc = transffld->scfmtfld->getScaler();
    if ( sc )
    {
	key = keybase; key += Output::scalekey;
	iop.set( key, sc->toString() );
	delete sc;
    }

    iop.set( "Target value", todofld->getAttrName() );
    BufferString linename;
    if ( todofld->is2D() )
    {
	MultiID ky;
	DescSet descset(true);
	if ( nlamodel )
	    descset.usePar( nlamodel->pars() );

	const Desc* desc = nlamodel ? descset.getFirstStored()
	    			    : clonedset->getFirstStored();
	if ( desc && desc->getMultiID(ky) )
	{
	    iop.set( "Input Line Set", ky );
	    linename = ky;
	}
    }

    EngineMan::getPossibleVolume( *clonedset, cs, linename, targetid );
    iop.set( sKeyMaxInlRg, cs.hrg.start.inl, cs.hrg.stop.inl, cs.hrg.step.inl );
    iop.set( sKeyMaxCrlRg, cs.hrg.start.crl, cs.hrg.stop.crl, cs.hrg.step.crl );
    delete clonedset;

    return true;
}


void uiAttrVolOut::addNLA( DescID& id )
{
    BufferString defstr("NN specification=");
    defstr += nlaid;

    BufferString errmsg;
    EngineMan::addNLADesc( defstr, id, ads, todofld->outputNr(), 
			   nlamodel, errmsg );

    if ( errmsg.size() )
        uiMSG().error( errmsg );
}
