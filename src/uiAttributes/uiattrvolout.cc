/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiattrvolout.h"

#include "uiattrsel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uimultoutsel.h"
#include "uiseisfmtscale.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseistransf.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "attribparam.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "errh.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "ptrman.h"
#include "scaler.h"
#include "seisioobjinfo.h"
#include "seismulticubeps.h"
#include "seisselection.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "survinfo.h"

using namespace Attrib;

const char* uiAttrVolOut::sKeyMaxCrlRg()  { return "Maximum Crossline Range"; }
const char* uiAttrVolOut::sKeyMaxInlRg()  { return "Maximum Inline Range"; }


uiAttrVolOut::uiAttrVolOut( uiParent* p, const DescSet& ad,
			    bool multioutput,
			    const NLAModel* n, const MultiID& id )
    : uiFullBatchDialog(p,Setup("Process"))
    , ctio_(mkCtxtIOObj(ad))
    , subselpar_(*new IOPar)
    , sel_(*new CurrentSel)
    , ads_(const_cast<DescSet&>(ad))
    , nlamodel_(n)
    , nlaid_(id)
    , todofld_(0)
    , attrselfld_(0)
    , datastorefld_(0)
{
    setHelpID( "101.2.0" );

    const bool is2d = ad.is2D();
    setCaption( is2d ? "Create LineSet Attribute":"Create Volume Attribute" );

    uiAttrSelData attrdata( ad, false );
    attrdata.nlamodel_ = nlamodel_;

    if ( !multioutput )
    {
	todofld_ = new uiAttrSel( uppgrp_, "Quantity to output", attrdata );
	todofld_->selectionDone.notify( mCB(this,uiAttrVolOut,attrSel) );
    }
    else
	attrselfld_ = new uiMultiAttribSel( uppgrp_, attrdata.attrSet() );

    transffld_ = new uiSeisTransfer( uppgrp_, uiSeisTransfer::Setup(is2d,false)
	    	.fornewentry(!is2d).withstep(false).multiline(true) );
    if ( todofld_ )
	transffld_->attach( alignedBelow, todofld_ );
    else
	transffld_->attach( centeredBelow, attrselfld_ );

    if ( transffld_->selFld2D() )
	transffld_->selFld2D()->singLineSel.notify(
				mCB(this,uiAttrVolOut,singLineSel) );

    ctio_.ctxt.toselect.dontallow_.set( sKey::Type(), sKey::Steering() );
    uiSeisSel::Setup su( is2d, false );
    su.selattr( true ).allowlinesetsel( false );
    
    objfld_ = new uiSeisSel( uppgrp_, ctio_, su );
    objfld_->attach( alignedBelow, transffld_ );
    objfld_->setConfirmOverwrite( !is2d );

    if ( multioutput )
    {
	IOObjContext ctxt( mIOObjContext(SeisPS3D) );
	ctxt.forread = false;
	ctxt.deftransl = ctio_.ctxt.toselect.allowtransls_ = "MultiCube";
	datastorefld_ = new uiIOObjSel( uppgrp_, ctxt, "Output data store" );
	datastorefld_->attach( alignedBelow, objfld_ );
    }

    uppgrp_->setHAlignObj( transffld_ );

    addStdFields( false, false, !is2d );
    if ( is2d && singmachfld_ ) singmachfld_->setSensitive( false );
}


uiAttrVolOut::~uiAttrVolOut()
{
    delete ctio_.ioobj;
    delete &ctio_;
    delete &sel_;
    delete &subselpar_;
}


CtxtIOObj& uiAttrVolOut::mkCtxtIOObj( const Attrib::DescSet& ad )
{
    return *uiSeisSel::mkCtxtIOObj( Seis::geomTypeOf(ad.is2D(),false), false );
}


void uiAttrVolOut::singLineSel( CallBacker* )
{
    if ( !transffld_->selFld2D() ) return;

    setMode( transffld_->selFld2D()->isSingLine() ? Single : Multi );
}


#define mSetObjFld(s) { objfld_->setInputText( s ); objfld_->processInput(); }

