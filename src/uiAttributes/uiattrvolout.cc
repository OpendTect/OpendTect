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
#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uimultoutsel.h"
#include "uiseisfmtscale.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseistransf.h"
#include "uibatchjobdispatchersel.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "attribparam.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
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
#include "seis2dline.h"
#include "survinfo.h"

const char* uiAttrVolOut::sKeyMaxCrlRg()  { return "Maximum Crossline Range"; }
const char* uiAttrVolOut::sKeyMaxInlRg()  { return "Maximum Inline Range"; }


uiAttrVolOut::uiAttrVolOut( uiParent* p, const Attrib::DescSet& ad,
			    bool multioutput,
			    const NLAModel* n, const MultiID& id )
    : uiDialog(p,Setup("",mNoDlgTitle,"101.2.0"))
    , ctio_(*uiSeisSel::mkCtxtIOObj(Seis::geomTypeOf(ad.is2D(),false),false))
    , subselpar_(*new IOPar)
    , sel_(*new Attrib::CurrentSel)
    , ads_(*new Attrib::DescSet(ad))
    , nlamodel_(n)
    , nlaid_(id)
    , todofld_(0)
    , attrselfld_(0)
    , datastorefld_(0)
{
    const bool is2d = ad.is2D();
    setCaption( is2d ? "Create Data Attribute" :
	( multioutput ? "Create Multi-attribute Output"
		      : "Create Volume Attribute") );

    uiAttrSelData attrdata( ads_, false );
    attrdata.nlamodel_ = nlamodel_;

    if ( !multioutput )
    {
	todofld_ = new uiAttrSel( this, "Quantity to output", attrdata );
	todofld_->selectionDone.notify( mCB(this,uiAttrVolOut,attrSel) );
    }
    else
	attrselfld_ = new uiMultiAttribSel( this, attrdata.attrSet() );

    transffld_ = new uiSeisTransfer( this, uiSeisTransfer::Setup(is2d,false)
		.fornewentry(!is2d).withstep(!is2d).multiline(true) );
    if ( todofld_ )
	transffld_->attach( alignedBelow, todofld_ );
    else
	transffld_->attach( centeredBelow, attrselfld_ );

    ctio_.ctxt.toselect.dontallow_.set( sKey::Type(), sKey::Steering() );
    uiSeisSel::Setup su( is2d, false );
    su.selattr( true ).allowlinesetsel( false );

    objfld_ = new uiSeisSel( this, ctio_, su );
    objfld_->attach( alignedBelow, transffld_ );
    objfld_->setConfirmOverwrite( !is2d );

    uiGroup* botgrp = objfld_;
    if ( multioutput && !is2d )
    {
	uiCheckBox* cb = new uiCheckBox( this, "Enable Prestack Analysis" );
	cb->activated.notify( mCB(this,uiAttrVolOut,psSelCB) );
	cb->attach( alignedBelow, objfld_ );

	IOObjContext ctxt( mIOObjContext(SeisPS3D) );
	ctxt.forread = false;
	ctxt.deftransl = ctio_.ctxt.toselect.allowtransls_ = "MultiCube";
	datastorefld_ = new uiIOObjSel( this, ctxt,
					"Output Prestack DataStore" );
	datastorefld_->attach( alignedBelow, cb );

	const Interval<float> offsets( 0, 100 );
	offsetfld_ = new uiGenInput( this, "Offset (start/step)",
				     FloatInpIntervalSpec(offsets) );
	offsetfld_->attach( alignedBelow, datastorefld_ );
	psSelCB( cb );
	botgrp = offsetfld_;
    }

    batchfld_ = new uiBatchJobDispatcherSel( this, false,
					     Batch::JobSpec::Attrib );
    IOPar& iop = jobSpec().pars_;
    iop.set( IOPar::compKey(sKey::Output(),sKey::Type()), "Cube" );
    batchfld_->attach( alignedBelow, botgrp );
}


uiAttrVolOut::~uiAttrVolOut()
{
    delete ctio_.ioobj;
    delete &ctio_;
    delete &sel_;
    delete &subselpar_;
    delete &ads_;
}


Batch::JobSpec& uiAttrVolOut::jobSpec()
{
    return batchfld_->jobSpec();
}


void uiAttrVolOut::psSelCB( CallBacker* cb )
{
    mDynamicCastGet(uiCheckBox*,box,cb)
    if ( !box || !datastorefld_ ) return;

    datastorefld_->setSensitive( box->isChecked() );
    offsetfld_->setSensitive( box->isChecked() );
}


