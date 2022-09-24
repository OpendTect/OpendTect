/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattremout.h"

#include "attribdescset.h"
#include "attribdesc.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "multiid.h"
#include "seisioobjinfo.h"

#include "uiattrsel.h"
#include "uibatchjobdispatchersel.h"
#include "uimultoutsel.h"
#include "uimsg.h"
#include "od_helpids.h"


using namespace Attrib;

uiAttrEMOut::uiAttrEMOut( uiParent* p, const DescSet& ad,
			  const NLAModel* model, const MultiID& mid,
			  const char* dlgnm )
    : uiBatchProcDlg(p,uiStrings::sEmptyString(),false,
		     Batch::JobSpec::AttribEM )
    , ads_(new Attrib::DescSet(ad))
    , nlamodel_(nullptr)
    , nlaid_(mid)
    , nladescid_( -1, true )
    , batchfld_(nullptr)
{
    if ( model )
	nlamodel_ = model->clone();

    setTitleText( uiString::emptyString() );
    attrfld_ = new uiAttrSel( pargrp_, *ads_, "Select Attribute" );
    attrfld_->setNLAModel( nlamodel_ );
    attrfld_->selectionDone.notify( mCB(this,uiAttrEMOut,attribSel) );
}


uiAttrEMOut::~uiAttrEMOut()
{
    delete ads_;
    delete nlamodel_;
}


void uiAttrEMOut::getDescNames( BufferStringSet& nms ) const
{
    nms = outdescnms_;
}


bool uiAttrEMOut::prepareProcessing()
{
    attrfld_->processInput();
    if ( !attrfld_->attribID().isValid() && attrfld_->outputNr() < 0 )
    {
	uiMSG().error( tr("Please select attribute to calculate") );
	return false;
    }

    return true;
}


Attrib::DescSet* uiAttrEMOut::getTargetDescSet(
	TypeSet<Attrib::DescID>& outdescids, const char* outputnm )
{
    if ( nlamodel_ && attrfld_->outputNr()>=0 )
    {
	if ( nlaid_.isUdf() )
	{
	    uiMSG().message(tr("NN needs to be stored before creating volume"));
	    return nullptr;
	}

	if ( !addNLA(nladescid_) )
	    return nullptr;
    }

    BufferStringSet& seloutnms = outdescnms_;
    seloutnms.erase();
    TypeSet<int> seloutputs;
    const DescID targetid =
	nladescid_.isValid() ? nladescid_ : attrfld_->attribID();
    const Attrib::Desc* seldesc = ads_->getDesc( targetid );
    if ( !seldesc )
	return nullptr;

    if ( seldesc->isStored() )
    // Component selection for stored data done in uiAttrSel
    {
	BufferStringSet compnms;
	const MultiID key( seldesc->getStoredID().buf() );
	SeisIOObjInfo::getCompNames( key, compnms );
	if ( compnms.size()>1 )
	{
	    const int compnr = attrfld_->compNr();
	    if ( compnr<0 )
	    {
		seloutnms = compnms;
		for ( int idx=0; idx<seloutnms.size(); idx++ )
		    seloutputs += idx;
	    }
	    else
	    {
		seloutnms.add(
		    compnms.validIdx(compnr) ? compnms.get(compnr).buf() : "" );
		seloutputs += compnr;
	    }
	}
    }
    else
    {
	uiMultOutSel dlg( this, *seldesc );
	if ( dlg.doDisp() )
	{
	    if ( dlg.go() )
	    {
		dlg.getSelectedOutputs( seloutputs );
		dlg.getSelectedOutNames( seloutnms );
	    }
	    else
		return nullptr;
	}
    }

    Attrib::DescSet* ret = ads_->optimizeClone( targetid );
    if ( !ret )
	return nullptr;

    if ( !seloutputs.isEmpty() )
    {
	const BufferString prefix( outputnm, " - " );
	for ( int idx=0; idx<seloutnms.size(); idx++ )
	{
	    BufferString& nm = seloutnms.get( idx );
	    nm.insertAt( 0, prefix.buf() );
	}

	if ( seloutnms.size()==1 )
	    seloutnms.get(0).set( outputnm );

	ret->createAndAddMultOutDescs( targetid, seloutputs,
					seloutnms, outdescids );
    }
    else
    {
	seloutnms.add( outputnm );
	outdescids += targetid;
    }

    return ret;
}


bool uiAttrEMOut::fillPar( IOPar& iopar )
{
    outdescids_.setEmpty();

    BufferString outputnm;
    iopar.get( sKey::Target(), outputnm );
    DescSet* clonedset = getTargetDescSet( outdescids_, outputnm.buf() );
    if ( !clonedset )
	return false;

    IOPar attrpar( "Attribute Descriptions" );
    clonedset->fillPar( attrpar );
    iopar.mergeComp( attrpar, SeisTrcStorOutput::attribkey() );

    if ( attrfld_->is2D() )
    {
	DescSet descset(true);
	if ( nlamodel_ )
	    descset.usePar( nlamodel_->pars() );

	const Desc* desc = nlamodel_ ? descset.getFirstStored()
				     : clonedset->getFirstStored();
	if (  desc )
	{
	    const BufferString storedid( desc->getStoredID() );
	    if ( !storedid.isEmpty() )
		iopar.set( "Input Line Set", storedid.buf() );
	}
    }

    ads_->removeDesc( nladescid_ );
    return true;
}


void uiAttrEMOut::fillOutPar( IOPar& iopar, const char* outtyp,
			      const char* idlbl, const MultiID& outid )
{
    iopar.set( IOPar::compKey( sKey::Output(), sKey::Type()), outtyp );
    BufferString key;
    BufferString tmpkey;
    mDefineStaticLocalObject( const BufferString, keybase,
			      ( IOPar::compKey(sKey::Output(), 0) ) );
    tmpkey = IOPar::compKey( keybase.buf(), SeisTrcStorOutput::attribkey() );
    key = IOPar::compKey( tmpkey.buf(), DescSet::highestIDStr() );
    iopar.set( key, outdescids_.size() );

    tmpkey = IOPar::compKey( keybase.buf(), SeisTrcStorOutput::attribkey() );
    for ( int idx=0; idx< outdescids_.size(); idx++ )
    {
	key = IOPar::compKey( tmpkey.buf(), idx );
	iopar.set( key, outdescids_[idx].asInt() );
    }

    key = IOPar::compKey( keybase.buf(), idlbl );
    iopar.set( key, outid );
}


#define mErrRet(str) { uiMSG().message( str ); return false; }

bool uiAttrEMOut::addNLA( DescID& id )
{
    BufferString defstr("NN specification=");
    defstr += nlaid_;

    const int outputnr = attrfld_->outputNr();
    uiString errmsg;
    EngineMan::addNLADesc( defstr, id, *ads_, outputnr, nlamodel_, errmsg );
    if ( !errmsg.isEmpty() )
	mErrRet( errmsg );

    return true;
}


void uiAttrEMOut::updateAttributes( const Attrib::DescSet& descset,
				    const NLAModel* nlamodel,
				    const MultiID& nlaid )
{
    delete ads_;
    ads_ = new Attrib::DescSet( descset );
    if ( nlamodel )
    {
	delete nlamodel_;
	nlamodel_ = nlamodel->clone();
    }

    attrfld_->setNLAModel( nlamodel_ );
    nlaid_ = nlaid;
    attrfld_->setDescSet( ads_ );
}