void uiAttrVolOut::attrSel( CallBacker* )
{
    CubeSampling cs;
    const bool is2d = todofld_->is2D();
    if ( todofld_->getRanges(cs) )
	transffld_->selfld->setInput( cs );

    Attrib::Desc* desc = ads_.getDesc( todofld_->attribID() );
    if ( !desc )
    {
	mSetObjFld("")
	if ( is2d )	//it could be 2D neural network
	{
	    Desc* firststoreddsc = ads_.getFirstStored();
	    if ( firststoreddsc )
	    {
		const LineKey lk( firststoreddsc->getValParam(
			Attrib::StorageProvider::keyStr())->getStringValue(0) );
		BufferString linenm = lk.lineName();
		if ( !linenm.isEmpty() && *linenm.buf() != '#' )
		    mSetObjFld( LineKey(IOM().nameOf( linenm.buf() ),
				todofld_->getInput()) )

		PtrMan<IOObj> ioobj =
			IOM().get( MultiID(firststoreddsc->getStoredID(true)) );
		if ( ioobj )
		    transffld_->setInput( *ioobj );
	    }
	}
    }
    else if ( !is2d )
	mSetObjFld( desc->isStored() ? "" : todofld_->getInput() )
    else
    {
        BufferString attrnm( todofld_->getAttrName() );

	BufferString errmsg;
	RefMan<Attrib::Provider> prov =
		Attrib::Provider::create( *desc, errmsg );
	PtrMan<IOObj> ioobj = 0;
	if ( prov )
	{
	    PosInfo::GeomID geomid = prov->getGeomID();
	    BufferString lsnm = S2DPOS().getLineSet( geomid.lsid_ );
	    SeisIOObjInfo info( lsnm );
	    ioobj = info.ioObj() ? info.ioObj()->clone() : 0;
	}

	if ( !ioobj )
	    mSetObjFld( LineKey(attrnm) )
	else
	{
	    if ( desc->isStored() )
		mSetObjFld( LineKey(todofld_->getInput()) )
	    else
		mSetObjFld( LineKey(ioobj->name(),attrnm) )
	    transffld_->setInput( *ioobj );
	}
    }

    setParFileNmDef( todofld_->getInput() );
    singLineSel(0);
}


bool uiAttrVolOut::prepareProcessing()
{
    if ( !objfld_->commitInput() || !ctio_.ioobj )
    {
	if ( objfld_->isEmpty() )
	    uiMSG().error( "Please enter output Seismic data name" );
	return false;
    }

    if ( todofld_ )
    {
	if ( !todofld_->checkOutput(*ctio_.ioobj) )
	    return false;

	if ( todofld_->is2D() )
	{
	    const char* outputnm = objfld_->getInput();
	    BufferString attrnm = LineKey( outputnm ).attrName();
	    if ( attrnm.isEmpty() || attrnm == LineKey::sKeyDefAttrib() )
	    {
		const bool res = uiMSG().askGoOn(
		    "No attribute name given. Do you want to continue? "
		    "Click on 'Yes' if you want 'Seis' as attribute name. "
		    "Click on 'No' to provide another name." );
		if ( !res ) return false; 
	    }

	    if ( attrnm.isEmpty() )
		attrnm = LineKey::sKeyDefAttrib();

	    SeisIOObjInfo info( ctio_.ioobj );
	    BufferStringSet lnms;
	    info.getLineNamesWithAttrib( attrnm.buf(), lnms );
	    const bool singline = transffld_->selFld2D()->isSingLine();
	    const char* lnm =
		singline ? transffld_->selFld2D()->selectedLine() : 0;
	    if ( (!singline && lnms.size()) ||
		 (singline && lnms.isPresent(lnm)) )
	    {
		const bool rv = uiMSG().askGoOn(
		    "Output attribute already exists.", "Overwrite", "Cancel" );
		if ( !rv ) return false; 
	    }
	}

	sel_.ioobjkey_ = ctio_.ioobj->key();
	sel_.attrid_ = todofld_->attribID();
	sel_.outputnr_ = todofld_->outputNr();
	if ( sel_.outputnr_ < 0 && !sel_.attrid_.isValid() )
	{
	    uiMSG().error( "Please select the output quantity" );
	    return false;
	}

	if ( todofld_->is3D() )
	{
	    ctio_.ioobj->pars().set( sKey::Type(), sKey::Attribute() );
	    IOM().commitChanges( *ctio_.ioobj );
	}

	Desc* seldesc = ads_.getDesc( todofld_->attribID() );
	if ( seldesc )
	{
	    uiMultOutSel multoutdlg( this, *seldesc );
	    if ( multoutdlg.doDisp() )
	    {
		if ( multoutdlg.go() )
		{
		    seloutputs_.erase();
		    multoutdlg.getSelectedOutputs( seloutputs_ );
		    multoutdlg.getSelectedOutNames( seloutnms_ );
		}
		else
		    return false;
	    }
	}
    }

    uiSeisIOObjInfo ioobjinfo( *ctio_.ioobj, true );
    SeisIOObjInfo::SpaceInfo spi( transffld_->spaceInfo() );
    subselpar_.setEmpty();
    transffld_->selfld->fillPar( subselpar_ );
    subselpar_.set( "Estimated MBs", ioobjinfo.expectedMBs(spi) );
    return ioobjinfo.checkSpaceLeft(spi);
}