#define mSetObjFld(s) \
{ \
    objfld_->setInputText( s ); \
    objfld_->processInput(); \
    if( is2d ) objfld_->inpBox()->setReadOnly( true ); \
}

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
	    Attrib::Desc* firststoreddsc = ads_.getFirstStored();
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
	    MultiID mid ( desc->getStoredID().buf() );
	    ioobj = IOM().get( mid );
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

    batchfld_->setJobName( todofld_->getInput() );
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
	    BufferString outputnm = objfld_->getInput();
	    BufferString attrnm = LineKey( outputnm ).attrName();
	    const int nroccuer = outputnm.count( '|' );
	    outputnm.replace( '|', '_' );
	    if( nroccuer )
	    {
		BufferString msg( "Invalid charactor  '|' " );
		msg.add( " found in attribute name. " )
		   .add( "It will be renamed to: '" )
		   .add( outputnm.buf() ).add("'." )
		   .add( "\nDo you want to continue?" );
		if( !uiMSG().askGoOn( msg.buf() ) )
		    return false;
	    }

	    if ( outputnm.isEmpty() )
	    {
		uiMSG().error(
		       "No dataset name given. Please provide a valid name. " );
		return false;
	    }

	    SeisIOObjInfo info( ctio_.ioobj );
	    BufferStringSet lnms;
	    info.getLineNames( lnms );
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

	Attrib::Desc* seldesc = ads_.getDesc( todofld_->attribID() );
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

    if ( datastorefld_ && datastorefld_->sensitive() )
    {
	if ( !datastorefld_->ioobj() )
	    return false;

	TypeSet<Attrib::DescID> ids;
	attrselfld_->getSelIds( ids );
	ObjectSet<MultiID> mids; TypeSet<float> offs; TypeSet<int> comps;
	for ( int idx=0; idx<ids.size(); idx++ )
	{
	    mids += new MultiID( ctio_.ioobj->key() );
	    offs += offsetfld_->getfValue(0) + idx*offsetfld_->getfValue(1);
	    comps += idx;
	}

	BufferString errmsg;
	const bool res = MultiCubeSeisPSReader::writeData(
		datastorefld_->ioobj()->fullUserExpr(false),
		mids, offs, comps, errmsg );
	deepErase( mids );
	if ( !res )
	{
	    uiMSG().error( errmsg );
	    return false;
	}
    }

    uiSeisIOObjInfo ioobjinfo( *ctio_.ioobj, true );
    SeisIOObjInfo::SpaceInfo spi( transffld_->spaceInfo() );
    subselpar_.setEmpty();
    transffld_->selfld->fillPar( subselpar_ );
    subselpar_.set( "Estimated MBs", ioobjinfo.expectedMBs(spi) );
    return ioobjinfo.checkSpaceLeft(spi);
}


Attrib::DescSet* uiAttrVolOut::getFromToDoFld(
		TypeSet<Attrib::DescID>& outdescids, int& nrseloutputs )
{
    Attrib::DescID nlamodel_id(-1, false);
    if ( nlamodel_ && todofld_ && todofld_->outputNr() >= 0 )
    {
	if ( !nlaid_ || !(*nlaid_) )
	{
	    uiMSG().message("NN needs to be stored before creating volume");
	    return 0;
	}
	addNLA( nlamodel_id );
    }

    Attrib::DescID targetid = nlamodel_id.isValid() ? nlamodel_id
					    : todofld_->attribID();

    Attrib::Desc* seldesc = ads_.getDesc( targetid );
    if ( seldesc )
    {
	const bool is2d = todofld_->is2D();
	Attrib::DescID multoiid = seldesc->getMultiOutputInputID();
	if ( multoiid != Attrib::DescID::undef() )
	{
	    uiAttrSelData attrdata( ads_ );
	    Attrib::SelInfo attrinf( &attrdata.attrSet(), attrdata.nlamodel_,
				is2d, Attrib::DescID::undef(), false, false );
	    TypeSet<Attrib::SelSpec> targetspecs;
	    if ( !uiMultOutSel::handleMultiCompChain( targetid, multoiid,
				is2d, attrinf, &ads_, this, targetspecs ))
		return 0;
	    for ( int idx=0; idx<targetspecs.size(); idx++ )
		outdescids += targetspecs[idx].id();
	}
    }
    const int outdescidsz = outdescids.size();
    Attrib::DescSet* ret = outdescidsz ? ads_.optimizeClone( outdescids )
			    : ads_.optimizeClone( targetid );
    if ( !ret )
	return 0;

    nrseloutputs = seloutputs_.size() ? seloutputs_.size()
				      : outdescidsz ? outdescidsz : 1;
    if ( !seloutputs_.isEmpty() )
	//TODO make use of the multiple targetspecs (prestack for inst)
	ret->createAndAddMultOutDescs( targetid, seloutputs_,
					     seloutnms_, outdescids );
    else if ( outdescids.isEmpty() )
	outdescids += targetid;

    return ret;
}


