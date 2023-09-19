/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattrvolout.h"

#include "uiattrsel.h"
#include "uibatchjobdispatchersel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uimultoutsel.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseistransf.h"
#include "uiseparator.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "attribparambase.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "nlamodel.h"
#include "od_helpids.h"
#include "ptrman.h"
#include "scaler.h"
#include "seisioobjinfo.h"
#include "seismulticubeps.h"
#include "seisselection.h"
#include "seistrc.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

const char* uiAttrVolOut::sKeyMaxCrlRg()  { return "Maximum Crossline Range"; }
const char* uiAttrVolOut::sKeyMaxInlRg()  { return "Maximum Inline Range"; }


uiAttrVolOut::uiAttrVolOut( uiParent* p, const Attrib::DescSet& ad,
			    bool multioutput,
			    const NLAModel* model, const MultiID& id )
    : uiBatchProcDlg(p,uiString::emptyString(),false, Batch::JobSpec::Attrib)
    , subselpar_(*new IOPar)
    , sel_(*new Attrib::CurrentSel)
    , ads_(new Attrib::DescSet(ad))
    , nlamodel_(0)
    , nlaid_(id)
    , todofld_(0)
    , attrselfld_(0)
    , datastorefld_(0)
    , needSaveNLA(this)
{
    const bool is2d = ad.is2D();
    const Seis::GeomType gt = Seis::geomTypeOf( is2d, false );

    setCaption( is2d ? tr("Create 2D Data Attribute") :
	(multioutput ? tr("Create Multi-Attribute Volume")
		     : tr("Create Single-Attribute Volume")) );

    setHelpKey( is2d ? mODHelpKey(mAttrVolOut2DHelpID)
		     : mODHelpKey(mAttrVolOutHelpID) );

    if ( model )
	nlamodel_ = model->clone();

    uiAttrSelData attrdata( *ads_, false );
    attrdata.nlamodel_ = nlamodel_;

    uiSeparator* sep1 = 0;
    if ( !multioutput )
    {
	todofld_ = new uiAttrSel( pargrp_, "Select Attribute", attrdata );
	todofld_->selectionDone.notify( mCB(this,uiAttrVolOut,attrSel) );
    }
    else
    {
	attrselfld_ = new uiMultiAttribSel( pargrp_, &attrdata.attrSet() );
	sep1 = new uiSeparator( pargrp_, "Attribute Selection Separator" );
	sep1->attach( stretchedBelow, attrselfld_ );
    }

    transffld_ = new uiSeisTransfer( pargrp_,
		uiSeisTransfer::Setup(is2d,false)
			.fornewentry(true).withstep(!is2d).multiline(true) );
    if ( todofld_ )
	transffld_->attach( alignedBelow, todofld_ );
    else
	transffld_->attach( centeredBelow, attrselfld_ );

    if ( sep1 ) transffld_->attach( ensureBelow, sep1 );

    objfld_ = new uiSeisSel( pargrp_, uiSeisSel::ioContext(gt,false),
				uiSeisSel::Setup(is2d,false) );
    objfld_->selectionDone.notify( mCB(this,uiAttrVolOut,outSelCB) );
    objfld_->attach( alignedBelow, transffld_ );
    objfld_->setConfirmOverwrite( !is2d );

    uiGroup* botgrp = objfld_;
    uiSeparator* sep3 = 0;
    if ( multioutput && !is2d )
    {
	uiSeparator* sep2 = new uiSeparator( pargrp_, "PS Start Separator" );
	sep2->attach( stretchedBelow, objfld_ );

	uiCheckBox* cb = new uiCheckBox(pargrp_,tr("Enable Prestack Analysis"));
	cb->activated.notify( mCB(this,uiAttrVolOut,psSelCB) );
	cb->attach( alignedBelow, objfld_ );
	cb->attach( ensureBelow, sep2 );

	IOObjContext ctxt( mIOObjContext(SeisPS3D) );
	ctxt.forread_ = false;
	ctxt.fixTranslator( "MultiCube" );
	datastorefld_ = new uiIOObjSel( pargrp_, ctxt,
			    uiStrings::phrOutput(uiStrings::phrJoinStrings(
			    uiStrings::sPreStack(),tr("DataStore"))) );
	datastorefld_->attach( alignedBelow, cb );

	const Interval<float> offsets( 0, 100 );
	const uiString lbl = tr( "Offset (start/step) %1" )
					.arg( SI().getXYUnitString() );
	offsetfld_ = new uiGenInput( pargrp_, lbl,
				     FloatInpIntervalSpec(offsets) );
	offsetfld_->attach( alignedBelow, datastorefld_ );

	sep3 = new uiSeparator( pargrp_, "PS End Separator" );
	sep3->attach( stretchedBelow, offsetfld_ );

	psSelCB( cb );
	botgrp = offsetfld_;
    }

    pargrp_->setHAlignObj( botgrp );
}


uiAttrVolOut::~uiAttrVolOut()
{
    delete &sel_;
    delete &subselpar_;
    delete ads_;
    delete nlamodel_;
}