bool uiAttrVolOut::fillPar( IOPar& iop )
{
    DescSet* clonedset = 0;
    TypeSet<DescID> outdescids;
    int nrseloutputs = 1;

    if ( todofld_ )
    {
	DescID nlamodel_id(-1, false);
	if ( nlamodel_ && todofld_ && todofld_->outputNr() >= 0 )
	{
	    if ( !nlaid_ || !(*nlaid_) )
	    { 
		uiMSG().message("NN needs to be stored before creating volume");
		return false; 
	    }
	    addNLA( nlamodel_id );
	}

	const DescID targetid = nlamodel_id.isValid() ? nlamodel_id
						      : todofld_->attribID();
	clonedset = ads_.optimizeClone( targetid );

	nrseloutputs = seloutputs_.size() ? seloutputs_.size() : 1;
	if ( clonedset && seloutputs_.size() )
	    clonedset->createAndAddMultOutDescs( targetid, seloutputs_,
						 seloutnms_, outdescids );
	else
	    outdescids += targetid;
    }
    else
    {
	attrselfld_->getSelIds( outdescids );
	clonedset = ads_.optimizeClone( outdescids );
	nrseloutputs = outdescids.size();
    }

    if ( !clonedset )
	return false;

    IOPar attrpar( "Attribute Descriptions" );
    clonedset->fillPar( attrpar );
    for ( int idx=0; idx<attrpar.size(); idx++ )
    {
        const char* nm = attrpar.getKey(idx);
        iop.add( IOPar::compKey(SeisTrcStorOutput::attribkey(),nm),
		 attrpar.getValue(idx) );
    }

    iop.set( IOPar::compKey(sKey::Output(),sKey::Type()), "Cube" );
    const BufferString keybase = IOPar::compKey( Output::outputstr(), 0 );
    const BufferString attribkey =
	IOPar::compKey( keybase, SeisTrcStorOutput::attribkey() );

    iop.set( IOPar::compKey(attribkey,DescSet::highestIDStr()), nrseloutputs );
    if ( nrseloutputs != outdescids.size() ) return false;

    for ( int idx=0; idx<nrseloutputs; idx++ )
	iop.set( IOPar::compKey(attribkey,idx), outdescids[idx].asInt() );

    const bool is2d = todofld_ ? todofld_->is2D() : attrselfld_->is2D();
    BufferString outseisid;
    outseisid += ctio_.ioobj->key();
    if ( is2d )
    {
	outseisid += "|";
	outseisid += objfld_->attrNm();
    }

    iop.set( IOPar::compKey(keybase,SeisTrcStorOutput::seisidkey()), outseisid);

    transffld_->scfmtfld->updateIOObj( ctio_.ioobj );
    iop.setYN( IOPar::compKey(keybase,SeisTrc::sKeyExtTrcToSI()),
	       transffld_->scfmtfld->extendTrcToSI() );

    IOPar tmpiop;
    CubeSampling cs;
    transffld_->selfld->fillPar( tmpiop );
    BufferString typestr;
    //Subselectio_n type and geometry will have an extra level key: 'Subsel
    if ( tmpiop.get( sKey::Type(), typestr ) )
	tmpiop.removeWithKey( sKey::Type() );
    
    CubeSampling::removeInfo( tmpiop );
    iop.mergeComp( tmpiop, keybase );
    tmpiop.setEmpty();
    if ( strcmp( typestr.buf(), "" ) )
	tmpiop.set( sKey::Type(), typestr );
    
    const bool usecs = strcmp( typestr.buf(), "None" );
    if ( usecs )
    {
	cs.usePar( subselpar_ );
	if ( !cs.hrg.isEmpty() )
	    cs.fillPar( tmpiop );
	else if ( todofld_ )
	{
	    CubeSampling curcs; todofld_->getRanges( curcs );
	    curcs.fillPar( tmpiop );
	}
    }

    const BufferString subkey = IOPar::compKey( sKey::Output(), sKey::Subsel());
    iop.mergeComp( tmpiop, subkey );

    CubeSampling::removeInfo( subselpar_ );
    subselpar_.removeWithKey( sKey::Type() );
    iop.mergeComp( subselpar_, sKey::Output() );

    Scaler* sc = transffld_->scfmtfld->getScaler();
    if ( sc )
    {
	iop.set( IOPar::compKey(keybase,Output::scalekey()), sc->toString() );
	delete sc;
    }

    BufferString attrnm = todofld_ ? todofld_->getAttrName() : "Multi-attribs";
    if ( is2d )
    {
	const char* outputnm = objfld_->getInput();
	attrnm = LineKey( outputnm ).attrName();
    }
    iop.set( sKey::Target(), attrnm.buf() );
    BufferString linename;
    if ( is2d )
    {
	MultiID ky;
	DescSet descset(true);
	if ( nlamodel_ )
	    descset.usePar( nlamodel_->pars(), nlamodel_->versionNr() );

	Desc* desc = nlamodel_ ? descset.getFirstStored()
	    		      : ads_.getDesc( todofld_->attribID() );
	if ( desc )
	{
	    BufferString storedid = desc->getStoredID();
	    if ( storedid.isEmpty() )
	    {
		BufferString errmsg;
		RefMan<Attrib::Provider> prov =
		    Attrib::Provider::create( *desc, errmsg );
		if ( prov )
		{
		    PosInfo::GeomID geomid = prov->getGeomID();
		    BufferString lsnm = S2DPOS().getLineSet( geomid.lsid_ );
		    SeisIOObjInfo info( lsnm );
		    if ( info.ioObj() )
			storedid = info.ioObj()->key();
		}
	    }

	    if ( !storedid.isEmpty() )
	    {
		LineKey lk( storedid.buf() );
		iop.set( "Input Line Set", lk.lineName() );
		linename = lk.lineName();
	    }
	}

	Seis2DLineSet::invalidateCache();
    }

    if ( usecs )
    {
	EngineMan::getPossibleVolume( *clonedset, cs, linename, outdescids[0] );
	iop.set( sKeyMaxInlRg(),
		 cs.hrg.start.inl, cs.hrg.stop.inl, cs.hrg.step.inl );
	iop.set( sKeyMaxCrlRg(),
		 cs.hrg.start.crl, cs.hrg.stop.crl, cs.hrg.step.crl );
    }
    delete clonedset;

    FilePath fp( ctio_.ioobj->fullUserExpr() );
    fp.setExtension( "proc" );
    iop.write( fp.fullPath(), sKey::Pars() );
    return true;
}


void uiAttrVolOut::addNLA( DescID& id )
{
    BufferString defstr("NN specification=");
    defstr += nlaid_;

    BufferString errmsg;
    EngineMan::addNLADesc( defstr, id, ads_, todofld_->outputNr(), 
			   nlamodel_, errmsg );

    if ( errmsg.size() )
        uiMSG().error( errmsg );
}
