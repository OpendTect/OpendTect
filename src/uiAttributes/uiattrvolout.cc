/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiattrvolout.cc,v 1.64 2009-06-02 09:11:47 cvsbert Exp $";

#include "uiattrvolout.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attriboutput.h"
#include "attribengman.h"
#include "uiattrsel.h"
#include "uimultoutsel.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseisfmtscale.h"
#include "uiseistransf.h"
#include "uiseisioobjinfo.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "seistrc.h"
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
	, ctio(mkCtxtIOObj(ad))
    	, subselpar(*new IOPar)
    	, sel(*new CurrentSel)
	, ads(const_cast<DescSet&>(ad))
	, nlamodel(n)
	, nlaid(id)
{
    setHelpID( "101.2.0" );

    bool is2d = ad.is2D();
    setTitleText( is2d ? "Create 2D seismic output":"Create 3D seismic output");

    uiAttrSelData attrdata( ad, false );
    attrdata.nlamodel = nlamodel;
    todofld = new uiAttrSel( uppgrp_, "Quantity to output", attrdata );
    todofld->selectiondone.notify( mCB(this,uiAttrVolOut,attrSel) );

    transffld = new uiSeisTransfer( uppgrp_, uiSeisTransfer::Setup(is2d,false)
	    	.fornewentry(true).withstep(false).multiline(true) );
    transffld->attach( alignedBelow, todofld );
    if ( transffld->selFld2D() )
	transffld->selFld2D()->singLineSel.notify(
				mCB(this,uiAttrVolOut,singLineSel) );

    ctio.ctxt.parconstraints.set( sKey::Type, sKey::Steering );
    ctio.ctxt.includeconstraints = false;
    ctio.ctxt.allowcnstrsabsent = true;
    objfld = new uiSeisSel( uppgrp_, ctio,
			    uiSeisSel::Setup(is2d,false).selattr(true));
    objfld->attach( alignedBelow, transffld );
    objfld->setConfirmOverwrite( !is2d );

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


CtxtIOObj& uiAttrVolOut::mkCtxtIOObj( const Attrib::DescSet& ad )
{
    return *uiSeisSel::mkCtxtIOObj( Seis::geomTypeOf(ad.is2D(),false), false );
}


void uiAttrVolOut::singLineSel( CallBacker* )
{
    if ( !transffld->selFld2D() ) return;

    if ( singmachfld_ )
	singmachfld_->setValue( transffld->selFld2D()->isSingLine() );
    singTogg( 0 );
    if ( singmachfld_ ) singmachfld_->display( false );
}


#define mSetObjFld(s) { objfld->setInputText( s ); objfld->processInput(); }

void uiAttrVolOut::attrSel( CallBacker* )
{
    CubeSampling cs;
    const bool is2d = todofld->is2D();
    if ( todofld->getRanges(cs) )
	transffld->selfld->setInput( cs );

    const Attrib::Desc* desc = ads.getDesc( todofld->attribID() );
    if ( !desc ) mSetObjFld("")
    else if ( !is2d )
	mSetObjFld( desc->isStored() ? "" : todofld->getInput() )
    else
    {
	const BufferString attrnm( todofld->getInput() );
	PtrMan<IOObj> ioobj = IOM().get( MultiID(desc->getStoredID(true)) );
	if ( !ioobj )
	    mSetObjFld( LineKey("",attrnm) )
	else
	{
	    mSetObjFld( LineKey(ioobj->key(),attrnm) )
	    objfld->setAttrNm( attrnm );
	    transffld->setInput( *ioobj );
	}
    }

    setParFileNmDef( todofld->getInput() );
    singLineSel(0);
}


bool uiAttrVolOut::prepareProcessing()
{
    if ( !objfld->commitInput() || !ctio.ioobj )
    {
	uiMSG().error( "Please enter an output Seismic data set name" );
	return false;
    }
    else if ( !todofld->checkOutput(*ctio.ioobj) )
	return false;

    if ( todofld->is2D() )
    {
	const char* outputnm = objfld->getInput();
	BufferString attrnm = LineKey( outputnm ).attrName();
	if ( attrnm.isEmpty() || attrnm == LineKey::sKeyDefAttrib() )
	{
	    const bool res = uiMSG().askContinue(
		"No attribute name given. Do you want to continue? "
		"Click on 'Yes' if you want 'Seis' as attribute name. "
		"Click on 'No' to provide another name." );
	    if ( !res ) return false; 
	}
    }

    sel.ioobjkey = ctio.ioobj->key();
    sel.attrid = todofld->attribID();
    sel.outputnr = todofld->outputNr();
    if ( sel.outputnr < 0 && !sel.attrid.isValid() )
    {
	uiMSG().error( "Please select the output quantity" );
	return false;
    }

    if ( todofld->is3D() )
    {
	ctio.ioobj->pars().set( sKey::Type, sKey::Attribute );
	IOM().commitChanges( *ctio.ioobj );
    }

    Desc* seldesc = ads.getDesc( todofld->attribID() );
    if ( seldesc )
    {
	uiMultOutSel multoutdlg( parent(), *seldesc );
	if ( multoutdlg.doDisp() && multoutdlg.go() )
	{
	    seloutputs.erase();
	    multoutdlg.getSelectedOutputs( seloutputs );
	    multoutdlg.getSelectedOutNames( seloutnms );
	}
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
    const DescID targetid = nlamodelid.isValid() ? nlamodelid
						 : todofld->attribID();
    IOPar attrpar( "Attribute Descriptions" );
    DescSet* clonedset = ads.optimizeClone( targetid );
    if ( !clonedset )
	return false;

    const int nrseloutputs = seloutputs.size() ? seloutputs.size() : 1;
    TypeSet<DescID> outdescids;
    if ( seloutputs.size() )
	clonedset->createAndAddMultOutDescs( targetid, seloutputs, seloutnms,
					     outdescids );

    clonedset->fillPar( attrpar );

    for ( int idx=0; idx<attrpar.size(); idx++ )
    {
        const char* nm = attrpar.getKey(idx);
        iop.add( IOPar::compKey(SeisTrcStorOutput::attribkey(),nm),
		 attrpar.getValue(idx) );
    }

    iop.set( IOPar::compKey(sKey::Output,sKey::Type), "Cube" );
    const BufferString keybase = IOPar::compKey( Output::outputstr(), 0 );
    const BufferString attribkey =
	IOPar::compKey( keybase, SeisTrcStorOutput::attribkey() );

    iop.set( IOPar::compKey(attribkey,DescSet::highestIDStr()), nrseloutputs );

    if ( seloutputs.size() )
    {
	if ( nrseloutputs != outdescids.size() ) return false;
	for ( int idx=0; idx<nrseloutputs; idx++ )
	    iop.set( IOPar::compKey(attribkey,idx), outdescids[idx].asInt() );
    }
    else
	iop.set( IOPar::compKey(attribkey,0), targetid.asInt() );

    BufferString outseisid;
    outseisid += ctio.ioobj->key();
    if ( todofld->is2D() )
    {
	outseisid += "|";
	outseisid += objfld->attrNm();
    }

    iop.set( IOPar::compKey(keybase,SeisTrcStorOutput::seisidkey()), outseisid );

    transffld->scfmtfld->updateIOObj( ctio.ioobj );
    iop.setYN( IOPar::compKey(keybase,SeisTrc::sKeyExtTrcToSI()),
	       transffld->scfmtfld->extendTrcToSI() );

    IOPar tmpiop;
    CubeSampling cs;
    transffld->selfld->fillPar( tmpiop );
    BufferString typestr;
    //Subselection type and geometry will have an extra level key: 'Subsel
    if ( tmpiop.get( sKey::Type, typestr ) )
	tmpiop.removeWithKey( sKey::Type );
    
    CubeSampling::removeInfo( tmpiop );
    iop.mergeComp( tmpiop, keybase );
    tmpiop.clear();
    if ( strcmp( typestr.buf(), "" ) )
	tmpiop.set( sKey::Type, typestr );
    
    bool usecs = strcmp( typestr.buf(), "None" );
    if ( usecs )
    {
	cs.usePar( subselpar );
	if ( !cs.hrg.isEmpty() )
	    cs.fillPar( tmpiop );
	else
	{
	    CubeSampling curcs; todofld->getRanges( curcs );
	    curcs.fillPar( tmpiop );
	}
    }

    const BufferString subkey = IOPar::compKey( sKey::Output, sKey::Subsel );
    iop.mergeComp( tmpiop, subkey );

    CubeSampling::removeInfo( subselpar );
    subselpar.removeWithKey( sKey::Type );
    iop.mergeComp( subselpar, sKey::Output );

    Scaler* sc = transffld->scfmtfld->getScaler();
    if ( sc )
    {
	iop.set( IOPar::compKey(keybase,Output::scalekey()), sc->toString() );
	delete sc;
    }

    BufferString attrnm = todofld->getAttrName();
    if ( todofld->is2D() )
    {
	const char* outputnm = objfld->getInput();
	attrnm = LineKey( outputnm ).attrName();
    }
    iop.set( sKey::Target, attrnm.buf() );
    BufferString linename;
    if ( todofld->is2D() )
    {
	MultiID ky;
	DescSet descset(true);
	if ( nlamodel )
	    descset.usePar( nlamodel->pars() );

	const Desc* desc = nlamodel ? descset.getFirstStored()
	    			    : clonedset->getFirstStored();
	if ( desc )
	{
	    BufferString storedid = desc->getStoredID();
	    if ( !storedid.isEmpty() )
	    {
		LineKey lk( storedid.buf() );
		iop.set( "Input Line Set", lk.lineName() );
		linename = lk.lineName();
	    }
	}
    }

    if ( usecs )
    {
	EngineMan::getPossibleVolume( *clonedset, cs, linename, targetid );
	iop.set(sKeyMaxInlRg,cs.hrg.start.inl,cs.hrg.stop.inl,cs.hrg.step.inl);
	iop.set(sKeyMaxCrlRg,cs.hrg.start.crl,cs.hrg.stop.crl,cs.hrg.step.crl);
    }
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