void uiAttrVolOut::setInput( const Attrib::DescID& descid )
{
    Attrib::Desc* desc = ads_->getDesc( descid );
    todofld_->setDesc( desc );
    attrSel( 0 );
}


void uiAttrVolOut::updateAttributes( const Attrib::DescSet& descset,
				     const NLAModel* nlamodel,
				     const MultiID& nlaid )
{
    delete ads_;
    ads_ = new Attrib::DescSet( descset );
    if ( todofld_ )
    { // Do not change this order!
	todofld_->setNLAModel( nlamodel );
	todofld_->setDescSet( ads_ );
    }

    if ( attrselfld_ )
	attrselfld_->setDescSet( ads_ );

    if ( nlamodel )
    {
	delete nlamodel_;
	nlamodel_ = nlamodel->clone();
    }

    nlaid_ = nlaid;
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
    outSelCB(0); \
}

void uiAttrVolOut::attrSel( CallBacker* )
{
    TrcKeyZSampling cs;
    const bool is2d = todofld_->is2D();
    if ( todofld_->getRanges(cs) )
	transffld_->selfld->setInput( cs );

    Attrib::Desc* desc = ads_->getDesc( todofld_->attribID() );
    if ( !desc )
    {
	mSetObjFld("")
	if ( is2d )	//it could be 2D neural network
	{
	    Attrib::Desc* firststoreddsc = ads_->getFirstStored();
	    if ( firststoreddsc )
	    {
		const LineKey lk( firststoreddsc->getValParam(
			Attrib::StorageProvider::keyStr())->getStringValue(0) );
		BufferString linenm = lk.lineName();
		if ( !linenm.isEmpty() && *linenm.buf() != '#' )
		    mSetObjFld( LineKey(IOM().nameOf( linenm.buf() ),
				todofld_->getInput()) )

		PtrMan<IOObj> ioobj = IOM().get(
		    MultiID(firststoreddsc->getStoredID(true).buf()) );
		if ( ioobj )
		    transffld_->setInput( *ioobj );
	    }
	}
    }
    else
    {
	mSetObjFld( desc->isStored() ? "" : todofld_->getInput() )
	if ( is2d )
	{
	    uiString errmsg;
	    RefMan<Attrib::Provider> prov =
		    Attrib::Provider::create( *desc, errmsg );
	    PtrMan<IOObj> ioobj = 0;
	    if ( prov )
	    {
		MultiID mid( desc->getStoredID(true).buf() );
		ioobj = IOM().get( mid );
	    }

	    if ( ioobj )
		transffld_->setInput( *ioobj );
	}
    }
}


void uiAttrVolOut::outSelCB( CallBacker* )
{
}


bool uiAttrVolOut::prepareProcessing()
{
    seloutnms_.erase();
    seloutputs_.erase();

    const IOObj* outioobj = objfld_->ioobj();
    if ( !outioobj )
	return false;

    if ( todofld_ )
    {
	if ( !todofld_->checkOutput(*outioobj) )
	    return false;

	if ( todofld_->is2D() )
	{
	    BufferString outputnm = objfld_->getInput();
	    const int nroccuer = outputnm.count( '|' );
	    outputnm.replace( '|', '_' );
	    if( nroccuer )
	    {
		uiString msg = tr("Invalid charactor '|' "
				  " found in output name. "
				  "It will be renamed to: '%1'"
				  "\n\nDo you want to continue?")
			     .arg(outputnm.buf());
		if ( !uiMSG().askContinue(msg) )
		    return false;
	    }

	    if ( outputnm.isEmpty() )
	    {
		uiMSG().error(
		    tr("No dataset name given. Please provide a valid name.") );
		return false;
	    }

	    SeisIOObjInfo info( outioobj );
	    BufferStringSet lnms;
	    info.getLineNames( lnms );
	    const bool singline = transffld_->selFld2D()->isSingLine();
	    const char* lnm =
		singline ? transffld_->selFld2D()->selectedLine() : 0;
	    if ( (!singline && lnms.size()) ||
		 (singline && lnms.isPresent(lnm)) )
	    {
		const bool rv = uiMSG().askGoOn(
		    tr("Output attribute already exists."),
		       uiStrings::sOverwrite(), uiStrings::sCancel());
		if ( !rv ) return false;
	    }
	}

	sel_.ioobjkey_ = outioobj->key();
	sel_.attrid_ = todofld_->attribID();
	sel_.outputnr_ = todofld_->outputNr();
	if ( sel_.outputnr_ < 0 && !sel_.attrid_.isValid() )
	{
	    uiMSG().error( tr("Please select attribute to calculate") );
	    return false;
	}

	Attrib::Desc* seldesc = ads_->getDesc( todofld_->attribID() );
	if ( seldesc )
	{
	    const bool ismctransl = objfld_->outputSupportsMultiComp();
	    uiMultOutSel multoutdlg( this, *seldesc, !ismctransl );
	    if ( multoutdlg.doDisp() )
	    {
		if ( multoutdlg.go() )
		{
		    multoutdlg.getSelectedOutputs( seloutputs_ );
		    multoutdlg.getSelectedOutNames( seloutnms_ );
		}
		else
		    return false;
	    }
	}

	BufferString typestr;
	if ( seldesc && seldesc->isStored() )
	{
	    BufferStringSet availcomps;
	    uiMultOutSel::fillInAvailOutNames( *seldesc, availcomps );
	    const bool copyallcomps = availcomps.size() == seloutnms_.size();
	    if ( copyallcomps )
		typestr = seldesc->getStoredType();
	}

	if ( typestr.isEmpty() )
	    typestr = sKey::Attribute();

	const IOObj* chioobj = outioobj->clone();
	chioobj->pars().set( sKey::Type(), typestr );
	IOM().commitChanges( *chioobj );
	delete chioobj;
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
	    mids += new MultiID( outioobj->key() );
	    offs += offsetfld_->getFValue(0) + idx*offsetfld_->getFValue(1);
	    comps += idx;
	}

	uiString errmsg;
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

    uiSeisIOObjInfo ioobjinfo( *outioobj, true );
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
	if ( nlaid_.isUdf() )
	{
	    needSaveNLA.trigger();
	    //check if we have retrieved the newly saved DBKey
	    if ( nlaid_.isUdf() )
		uiMSG().message(
			tr("NN needs to be stored before creating volume"));
	    return 0;
	}
	addNLA( nlamodel_id );
    }

    Attrib::DescID targetid = nlamodel_id.isValid() ? nlamodel_id
					    : todofld_->attribID();

    Attrib::Desc* seldesc = ads_->getDesc( targetid );
    if ( seldesc )
    {
	const bool is2d = todofld_->is2D();
	Attrib::DescID multoiid = seldesc->getMultiOutputInputID();
	if ( multoiid != Attrib::DescID::undef() )
	{
	    uiAttrSelData attrdata( *ads_ );
	    Attrib::SelInfo attrinf( &attrdata.attrSet(), attrdata.nlamodel_,
				is2d, Attrib::DescID::undef(), false, false );
	    TypeSet<Attrib::SelSpec> targetspecs;
	    if ( !uiMultOutSel::handleMultiCompChain( targetid, multoiid,
				is2d, attrinf, ads_, this, targetspecs ))
		return 0;
	    for ( int idx=0; idx<targetspecs.size(); idx++ )
		outdescids += targetspecs[idx].id();
	}
    }
    const int outdescidsz = outdescids.size();
    Attrib::DescSet* ret = outdescidsz ? ads_->optimizeClone( outdescids )
			    : ads_->optimizeClone( targetid );
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


