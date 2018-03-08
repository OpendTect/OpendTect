/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          January 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiattremout.h"

#include "attribdescset.h"
#include "attribdesc.h"
#include "attribengman.h"
#include "attriboutput.h"
#include "hiddenparam.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "multiid.h"

#include "uiattrsel.h"
#include "uibatchjobdispatchersel.h"
#include "uimultoutsel.h"
#include "uimsg.h"
#include "od_helpids.h"

static HiddenParam<uiAttrEMOut,TypeSet<Attrib::DescID>*> outdescids_(0);
static HiddenParam<uiAttrEMOut,BufferStringSet*> outdescnms_(0);

using namespace Attrib;

uiAttrEMOut::uiAttrEMOut( uiParent* p, const DescSet& ad,
			  const NLAModel* model, const MultiID& mid,
			  const char* dlgnm )
    : uiBatchProcDlg(p,uiStrings::sEmptyString(),false,
		     Batch::JobSpec::AttribEM )
    , ads_(new Attrib::DescSet(ad))
    , nlamodel_(0)
    , nlaid_(mid)
    , nladescid_( -1, true )
    , batchfld_(0)
{
    if ( model )
	nlamodel_ = model->clone();

    setTitleText( uiString::emptyString() );
    attrfld_ = new uiAttrSel( pargrp_, *ads_, "Quantity to output" );
    attrfld_->setNLAModel( nlamodel_ );
    attrfld_->selectionDone.notify( mCB(this,uiAttrEMOut,attribSel) );

    outdescids_.setParam( this, new TypeSet<Attrib::DescID> );
    outdescnms_.setParam( this, new BufferStringSet );
}


uiAttrEMOut::~uiAttrEMOut()
{
    delete ads_;
    delete nlamodel_;

    outdescids_.removeAndDeleteParam( this );
    outdescnms_.removeAndDeleteParam( this );
}


void uiAttrEMOut::getDescNames( BufferStringSet& nms ) const
{
    nms = *outdescnms_.getParam( this );
}


bool uiAttrEMOut::prepareProcessing()
{
    attrfld_->processInput();
    if ( !attrfld_->attribID().isValid() && attrfld_->outputNr() < 0 )
    {
	uiMSG().error( tr("Please select the output quantity") );
	return false;
    }

    return true;
}


Attrib::DescSet* uiAttrEMOut::getTargetDescSet(
	TypeSet<Attrib::DescID>& outdescids, const char* outputnm )
{
    if ( nlamodel_ && attrfld_->outputNr()>=0 )
    {
	if ( !nlaid_ || !(*nlaid_) )
	{
	    uiMSG().message(tr("NN needs to be stored before creating volume"));
	    return 0;
	}

	if ( !addNLA(nladescid_) )
	    return 0;
    }

    BufferStringSet& seloutnms = *outdescnms_.getParam( this );
    seloutnms.erase();
    TypeSet<int> seloutputs;
    const DescID targetid =
	nladescid_.isValid() ? nladescid_ : attrfld_->attribID();
    const Attrib::Desc* seldesc = ads_->getDesc( targetid );
    if ( seldesc )
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
		return 0;
	}
    }

    Attrib::DescSet* ret = ads_->optimizeClone( targetid );
    if ( !ret )
	return 0;

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
    TypeSet<Attrib::DescID>* outdescids = outdescids_.getParam( this );
    outdescids->setEmpty();

    BufferString outputnm;
    iopar.get( sKey::Target(), outputnm );
    DescSet* clonedset = getTargetDescSet( *outdescids, outputnm.buf() );
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
	BufferString storedid = desc ? desc->getStoredID() : "";
	if ( !storedid.isEmpty() )
	    iopar.set( "Input Line Set", storedid.buf() );
    }

    ads_->removeDesc( nladescid_ );
    return true;
}


void uiAttrEMOut::fillOutPar( IOPar& iopar, const char* outtyp,
			      const char* idlbl, const char* outid )
{
    const TypeSet<Attrib::DescID>& outdescids = *outdescids_.getParam( this );

    iopar.set( IOPar::compKey( sKey::Output(), sKey::Type()), outtyp );
    BufferString key;
    BufferString tmpkey;
    mDefineStaticLocalObject( const BufferString, keybase,
			      ( IOPar::compKey(sKey::Output(), 0) ) );
    tmpkey = IOPar::compKey( keybase.buf(), SeisTrcStorOutput::attribkey() );
    key = IOPar::compKey( tmpkey.buf(), DescSet::highestIDStr() );
    iopar.set( key, outdescids.size() );

    tmpkey = IOPar::compKey( keybase.buf(), SeisTrcStorOutput::attribkey() );
    for ( int idx=0; idx<outdescids.size(); idx++ )
    {
	key = IOPar::compKey( tmpkey.buf(), idx );
	iopar.set( key, outdescids[idx].asInt() );
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