bool uiAttrVolOut::fillPar()
{
    IOPar& iop = jobSpec().pars_;

    Attrib::DescSet* clonedset = 0;
    TypeSet<Attrib::DescID> outdescids;
    int nrseloutputs = 1;

    if ( todofld_ )
	clonedset = getFromToDoFld( outdescids, nrseloutputs );
    else
    {
	attrselfld_->getSelIds( outdescids );
	nrseloutputs = outdescids.size();
	clonedset = ads_.optimizeClone( outdescids );
    }
    if ( !clonedset )
	return false;

    IOPar attrpar( "Attribute Descriptions" );
    clonedset->fillPar( attrpar );
    for ( int idx=0; idx<attrpar.size(); idx++ )
    {
        const char* nm = attrpar.getKey(idx);
        iop.add( IOPar::compKey(Attrib::SeisTrcStorOutput::attribkey(),nm),
		 attrpar.getValue(idx) );
    }

    const BufferString keybase = IOPar::compKey(Attrib::Output::outputstr(),0);
    const BufferString attribkey =
	IOPar::compKey( keybase, Attrib::SeisTrcStorOutput::attribkey() );

    iop.set( IOPar::compKey(attribkey,Attrib::DescSet::highestIDStr()),
		nrseloutputs );
    if ( nrseloutputs != outdescids.size() ) return false;

    for ( int idx=0; idx<nrseloutputs; idx++ )
	iop.set( IOPar::compKey(attribkey,idx), outdescids[idx].asInt() );

    const bool is2d = todofld_ ? todofld_->is2D() : attrselfld_->is2D();
    BufferString outseisid;
    outseisid += ctio_.ioobj->key();

    iop.set( IOPar::compKey(keybase,Attrib::SeisTrcStorOutput::seisidkey()),
			    outseisid);

    transffld_->scfmtfld->updateIOObj( ctio_.ioobj );
    iop.setYN( IOPar::compKey(keybase,SeisTrc::sKeyExtTrcToSI()),
	       transffld_->scfmtfld->extendTrcToSI() );

    IOPar tmpiop; CubeSampling cs;
    transffld_->selfld->fillPar( tmpiop );
    BufferString typestr;
    //Subselection type and geometry will have an extra level key: 'Subsel
    if ( tmpiop.get( sKey::Type(), typestr ) )
	tmpiop.removeWithKey( sKey::Type() );

    CubeSampling::removeInfo( tmpiop );
    iop.mergeComp( tmpiop, keybase );
    tmpiop.setEmpty();
    if ( !typestr.isEmpty() )
	tmpiop.set( sKey::Type(), typestr );

    const bool usecs = typestr != "None";
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
	iop.set( IOPar::compKey(keybase,Attrib::Output::scalekey()),
				sc->toString() );
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
	Attrib::DescSet descset(true);
	if ( nlamodel_ )
	    descset.usePar( nlamodel_->pars() );

	Attrib::Desc* desc = nlamodel_ ? descset.getFirstStored()
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
		    PosInfo::Line2DKey l2dkey = prov->getLine2DKey();
		    BufferString lsnm = S2DPOS().getLineSet( l2dkey.lsID() );
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
	Attrib::EngineMan::getPossibleVolume( *clonedset, cs, linename,
						outdescids[0] );
	iop.set( sKeyMaxInlRg(),
		 cs.hrg.start.inl(), cs.hrg.stop.inl(), cs.hrg.step.inl() );
	iop.set( sKeyMaxCrlRg(),
		 cs.hrg.start.crl(), cs.hrg.stop.crl(), cs.hrg.step.crl() );
    }
    delete clonedset;

    return true;
}


void uiAttrVolOut::addNLA( Attrib::DescID& id )
{
    BufferString defstr("NN specification=");
    defstr += nlaid_;

    BufferString errmsg;
    Attrib::EngineMan::addNLADesc( defstr, id, ads_, todofld_->outputNr(),
			   nlamodel_, errmsg );

    if ( errmsg.size() )
        uiMSG().error( errmsg );
}


bool uiAttrVolOut::acceptOK( CallBacker* )
{
    if ( !prepareProcessing() || !fillPar() )
	return false;

    return batchfld_->start();
}