bool uiAttrVolOut::fillPar( IOPar& iop )
{
    iop.set( IOPar::compKey(sKey::Output(),sKey::Type()), "Cube" );
    const IOObj* outioobj = objfld_->ioobj();
    if ( !outioobj )
	return false;

    Attrib::DescSet* clonedset = 0;
    TypeSet<Attrib::DescID> outdescids;
    int nrseloutputs = 1;

    if ( todofld_ )
	clonedset = getFromToDoFld( outdescids, nrseloutputs );
    else
    {
	attrselfld_->getSelIds( outdescids );
	nrseloutputs = outdescids.size();
	clonedset = ads_->optimizeClone( outdescids );
    }
    if ( !clonedset )
	return false;

    IOPar attrpar( "Attribute Descriptions" );
    clonedset->fillPar( attrpar );
    iop.mergeComp( attrpar, Attrib::SeisTrcStorOutput::attribkey() );

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
    outseisid += outioobj->key();

    iop.set( IOPar::compKey(keybase,Attrib::SeisTrcStorOutput::seisidkey()),
			    outseisid);

    iop.setYN( IOPar::compKey(keybase,SeisTrc::sKeyExtTrcToSI()),
	       transffld_->extendTrcsToSI() );
    iop.mergeComp( subselpar_, IOPar::compKey(sKey::Output(),sKey::Subsel()) );
    Scaler* sc = transffld_->getScaler();
    if ( sc )
    {
	BufferString scalerstr( 1024, false );
	sc->put( scalerstr.getCStr(), scalerstr.bufSize() );
	delete sc;
	iop.set( IOPar::compKey(keybase,Attrib::Output::scalekey()),
		 scalerstr.buf() );
    }

    iop.set( sKey::Target(), outioobj->name() );
    BufferString linename;
    if ( is2d )
    {
	Attrib::DescSet descset(true);
	if ( nlamodel_ )
	    descset.usePar( nlamodel_->pars() );

	Attrib::Desc* desc = nlamodel_ ? descset.getFirstStored()
			      : ads_->getDesc( todofld_->attribID() );
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

    delete clonedset;
    batchjobfld_->saveProcPars( *outioobj );
    return true;
}


void uiAttrVolOut::addNLA( Attrib::DescID& id )
{
    BufferString defstr("NN specification=");
    defstr += nlaid_;

    uiString errmsg;
    Attrib::EngineMan::addNLADesc( defstr, id, *ads_, todofld_->outputNr(),
			   nlamodel_, errmsg );

    if ( !errmsg.isEmpty() )
	uiMSG().error( errmsg );
}


void uiAttrVolOut::getJobName( BufferString& jobnm ) const
{
    jobnm = objfld_->getInput();
}
